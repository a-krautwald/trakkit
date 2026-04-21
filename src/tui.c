#include "tui.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#include "project_io.h"
#include "sample_bank.h"

static int cycle_index(int value, int delta, int max) {
    if (max <= 0) {
        return 0;
    }
    value += delta;
    while (value < 0) {
        value += max;
    }
    while (value >= max) {
        value -= max;
    }
    return value;
}

static void draw_ui(TrackerProject *project, int cur_ch, int cur_step, char *status) {
    erase();
    mvprintw(0, 0, "SPACE play | Arrows move | X step | N/B sample +/-1 | [ ] note | -/+ step vol | </> ch vol | E fx | ;/' fx amt");
    mvprintw(1, 0, "S save | L load | ,/. bpm | M mute ch | Q quit");
    mvprintw(2, 0, "BPM:%d  PlayStep:%02d  Project:%s", project->bpm, project->playing_step, project->project_name[0] ? project->project_name : "(unnamed)");

    for (int ch = 0; ch < TRACKER_CHANNELS; ++ch) {
        attron(COLOR_PAIR((ch % 6) + 1));
        mvprintw(4 + ch, 0, "CH%d%s ", ch + 1, project->channels[ch].mute ? "M" : " ");
        attroff(COLOR_PAIR((ch % 6) + 1));
        for (int step = 0; step < TRACKER_STEPS; ++step) {
            StepEvent *ev = &project->pattern.steps[ch][step];
            int x = 6 + step + (step / 4);
            if (step % 4 == 0) {
                mvaddch(4 + ch, x - 1, '|');
            }
            if (step == cur_step && ch == cur_ch) {
                attron(A_REVERSE);
            }
            if (ev->active) {
                mvaddch(4 + ch, x, 'X');
            } else {
                mvaddch(4 + ch, x, '.');
            }
            if (step == cur_step && ch == cur_ch) {
                attroff(A_REVERSE);
            }
        }
        mvprintw(4 + ch,
                 95,
                 "Smp:%02d/%02d ChLv:%.2f",
                 project->channels[ch].selected_sample,
                 project->channels[ch].sample_count,
                 project->channels[ch].level);
    }
    StepEvent *sel = &project->pattern.steps[cur_ch][cur_step];
    mvprintw(14,
             0,
             "Edit CH:%d STEP:%02d Active:%d Sample:%d Note:%d Vol:%.2f Fx:%s Amt:%.2f",
             cur_ch + 1,
             cur_step,
             sel->active ? 1 : 0,
             sel->sample_index,
             sel->note,
             sel->volume,
             sample_bank_effect_name(sel->effect),
             sel->effect_amount);
    mvprintw(15, 0, "Status: %s", status ? status : "");
    refresh();
}

int tui_run(TrackerProject *project, EngineState *engine, const char *sample_root) {
    int cur_ch = 0;
    int cur_step = 0;
    int running = 1;
    char status[256];
    snprintf(status, sizeof(status), "Loaded samples root: %s", sample_root);

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    start_color();
    use_default_colors();
    init_pair(1, COLOR_RED, -1);
    init_pair(2, COLOR_GREEN, -1);
    init_pair(3, COLOR_YELLOW, -1);
    init_pair(4, COLOR_BLUE, -1);
    init_pair(5, COLOR_MAGENTA, -1);
    init_pair(6, COLOR_CYAN, -1);

    while (running) {
        engine_lock(engine);
        draw_ui(project, cur_ch, cur_step, status);
        engine_unlock(engine);
        int c = getch();
        if (c == ERR) {
            napms(10);
            continue;
        }
        engine_lock(engine);
        StepEvent *ev = &project->pattern.steps[cur_ch][cur_step];
        switch (c) {
            case 'q':
            case 'Q':
                running = 0;
                break;
            case ' ':
                engine_set_playing(engine, !engine_is_playing(engine));
                snprintf(status, sizeof(status), "Playback: %s", engine_is_playing(engine) ? "ON" : "OFF");
                break;
            case KEY_UP:
                cur_ch = (cur_ch - 1 + TRACKER_CHANNELS) % TRACKER_CHANNELS;
                break;
            case KEY_DOWN:
                cur_ch = (cur_ch + 1) % TRACKER_CHANNELS;
                break;
            case KEY_LEFT:
                cur_step = (cur_step - 1 + TRACKER_STEPS) % TRACKER_STEPS;
                break;
            case KEY_RIGHT:
                cur_step = (cur_step + 1) % TRACKER_STEPS;
                break;
            case 'x':
            case 'X':
                ev->active = !ev->active;
                ev->sample_index = project->channels[cur_ch].selected_sample;
                snprintf(status, sizeof(status), "Step %d/%d %s", cur_ch + 1, cur_step, ev->active ? "enabled" : "disabled");
                break;
            case '[':
                ev->note--;
                snprintf(status, sizeof(status), "Note offset: %d", ev->note);
                break;
            case ']':
                ev->note++;
                snprintf(status, sizeof(status), "Note offset: %d", ev->note);
                break;
            case '-':
                ev->volume -= 0.05f;
                if (ev->volume < 0.0f) ev->volume = 0.0f;
                snprintf(status, sizeof(status), "Volume: %.2f", ev->volume);
                break;
            case '=':
            case '+':
                ev->volume += 0.05f;
                if (ev->volume > 1.0f) ev->volume = 1.0f;
                snprintf(status, sizeof(status), "Volume: %.2f", ev->volume);
                break;
            case 'e':
            case 'E':
                ev->effect = (StepEffectType)(((int)ev->effect + 1) % 5);
                ev->effect_amount = 0.5f;
                snprintf(status, sizeof(status), "Effect: %s", sample_bank_effect_name(ev->effect));
                break;
            case 's':
            case 'S':
                engine_set_playing(engine, 0);
                if (project_save(project, "project.trk") == 0) {
                    snprintf(status, sizeof(status), "Saved project.trk");
                } else {
                    snprintf(status, sizeof(status), "Save failed");
                }
                break;
            case 'l':
            case 'L':
                engine_set_playing(engine, 0);
                if (project_load(project, "project.trk") == 0) {
                    snprintf(status, sizeof(status), "Loaded project.trk");
                } else {
                    snprintf(status, sizeof(status), "Load failed");
                }
                break;
            case 'n':
            case 'N':
                project->channels[cur_ch].selected_sample =
                    cycle_index(project->channels[cur_ch].selected_sample, 1, project->channels[cur_ch].sample_count);
                snprintf(status, sizeof(status), "Selected sample ch%d: %d", cur_ch + 1, project->channels[cur_ch].selected_sample);
                break;
            case 'b':
            case 'B':
                project->channels[cur_ch].selected_sample =
                    cycle_index(project->channels[cur_ch].selected_sample, -1, project->channels[cur_ch].sample_count);
                snprintf(status, sizeof(status), "Selected sample ch%d: %d", cur_ch + 1, project->channels[cur_ch].selected_sample);
                break;
            case ',':
                if (project->bpm > 40) {
                    project->bpm--;
                }
                snprintf(status, sizeof(status), "BPM: %d", project->bpm);
                break;
            case '.':
                if (project->bpm < 240) {
                    project->bpm++;
                }
                snprintf(status, sizeof(status), "BPM: %d", project->bpm);
                break;
            case '<':
                project->channels[cur_ch].level -= 0.05f;
                if (project->channels[cur_ch].level < 0.0f) project->channels[cur_ch].level = 0.0f;
                snprintf(status, sizeof(status), "Channel %d level: %.2f", cur_ch + 1, project->channels[cur_ch].level);
                break;
            case '>':
                project->channels[cur_ch].level += 0.05f;
                if (project->channels[cur_ch].level > 2.0f) project->channels[cur_ch].level = 2.0f;
                snprintf(status, sizeof(status), "Channel %d level: %.2f", cur_ch + 1, project->channels[cur_ch].level);
                break;
            case ';':
                ev->effect_amount -= 0.05f;
                if (ev->effect_amount < 0.0f) ev->effect_amount = 0.0f;
                snprintf(status, sizeof(status), "Effect amount: %.2f", ev->effect_amount);
                break;
            case '\'':
                ev->effect_amount += 0.05f;
                if (ev->effect_amount > 1.0f) ev->effect_amount = 1.0f;
                snprintf(status, sizeof(status), "Effect amount: %.2f", ev->effect_amount);
                break;
            case 'm':
            case 'M':
                project->channels[cur_ch].mute = !project->channels[cur_ch].mute;
                snprintf(status, sizeof(status), "Channel %d mute: %s", cur_ch + 1, project->channels[cur_ch].mute ? "ON" : "OFF");
                break;
            default:
                break;
        }
        engine_unlock(engine);
    }

    endwin();
    return 0;
}
