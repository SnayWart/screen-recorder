#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include "types.h"
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <pulse/simple.h>

class AudioCapture {
public:
    AudioCapture();
    ~AudioCapture();

    bool init(const AudioConfig &cfg);
    void start();
    void stop();
    bool pop_samples(std::vector<int16_t> &samples, int timeout_ms = 100);

    static std::string get_default_monitor_device();
    static std::string get_default_source_device();

private:
    void capture_thread_func();

    AudioConfig config_;
    std::atomic<bool> is_running_{false};
    std::thread worker_thread_;

    pa_simple *desktop_stream_{nullptr};
    pa_simple *mic_stream_{nullptr};

    std::queue<std::vector<int16_t>> queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    const size_t max_queue_size_{100};
};

#endif
