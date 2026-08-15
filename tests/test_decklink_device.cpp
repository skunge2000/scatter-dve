// WU-14 -- DeckLink device enumeration and ComPtr.
//
// Runs only against real hardware, on the M1 Max with the UltraStudio 4K
// Mini attached -- this cannot be built or tested in the Linux cloud sandbox
// at all (no Blackmagic SDK there; see CMakeLists.txt's BLACKMAGIC_SDK_DIR
// guard and DECISIONS.md ADR-031). Everything checked here is true of
// enumeration and attribute queries alone -- nothing calls
// EnableVideoInput()/EnableVideoOutput() or opens a stream; that is WU-15
// onward's own job, per ADR-031's own scoping note.

#include "io/decklink_device.hpp"
#include "io/com_ptr.hpp"
#include "harness.hpp"

#include "DeckLinkAPI.h"

using namespace scatter::io;

static void test_at_least_one_device_enumerates() {
    // Confirmed by hand this session (HANDOFF.md): the UltraStudio 4K Mini
    // enumerates. A driver/hardware problem should fail loudly here, not
    // silently pass on an empty list.
    const auto devices = enumerateDeckLinkDevices();
    CHECK(!devices.empty());
}

static void test_every_device_has_names() {
    for (const auto& d : enumerateDeckLinkDevices()) {
        CHECK_ONCE(!d.modelName.empty());
        CHECK_ONCE(!d.displayName.empty());
    }
}

static void test_at_least_one_device_is_full_duplex() {
    // architecture.md 7: "The UltraStudio 4K Mini is full duplex: one
    // IDeckLink exposing both IDeckLinkInput and IDeckLinkOutput." Checked
    // two independent ways: the BMDDeckLinkVideoIOSupport attribute bits
    // (already read into DeviceInfo) and that QueryInterface for both
    // interfaces actually succeeds against the live device -- the attribute
    // could in principle be right while QueryInterface itself is broken, or
    // vice versa, and checking both is cheap.
    bool foundDuplexDevice = false;
    for (const auto& d : enumerateDeckLinkDevices()) {
        if (!d.supportsCapture || !d.supportsPlayback) continue;

        ComPtr<IDeckLinkInput> input(IID_IDeckLinkInput, d.device);
        ComPtr<IDeckLinkOutput> output(IID_IDeckLinkOutput, d.device);
        if (input && output) {
            foundDuplexDevice = true;
            break;
        }
    }
    CHECK(foundDuplexDevice);
}

static void test_queryinterface_identity() {
    // COM's own identity rule: QueryInterface for the same interface ID on
    // the same object always returns the same pointer value. A real check of
    // ComPtr's QueryInterface-converting constructor against live hardware,
    // not just "it compiled".
    const auto devices = enumerateDeckLinkDevices();
    CHECK(!devices.empty());
    if (devices.empty()) return;

    const auto& d = devices.front();
    ComPtr<IDeckLinkProfileAttributes> a(IID_IDeckLinkProfileAttributes, d.device);
    ComPtr<IDeckLinkProfileAttributes> b(IID_IDeckLinkProfileAttributes, d.device);
    CHECK(a.get() != nullptr);
    CHECK(a.get() == b.get());
}

static void test_repeated_enumeration_is_stable() {
    // A leaked or double-released reference (ADR-031's own concern --
    // architecture.md 12's "reference-count leaks lock the device") would
    // not necessarily change the device count by itself, but a broken
    // adopt()/releaseAndGetAddressOf() could plausibly corrupt the iterator
    // or a later call; checking that repeated enumeration is at least
    // stable is a cheap regression guard for that.
    const auto first = enumerateDeckLinkDevices();
    const auto second = enumerateDeckLinkDevices();
    CHECK(first.size() == second.size());
}

int main() {
    test_at_least_one_device_enumerates();
    test_every_device_has_names();
    test_at_least_one_device_is_full_duplex();
    test_queryinterface_identity();
    test_repeated_enumeration_is_stable();
    return scatter::test::summary("test_decklink_device");
}
