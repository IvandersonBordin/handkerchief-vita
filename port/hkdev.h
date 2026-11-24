// Developer tools switch. HK_DEVTOOLS=1 (CMake option HK_DEVTOOLS=ON, `make HK_DEVTOOLS=1` on the PC) builds the test
// aids: prof, hitch log, memlog, stress driver, heap trace, audio recorder, HK_TRACE_* switches, DS trace, [mix] lines,
// verbose diagnostics. HK_DEVTOOLS=0 (the player build, default for the Vita) compiles all of it out: rt_dlog() and
// HK_ENV() vanish with their arguments, so the format strings and variable names are not in the binary.
#pragma once
#include <stdlib.h>
#ifndef HK_DEVTOOLS
#define HK_DEVTOOLS 0
#endif
#if HK_DEVTOOLS
#define HK_ENV(name) getenv(name)
#else
#define HK_ENV(name) ((const char *)0)
#endif
// Console-verified improvements (preopen, userfast, preload_se, savefast): on by default in the player build, off by
// default in the developer build (as before); the environment variable (hkport.cfg key) overrides with 1 / 0.
static inline int hk_env_flag(const char *name, int dev_default) {
    const char *e = getenv(name);
    if (e && *e) return *e == '1';
    return HK_DEVTOOLS ? dev_default : 1;
}
