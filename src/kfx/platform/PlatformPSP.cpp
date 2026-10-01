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
#include "bflib_basics.h" // LbJustLog
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <pspkernel.h>
#include <psppower.h>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cstdlib>
#include <malloc.h>
#include <unistd.h>
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
    PSP_TRACE("SDL_Init video");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        PSP_TRACE("SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    PSP_TRACE("SDL_Init ok");
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

#if defined(KFX_PSP_TRACE)
/* Startup tracing for real hardware: each line is written and the file
 * closed immediately, so the last step survives a hang or crash. */
extern "C" void psp_trace(const char* fmt, ...)
{
    FILE* f = fopen("psp_trace.txt", "a");
    if (f == NULL)
        return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fputc('\n', f);
    fclose(f);
}
#endif

extern "C" void psp_phase(const char* name)
{
    static Uint64 last_ms = 0;
    const Uint64 now = SDL_GetTicks();
    LbJustLog("PSP load: %-22s at %6u ms (+%u ms)\n", name, (unsigned)now, (unsigned)(now - last_ms));
    last_ms = now;
}

/** Largest single allocation currently possible, found by bisection. */
static unsigned int largest_heap_block_kb(void)
{
    unsigned int lo = 0, hi = 64 * 1024;
    while (lo < hi) {
        unsigned int mid = (lo + hi + 1) / 2;
        // volatile + a write, or GCC folds the malloc/free pair into "success".
        unsigned char* volatile p = (unsigned char*)malloc((size_t)mid * 1024);
        if (p != NULL) { p[0] = 0; free(p); lo = mid; } else { hi = mid - 1; }
    }
    return lo;
}

/* Highest heap address, found once at boot: libcglue's sbrk limit isn't exported. */
static char* psp_heap_top;

static void find_heap_top(void)
{
    const unsigned int kb = largest_heap_block_kb();
    char* volatile p = (char*)malloc((size_t)kb * 1024);
    if (p != NULL) {
        p[0] = 0;
        psp_heap_top = p + (size_t)kb * 1024;
        free(p);
    }
}

/** Free heap bytes: free space inside the arena plus what sbrk can still hand out. */
extern "C" size_t psp_heap_free_bytes(void)
{
    struct mallinfo mi = mallinfo();
    size_t unsbrked = 0;
    char* brk = (char*)sbrk(0);
    if (psp_heap_top != NULL && brk != (char*)-1 && brk < psp_heap_top)
        unsbrked = (size_t)(psp_heap_top - brk);
    return (size_t)mi.fordblks + unsbrked;
}

extern "C" void psp_log_memory(const char* where)
{
    struct mallinfo mi = mallinfo();
    LbJustLog("PSP memory (%s): heap used %u KB, largest free block %u KB, system free %u KB\n",
        where, (unsigned)(mi.uordblks / 1024), largest_heap_block_kb(),
        (unsigned)(sceKernelTotalFreeMemSize() / 1024));
}

/** Rewrites psp_status.txt (closed each time, so it survives a HOME exit). */
extern "C" void psp_write_status(const char* text)
{
    FILE* f = fopen("psp_status.txt", "w");
    if (f == NULL)
        return;
    struct mallinfo mi = mallinfo();
    fprintf(f, "%s\nheap used %u KB, heap free %u KB, largest free block %u KB, system free %u KB\n", text,
        (unsigned)(mi.uordblks / 1024), (unsigned)(psp_heap_free_bytes() / 1024), largest_heap_block_kb(),
        (unsigned)(sceKernelTotalFreeMemSize() / 1024));
    fclose(f);
}

// Written before anything else, so there is a trace even if the game dies
// before keeperfx.log is created.
static void write_boot_report(void)
{
    FILE* f = fopen("psp_boot.txt", "w");
    if (f == NULL)
        return;
    fprintf(f, "KeeperFX PSP boot\n");
    fprintf(f, "system memory outside the heap: %u KB total, %u KB largest block\n",
        (unsigned)(sceKernelTotalFreeMemSize() / 1024), (unsigned)(sceKernelMaxFreeMemSize() / 1024));
#if defined(KFX_GAME_ON_HEAP)
    fprintf(f, "game state: %p (%u KB)\n", (void*)kfx_game_ptr, (unsigned)(sizeof(*kfx_game_ptr) / 1024));
#endif
    fclose(f);
    // The heap figures come from psp_log_memory() in keeperfx.log.
}

int main(int argc, char *argv[])
{
    find_heap_top();
    write_boot_report();
    // Homebrew starts at 222 MHz; the game needs the full 333 MHz.
    scePowerSetClockFrequency(333, 333, 166);
    static char* args[64];
    int nargs = append_args_file(argc, argv, args, 63);
    args[nargs] = NULL;
    return kfxmain(nargs, args);
}
