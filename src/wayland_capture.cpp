#include "wayland_capture.h"
#include "thread_utils.h"
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <iostream>

static void cb_frame_buffer(void *data, struct zwlr_screencopy_frame_v1 *,
                            uint32_t format, uint32_t width, uint32_t height, uint32_t stride) {
    auto *cap = static_cast<WaylandCapture*>(data);
    cap->on_frame_buffer(format, width, height, stride);
}

static void cb_frame_flags(void *data, struct zwlr_screencopy_frame_v1 *, uint32_t flags) {
    auto *cap = static_cast<WaylandCapture*>(data);
    cap->on_frame_flags(flags);
}

static void cb_frame_ready(void *data, struct zwlr_screencopy_frame_v1 *,
                           uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec) {
    auto *cap = static_cast<WaylandCapture*>(data);
    cap->on_frame_ready(tv_sec_hi, tv_sec_lo, tv_nsec);
}

static void cb_frame_failed(void *data, struct zwlr_screencopy_frame_v1 *) {
    auto *cap = static_cast<WaylandCapture*>(data);
    cap->on_frame_failed();
}

static void cb_frame_damage(void *, struct zwlr_screencopy_frame_v1 *,
                            uint32_t, uint32_t, uint32_t, uint32_t) {
}

static void cb_frame_linux_dmabuf(void *, struct zwlr_screencopy_frame_v1 *,
                                  uint32_t, uint32_t, uint32_t) {
}

static void cb_frame_buffer_done(void *data, struct zwlr_screencopy_frame_v1 *) {
    auto *cap = static_cast<WaylandCapture*>(data);
    cap->on_frame_buffer_done();
}

static const struct zwlr_screencopy_frame_v1_listener screencopy_frame_listener = {
    cb_frame_buffer,
    cb_frame_flags,
    cb_frame_ready,
    cb_frame_failed,
    cb_frame_damage,
    cb_frame_linux_dmabuf,
    cb_frame_buffer_done,
};

static void cb_registry_global(void *data, struct wl_registry *registry,
                               uint32_t name, const char *interface, uint32_t version) {
    auto *cap = static_cast<WaylandCapture*>(data);
    cap->on_registry_global(registry, name, interface, version);
}

static void cb_registry_remove(void *, struct wl_registry *, uint32_t) {
}

static const struct wl_registry_listener registry_listener = {
    cb_registry_global,
    cb_registry_remove,
};

WaylandCapture::WaylandCapture() {
}

WaylandCapture::~WaylandCapture() {
    stop();
    cleanup_wayland();
}

void WaylandCapture::on_registry_global(struct wl_registry *registry, uint32_t name,
                                       const char *interface, uint32_t) {
    if (std::strcmp(interface, wl_shm_interface.name) == 0) {
        shm_ = static_cast<struct wl_shm*>(
            wl_registry_bind(registry, name, &wl_shm_interface, 1));
    } else if (std::strcmp(interface, zwlr_screencopy_manager_v1_interface.name) == 0) {
        screencopy_manager_ = static_cast<struct zwlr_screencopy_manager_v1*>(
            wl_registry_bind(registry, name, &zwlr_screencopy_manager_v1_interface, 3));
    } else if (std::strcmp(interface, wl_output_interface.name) == 0 && !output_) {
        output_ = static_cast<struct wl_output*>(
            wl_registry_bind(registry, name, &wl_output_interface, 1));
    }
}

void WaylandCapture::on_frame_buffer(uint32_t format, uint32_t width, uint32_t height, uint32_t stride) {
    format_ = format;
    width_ = width;
    height_ = height;
    stride_ = stride;
}

void WaylandCapture::on_frame_flags(uint32_t flags) {
    y_invert_ = (flags & ZWLR_SCREENCOPY_FRAME_V1_FLAGS_Y_INVERT) != 0;
}

void WaylandCapture::on_frame_ready(uint32_t, uint32_t, uint32_t) {
    frame_ready_ = true;
}

void WaylandCapture::on_frame_failed() {
    frame_failed_ = true;
}

void WaylandCapture::on_frame_buffer_done() {
    frame_buffer_done_ = true;
}

bool WaylandCapture::setup_wayland() {
    display_ = wl_display_connect(nullptr);
    if (!display_) {
        return false;
    }
    registry_ = wl_display_get_registry(display_);
    wl_registry_add_listener(registry_, &registry_listener, this);
    wl_display_roundtrip(display_);

    if (!shm_ || !screencopy_manager_ || !output_) {
        return false;
    }
    return true;
}

void WaylandCapture::cleanup_wayland() {
    for (size_t i = 0; i < kBufferPoolSize; ++i) {
        free_shm_buffer(buffers_[i]);
    }

    if (output_) {
        wl_output_destroy(output_);
        output_ = nullptr;
    }
    if (screencopy_manager_) {
        zwlr_screencopy_manager_v1_destroy(screencopy_manager_);
        screencopy_manager_ = nullptr;
    }
    if (shm_) {
        wl_shm_destroy(shm_);
        shm_ = nullptr;
    }
    if (registry_) {
        wl_registry_destroy(registry_);
        registry_ = nullptr;
    }
    if (display_) {
        wl_display_disconnect(display_);
        display_ = nullptr;
    }
}

bool WaylandCapture::allocate_shm_buffer(ShmBuffer &buf, uint32_t width, uint32_t height,
                                         uint32_t stride, uint32_t format) {
    buf.size = static_cast<size_t>(stride) * height;
    int fd = memfd_create("screencopy-buf", MFD_CLOEXEC);
    if (fd < 0) {
        return false;
    }
    if (ftruncate(fd, buf.size) < 0) {
        close(fd);
        return false;
    }
    buf.data = mmap(nullptr, buf.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buf.data == MAP_FAILED) {
        close(fd);
        buf.data = nullptr;
        return false;
    }
    struct wl_shm_pool *pool = wl_shm_create_pool(shm_, fd, buf.size);
    buf.wl_buf = wl_shm_pool_create_buffer(pool, 0, width, height, stride, format);
    wl_shm_pool_destroy(pool);
    close(fd);
    return buf.wl_buf != nullptr;
}

void WaylandCapture::free_shm_buffer(ShmBuffer &buf) {
    if (buf.wl_buf) {
        wl_buffer_destroy(buf.wl_buf);
        buf.wl_buf = nullptr;
    }
    if (buf.data && buf.data != MAP_FAILED) {
        munmap(buf.data, buf.size);
        buf.data = nullptr;
    }
    buf.size = 0;
}

bool WaylandCapture::init(CaptureMode mode, RegionRect region, int target_fps) {
    mode_ = mode;
    region_ = region;
    target_fps_ = target_fps > 0 ? target_fps : 60;

    if (!setup_wayland()) {
        return false;
    }

    struct zwlr_screencopy_frame_v1 *probe_frame = nullptr;
    if (mode_ == CaptureMode::REGION && region_.width > 0 && region_.height > 0) {
        probe_frame = zwlr_screencopy_manager_v1_capture_output_region(
            screencopy_manager_, 1, output_, region_.x, region_.y, region_.width, region_.height);
    } else {
        probe_frame = zwlr_screencopy_manager_v1_capture_output(
            screencopy_manager_, 1, output_);
    }

    frame_buffer_done_ = false;
    zwlr_screencopy_frame_v1_add_listener(probe_frame, &screencopy_frame_listener, this);

    while (!frame_buffer_done_ && wl_display_dispatch(display_) != -1) {
    }

    zwlr_screencopy_frame_v1_destroy(probe_frame);

    if (width_ == 0 || height_ == 0 || stride_ == 0) {
        return false;
    }

    for (size_t i = 0; i < kBufferPoolSize; ++i) {
        if (!allocate_shm_buffer(buffers_[i], width_, height_, stride_, format_)) {
            return false;
        }
    }

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        free_buffer_indices_.clear();
        free_frames_.clear();
        while (!frame_queue_.empty()) {
            frame_queue_.pop();
        }
        for (size_t i = 0; i < kBufferPoolSize; ++i) {
            free_buffer_indices_.push_back(static_cast<int>(i));
            auto cf = std::make_shared<CapturedFrame>();
            free_frames_.push_back(cf);
        }
    }

    return true;
}

void WaylandCapture::start() {
    is_running_ = true;
    capture_thread_ = std::thread(&WaylandCapture::capture_loop, this);
}

void WaylandCapture::stop() {
    if (is_running_) {
        is_running_ = false;
        if (capture_thread_.joinable()) {
            capture_thread_.join();
        }
    }

    std::lock_guard<std::mutex> lock(queue_mutex_);
    while (!frame_queue_.empty()) {
        frame_queue_.pop();
    }
    queue_cv_.notify_all();
}

void WaylandCapture::release_frame(std::shared_ptr<CapturedFrame> frame) {
    if (!frame) {
        return;
    }
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (frame->buffer_idx >= 0) {
        free_buffer_indices_.push_back(frame->buffer_idx);
        frame->buffer_idx = -1;
        frame->data = nullptr;
    }
    free_frames_.push_back(frame);
}

void WaylandCapture::capture_loop() {
    set_background_thread_affinity_and_priority();
    const int64_t frame_interval_us = 1000000 / target_fps_;
    auto next_frame_time = std::chrono::steady_clock::now();

    while (is_running_) {
        auto now = std::chrono::steady_clock::now();
        if (next_frame_time > now) {
            std::this_thread::sleep_until(next_frame_time);
        }
        next_frame_time += std::chrono::microseconds(frame_interval_us);
        now = std::chrono::steady_clock::now();
        if (next_frame_time < now) {
            next_frame_time = now + std::chrono::microseconds(frame_interval_us);
        }

        int buf_idx = -1;
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (!free_buffer_indices_.empty()) {
                buf_idx = free_buffer_indices_.back();
                free_buffer_indices_.pop_back();
            }
        }
        if (buf_idx < 0) {
            continue;
        }

        frame_ready_ = false;
        frame_failed_ = false;
        frame_buffer_done_ = false;

        struct zwlr_screencopy_frame_v1 *frame = nullptr;
        if (mode_ == CaptureMode::REGION && region_.width > 0 && region_.height > 0) {
            frame = zwlr_screencopy_manager_v1_capture_output_region(
                screencopy_manager_, 1, output_, region_.x, region_.y, region_.width, region_.height);
        } else {
            frame = zwlr_screencopy_manager_v1_capture_output(
                screencopy_manager_, 1, output_);
        }

        if (!frame) {
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                free_buffer_indices_.push_back(buf_idx);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        zwlr_screencopy_frame_v1_add_listener(frame, &screencopy_frame_listener, this);

        while (!frame_buffer_done_ && is_running_) {
            if (wl_display_dispatch(display_) == -1) {
                break;
            }
        }

        if (!is_running_) {
            zwlr_screencopy_frame_v1_destroy(frame);
            std::lock_guard<std::mutex> lock(queue_mutex_);
            free_buffer_indices_.push_back(buf_idx);
            break;
        }

        ShmBuffer &cur_buf = buffers_[buf_idx];

        zwlr_screencopy_frame_v1_copy(frame, cur_buf.wl_buf);
        wl_display_flush(display_);

        while (!frame_ready_ && !frame_failed_ && is_running_) {
            if (wl_display_dispatch(display_) == -1) {
                break;
            }
        }

        if (frame_ready_ && is_running_) {
            int64_t ts = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();

            std::shared_ptr<CapturedFrame> cf;
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                if (!free_frames_.empty()) {
                    cf = free_frames_.back();
                    free_frames_.pop_back();
                }
            }

            if (cf) {
                cf->buffer_idx = buf_idx;
                cf->data = static_cast<uint8_t*>(cur_buf.data);
                cf->size = cur_buf.size;
                cf->timestamp_us = ts;
                cf->width = width_;
                cf->height = height_;
                cf->stride = stride_;
                cf->format = format_;
                cf->y_invert = y_invert_;

                {
                    std::lock_guard<std::mutex> lock(queue_mutex_);
                    frame_queue_.push(cf);
                }
                queue_cv_.notify_one();
            } else {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                free_buffer_indices_.push_back(buf_idx);
            }
        } else {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            free_buffer_indices_.push_back(buf_idx);
        }

        zwlr_screencopy_frame_v1_destroy(frame);
    }
}

bool WaylandCapture::pop_frame(std::shared_ptr<CapturedFrame> &frame, int timeout_ms) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    if (queue_cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] {
        return !frame_queue_.empty() || !is_running_;
    })) {
        if (!frame_queue_.empty()) {
            frame = frame_queue_.front();
            frame_queue_.pop();
            return true;
        }
    }
    return false;
}
