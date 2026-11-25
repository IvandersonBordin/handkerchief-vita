// What the platform layer needs to draw the game's Win32 dialogs and its menu bar crisply at 960x544 (port/vita/dlg_menu.h),
// in the style of the pause menu. port/hk/dialog.c fills it from the dialog on top (the game's dialog procedures still
// run unchanged and own all state); the platform only draws it. Plain data, no pointers, so two views compare with memcmp.
#ifndef HK_DLGVIEW_H
#define HK_DLGVIEW_H
#include <stdint.h>
enum { DV_NONE, DV_FORM, DV_SLOTS, DV_MENU };
enum { RK_TEXT, RK_SECTION, RK_COLHEAD, RK_CHECK, RK_RADIO, RK_VALUE, RK_EDIT, RK_BUTTON };
#define DV_ROWS 28
#define DV_DROP 9
typedef struct {
    uint8_t kind, ncell, enabled, checked;   // ncell: value cells of RK_VALUE / RK_COLHEAD (1 or 2)
    int8_t focus;                            // focused cell, -1 none
    int16_t ctl[2];                          // controls behind the cells (-1: none), for navigation
    char label[128];
    char val[2][80];
} DvRow;
typedef struct { char label[32]; uint8_t focus, enabled; uint16_t id; } DvBtn;
typedef struct { char name[64], date[48], area[12]; uint8_t sel, empty, enabled; } DvSlot;
typedef struct {
    int kind;
    char title[96];                // heading (the group box text, or the window caption)
    char hint[200];                // pad / key hints line
    int nrows; DvRow row[DV_ROWS];
    int nbtn; DvBtn btn[4];        // OK / Cancel / Reset (forms); Load-Save / Cancel / Rename / Delete (slots)
    int nslots; DvSlot slot[10];
    char note[160];                // slots: the static line under the table
    int drop, dropn, dropsel;      // an open combo list: a window of up to DV_DROP items, the selected one in it
    char dropitem[DV_DROP][40];
    int ntop, topsel, open, nitems, itemsel;   // menu bar
    char top[10][32]; uint8_t topgray[10];
    struct { char text[48], key[16]; uint8_t gray, sep; } item[24];
} HkDlgView;
// port/hk/dialog.c. Returns 1 and fills *v (zeroed first) when a dialog or the menu bar is up and the port draws it
// itself (hk_dlg_set_pretty(1)); 0 otherwise.
int hk_dlg_view(HkDlgView *v);
void hk_dlg_set_pretty(int on);   // the platform draws dialogs itself: dialog.c stops compositing its own 640x480 layer
// Canonical content hash of a view: every meaningful field, strings only up to their NUL, so bytes after a terminator
// (or padding) can never make an unchanged view look changed. The platform rebuilds its overlay only when it differs.
#define DVH_N(h, x) do { uint32_t v_ = (uint32_t)(x); (h) = ((h) ^ v_) * 1099511628211ull; } while (0)
#define DVH_S(h, str) do { const char *p_ = (str); while (*p_) DVH_N(h, (unsigned char)*p_++); DVH_N(h, 0x100); } while (0)
static inline uint64_t hk_dlgview_hash(const HkDlgView *v) {
    uint64_t h = 1469598103934665603ull;
    DVH_N(h, v->kind); DVH_S(h, v->title); DVH_S(h, v->hint); DVH_S(h, v->note);
    DVH_N(h, v->nrows);
    for (int i = 0; i < v->nrows && i < DV_ROWS; i++) {
        const DvRow *r = &v->row[i];
        DVH_N(h, r->kind); DVH_N(h, r->ncell); DVH_N(h, r->enabled); DVH_N(h, r->checked); DVH_N(h, r->focus);
        DVH_N(h, r->ctl[0]); DVH_N(h, r->ctl[1]); DVH_S(h, r->label); DVH_S(h, r->val[0]); DVH_S(h, r->val[1]);
    }
    DVH_N(h, v->nbtn);
    for (int i = 0; i < v->nbtn && i < 4; i++) { DVH_S(h, v->btn[i].label); DVH_N(h, v->btn[i].focus); DVH_N(h, v->btn[i].enabled); DVH_N(h, v->btn[i].id); }
    DVH_N(h, v->nslots);
    for (int i = 0; i < v->nslots && i < 10; i++) {
        const DvSlot *s = &v->slot[i];
        DVH_S(h, s->name); DVH_S(h, s->date); DVH_S(h, s->area); DVH_N(h, s->sel); DVH_N(h, s->empty); DVH_N(h, s->enabled);
    }
    DVH_N(h, v->drop); DVH_N(h, v->dropn); DVH_N(h, v->dropsel);
    for (int i = 0; i < DV_DROP; i++) DVH_S(h, v->dropitem[i]);
    DVH_N(h, v->ntop); DVH_N(h, v->topsel); DVH_N(h, v->open); DVH_N(h, v->nitems); DVH_N(h, v->itemsel);
    for (int i = 0; i < v->ntop && i < 10; i++) { DVH_S(h, v->top[i]); DVH_N(h, v->topgray[i]); }
    for (int i = 0; i < v->nitems && i < 24; i++) { DVH_S(h, v->item[i].text); DVH_S(h, v->item[i].key); DVH_N(h, v->item[i].gray); DVH_N(h, v->item[i].sep); }
    return h;
}
#endif
