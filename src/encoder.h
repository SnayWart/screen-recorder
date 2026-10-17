#ifndef RECORDER_ENCODER_H
#define RECORDER_ENCODER_H

#include <string>
#include <vector>
#include <cstdint>
#include <mutex>
#include <atomic>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

class MediaEncoder {
public:
    MediaEncoder();
    ~MediaEncoder();

    bool init(const std::string &filename, int width, int height, int fps,
              int crf, const std::string &preset, bool enable_audio,
              uint32_t sample_rate = 48000, uint32_t channels = 2);
    bool encode_video_frame(const uint8_t *src_data, int src_stride,
                            int64_t timestamp_us, bool y_invert);
    bool encode_audio_samples(const int16_t *samples, size_t count);
    void close();

    uint64_t get_bytes_written() const {
        if (format_ctx_ && format_ctx_->pb) {
            int64_t pos = avio_tell(format_ctx_->pb);
            if (pos > 0) {
                return static_cast<uint64_t>(pos);
            }
        }
        return bytes_written_.load();
    }
    uint64_t get_frames_encoded() const { return frames_encoded_.load(); }

private:
    bool init_video(int width, int height, int fps, int crf, const std::string &preset);
    bool init_audio(uint32_t sample_rate, uint32_t channels);
    void flush_encoders();

    AVFormatContext *format_ctx_{nullptr};
    AVStream *video_stream_{nullptr};
    AVCodecContext *video_codec_ctx_{nullptr};
    SwsContext *sws_ctx_{nullptr};
    AVFrame *video_frame_{nullptr};

    AVStream *audio_stream_{nullptr};
    AVCodecContext *audio_codec_ctx_{nullptr};
    SwrContext *swr_ctx_{nullptr};
    AVFrame *audio_frame_{nullptr};
    AVChannelLayout audio_ch_layout_{};

    std::vector<int16_t> audio_fifo_;
    int audio_frame_size_{1024};
    int64_t audio_samples_written_{0};

    int64_t start_time_us_{0};
    int64_t frame_index_{0};

    std::atomic<uint64_t> bytes_written_{0};
    std::atomic<uint64_t> frames_encoded_{0};
    std::mutex muxer_mutex_;
    bool is_open_{false};
};

#endif
