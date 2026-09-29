/* PSP memory diagnostics (-DKFX_PSP_MEMDEBUG=ON).
 *
 * Wraps malloc/calloc/realloc/free (linker --wrap) and tracks every live
 * allocation with its caller. Large allocations are logged as they happen;
 * on the first allocation failure the live heap is dumped, grouped by caller,
 * to memdebug.log next to the EBOOT. Writes go through sceIo directly since
 * stdio would allocate and recurse into the wrappers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <psptypes.h>
#include <pspiofilemgr.h>

void *__real_malloc(size_t size);
void *__real_calloc(size_t n, size_t size);
void *__real_realloc(void *p, size_t size);
void __real_free(void *p);

#define MEMDEBUG_BIG   (128 * 1024)
#define TRACK_SLOTS    (1 << 17)   /* power of two */
#define CALLER_SLOTS   4096

struct track { void *ptr; unsigned size; void *caller; };
static struct track tracks[TRACK_SLOTS];
static int dumped;

static void log_line(const char *line, int n)
{
    static SceUID fd = -1;
    if (fd < 0)
        fd = sceIoOpen("memdebug.log", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (fd >= 0)
        sceIoWrite(fd, line, n);
}

static unsigned slot_of(void *p) { return ((unsigned)p >> 3) * 2654435761u & (TRACK_SLOTS - 1); }

static void track_add(void *p, unsigned size, void *caller)
{
    for (unsigned i = slot_of(p), n = 0; n < TRACK_SLOTS; i = (i + 1) & (TRACK_SLOTS - 1), n++) {
        if (tracks[i].ptr == NULL || tracks[i].ptr == (void *)1) {
            tracks[i].ptr = p; tracks[i].size = size; tracks[i].caller = caller;
            return;
        }
    }
}

static void track_del(void *p)
{
    for (unsigned i = slot_of(p), n = 0; n < TRACK_SLOTS; i = (i + 1) & (TRACK_SLOTS - 1), n++) {
        if (tracks[i].ptr == NULL)
            return;
        if (tracks[i].ptr == p) {
            tracks[i].ptr = (void *)1; /* tombstone */
            return;
        }
    }
}

static void dump_live(void)
{
    static struct { void *caller; unsigned bytes, count; } by_caller[CALLER_SLOTS];
    int used = 0;
    unsigned total = 0;
    for (unsigned i = 0; i < TRACK_SLOTS; i++) {
        if ((unsigned)tracks[i].ptr <= 1)
            continue;
        total += tracks[i].size;
        int j;
        for (j = 0; j < used; j++)
            if (by_caller[j].caller == tracks[i].caller) break;
        if (j == used) {
            if (used == CALLER_SLOTS) continue;
            by_caller[used].caller = tracks[i].caller; by_caller[used].bytes = 0; by_caller[used].count = 0;
            used++;
        }
        by_caller[j].bytes += tracks[i].size;
        by_caller[j].count++;
    }
    char line[128];
    int n = snprintf(line, sizeof(line), "=== live heap at first failure: %u bytes ===\n", total);
    log_line(line, n);
    for (int j = 0; j < used; j++) {
        if (by_caller[j].bytes < 16 * 1024) continue;
        n = snprintf(line, sizeof(line), "live %u x%u from %p\n", by_caller[j].bytes, by_caller[j].count, by_caller[j].caller);
        log_line(line, n);
    }
}

static void report(const char *what, size_t size, void *res, void *caller)
{
    if (res != NULL)
        track_add(res, size, caller);
    if (size >= MEMDEBUG_BIG || res == NULL) {
        struct mallinfo mi = mallinfo();
        char line[160];
        int n = snprintf(line, sizeof(line), "%s %u -> %p from %p (heap used %u, arena %u)\n",
                what, (unsigned)size, res, caller, (unsigned)mi.uordblks, (unsigned)mi.arena);
        log_line(line, n);
    }
    if (res == NULL && size > 0 && !dumped) {
        dumped = 1;
        dump_live();
    }
}

void *__wrap_malloc(size_t size)
{
    void *r = __real_malloc(size);
    report("malloc", size, r, __builtin_return_address(0));
    return r;
}

void *__wrap_calloc(size_t n, size_t size)
{
    void *r = __real_calloc(n, size);
    report("calloc", n * size, r, __builtin_return_address(0));
    return r;
}

void *__wrap_realloc(void *p, size_t size)
{
    void *r = __real_realloc(p, size);
    if (r != NULL && p != NULL)
        track_del(p);
    report("realloc", size, r, __builtin_return_address(0));
    return r;
}

void __wrap_free(void *p)
{
    if (p != NULL)
        track_del(p);
    __real_free(p);
}
