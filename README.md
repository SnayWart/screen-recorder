# screen-recorder

> **Ultra-lightweight, zero-stutter Wayland screen recorder with a minimalist native Qt6 GUI.**  
> *Engineered from scratch in C++ for maximum gaming responsiveness and perfectly smooth 75 / 60 FPS video playback.*

[![Language](https://img.shields.io/badge/Language-C%2B%2B17%20%2F%20C-blue.svg)](#)
[![Wayland](https://img.shields.io/badge/Protocol-wlr--screencopy--v1-orange.svg)](#)
[![GUI](https://img.shields.io/badge/GUI-Qt6%20Widgets-green.svg)](#)
[![Multimedia](https://img.shields.io/badge/Encoding-FFmpeg%20%7C%20libx264-red.svg)](#)
[![License](https://img.shields.io/badge/License-MIT-purple.svg)](LICENSE)

---

## Why This Project?

### *Optimized for CPU — Especially If Your GPU Lacks a Hardware Encoder*

Many modern graphics cards (such as the **AMD Radeon RX 6400** and **RX 6500 XT**, Navi 24) physically **lack a hardware video encoder (VCN)**. On these GPUs, recording video with heavy tools like OBS Studio forces software encoding that often overwhelms the CPU, triggers `xdg-desktop-portal` DMA-BUF renegotiation loops, and causes severe in-game frame drops (e.g. dropping Roblox or competitive games from 240 FPS down to 170 FPS, with choppy video playback).

**screen-recorder** was specifically architected to solve this exact problem:

* **Surgical CPU Efficiency**: Encodes 1080p @ 75 FPS on CPU (`libx264`) while keeping game framerates rock-solid at uncapped **240 FPS**.
* **Zero Overhead**: Direct native Wayland client communication without portal layers, Electron, or heavy frameworks.
* **Compact Footprint**: Standalone native binary of just **~140 KB**.

---

## Key Features & Architecture

* **Direct `wlr-screencopy-unstable-v1` Protocol**  
  Bypasses desktop portal abstractions and talks directly to the Wayland compositor (Hyprland, Sway, River, etc.), pulling frames straight through POSIX shared memory (`wl_shm`).

* **Pre-Allocated 12-Frame Buffer Pool**  
  Zero heap memory allocation (`malloc`/`free`) during recording. Memory thrashing and GC-like pauses are completely eliminated.

* **Strict CFR Sequential PTS Engine**  
  Instead of relying on jittery system clocks or variable timestamps, the internal pipeline enforces strict Constant Frame Rate (CFR) sequential PTS generation (`frame_index++ * time_base`). Videos play back at a buttery-smooth 75.000 / 60.000 FPS without frame pacing judder.

* **Thread-Bounded Nice Scheduling**  
  The encoding worker is pinned to 3 threads and demoted to `nice +10` (`setpriority(PRIO_PROCESS, 0, 10)`). Your games maintain full CPU priority and cache access on the remaining CPU cores.

* **Bicubic Chroma Filtering (`SWS_BICUBIC`) & High Fidelity**  
  High-precision bicubic color subsampling preserves razor-sharp text, UI borders, and textures when converting desktop BGRA to MP4 YUV420P.

* **Dual-Stream Audio Mixer**  
  Simultaneous desktop audio (default monitor sink) and microphone capture via low-latency `libpulse-simple` (compatible with PulseAudio and PipeWire).

* **Minimalist Qt6 GUI**  
  Clean, native window with:
  * Fullscreen monitor capture or interactive area selection via `slurp`.
  * Desktop audio and microphone toggles.
  * FPS selector (75 / 60 / 30 FPS) and Quality selector (CRF 16 / 18 / 22 / 28).
  * System tray icon with live recording indicator.
  * Instant Rofi / application menu integration.

---

## Performance Benchmark

Tested on **AMD Ryzen 5 3600 (6C/12T) + AMD Radeon RX 6400 (Navi 24, no HW encoder)** under **Hyprland / Artix Linux**:

| Metric | Without Recording | During Recording (1080p @ 75 FPS, CRF 18) |
| :--- | :---: | :---: |
| **Roblox (Uncapped) FPS** | **240 FPS** | **240 FPS (Solid)** |
| **Video Playback FPS** | — | **74.97 / 75.00 FPS (Smooth)** |
| **CPU Usage (Recorder)** | 0% | ~18-22% (Across 3 threads) |
| **Binary Size** | — | **~143 KB** |

---

## Dependencies

Ensure the following development libraries are installed on your distribution:

### Arch Linux / Artix Linux
```bash
sudo pacman -S base-devel qt6-base ffmpeg libpulse wayland wayland-protocols slurp
```

### Fedora
```bash
sudo dnf install gcc-c++ qt6-qtbase-devel ffmpeg-free-devel pulseaudio-libs-devel wayland-devel wayland-protocols-devel slurp
```

### Ubuntu / Debian (22.04+)
```bash
sudo apt install build-essential qt6-base-dev libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev libpulse-dev libwayland-dev wayland-protocols slurp
```

---

## Building and Installation

Clone the repository and build using standard `make`:

```bash
git clone https://github.com/SnayWart/screen-recorder.git
cd screen-recorder
make -j$(nproc)
```

To install the binary and desktop launcher for your user (`~/.local/bin` and `~/.local/share/applications`):

```bash
make install
```

*(Optional)* To install system-wide to `/usr/local`:
```bash
sudo make PREFIX=/usr/local install
```

To uninstall:
```bash
make uninstall
```

---

## Usage

1. Launch **Screen Recorder** from your application launcher (`rofi`, `wofi`, `fuzzel`, or desktop menu) or run `screen-recorder` in your terminal.
2. Select capture mode:
   * **Весь экран**: Captures your primary Wayland monitor.
   * **Выбрать область**: Click and drag a rectangle on your screen via `slurp`.
3. Configure audio (Desktop Audio / Microphone) and Video Quality (CRF 18 recommended for crisp gaming).
4. Click **Начать запись** (Start Recording).
5. Stop recording anytime using the GUI button or via the system tray menu. Videos are automatically saved to your `~/Videos` directory.

---

## Project Structure

```
screen-recorder/
├── Makefile                # Fast multi-threaded build & install script
├── protocol/               # wlr-screencopy Wayland XML protocol definition
├── src/
│   ├── audio_capture.cpp   # PulseAudio / PipeWire capture & downmixing
│   ├── audio_capture.h
│   ├── encoder.cpp         # libavcodec x264 CFR encoder with bicubic scaling
│   ├── encoder.h
│   ├── gui_window.cpp      # Minimalist Qt6 graphical interface & tray
│   ├── gui_window.h
│   ├── recorder_engine.cpp # Coordinator, threading, and priority management
│   ├── recorder_engine.h
│   ├── screencopy_protocol.c # Generated Wayland screencopy protocol
│   ├── screencopy_protocol.h
│   ├── types.h             # Core configuration and stats structures
│   ├── wayland_capture.cpp # Native SHM buffer pool capture engine
│   ├── wayland_capture.h
│   └── main.cpp            # Entry point and signal handling
├── screen-recorder.desktop # Desktop application launcher specification
├── LICENSE                 # MIT License
└── README.md
```

---

## License

This project is licensed under the [MIT License](LICENSE).
