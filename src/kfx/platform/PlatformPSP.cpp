/******************************************************************************/
// Bullfrog Engine Emulation Library - for use to remake classic games like
// Syndicate Wars, Magic Carpet or Dungeon Keeper.
/******************************************************************************/
/** @file PlatformPSP.cpp
 *     Sony PSP platform services and process entry point.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "kfx/platform/PlatformPSP.h"
#include "platform.h" // kfxmain
#include "config.h"   // keeper_runtime_directory
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <pspkernel.h>
#include <psppower.h>
#include <cstdio>
#include <cstring>
#include "post_inc.h"

/* Take all user memory for the newlib heap except 4 MB kept back for thread
 * stacks and kernel-side allocations (audio, input, file I/O). A negative
 * heap size means "all but the threshold"; its magnitude is ignored, and the
 * default threshold left only ~0.7 MB free once the game had started. */
PSP_HEAP_SIZE_KB(-1);
PSP_HEAP_THRESHOLD_SIZE_KB(4096);
PSP_MAIN_THREAD_STACK_SIZE_KB(1024);

const char* PlatformPSP::GetOSVersion() const { return "PSP"; }

const char* PlatformPSP::GetUserPrefDir()
{
    // Settings live next to the EBOOT, like keeperfx.cfg.
    return keeper_runtime_directory;
}

bool PlatformPSP::VideoInit()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
        return false;
    atexit(SDL_Quit);
    return true;
}

/******************************************************************************/
// Process entry point; SDL_main.h wraps it with SDL's PSP startup (module
// info, exit callback thread, GPU init).

// The XMB passes no arguments, so extra command line options (e.g. "-level 1")
// can be put in keeperfx_args.txt next to the EBOOT, whitespace-separated.
static int append_args_file(int argc, char **argv, char **out, int max_args)
{
    int n = 0;
    for (; n < argc && n < max_args; n++)
        out[n] = argv[n];
    FILE* f = fopen("keeperfx_args.txt", "r");
    if (f == NULL)
        return n;
    static char buf[1024];
    size_t len = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[len] = '\0';
    for (char* tok = strtok(buf, " \t\r\n"); tok != NULL && n < max_args; tok = strtok(NULL, " \t\r\n"))
        out[n++] = tok;
    return n;
}

#if defined(KFX_GAME_ON_HEAP)
#include "game_legacy.h" // kfx_game_ptr
#endif

// Written before anything else, so there is a trace even if the game dies
// before keeperfx.log is created.
static void write_boot_report(void)
{
    FILE* f = fopen("psp_boot.txt", "w");
    if (f == NULL)
        return;
    fprintf(f, "KeeperFX PSP boot\n");
    fprintf(f, "free memory: %u KB total, %u KB largest block\n",
        (unsigned)(sceKernelTotalFreeMemSize() / 1024), (unsigned)(sceKernelMaxFreeMemSize() / 1024));
#if defined(KFX_GAME_ON_HEAP)
    fprintf(f, "game state: %p (%u KB)\n", (void*)kfx_game_ptr, (unsigned)(sizeof(*kfx_game_ptr) / 1024));
#endif
    fclose(f);
}

int main(int argc, char *argv[])
{
    write_boot_report();
    // Homebrew starts at 222 MHz; the game needs the full 333 MHz.
    scePowerSetClockFrequency(333, 333, 166);
    static char* args[64];
    int nargs = append_args_file(argc, argv, args, 63);
    args[nargs] = NULL;
    return kfxmain(nargs, args);
}
