#include "encoder.h"
#include <iostream>
#include <algorithm>

MediaEncoder::MediaEncoder() {
}

MediaEncoder::~MediaEncoder() {
    close();
}

bool MediaEncoder::init(const std::string &filename, int width, int height,
                        int fps, int crf, const std::string &preset,
                        bool enable_audio, uint32_t sample_rate, uint32_t channels) {
    close();

    avformat_alloc_output_context2(&format_ctx_, nullptr, nullptr, filename.c_str());
    if (!format_ctx_) {
        return false;
    }

    if (!init_video(width, height, fps, crf, preset)) {
        close();
        return false;
    }

    if (enable_audio) {
        if (!init_audio(sample_rate, channels)) {
            close();
            return false;
        }
    }

    if (!(format_ctx_->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&format_ctx_->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) {
            close();
            return false;
        }
    }

    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "movflags", "frag_keyframe+empty_moov+default_base_moof", 0);
    format_ctx_->max_interleave_delta = 0;

    if (avformat_write_header(format_ctx_, &opts) < 0) {
        av_dict_free(&opts);
        close();
        return false;
    }
    av_dict_free(&opts);

    is_open_ = true;
    start_time_us_ = 0;
    frame_index_ = 0;
    audio_samples_written_ = 0;
    bytes_written_ = 0;
    frames_encoded_ = 0;

    return true;
}

bool MediaEncoder::init_video(int width, int height, int fps, int crf, const std::string &preset) {
    int enc_w = (width / 2) * 2;
    int enc_h = (height / 2) * 2;

    const AVCodec *codec = avcodec_find_encoder_by_name("libx264");
    if (!codec) {
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    }
    if (!codec) {
        return false;
    }

    video_stream_ = avformat_new_stream(format_ctx_, nullptr);
    if (!video_stream_) {
        return false;
    }

    video_codec_ctx_ = avcodec_alloc_context3(codec);
    if (!video_codec_ctx_) {
        return false;
    }

    video_codec_ctx_->width = enc_w;
    video_codec_ctx_->height = enc_h;
    video_codec_ctx_->time_base = AVRational{1, fps};
    video_codec_ctx_->framerate = AVRational{fps, 1};
    video_codec_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;
    video_codec_ctx_->gop_size = fps * 2;
    video_codec_ctx_->max_b_frames = 0;
    video_codec_ctx_->thread_count = (fps <= 60) ? 2 : 3;

    av_opt_set(video_codec_ctx_->priv_data, "preset", preset.c_str(), 0);
    av_opt_set(video_codec_ctx_->priv_data, "tune", "zerolatency", 0);
    av_opt_set(video_codec_ctx_->priv_data, "crf", std::to_string(crf).c_str(), 0);

    if (format_ctx_->oformat->flags & AVFMT_GLOBALHEADER) {
        video_codec_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (avcodec_open2(video_codec_ctx_, codec, nullptr) < 0) {
        return false;
    }

    if (avcodec_parameters_from_context(video_stream_->codecpar, video_codec_ctx_) < 0) {
        return false;
    }

    video_stream_->time_base = video_codec_ctx_->time_base;

    video_frame_ = av_frame_alloc();
    if (!video_frame_) {
        return false;
    }
    video_frame_->format = video_codec_ctx_->pix_fmt;
    video_frame_->width = enc_w;
    video_frame_->height = enc_h;
    if (av_frame_get_buffer(video_frame_, 32) < 0) {
        return false;
    }

    int sws_flags = (crf <= 18) ? SWS_BICUBIC : SWS_FAST_BILINEAR;
    sws_ctx_ = sws_getContext(width, height, AV_PIX_FMT_BGR0,
                              enc_w, enc_h, AV_PIX_FMT_YUV420P,
                              sws_flags, nullptr, nullptr, nullptr);
    if (!sws_ctx_) {
        return false;
    }

    return true;
}

bool MediaEncoder::init_audio(uint32_t sample_rate, uint32_t channels) {
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!codec) {
        return false;
    }

    audio_stream_ = avformat_new_stream(format_ctx_, nullptr);
    if (!audio_stream_) {
        return false;
    }

    audio_codec_ctx_ = avcodec_alloc_context3(codec);
    if (!audio_codec_ctx_) {
        return false;
    }

    audio_codec_ctx_->bit_rate = 192000;
    audio_codec_ctx_->sample_rate = static_cast<int>(sample_rate);
    audio_codec_ctx_->sample_fmt = AV_SAMPLE_FMT_FLTP;
    av_channel_layout_default(&audio_codec_ctx_->ch_layout, static_cast<int>(channels));
    audio_codec_ctx_->time_base = AVRational{1, static_cast<int>(sample_rate)};

    if (format_ctx_->oformat->flags & AVFMT_GLOBALHEADER) {
        audio_codec_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (avcodec_open2(audio_codec_ctx_, codec, nullptr) < 0) {
        return false;
    }

    if (avcodec_parameters_from_context(audio_stream_->codecpar, audio_codec_ctx_) < 0) {
        return false;
    }

    audio_stream_->time_base = audio_codec_ctx_->time_base;

    audio_frame_size_ = audio_codec_ctx_->frame_size > 0 ? audio_codec_ctx_->frame_size : 1024;

    audio_frame_ = av_frame_alloc();
    if (!audio_frame_) {
        return false;
    }
    audio_frame_->nb_samples = audio_frame_size_;
    audio_frame_->format = audio_codec_ctx_->sample_fmt;
    av_channel_layout_copy(&audio_frame_->ch_layout, &audio_codec_ctx_->ch_layout);
    if (av_frame_get_buffer(audio_frame_, 0) < 0) {
        return false;
    }

    av_channel_layout_default(&audio_ch_layout_, static_cast<int>(channels));
    swr_alloc_set_opts2(&swr_ctx_,
                        &audio_codec_ctx_->ch_layout, AV_SAMPLE_FMT_FLTP, static_cast<int>(sample_rate),
                        &audio_ch_layout_, AV_SAMPLE_FMT_S16, static_cast<int>(sample_rate),
                        0, nullptr);
    if (!swr_ctx_ || swr_init(swr_ctx_) < 0) {
        return false;
    }

    return true;
}

bool MediaEncoder::encode_video_frame(const uint8_t *src_data, int src_stride,
                                      int64_t timestamp_us, bool y_invert) {
    if (!is_open_ || !video_codec_ctx_ || !sws_ctx_) {
        return false;
    }

    if (start_time_us_ == 0) {
        start_time_us_ = timestamp_us;
        frame_index_ = 0;
    }

    if (av_frame_make_writable(video_frame_) < 0) {
        return false;
    }

    const uint8_t *in_data[4] = { src_data, nullptr, nullptr, nullptr };
    int in_stride[4] = { src_stride, 0, 0, 0 };

    if (y_invert) {
        in_data[0] = src_data + static_cast<size_t>(src_stride) * (video_codec_ctx_->height - 1);
        in_stride[0] = -src_stride;
    }

    sws_scale(sws_ctx_, in_data, in_stride, 0, video_codec_ctx_->height,
              video_frame_->data, video_frame_->linesize);

    video_frame_->pts = frame_index_++;

    if (avcodec_send_frame(video_codec_ctx_, video_frame_) < 0) {
        return false;
    }

    AVPacket *pkt = av_packet_alloc();
    while (avcodec_receive_packet(video_codec_ctx_, pkt) == 0) {
        pkt->duration = 1;
        av_packet_rescale_ts(pkt, video_codec_ctx_->time_base, video_stream_->time_base);
        pkt->stream_index = video_stream_->index;

        {
            std::lock_guard<std::mutex> lock(muxer_mutex_);
            av_interleaved_write_frame(format_ctx_, pkt);
            bytes_written_ += pkt->size;
        }
        frames_encoded_++;
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);

    return true;
}

bool MediaEncoder::encode_audio_samples(const int16_t *samples, size_t count) {
    if (!is_open_ || !audio_codec_ctx_ || !swr_ctx_) {
        return false;
    }

    audio_fifo_.insert(audio_fifo_.end(), samples, samples + count);

    const int channels = audio_codec_ctx_->ch_layout.nb_channels;
    const size_t needed_samples = static_cast<size_t>(audio_frame_size_) * channels;

    AVPacket *pkt = av_packet_alloc();

    while (audio_fifo_.size() >= needed_samples) {
        if (av_frame_make_writable(audio_frame_) < 0) {
            break;
        }

        const uint8_t *in_ptr = reinterpret_cast<const uint8_t*>(audio_fifo_.data());
        swr_convert(swr_ctx_, audio_frame_->data, audio_frame_size_,
                    &in_ptr, audio_frame_size_);

        audio_frame_->pts = audio_samples_written_;
        audio_samples_written_ += audio_frame_size_;

        if (avcodec_send_frame(audio_codec_ctx_, audio_frame_) == 0) {
            while (avcodec_receive_packet(audio_codec_ctx_, pkt) == 0) {
                pkt->duration = audio_frame_size_;
                av_packet_rescale_ts(pkt, audio_codec_ctx_->time_base, audio_stream_->time_base);
                pkt->stream_index = audio_stream_->index;

                {
                    std::lock_guard<std::mutex> lock(muxer_mutex_);
                    av_interleaved_write_frame(format_ctx_, pkt);
                    bytes_written_ += pkt->size;
                }
                av_packet_unref(pkt);
            }
        }

        audio_fifo_.erase(audio_fifo_.begin(), audio_fifo_.begin() + needed_samples);
    }

    av_packet_free(&pkt);
    return true;
}

void MediaEncoder::flush_encoders() {
    if (!is_open_) {
        return;
    }

    AVPacket *pkt = av_packet_alloc();

    if (video_codec_ctx_) {
        avcodec_send_frame(video_codec_ctx_, nullptr);
        while (avcodec_receive_packet(video_codec_ctx_, pkt) == 0) {
            pkt->duration = 1;
            av_packet_rescale_ts(pkt, video_codec_ctx_->time_base, video_stream_->time_base);
            pkt->stream_index = video_stream_->index;
            {
                std::lock_guard<std::mutex> lock(muxer_mutex_);
                av_interleaved_write_frame(format_ctx_, pkt);
                bytes_written_ += pkt->size;
            }
            frames_encoded_++;
            av_packet_unref(pkt);
        }
    }

    if (audio_codec_ctx_) {
        avcodec_send_frame(audio_codec_ctx_, nullptr);
        while (avcodec_receive_packet(audio_codec_ctx_, pkt) == 0) {
            pkt->duration = audio_frame_size_;
            av_packet_rescale_ts(pkt, audio_codec_ctx_->time_base, audio_stream_->time_base);
            pkt->stream_index = audio_stream_->index;
            {
                std::lock_guard<std::mutex> lock(muxer_mutex_);
                av_interleaved_write_frame(format_ctx_, pkt);
                bytes_written_ += pkt->size;
            }
            av_packet_unref(pkt);
        }
    }

    av_packet_free(&pkt);
}

void MediaEncoder::close() {
    if (!is_open_) {
        return;
    }

    flush_encoders();

    if (format_ctx_) {
        av_write_trailer(format_ctx_);
    }

    if (video_frame_) {
        av_frame_free(&video_frame_);
    }
    if (sws_ctx_) {
        sws_freeContext(sws_ctx_);
        sws_ctx_ = nullptr;
    }
    if (video_codec_ctx_) {
        avcodec_free_context(&video_codec_ctx_);
    }

    if (audio_frame_) {
        av_frame_free(&audio_frame_);
    }
    if (swr_ctx_) {
        swr_free(&swr_ctx_);
    }
    if (audio_codec_ctx_) {
        avcodec_free_context(&audio_codec_ctx_);
    }
    av_channel_layout_uninit(&audio_ch_layout_);

    if (format_ctx_) {
        if (!(format_ctx_->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&format_ctx_->pb);
        }
        avformat_free_context(format_ctx_);
        format_ctx_ = nullptr;
    }

    audio_fifo_.clear();
    is_open_ = false;
    frame_index_ = 0;
    start_time_us_ = 0;
}
