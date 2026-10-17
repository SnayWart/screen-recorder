#ifndef RECORDER_TYPES_H
#define RECORDER_TYPES_H

#include <cstdint>
#include <string>

enum class CaptureMode {
    FULL_SCREEN,
    REGION
};

struct RegionRect {
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;
};

struct VideoConfig {
    int width = 1920;
    int height = 1080;
    int fps = 75;
    int crf = 18;
    std::string preset = "veryfast";
};

struct AudioConfig {
    bool capture_desktop = true;
    bool capture_mic = false;
    std::string desktop_device = "";
    std::string mic_device = "";
    uint32_t sample_rate = 48000;
    uint32_t channels = 2;
};

struct RecorderConfig {
    VideoConfig video;
    AudioConfig audio;
    CaptureMode mode = CaptureMode::FULL_SCREEN;
    RegionRect region;
    std::string output_path = "";
};

struct RecorderStats {
    uint64_t frames_captured = 0;
    uint64_t frames_encoded = 0;
    uint64_t bytes_written = 0;
    double elapsed_seconds = 0.0;
    double current_fps = 0.0;
};

#endif
