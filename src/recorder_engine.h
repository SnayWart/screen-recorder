#ifndef RECORDER_ENGINE_H
#define RECORDER_ENGINE_H

#include "types.h"
#include "wayland_capture.h"
#include "audio_capture.h"
#include "encoder.h"
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>

class RecorderEngine {
public:
    RecorderEngine();
    ~RecorderEngine();

    bool start(const RecorderConfig &config);
    void stop();
    bool is_recording() const { return is_recording_.load(); }
    RecorderStats get_stats() const;

private:
    void video_encode_worker();
    void audio_encode_worker();

    RecorderConfig config_;
    std::atomic<bool> is_recording_{false};
    std::atomic<bool> stop_requested_{false};

    std::unique_ptr<WaylandCapture> wayland_cap_;
    std::unique_ptr<AudioCapture> audio_cap_;
    std::unique_ptr<MediaEncoder> encoder_;

    std::thread video_worker_;
    std::thread audio_worker_;

    std::chrono::steady_clock::time_point start_time_;
    std::atomic<uint64_t> frames_captured_count_{0};
    std::atomic<uint64_t> frames_encoded_count_{0};
};

#endif
