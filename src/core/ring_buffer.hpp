// scatter-dve — WU-20a: a fixed-capacity, single-producer/single-consumer,
// allocation-free ring buffer for retained handles crossing a driver-owned
// callback thread.
//
// See DECISIONS.md ADR-046 for the full design and for why this is split out
// of WU-20 ("DeckLink input, format detection, ring buffer") as its own
// half-unit, WU-20a: this file has zero DeckLink dependency and zero
// platform dependency at all -- ordinary portable C++20, the same as every
// other core/ file (architecture.md 8's own "core/ -- portable, zero
// platform dependencies" charter) -- so unlike every prior DeckLink-touching
// unit (WU-14, WU-15a), it can be built AND genuinely run in this project's
// own Linux cloud sandbox, including under ThreadSanitizer with a real
// concurrent producer and consumer, the same "check concurrency
// empirically, not just by inspection" standard WU-16a (ADR-040) established
// for this project's first concurrent code. WU-20b -- the DeckLink-specific
// capture object that actually calls EnableVideoInput(), implements
// IDeckLinkInputCallback and pushes arriving frames into an instance of this
// class -- is not built this session; see ADR-046 and WORK-UNITS.md.
//
// architecture.md section 6: "Capture callback thread (driver-owned).
// Retains the frame, pushes to a lock-free ring, returns immediately. Never
// blocks, never allocates." This class is the "lock-free ring" that
// sentence names. It is not adapted from the Blackmagic SDK's own samples --
// none of the ones this project's own WU-20 reading surveyed implement one:
// CaptureStills hands a frame off via a std::queue guarded by a std::mutex/
// std::condition_variable (allocates on every push, and a full queue would
// block a producer, though that sample never bounds its own queue to make
// that a live concern); InputLoopThrough and CapturePreview instead invoke a
// std::function callback synchronously, on the callback thread itself,
// which is not "push and return immediately" at all -- whatever the
// callback body does runs with the driver's own callback thread blocked for
// its duration. architecture.md's own "never blocks, never allocates"
// requirement is stricter than any real sample this project has read
// provides, so this is this project's own design, built against that
// requirement directly rather than adapted from an existing idiom the way
// ComPtr (ADR-031) and the preroll/refill mechanism (ADR-032) were.

#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <optional>
#include <utility>

namespace scatter {

// Single producer, single consumer -- exactly one thread may call tryPush(),
// exactly one (possibly different) thread may call tryPop(); concurrent
// calls from more than one thread on the same side are not supported and
// not guarded against, the same "caller's own bug, not guarded against
// here" convention this codebase already uses for unchecked preconditions
// elsewhere (e.g. Lattice::at()'s row/col bounds, core/pipeline.hpp's
// ThreadPool::runOnAll() requiring fn not to throw).
//
// Capacity is fixed at compile time and the backing storage -- Capacity + 1
// default-constructed T's -- is allocated once, as a class member, and
// never again: tryPush()/tryPop() themselves never allocate, matching
// architecture.md 6's own requirement for whatever runs on the capture
// callback thread. T must be default-constructible (std::array<T, N>'s own
// requirement) and move-constructible/move-assignable; every retained-handle
// type this project has -- ComPtr<T>'s own move constructor and move
// assignment, both noexcept (src/io/com_ptr.hpp) -- already satisfies this,
// and this header itself depends on neither ComPtr nor anything DeckLink- or
// platform-specific, so it is exercised directly against a plain
// instrumented type in tests/test_ring_buffer.cpp rather than requiring the
// Blackmagic SDK to test at all.
//
// One slot is deliberately left always-empty (the classic circular-buffer
// technique): head_ == tail_ is then an unambiguous "empty" test with no
// separate size counter needed -- a second piece of shared state the
// producer and consumer would otherwise have to keep consistent with each
// other, for no benefit here. Usable capacity is therefore Capacity, not
// Capacity + 1.
template <typename T, std::size_t Capacity>
class RingBuffer {
    static_assert(Capacity >= 1, "RingBuffer needs room for at least one element");

public:
    RingBuffer() = default;
    ~RingBuffer() = default;

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;
    RingBuffer(RingBuffer&&) = delete;
    RingBuffer& operator=(RingBuffer&&) = delete;

    // Producer side only. Never blocks: if the ring is already full (the
    // consumer has fallen behind), item is dropped -- not queued, not
    // waited for room -- and droppedCount() is incremented. Returns whether
    // the push succeeded, so a caller that wants to know can; WU-20b's own
    // future capture callback is not expected to check it on every call
    // (per architecture.md 6, that callback "returns immediately" either
    // way), only to surface droppedCount() periodically.
    bool tryPush(T&& item) noexcept {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t nextHead = advance(head);
        // Acquire: synchronizes with the consumer's own release store to
        // tail_ in tryPop(), so this load is guaranteed to see the most
        // recent slot the consumer has actually finished freeing, not a
        // stale one that would make a genuinely full ring look like it has
        // room.
        if (nextHead == tail_.load(std::memory_order_acquire)) {
            dropped_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        slots_[head] = std::move(item);
        // Release: publishes both the slot write immediately above and the
        // new head_ value together, so the consumer's own acquire load in
        // tryPop() cannot observe the new head_ without also observing the
        // slot contents that go with it.
        head_.store(nextHead, std::memory_order_release);
        return true;
    }

    // Consumer side only. Never blocks: returns std::nullopt immediately if
    // the ring is empty rather than waiting for an item to arrive.
    std::optional<T> tryPop() noexcept {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) return std::nullopt;
        std::optional<T> result(std::move(slots_[tail]));
        // Explicitly reset the vacated slot to a default-constructed T,
        // rather than relying on whatever state T's own move constructor
        // happens to leave a moved-from object in. Every retained-handle
        // type this project has (ComPtr<T>) already nulls its moved-from
        // pointer, making this redundant for that specific case -- but a
        // ring buffer whose whole purpose is not silently retaining a
        // reference past its owner's intent (architecture.md 12's own risk,
        // "reference-count leaks lock the device," is exactly this failure
        // mode by a different route) is worth making that property true by
        // construction, for any T, rather than true only for the one T this
        // project happens to use today.
        slots_[tail] = T{};
        tail_.store(advance(tail), std::memory_order_release);
        return result;
    }

    // Producer-observed count of tryPush() calls that found the ring full.
    // Relaxed: a caller reading this from any thread gets a recent, not
    // necessarily instantaneous, value -- the same convention
    // io/decklink_output.hpp's PlaybackStats already uses for its own
    // atomics (WU-15a).
    std::size_t droppedCount() const noexcept { return dropped_.load(std::memory_order_relaxed); }

    static constexpr std::size_t capacity() noexcept { return Capacity; }

private:
    static constexpr std::size_t advance(std::size_t index) noexcept {
        return (index + 1) % (Capacity + 1);
    }

    std::array<T, Capacity + 1> slots_{};
    std::atomic<std::size_t> head_{0};
    std::atomic<std::size_t> tail_{0};
    std::atomic<std::size_t> dropped_{0};
};

}  // namespace scatter
