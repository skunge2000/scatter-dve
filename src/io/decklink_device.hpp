// scatter-dve — WU-14: DeckLink device enumeration.
//
// See DECISIONS.md ADR-031 for the design this header declares: what
// IDeckLink actually is in the real SDK (a minimal interface -- model and
// display name only; IDeckLinkInput/IDeckLinkOutput/IDeckLinkProfileAttributes
// are all obtained separately via QueryInterface), CreateDeckLinkIteratorInstance()
// as the enumeration entry point (architecture.md 7), and why this unit's own
// scope stops at enumeration and capability queries -- no
// EnableVideoInput()/EnableVideoOutput()/StartStreams() call anywhere here or
// in decklink_device.cpp.
//
// Includes DeckLinkAPI.h directly rather than forward-declaring IDeckLink --
// consistent with every other header in this project (video/raster.hpp,
// core/resolve.hpp, ...), none of which use a forward-declare-only pattern
// for their own dependencies. This is therefore a macOS-only header: it
// belongs to the scatter-decklink CMake target (WU-14 onward), never to
// scatter-core, exactly as ADR-013/ADR-021 already require.

#pragma once

#include "io/com_ptr.hpp"

#include "DeckLinkAPI.h"

#include <string>
#include <vector>

namespace scatter::io {

struct DeviceInfo {
    std::string modelName;
    std::string displayName;
    bool supportsCapture = false;
    bool supportsPlayback = false;
    ComPtr<IDeckLink> device;
};

// Enumerates every DeckLink device the driver currently reports, via
// CreateDeckLinkIteratorInstance()/IDeckLinkIterator::Next() (architecture.md
// 7's own entry point), reading each device's model/display name and
// BMDDeckLinkVideoIOSupport capture/playback support bits -- without calling
// EnableVideoInput(), EnableVideoOutput() or otherwise opening a stream on
// any device; WU-15 onward do that. Returns an empty vector, not an error,
// if no DeckLink driver is installed or no device is attached -- matching
// architecture.md 10 Phase 0's own "confirm the device enumerates" check,
// which this function makes queryable in code rather than only by hand in
// Desktop Video Setup.
std::vector<DeviceInfo> enumerateDeckLinkDevices();

}  // namespace scatter::io
