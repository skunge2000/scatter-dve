// scatter-dve — WU-14: DeckLink device enumeration.
//
// See io/decklink_device.hpp and DECISIONS.md ADR-031 for the design. In
// short: CreateDeckLinkIteratorInstance() and IDeckLinkIterator::Next() both
// hand the caller an already-owned reference (ordinary COM enumerator
// convention), so both are captured via ComPtr::adopt()/
// releaseAndGetAddressOf() rather than ComPtr's AddRef'ing raw-pointer
// constructor -- using the latter here would leak one reference per device,
// per call, exactly the failure architecture.md 12's risk table names
// ("Reference-count leaks lock the device").

#include "io/decklink_device.hpp"
#include "io/com_ptr.hpp"

#include "DeckLinkAPI.h"

#include <cstddef>
#include <vector>

namespace scatter::io {

namespace {

// Converts and immediately releases a DeckLink-owned CFStringRef -- every
// IDeckLink*::Get*(CFStringRef*) call in this SDK returns one the caller
// must release (the SDK's own samples do this via DeleteString ==
// CFRelease). Folded into one step here since nothing in this unit needs to
// hold the CFStringRef itself past the conversion.
std::string cfStringToStd(CFStringRef str) {
    if (str == nullptr) return {};
    std::string result;
    const CFIndex length = CFStringGetLength(str);
    const CFIndex maxBytes = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
    std::vector<char> buffer(static_cast<std::size_t>(maxBytes));
    if (CFStringGetCString(str, buffer.data(), maxBytes, kCFStringEncodingUTF8))
        result.assign(buffer.data());
    CFRelease(str);
    return result;
}

}  // namespace

std::vector<DeviceInfo> enumerateDeckLinkDevices() {
    std::vector<DeviceInfo> devices;

    ComPtr<IDeckLinkIterator> iterator;
    iterator.adopt(CreateDeckLinkIteratorInstance());
    if (!iterator) return devices;  // no driver installed, or SDK bundle absent

    ComPtr<IDeckLink> deckLink;
    while (iterator->Next(deckLink.releaseAndGetAddressOf()) == S_OK) {
        DeviceInfo info;
        info.device = deckLink;  // copy: AddRefs: info.device outlives deckLink's own next reassignment

        CFStringRef modelName = nullptr;
        if (deckLink->GetModelName(&modelName) == S_OK)
            info.modelName = cfStringToStd(modelName);

        CFStringRef displayName = nullptr;
        if (deckLink->GetDisplayName(&displayName) == S_OK)
            info.displayName = cfStringToStd(displayName);

        // IDeckLinkProfileAttributes is obtained via QueryInterface, not
        // exposed on IDeckLink itself -- see ADR-031. QueryInterface already
        // AddRefs its own result, so ComPtr's converting constructor does
        // not AddRef again (see com_ptr.hpp).
        ComPtr<IDeckLinkProfileAttributes> attributes(IID_IDeckLinkProfileAttributes, deckLink);
        if (attributes) {
            int64_t ioSupport = 0;
            if (attributes->GetInt(BMDDeckLinkVideoIOSupport, &ioSupport) == S_OK) {
                info.supportsCapture = (ioSupport & bmdDeviceSupportsCapture) != 0;
                info.supportsPlayback = (ioSupport & bmdDeviceSupportsPlayback) != 0;
            }
        }

        devices.push_back(std::move(info));
    }

    return devices;
}

}  // namespace scatter::io
