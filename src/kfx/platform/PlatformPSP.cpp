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
#include <cstdio>
#include <cstring>
#include "post_inc.h"

/* Take all user memory for the newlib heap, keeping 2 MB back for thread
 * stacks and SDL/audio kernel allocations. */
PSP_HEAP_SIZE_KB(-2048);
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

int main(int argc, char *argv[])
{
    static char* args[64];
    int nargs = append_args_file(argc, argv, args, 63);
    args[nargs] = NULL;
    return kfxmain(nargs, args);
}
