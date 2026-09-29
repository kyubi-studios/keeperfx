/* Force-included into every KeeperFX translation unit on the PSP build. */
#ifndef KFX_PSP_COMPAT_H
#define KFX_PSP_COMPAT_H

#include <strings.h>
#include "psp_trace.h"
#include <signal.h>

#ifndef stricmp
#define stricmp strcasecmp
#endif
#ifndef strnicmp
#define strnicmp strncasecmp
#endif
/* Windows-only console break signal; the crash parachute installs a handler for it. */
#ifndef SIGBREAK
#define SIGBREAK 32 /* outside newlib's range; signal() just rejects it */
#endif

/* PSP OpenAL's alext.h predates AL_SOFT_MSADPCM; bflib_sndlib only uses these as tags. */
#ifndef AL_FORMAT_MONO_MSADPCM_SOFT
#define AL_FORMAT_MONO_MSADPCM_SOFT   0x1302
#define AL_FORMAT_STEREO_MSADPCM_SOFT 0x1303
#endif

#endif
