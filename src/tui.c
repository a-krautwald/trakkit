#define _POSIX_C_SOURCE 200809L

#include "tui.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#include "project_io.h"
#include "sample_bank.h"

#define VIEW_STEPS 32   /* how many steps visible at once */

static int cycle_index(int value, int delta, int max) {
    if (max <= 0) return 0;
    value += delta;
    while (value < 0)   value += max;
    while (value >= max) value -= max;
    return value;
}

static void draw_ui(TrackerProject *project, int cur_ch, int cur_step,
                    int view_start, char *status) {
    erase();

    /* ── help bar ── */
    mvprintw(0, 0, "SPC play  Arrows move  X step  N/B smp  [/] note  -/+ vol  </> ch vol");
    mvprintw(1, 0, "E fx  ;/' fx amt  M mute  S save  L load  ,/. bpm  Q quit");

    /* ── header ── */
    mvprintw(2, 0, "BPM:%-3d  Step:%02d/%02d  Project:%s",
             project->bpm,
             project->playing_step,
             TRACKER_STEPS,
             project->project_name[0] ? project->project_name : "(unnamed)");

    /* ── step-number ruler ── */
    mvprintw(3, 6, "");
    for (int s = 0; s < VIEW_STEPS; ++s) {
        int abs_step = view_start + s;
        int x = 6 + s + (s / 4);          /* gap every 4 steps */
        if (s % 4 == 0)
            mvaddch(3, x - 1, '|');
        if (abs_step % 8 == 0)
            mvprintw(3, x, "%-2d", abs_step);
        else
            mvaddch(3, x, ' ');
    }

    /* ── channel rows ── */
    for (int ch = 0; ch < TRACKER_CHANNELS; ++ch) {
        int row = 4 + ch;

        attron(COLOR_PAIR((ch % 6) + 1));
        mvprintw(row, 0, "CH%d%s ", ch + 1,
                 project->channels[ch].mute ? "M" : " ");
        attroff(COLOR_PAIR((ch % 6) + 1));

        for (int s = 0; s < VIEW_STEPS; ++s) {
            int abs_step = view_start + s;
            int x = 6 + s + (s / 4);

            if (s % 4 == 0)
                mvaddch(row, x - 1, '|');

            StepEvent *ev = &project->pattern.steps[ch][abs_step];

            /* playhead highlight */
            bool is_playing_step = (abs_step == project->playing_step);
            /* cursor highlight */
            bool is_cur
