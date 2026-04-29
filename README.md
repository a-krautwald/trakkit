# trakkit

A terminal-based step sequencer / tracker for Linux, written in C.

---

## Dependencies

Install the required libraries before building:

```bash
# Debian / Ubuntu / Mint
sudo apt install build-essential libasound2-dev libncurses-dev

# Arch / Manjaro
sudo pacman -S base-devel alsa-lib ncurses

# Fedora
sudo dnf install gcc alsa-lib-devel ncurses-devel
```

---

## Building

From the project root:

```bash
gcc -o trakkit \
    main.c engine.c sample_bank.c project_io.c tui.c \
    -lasound -lncurses -lm -lpthread \
    -O2 -Wall -Wextra
```

Or if you have a `Makefile`:

```bash
make
```

A minimal `Makefile` if you don't have one yet:

```makefile
CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lasound -lncurses -lm -lpthread

SRCS = main.c engine.c sample_bank.c project_io.c tui.c
OBJ  = $(SRCS:.c=.o)

trakkit: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f trakkit $(OBJ)
```

---

## Sample directory layout

trakkit expects WAV files organised into 8 channel folders under a root directory.
Only **16-bit PCM WAV** files are supported.

```
samples/
  1/   kick.wav, snare.wav, ...
  2/   hihat_closed.wav, hihat_open.wav, ...
  3/   bass_c.wav, bass_d.wav, ...
  4/   ...
  5/   ...
  6/   ...
  7/   ...
  8/   ...
```

Each numbered folder maps to one channel (CH1–CH8).
Folders that don't exist are skipped — you don't need all 8.

---

## Running

```bash
# Use the default ./samples directory
./trakkit

# Use a custom sample root
./trakkit /path/to/my/samples
```

trakkit will print how many samples it loaded, then open the TUI.

---

## Interface

```
 ┌─────────────────────────────────────────────────────────────┐
 │ BPM:125  Step:00/64  Project:(unnamed)                      │
 │       |0   |4   |8   |12  |16  |20  |24  |28              │
 │ CH1   |X...X...|........|........|........  Smp:00/4 Lv:1.00│
 │ CH2   |....X...|........|........|........  Smp:00/2 Lv:1.00│
 │ ...                                                         │
 │ View:[00-31]                                                │
 │ Edit CH:1 STEP:00  Active:1  Smp:0  Note:+0  Vol:1.00 ...  │
 │ Status: Loaded samples root: samples                        │
 └─────────────────────────────────────────────────────────────┘
```

The grid shows 32 steps at a time. Steps are grouped in bars of 4.

**Step cell symbols:**

| Symbol | Meaning |
|--------|---------|
| `.`    | Empty step |
| `X`    | Active step (plain hit) |
| `N`    | Active step with a note offset |
| `F`    | Active step with an effect |

The **playing step** is highlighted in bold blue during playback.
The **cursor** is shown in reverse video.

---

## Keybindings

### Playback

| Key | Action |
|-----|--------|
| `Space` | Toggle play / stop |
| `,` | BPM down |
| `.` | BPM up |

### Navigation

| Key | Action |
|-----|--------|
| `←` `→` | Move cursor left / right (scrolls view) |
| `↑` `↓` | Move cursor up / down (channel) |

### Editing

| Key | Action |
|-----|--------|
| `X` | Toggle step on/off |
| `[` / `]` | Note offset down / up (semitones) |
| `-` / `+` | Step volume down / up |
| `N` / `B` | Next / previous sample on this channel |
| `<` / `>` | Channel level down / up |
| `M` | Toggle mute on current channel |
| `E` | Cycle effect: none → DIST → LPF → HPF → RVB |
| `;` / `'` | Effect amount down / up |

### Project

| Key | Action |
|-----|--------|
| `S` | Save to `project.trk` |
| `L` | Load from `project.trk` |
| `Q` | Quit |

---

## Effects

Effects are per-step and applied in the audio engine.

| Code | Name | Description |
|------|------|-------------|
| `---` | None | No effect |
| `DIST` | Distortion | Tanh waveshaper; amount controls drive |
| `LPF` | Low-pass filter | Simple one-pole; amount controls cutoff |
| `HPF` | High-pass filter | Simple one-pole; amount controls cutoff |
| `RVB` | Reverb | Feedback delay; amount controls wet/dry |

Effect amount is adjusted with `;` (down) and `'` (up), range 0.00–1.00.

---

## Project file

Projects are saved as plain text to `project.trk` in the working directory.
The format is human-readable and starts with the header `TRAKKIT1`.

```
TRAKKIT1
bpm=125
swing=0
name=my_groove
channel=0 selected=1 level=0.8000 mute=0
...
step ch=0 idx=0 sample=1 note=0 vol=1.0000 fx=0 fxamt=0.0000
...
```

Only active steps are written — empty steps are omitted.

---

## Notes

- Audio output uses ALSA (`default` device). If you use PipeWire or PulseAudio,
  the `default` ALSA device is usually bridged automatically. If you get an error,
  try `ALSA_CARD=0 ./trakkit` or install `pipewire-alsa`.
- 48000 Hz sample rate, stereo, 16-bit output, 256-frame buffer (~5ms latency).
- Up to 64 simultaneous voices.
- Steps: 64 per pattern. BPM range: 40–240.
