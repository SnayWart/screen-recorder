#ifndef WAYLAND_CAPTURE_H
#define WAYLAND_CAPTURE_H

#include "types.h"
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <memory>
#include <wayland-client.h>
#include "screencopy_protocol.h"

struct CapturedFrame {
    int buffer_idx = -1;
    uint8_t *data = nullptr;
    size_t size = 0;
    int64_t timestamp_us = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t stride = 0;
    uint32_t format = 0;
    bool y_invert = false;
};

class WaylandCapture {
public:
    WaylandCapture();
    ~WaylandCapture();

    bool init(CaptureMode mode, RegionRect region, int target_fps);
    void start();
    void stop();
    bool pop_frame(std::shared_ptr<CapturedFrame> &frame, int timeout_ms = 100);
    void release_frame(std::shared_ptr<CapturedFrame> frame);

    int get_width() const { return width_; }
    int get_height() const { return height_; }
    int get_stride() const { return stride_; }
    uint32_t get_format() const { return format_; }

    void on_frame_buffer(uint32_t format, uint32_t width, uint32_t height, uint32_t stride);
    void on_frame_flags(uint32_t flags);
    void on_frame_ready(uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec);
    void on_frame_failed();
    void on_frame_buffer_done();

    void on_registry_global(struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);

private:
    struct ShmBuffer {
        struct wl_buffer *wl_buf = nullptr;
        void *data = nullptr;
        size_t size = 0;
    };

    bool setup_wayland();
    void cleanup_wayland();
    bool allocate_shm_buffer(ShmBuffer &buf, uint32_t width, uint32_t height, uint32_t stride, uint32_t format);
    void free_shm_buffer(ShmBuffer &buf);
    void capture_loop();

    CaptureMode mode_{CaptureMode::FULL_SCREEN};
    RegionRect region_{};
    int target_fps_{75};

    struct wl_display *display_{nullptr};
    struct wl_registry *registry_{nullptr};
    struct wl_shm *shm_{nullptr};
    struct zwlr_screencopy_manager_v1 *screencopy_manager_{nullptr};
    struct wl_output *output_{nullptr};

    uint32_t width_{0};
    uint32_t height_{0};
    uint32_t stride_{0};
    uint32_t format_{0};
    bool y_invert_{false};

    std::atomic<bool> frame_buffer_done_{false};
    std::atomic<bool> frame_ready_{false};
    std::atomic<bool> frame_failed_{false};

    static constexpr size_t kBufferPoolSize = 6;
    ShmBuffer buffers_[kBufferPoolSize];
    std::vector<int> free_buffer_indices_;

    std::atomic<bool> is_running_{false};
    std::thread capture_thread_;

    std::vector<std::shared_ptr<CapturedFrame>> free_frames_;
    std::queue<std::shared_ptr<CapturedFrame>> frame_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
};

#endif
