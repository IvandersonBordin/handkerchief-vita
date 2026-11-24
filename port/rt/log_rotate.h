#ifndef SH3_LOG_ROTATE_H
#define SH3_LOG_ROTATE_H

#include <stdio.h>
#include <string.h>

/* Previous launches are kept: the last one as NAME.prev.txt (hklog.prev.txt on the Vita, sh3log.prev.txt on PC),
 * older ones as NAME.txt.01 .. NAME.txt.18 (01 = the launch before .prev). A long console session must survive at
 * least one relaunch (an owner's log of a long play session was reported lost after the next launch).
 * Existence is tested by opening the file instead of trusting errno after remove/rename (the Vita's newlib maps
 * SCE error codes to errno, not verified here), and a failed rename falls back to a copy, so a session log is
 * never truncated before it was preserved. Returns 1 when the current log may be truncated, 0 when the caller
 * must append to it instead. */
static int sh3_log_exists(const char *p) { FILE *f = fopen(p, "rb"); if (!f) return 0; fclose(f); return 1; }
static int sh3_log_move(const char *src, const char *dst) {
    if (!sh3_log_exists(src)) return 1;
    remove(dst);
    if (rename(src, dst) == 0) return 1;
    FILE *in = fopen(src, "rb"), *out = in ? fopen(dst, "wb") : NULL;   // fallback: copy
    int ok = in && out;
    char buf[8192]; size_t n;
    while (ok && (n = fread(buf, 1, sizeof buf, in)) > 0) ok = fwrite(buf, 1, n, out) == n;
    if (in) fclose(in);
    if (out && fclose(out) != 0) ok = 0;
    return ok;
}
static void sh3_log_prev_name(const char *path, char *out, size_t cap) {
    size_t n = strlen(path);
    if (n > 4 && strcmp(path + n - 4, ".txt") == 0) snprintf(out, cap, "%.*s.prev.txt", (int)(n - 4), path);
    else snprintf(out, cap, "%s.prev", path);
}
static int sh3_log_rotate(const char *path) {
    char src[1100], dst[1100], prev[1100];
    if (strlen(path) > 1000) return 0;
    sh3_log_prev_name(path, prev, sizeof prev);
    snprintf(dst, sizeof dst, "%s.%02d", path, 18);
    remove(dst);
    for (int i = 17; i >= 1; --i) {   // older history: a failure here only loses an old log
        snprintf(src, sizeof src, "%s.%02d", path, i);
        snprintf(dst, sizeof dst, "%s.%02d", path, i + 1);
        sh3_log_move(src, dst);
    }
    snprintf(dst, sizeof dst, "%s.%02d", path, 1);
    if (!sh3_log_move(prev, dst)) return 0;
    return sh3_log_move(path, prev);
}
#endif
