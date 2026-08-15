// scatter-dve — WU-20a: ring buffer (DECISIONS.md ADR-046).
//
// Fully portable: links only scatter-core, no Blackmagic SDK dependency at
// all, unlike tests/test_decklink_device.cpp / test_decklink_output.cpp.
// Genuinely built and run in this project's own Linux cloud sandbox,
// including a real concurrent producer/consumer under ThreadSanitizer --
// the same "verify concurrency empirically, not just by inspection"
// standard WU-16a (ADR-040) established, applied here for the first time to
// a project-owned data structure whose correctness rests on atomic
// acquire/release ordering rather than a mutex.

#include "core/ring_buffer.hpp"
#include "harness.hpp"

#include <atomic>
#include <cstddef>
#include <optional>
#include <thread>

using namespace scatter;

namespace {

// Move-only, instrumented: tracks live/constructed/destroyed instance
// counts so a test can check for leaked or duplicated instances -- the
// generic equivalent of architecture.md 12's own "reference-count leaks
// lock the device" risk, for whichever T a caller instantiates RingBuffer
// with (WU-20b's own future ComPtr<IDeckLinkVideoInputFrame>, in
// particular).
struct Tracked {
    static inline std::atomic<int> liveCount{0};
    static inline std::atomic<long> totalConstructed{0};

    int value = -1;

    Tracked() noexcept {
        liveCount.fetch_add(1, std::memory_order_relaxed);
        totalConstructed.fetch_add(1, std::memory_order_relaxed);
    }
    explicit Tracked(int v) noexcept : value(v) {
        liveCount.fetch_add(1, std::memory_order_relaxed);
        totalConstructed.fetch_add(1, std::memory_order_relaxed);
    }

    Tracked(const Tracked&) = delete;
    Tracked& operator=(const Tracked&) = delete;

    Tracked(Tracked&& other) noexcept : value(other.value) {
        other.value = -1;
        liveCount.fetch_add(1, std::memory_order_relaxed);
        totalConstructed.fetch_add(1, std::memory_order_relaxed);
    }
    Tracked& operator=(Tracked&& other) noexcept {
        value = other.value;
        other.value = -1;
        return *this;
    }

    ~Tracked() noexcept { liveCount.fetch_sub(1, std::memory_order_relaxed); }
};

void test_empty_pop_returns_nullopt() {
    RingBuffer<Tracked, 4> ring;
    CHECK(!ring.tryPop().has_value());
    CHECK(ring.droppedCount() == std::size_t(0));
}

void test_push_pop_preserves_fifo_order() {
    RingBuffer<Tracked, 4> ring;
    CHECK(ring.tryPush(Tracked(10)));
    CHECK(ring.tryPush(Tracked(20)));
    CHECK(ring.tryPush(Tracked(30)));

    auto a = ring.tryPop();
    auto b = ring.tryPop();
    auto c = ring.tryPop();
    CHECK(a.has_value() && a->value == 10);
    CHECK(b.has_value() && b->value == 20);
    CHECK(c.has_value() && c->value == 30);
    CHECK(!ring.tryPop().has_value());
}

// Usable capacity is Capacity, not Capacity + 1 -- the always-empty-slot
// technique the header documents -- checked directly, not just asserted:
// filling exactly Capacity items must succeed, and the next push must be
// the one that gets dropped.
void test_full_ring_drops_and_counts_without_corrupting_existing_entries() {
    RingBuffer<Tracked, 3> ring;
    CHECK(ring.tryPush(Tracked(1)));
    CHECK(ring.tryPush(Tracked(2)));
    CHECK(ring.tryPush(Tracked(3)));
    CHECK(ring.droppedCount() == std::size_t(0));

    CHECK(!ring.tryPush(Tracked(4)));
    CHECK(ring.droppedCount() == std::size_t(1));
    CHECK(!ring.tryPush(Tracked(5)));
    CHECK(ring.droppedCount() == std::size_t(2));

    auto a = ring.tryPop();
    auto b = ring.tryPop();
    auto c = ring.tryPop();
    CHECK(a.has_value() && a->value == 1);
    CHECK(b.has_value() && b->value == 2);
    CHECK(c.has_value() && c->value == 3);
    CHECK(!ring.tryPop().has_value());
}

void test_capacity_reports_usable_slots_not_backing_storage() {
    using Ring5 = RingBuffer<Tracked, 5>;  // CHECK() is a single-argument macro
                                            // (harness.hpp); a raw template
                                            // argument list's own comma would
                                            // otherwise split into two macro
                                            // arguments.
    Ring5 ring;
    CHECK(Ring5::capacity() == std::size_t(5));
    for (int i = 0; i < 5; ++i) CHECK(ring.tryPush(Tracked(i)));
    CHECK(!ring.tryPush(Tracked(99)));  // the always-empty slot must not be usable
    CHECK(ring.droppedCount() == std::size_t(1));
}

// Forces the internal head_/tail_ indices to wrap around the backing
// array's own bound many times over (10000 cycles against a 4-slot ring),
// checking two things a single push/pop pair could not: the modulo-based
// advance() arithmetic itself, and that repeated wraparound does not
// eventually leak a Tracked instance (liveCount drifting upward) or destroy
// one twice (a double-decrement would show up the same way, since
// liveCount is a plain running total, not a per-slot flag).
void test_many_cycles_exercise_index_wraparound_with_no_leak_or_duplication() {
    constexpr std::size_t kCapacity = 4;
    RingBuffer<Tracked, kCapacity> ring;
    constexpr int kCycles = 10000;

    long expectedSum = 0;
    long actualSum = 0;

    for (int i = 0; i < kCycles; ++i) {
        CHECK(ring.tryPush(Tracked(i)));
        expectedSum += i;
        auto popped = ring.tryPop();
        CHECK(popped.has_value());
        if (popped) actualSum += popped->value;
    }

    CHECK(ring.droppedCount() == std::size_t(0));  // one in, one out every cycle -- never full
    CHECK(actualSum == expectedSum);

    // Everything this loop constructed beyond the ring's own backing
    // storage (Capacity + 1 default-constructed slots, alive for the
    // ring's whole lifetime) must have been destroyed by now -- each
    // `popped` goes out of scope at the end of its own iteration.
    CHECK(Tracked::liveCount.load() == int(kCapacity + 1));
}

// The one check this file cares most about: real concurrent access from two
// threads, checked under ThreadSanitizer in this project's own sandbox
// matrix.
void test_concurrent_single_producer_single_consumer() {
    constexpr std::size_t kCapacity = 16;
    constexpr int kItemCount = 200000;  // deliberately >> capacity
    RingBuffer<Tracked, kCapacity> ring;

    std::atomic<bool> producerDone{false};
    std::thread producer([&] {
        for (int i = 0; i < kItemCount; ++i) {
            // Busy-retry on a full ring rather than counting drops here --
            // this test's job is to check the ring never corrupts or loses
            // an item it actually accepted, not to exercise the drop path
            // (test_full_ring_drops_and_counts_without_corrupting_existing_entries
            // already does that, single-threaded and deterministically).
            while (!ring.tryPush(Tracked(i))) {
                std::this_thread::yield();
            }
        }
        producerDone.store(true, std::memory_order_release);
    });

    long receivedCount = 0;
    long checksum = 0;
    int expectedNext = 0;
    bool orderOk = true;

    for (;;) {
        auto item = ring.tryPop();
        if (item) {
            checksum += item->value;
            if (item->value != expectedNext) orderOk = false;
            ++expectedNext;
            ++receivedCount;
            continue;
        }
        if (producerDone.load(std::memory_order_acquire)) {
            // producerDone's own release store happens-after every prior
            // tryPush() on the producer thread; observing it true here
            // (acquire) therefore also makes every one of those pushes'
            // effects visible -- draining until empty from this point picks
            // up everything the producer ever queued, with no race window.
            while (auto drained = ring.tryPop()) {
                checksum += drained->value;
                if (drained->value != expectedNext) orderOk = false;
                ++expectedNext;
                ++receivedCount;
            }
            break;
        }
        std::this_thread::yield();
    }
    producer.join();

    CHECK(receivedCount == long(kItemCount));
    CHECK(orderOk);  // SPSC + FIFO slot order together guarantee arrival order
    long expectedChecksum = 0;
    for (int i = 0; i < kItemCount; ++i) expectedChecksum += i;
    CHECK(checksum == expectedChecksum);
    CHECK(Tracked::liveCount.load() == int(kCapacity + 1));
}

}  // namespace

int main() {
    test_empty_pop_returns_nullopt();
    test_push_pop_preserves_fifo_order();
    test_full_ring_drops_and_counts_without_corrupting_existing_entries();
    test_capacity_reports_usable_slots_not_backing_storage();
    test_many_cycles_exercise_index_wraparound_with_no_leak_or_duplication();
    test_concurrent_single_producer_single_consumer();
    return scatter::test::summary("test_ring_buffer");
}
