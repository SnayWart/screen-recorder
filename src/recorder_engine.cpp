#include "recorder_engine.h"
#include "thread_utils.h"
#include <sys/resource.h>

RecorderEngine::RecorderEngine() {
}

RecorderEngine::~RecorderEngine() {
    stop();
}

bool RecorderEngine::start(const RecorderConfig &config) {
    if (is_recording_) {
        return false;
    }

    config_ = config;
    stop_requested_ = false;

    wayland_cap_ = std::make_unique<WaylandCapture>();
    if (!wayland_cap_->init(config_.mode, config_.region, config_.video.fps)) {
        wayland_cap_.reset();
        return false;
    }

    int actual_w = wayland_cap_->get_width();
    int actual_h = wayland_cap_->get_height();

    bool enable_audio = config_.audio.capture_desktop || config_.audio.capture_mic;
    if (enable_audio) {
        audio_cap_ = std::make_unique<AudioCapture>();
        if (!audio_cap_->init(config_.audio)) {
            audio_cap_.reset();
            enable_audio = false;
        }
    }

    encoder_ = std::make_unique<MediaEncoder>();
    if (!encoder_->init(config_.output_path, actual_w, actual_h,
                        config_.video.fps, config_.video.crf,
                        config_.video.preset, enable_audio,
                        config_.audio.sample_rate, config_.audio.channels)) {
        encoder_.reset();
        if (audio_cap_) {
            audio_cap_->stop();
            audio_cap_.reset();
        }
        wayland_cap_->stop();
        wayland_cap_.reset();
        return false;
    }

    is_recording_ = true;
    start_time_ = std::chrono::steady_clock::now();
    frames_captured_count_ = 0;
    frames_encoded_count_ = 0;

    wayland_cap_->start();
    if (audio_cap_) {
        audio_cap_->start();
    }

    video_worker_ = std::thread(&RecorderEngine::video_encode_worker, this);
    if (enable_audio) {
        audio_worker_ = std::thread(&RecorderEngine::audio_encode_worker, this);
    }

    return true;
}

void RecorderEngine::stop() {
    if (!is_recording_) {
        return;
    }

    stop_requested_ = true;

    if (wayland_cap_) {
        wayland_cap_->stop();
    }
    if (audio_cap_) {
        audio_cap_->stop();
    }

    if (video_worker_.joinable()) {
        video_worker_.join();
    }
    if (audio_worker_.joinable()) {
        audio_worker_.join();
    }

    if (encoder_) {
        encoder_->close();
        encoder_.reset();
    }

    if (audio_cap_) {
        audio_cap_.reset();
    }
    if (wayland_cap_) {
        wayland_cap_.reset();
    }

    is_recording_ = false;
}

void RecorderEngine::video_encode_worker() {
    set_background_thread_affinity_and_priority();
    std::shared_ptr<CapturedFrame> cf;
    while (!stop_requested_) {
        if (wayland_cap_ && wayland_cap_->pop_frame(cf, 50)) {
            frames_captured_count_++;
            encoder_->encode_video_frame(cf->data, cf->stride,
                                         cf->timestamp_us, cf->y_invert);
            frames_encoded_count_++;
            wayland_cap_->release_frame(cf);
        }
    }
    while (wayland_cap_ && wayland_cap_->pop_frame(cf, 10)) {
        frames_captured_count_++;
        encoder_->encode_video_frame(cf->data, cf->stride,
                                     cf->timestamp_us, cf->y_invert);
        frames_encoded_count_++;
        wayland_cap_->release_frame(cf);
    }
}

void RecorderEngine::audio_encode_worker() {
    set_background_thread_affinity_and_priority();
    std::vector<int16_t> samples;
    while (!stop_requested_) {
        if (audio_cap_ && audio_cap_->pop_samples(samples, 50)) {
            encoder_->encode_audio_samples(samples.data(), samples.size());
        }
    }
    while (audio_cap_ && audio_cap_->pop_samples(samples, 10)) {
        encoder_->encode_audio_samples(samples.data(), samples.size());
    }
}

RecorderStats RecorderEngine::get_stats() const {
    RecorderStats stats;
    if (!is_recording_) {
        return stats;
    }

    auto now = std::chrono::steady_clock::now();
    stats.elapsed_seconds = std::chrono::duration<double>(now - start_time_).count();
    stats.frames_captured = frames_captured_count_.load();
    stats.frames_encoded = frames_encoded_count_.load();
    if (stats.elapsed_seconds > 0.05) {
        stats.current_fps = static_cast<double>(stats.frames_encoded) / stats.elapsed_seconds;
    }
    if (encoder_) {
        stats.bytes_written = encoder_->get_bytes_written();
    }

    return stats;
}
