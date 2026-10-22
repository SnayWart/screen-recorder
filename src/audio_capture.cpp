#include "audio_capture.h"
#include "thread_utils.h"
#include <pulse/error.h>
#include <cstdio>
#include <array>
#include <algorithm>

AudioCapture::AudioCapture() {}

AudioCapture::~AudioCapture() {
    stop();
}

std::string AudioCapture::get_default_monitor_device() {
    std::string result;
    FILE *pipe = popen("pactl get-default-sink 2>/dev/null", "r");
    if (!pipe) {
        return result;
    }
    std::array<char, 256> buffer;
    if (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result = buffer.data();
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
            result.pop_back();
        }
    }
    pclose(pipe);
    if (!result.empty()) {
        result += ".monitor";
    }
    return result;
}

std::string AudioCapture::get_default_source_device() {
    std::string result;
    FILE *pipe = popen("pactl get-default-source 2>/dev/null", "r");
    if (!pipe) {
        return result;
    }
    std::array<char, 256> buffer;
    if (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result = buffer.data();
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
            result.pop_back();
        }
    }
    pclose(pipe);
    return result;
}

bool AudioCapture::init(const AudioConfig &cfg) {
    config_ = cfg;
    if (!config_.capture_desktop && !config_.capture_mic) {
        return true;
    }

    pa_sample_spec ss;
    ss.format = PA_SAMPLE_S16LE;
    ss.rate = config_.sample_rate;
    ss.channels = config_.channels;

    int error = 0;

    if (config_.capture_desktop) {
        std::string dev = config_.desktop_device;
        if (dev.empty()) {
            dev = get_default_monitor_device();
        }
        const char *dev_ptr = dev.empty() ? nullptr : dev.c_str();
        desktop_stream_ = pa_simple_new(nullptr, "ScreenRecorder", PA_STREAM_RECORD,
                                        dev_ptr, "DesktopAudio", &ss, nullptr, nullptr, &error);
        if (!desktop_stream_) {
            return false;
        }
    }

    if (config_.capture_mic) {
        std::string dev = config_.mic_device;
        if (dev.empty()) {
            dev = get_default_source_device();
        }
        const char *dev_ptr = dev.empty() ? nullptr : dev.c_str();
        mic_stream_ = pa_simple_new(nullptr, "ScreenRecorder", PA_STREAM_RECORD,
                                    dev_ptr, "Microphone", &ss, nullptr, nullptr, &error);
        if (!mic_stream_) {
            if (desktop_stream_) {
                pa_simple_free(desktop_stream_);
                desktop_stream_ = nullptr;
            }
            return false;
        }
    }

    return true;
}

void AudioCapture::start() {
    if (!config_.capture_desktop && !config_.capture_mic) {
        return;
    }
    is_running_ = true;
    worker_thread_ = std::thread(&AudioCapture::capture_thread_func, this);
}

void AudioCapture::stop() {
    if (is_running_) {
        is_running_ = false;
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }

    if (desktop_stream_) {
        pa_simple_free(desktop_stream_);
        desktop_stream_ = nullptr;
    }
    if (mic_stream_) {
        pa_simple_free(mic_stream_);
        mic_stream_ = nullptr;
    }

    std::lock_guard<std::mutex> lock(queue_mutex_);
    while (!queue_.empty()) {
        queue_.pop();
    }
    queue_cv_.notify_all();
}

void AudioCapture::capture_thread_func() {
    set_background_thread_affinity_and_priority();
    const size_t frames_per_read = 1024;
    const size_t samples_count = frames_per_read * config_.channels;
    const size_t bytes_to_read = samples_count * sizeof(int16_t);

    std::vector<int16_t> desktop_buf(samples_count);
    std::vector<int16_t> mic_buf(samples_count);
    std::vector<int16_t> mixed_buf(samples_count);

    while (is_running_) {
        int error = 0;
        bool have_desktop = false;
        bool have_mic = false;

        if (desktop_stream_) {
            if (pa_simple_read(desktop_stream_, desktop_buf.data(), bytes_to_read, &error) == 0) {
                have_desktop = true;
            }
        }

        if (mic_stream_) {
            if (pa_simple_read(mic_stream_, mic_buf.data(), bytes_to_read, &error) == 0) {
                have_mic = true;
            }
        }

        if (!have_desktop && !have_mic) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        if (have_desktop && have_mic) {
            for (size_t i = 0; i < samples_count; ++i) {
                int32_t mixed = static_cast<int32_t>(desktop_buf[i]) + static_cast<int32_t>(mic_buf[i]);
                mixed = std::clamp(mixed, -32768, 32767);
                mixed_buf[i] = static_cast<int16_t>(mixed);
            }
        } else if (have_desktop) {
            std::copy(desktop_buf.begin(), desktop_buf.end(), mixed_buf.begin());
        } else {
            std::copy(mic_buf.begin(), mic_buf.end(), mixed_buf.begin());
        }

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (queue_.size() >= max_queue_size_) {
                queue_.pop();
            }
            queue_.push(mixed_buf);
        }
        queue_cv_.notify_one();
    }
}

bool AudioCapture::pop_samples(std::vector<int16_t> &samples, int timeout_ms) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    if (queue_cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] {
        return !queue_.empty() || !is_running_;
    })) {
        if (!queue_.empty()) {
            samples = std::move(queue_.front());
            queue_.pop();
            return true;
        }
    }
    return false;
}
