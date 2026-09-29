#ifndef PLATFORM_PSP_H
#define PLATFORM_PSP_H

#include "kfx/platform/PlatformLinux.h"

/** Sony PSP. Reuses the POSIX (newlib) file walking of PlatformLinux; the
 *  PSP owns its fixed 480x272 display. */
class PlatformPSP : public PlatformLinux {
public:
    const char* GetOSVersion() const override;
    const char* GetUserPrefDir() override;
    bool VideoInit() override;
    bool OwnsDisplay() const override { return true; }
    bool ForcesAllModesAvailable() const override { return true; }
};

#endif // PLATFORM_PSP_H
