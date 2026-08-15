// scatter-dve — WU-14: intrusive COM-style reference-counting smart pointer
// for the Blackmagic DeckLink SDK's IUnknown-derived interfaces.
//
// Modeled closely on the SDK's own Samples/*/com_ptr.h -- present, byte-for-
// byte near-identical, in essentially every one of the SDK's C++ samples
// (CapturePreview, DeviceStatus, FileCapture, FilePlayback,
// InputLoopThrough, KeyerOutput, MetalOutput, MultiPreview, SignalGenerator,
// SignalGenHDR): same public surface (copy/move, the QueryInterface-based
// converting constructor, get(), operator->/operator*, explicit
// operator bool, releaseAndGetAddressOf()), renamed to this project's own
// PascalCase type-naming convention (Lattice, AccumCell, EwaFootprint, ...)
// rather than invented from scratch. See DECISIONS.md ADR-031 for why:
// matching the SDK's own idiom exactly, rather than a differently-shaped
// hand-rolled version, is what architecture.md 7 means by "write a small
// intrusive ComPtr and use it everywhere" -- every Blackmagic sample and
// every piece of Blackmagic documentation already speaks this exact shape.
//
// One deliberate addition beyond the SDK sample: adopt(). The sample's own
// raw-pointer constructor and raw-pointer assignment always AddRef, which is
// correct for a *borrowed* pointer (a callback parameter valid only for the
// duration of the call, per architecture.md 7's own "copy or retain, then
// return") but wrong for a *factory* result such as
// CreateDeckLinkIteratorInstance()'s return value or
// IDeckLinkIterator::Next()'s out-parameter -- both already transfer one
// reference to the caller, per ordinary COM convention, and wrapping either
// in the AddRef'ing constructor leaks one reference per call. architecture.md
// 12's own risk table names this exact failure mode: "Reference-count leaks
// lock the device | ComPtr everywhere; never hold a raw interface pointer."
// adopt() (and releaseAndGetAddressOf(), already present in the SDK's own
// sample and unchanged here) are how this file avoids that leak wherever an
// already-owned pointer needs to be captured rather than a borrowed one
// retained. See ADR-031 for the full reasoning, including why the SDK's own
// sample does not always draw this distinction itself.

#pragma once

#include <CoreFoundation/CFPlugInCOM.h>

#include <cstddef>
#include <utility>

namespace scatter::io {

template <typename T>
class ComPtr {
    template <typename U>
    friend class ComPtr;

public:
    constexpr ComPtr() noexcept : m_ptr(nullptr) {}
    constexpr ComPtr(std::nullptr_t) noexcept : m_ptr(nullptr) {}

    // Borrowing constructor: AddRefs. For a pointer this ComPtr does not
    // already own a reference to -- a callback parameter, per the file
    // header above. Not the right choice for a factory/enumerator result;
    // use adopt() or releaseAndGetAddressOf() for those instead.
    explicit ComPtr(T* borrowedPtr) : m_ptr(borrowedPtr) {
        if (m_ptr) m_ptr->AddRef();
    }

    ComPtr(const ComPtr<T>& other) : m_ptr(other.m_ptr) {
        if (m_ptr) m_ptr->AddRef();
    }

    ComPtr(ComPtr<T>&& other) noexcept : m_ptr(other.m_ptr) {
        other.m_ptr = nullptr;
    }

    // QueryInterface-based converting constructor. QueryInterface itself
    // already AddRefs its result on success (COM convention), so this does
    // not AddRef again -- matching the SDK's own com_ptr sample exactly.
    template <typename U>
    ComPtr(REFIID iid, const ComPtr<U>& other) : m_ptr(nullptr) {
        if (other.m_ptr) {
            if (other.m_ptr->QueryInterface(iid, reinterpret_cast<void**>(&m_ptr)) != S_OK)
                m_ptr = nullptr;
        }
    }

    ~ComPtr() { release(); }

    ComPtr<T>& operator=(std::nullptr_t) {
        release();
        m_ptr = nullptr;
        return *this;
    }

    ComPtr<T>& operator=(T* borrowedPtr) {
        if (borrowedPtr) borrowedPtr->AddRef();
        release();
        m_ptr = borrowedPtr;
        return *this;
    }

    ComPtr<T>& operator=(const ComPtr<T>& other) { return (*this = other.m_ptr); }

    ComPtr<T>& operator=(ComPtr<T>&& other) noexcept {
        if (this != &other) {
            release();
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    // Takes ownership of an already-owned reference (a factory return such
    // as CreateDeckLinkIteratorInstance()) with no AddRef. Releases whatever
    // this ComPtr held first. See the file header.
    void adopt(T* alreadyOwnedPtr) noexcept {
        release();
        m_ptr = alreadyOwnedPtr;
    }

    T* get() const noexcept { return m_ptr; }

    // Releases whatever this ComPtr currently holds and returns the address
    // of its now-null internal pointer, for passing to an out-parameter
    // (e.g. IDeckLinkIterator::Next()) that will assign an already-owned
    // reference into it directly. No AddRef happens on either side of that
    // assignment -- the same ownership-transfer semantics as adopt().
    T** releaseAndGetAddressOf() noexcept {
        release();
        return &m_ptr;
    }

    const T* operator->() const noexcept { return m_ptr; }
    T* operator->() noexcept { return m_ptr; }
    const T& operator*() const noexcept { return *m_ptr; }
    T& operator*() noexcept { return *m_ptr; }

    explicit operator bool() const noexcept { return m_ptr != nullptr; }

    bool operator==(const ComPtr<T>& other) const noexcept { return m_ptr == other.m_ptr; }
    bool operator!=(const ComPtr<T>& other) const noexcept { return m_ptr != other.m_ptr; }

private:
    void release() {
        if (m_ptr) m_ptr->Release();
    }

    T* m_ptr;
};

}  // namespace scatter::io
