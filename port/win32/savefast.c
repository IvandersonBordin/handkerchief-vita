// savefast=1 (HK_SAVEFAST=1): cheaper save-file handling on slow storage (the Vita's memory card). Default off.
//  1. RAM read cache for the game's save slots (User/UserNN.HDK): the area loader re-opens and re-reads the slot at each
//     area change in tiny reads; a cached open is an fmemopen of a private copy (no open/fstat on ux0).
//     Every operation that can change the file invalidates its entry (open for write, delete, move, copy onto, commit).
//  2. Lazy backup: the game's save sequence is CopyFile(X, X.bak); write X.mck; Delete(X); Move(X.mck, X); Delete(X.bak).
//     The copy is only recorded. Delete(X) then turns into rename(X, X.bak) (one cheap operation that leaves exactly the
//     file the copy would have produced), and Delete(X.bak) just forgets the record. Any other access to X.bak, or a
//     write to X, first makes the record real (a true copy), so the on-disk state is never different from what the game
//     can observe. The final X is still replaced only by a completed file (the atomic path is untouched).
//  3. Save writes (and the in-place .ser/.mck files) get a large stdio buffer: one write per file instead of hundreds.
#include "win32.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

int hk_savefast_on(void) {
    static int on = -1;
    if (on < 0) on = hk_env_flag("HK_SAVEFAST", 0);
    return on;
}

// A save slot: ".../User/UserNN.HDK" (any case), possibly a host path with directories.
static int is_slot(const char *p) {
    size_t n = strlen(p);
    if (n < 15 || SDL_strcasecmp(p + n - 4, ".HDK")) return 0;
    const char *b = p + n - 10;   // "UserNN.HDK"
    return !SDL_strncasecmp(b, "User", 4) && b[4] >= '0' && b[4] <= '9' && b[5] >= '0' && b[5] <= '9' && (b == p || b[-1] == '/');
}

#define NCACHE 6
static struct { char path[600]; uint8_t *data; size_t n; } s_cache[NCACHE];
static unsigned s_cache_next;
static SDL_mutex *s_mx;
static void lock(void) { if (!s_mx) s_mx = SDL_CreateMutex(); SDL_LockMutex(s_mx); }
static void unlock(void) { SDL_UnlockMutex(s_mx); }

void hk_save_inval(const char *hp) {
    if (!hk_savefast_on()) return;
    lock();
    for (int i = 0; i < NCACHE; i++)
        if (s_cache[i].data && !SDL_strcasecmp(s_cache[i].path, hp)) { free(s_cache[i].data); s_cache[i].data = NULL; }
    unlock();
}
// A private malloc'd copy of the slot's bytes when `hp` is a save slot (read from disk once, then from RAM); NULL otherwise.
uint8_t *hk_save_cached_copy(const char *hp, size_t *n) {
    if (!hk_savefast_on() || !is_slot(hp)) return NULL;
    lock();
    uint8_t *out = NULL;
    int hit = -1, fresh = 0;
    for (int i = 0; i < NCACHE; i++) if (s_cache[i].data && !SDL_strcasecmp(s_cache[i].path, hp)) hit = i;
    if (hit < 0) {
        FILE *f = fopen(hp, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            uint8_t *d = sz > 0 && sz <= (4 << 20) ? malloc((size_t)sz) : NULL;
            if (d && fread(d, 1, (size_t)sz, f) == (size_t)sz) {
                hit = (int)(s_cache_next++ % NCACHE);
                free(s_cache[hit].data);
                snprintf(s_cache[hit].path, sizeof s_cache[hit].path, "%s", hp);
                s_cache[hit].data = d; s_cache[hit].n = (size_t)sz; fresh = 1;
            } else free(d);
            fclose(f);
        }
    }
    { static int logged_hit; if (hit >= 0 && !logged_hit && s_cache[hit].data && !fresh) { logged_hit = 1; rt_dlog("[savefast] first slot RAM hit: %s\n", hp); } }
    if (hit >= 0 && (out = malloc(s_cache[hit].n ? s_cache[hit].n : 1))) { memcpy(out, s_cache[hit].data, s_cache[hit].n); *n = s_cache[hit].n; }
    unlock();
    return out;
}

static int copy_whole(const char *from, const char *to) {
    FILE *a = fopen(from, "rb");
    if (!a) return 0;
    fseek(a, 0, SEEK_END);
    long sz = ftell(a);
    fseek(a, 0, SEEK_SET);
    uint8_t *d = malloc(sz > 0 ? (size_t)sz : 1);
    int ok = d && sz >= 0 && fread(d, 1, (size_t)sz, a) == (size_t)sz;
    fclose(a);
    if (ok) { FILE *b = fopen(to, "wb"); ok = b && fwrite(d, 1, (size_t)sz, b) == (size_t)sz; if (b && fclose(b)) ok = 0; }
    free(d);
    return ok;
}

#define NPEND 4
static struct { char src[600], bak[600]; int used; } s_pend[NPEND];
static int pend_find(const char *p, int want_bak) {
    for (int i = 0; i < NPEND; i++)
        if (s_pend[i].used && !SDL_strcasecmp(want_bak ? s_pend[i].bak : s_pend[i].src, p)) return i;
    return -1;
}
static int ends_bak(const char *p) { size_t n = strlen(p); return n > 4 && !SDL_strcasecmp(p + n - 4, ".bak"); }

// Makes the recorded copy real if `hp` is its source (about to be written) or its backup (about to be accessed).
// `as_writer`: a read-only access to the source needs nothing.
void hk_save_materialize(const char *hp, int src_writes) {
    if (!hk_savefast_on()) return;
    lock();
    int i = pend_find(hp, 1);
    if (i < 0 && src_writes) i = pend_find(hp, 0);
    if (i >= 0) {
        s_pend[i].used = 0;
        copy_whole(s_pend[i].src, s_pend[i].bak);
    }
    unlock();
}
// CopyFileA(from, to) when it is "X -> X.bak" of an existing file: only recorded. 1 = handled (success).
int hk_save_copy(const char *from, const char *to) {
    if (!hk_savefast_on() || !ends_bak(to) || strlen(to) != strlen(from) + 4 || SDL_strncasecmp(to, from, strlen(from))) return 0;
    struct stat st;
    if (stat(from, &st)) return 0;
    lock();
    int i = pend_find(to, 1);
    if (i < 0) for (i = 0; i < NPEND && s_pend[i].used; i++) ;
    int ok = i < NPEND;
    if (ok) { s_pend[i].used = 1; snprintf(s_pend[i].src, sizeof s_pend[i].src, "%s", from); snprintf(s_pend[i].bak, sizeof s_pend[i].bak, "%s", to); }
    unlock();
    return ok;
}
// DeleteFileA(hp): 1 = handled, *ok is the result.
int hk_save_delete(const char *hp, int *ok) {
    if (!hk_savefast_on()) return 0;
    hk_save_inval(hp);
    lock();
    int h = 0, i = pend_find(hp, 0);
    if (i >= 0) {   // delete the source of a recorded copy: it becomes the backup
        char bak[600];
        snprintf(bak, sizeof bak, "%s", s_pend[i].bak);
        s_pend[i].used = 0;
        remove(bak);
        if (rename(hp, bak)) { copy_whole(hp, bak); *ok = remove(hp) == 0; }
        else *ok = 1;
        h = 1;
    } else if ((i = pend_find(hp, 1)) >= 0) {   // delete the backup that was never made
        s_pend[i].used = 0;
        remove(hp);
        *ok = 1; h = 1;
    }
    unlock();
    return h;
}
// Large write buffer for save files (call right after fopen, before any I/O).
void hk_save_wbuf(FILE *f) { if (hk_savefast_on()) setvbuf(f, NULL, _IOFBF, 64 * 1024); }
// The source of a recorded copy is replaced through the atomic commit path (which deletes NAME.bak): forget the record.
void hk_save_drop(const char *src) {
    if (!hk_savefast_on()) return;
    lock();
    int i = pend_find(src, 0);
    if (i >= 0) s_pend[i].used = 0;
    unlock();
}
