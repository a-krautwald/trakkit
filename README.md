# Trakkit (C Linux Tracker MVP)

Trakkit is a terminal-based music tracker for Linux, written in C with:
- `ncurses` for an ASCII interface with color
- `ALSA` for audio output

## Features in this MVP

- 8 audio channels
- Pattern sequencer (`64` steps)
- Load WAV samples from `samples/1` to `samples/8` (folder `N` is channel `N`)
- Per-step:
  - sample trigger
  - pitch (`note` semitone offset)
  - volume
  - simple effect selection
- Save/load project to `project.trk`
- Basic effects in playback path:
  - distortion (soft clip)
  - low-pass filter
  - high-pass filter
  - simple reverb-style feedback delay

## Dependencies (Linux)

Install on Debian/Ubuntu:

```bash
sudo apt update
sudo apt install -y build-essential libasound2-dev libncurses-dev
```

## Build and run

```bash
make
make run
```

You can optionally pass a custom samples root:

```bash
./bin/trakkit /path/to/samples
```

The program expects subfolders `/1` ... `/8` under the chosen root.

## Controls

- `SPACE`: play/stop
- Arrow keys: move cursor
- `X`: toggle step on/off
- `N` / `B`: next/previous sample for current channel
- `[` / `]`: down/up pitch (semitones)
- `-` / `+`: decrease/increase step volume
- `<` / `>`: decrease/increase channel volume
- `E`: cycle effect (`---`, `DIST`, `LPF`, `HPF`, `RVB`)
- `;` / `'`: decrease/increase effect amount
- `,` / `.`: decrease/increase BPM
- `M`: mute/unmute current channel
- `S`: save project to `project.trk`
- `L`: load project from `project.trk`
- `Q`: quit

## Notes

- WAV loader currently expects uncompressed PCM 16-bit WAV.
- Tested target is Linux; ALSA is required at runtime.
