// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "PlatformDecoder.h"

#include "miniaudio.h"

#include <algorithm>

#if defined(__APPLE__)
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#elif defined(JX_HAVE_FFMPEG)
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}
#endif

namespace jx {

bool convertToStereo(const float* in, size_t frames, int channels, int rate, double sampleRate, std::vector<float>& out) {
    out.clear();
    if (frames == 0 || channels <= 0 || rate <= 0) return false;
    if (channels == 2 && rate == static_cast<int>(sampleRate)) {
        out.assign(in, in + frames * 2);
        return true;
    }
    ma_data_converter_config cfg = ma_data_converter_config_init(ma_format_f32, ma_format_f32, static_cast<ma_uint32>(channels), 2,
                                                                 static_cast<ma_uint32>(rate), static_cast<ma_uint32>(sampleRate));
    cfg.resampling.linear.lpfOrder = 8;
    ma_data_converter conv;
    if (ma_data_converter_init(&cfg, nullptr, &conv) != MA_SUCCESS) return false;
    out.reserve(static_cast<size_t>(frames * (sampleRate / rate) + 64) * 2);
    const size_t chunk = 8192;
    std::vector<float> buf((static_cast<size_t>(chunk * (sampleRate / rate)) + 256) * 2);
    size_t pos = 0;
    while (pos < frames) {
        ma_uint64 inN = std::min(chunk, frames - pos);
        ma_uint64 outN = buf.size() / 2;
        ma_data_converter_process_pcm_frames(&conv, in + pos * channels, &inN, buf.data(), &outN);
        out.insert(out.end(), buf.begin(), buf.begin() + static_cast<long>(outN * 2));
        if (inN == 0 && outN == 0) break;
        pos += inN;
    }
    ma_data_converter_uninit(&conv, nullptr);
    return !out.empty();
}

#if defined(__APPLE__)

const char* platformDecoderName() { return "AudioToolbox"; }

bool platformDecode(const QString& path, double sampleRate, std::vector<float>& stereo, QString* error) {
    stereo.clear();
    const QByteArray utf8 = path.toUtf8();
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(nullptr, reinterpret_cast<const UInt8*>(utf8.constData()),
                                                           utf8.size(), false);
    if (!url) return false;
    ExtAudioFileRef file = nullptr;
    OSStatus st = ExtAudioFileOpenURL(url, &file);
    CFRelease(url);
    if (st != noErr || !file) {
        if (error) *error = QStringLiteral("AudioToolbox no pudo abrir el archivo");
        return false;
    }
    // Formato de salida: float32 estéreo intercalado a la frecuencia del motor (ExtAudioFile remuestrea)
    AudioStreamBasicDescription client{};
    client.mSampleRate = sampleRate;
    client.mFormatID = kAudioFormatLinearPCM;
    client.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    client.mChannelsPerFrame = 2;
    client.mBitsPerChannel = 32;
    client.mFramesPerPacket = 1;
    client.mBytesPerFrame = 8;
    client.mBytesPerPacket = 8;
    st = ExtAudioFileSetProperty(file, kExtAudioFileProperty_ClientDataFormat, sizeof(client), &client);
    if (st != noErr) {
        ExtAudioFileDispose(file);
        return false;
    }
    std::vector<float> buf(16384 * 2);
    for (;;) {
        AudioBufferList list;
        list.mNumberBuffers = 1;
        list.mBuffers[0].mNumberChannels = 2;
        list.mBuffers[0].mDataByteSize = static_cast<UInt32>(buf.size() * sizeof(float));
        list.mBuffers[0].mData = buf.data();
        UInt32 frames = 16384;
        if (ExtAudioFileRead(file, &frames, &list) != noErr || frames == 0) break;
        stereo.insert(stereo.end(), buf.begin(), buf.begin() + frames * 2);
        if (stereo.size() > static_cast<size_t>(sampleRate * 2 * 7200)) break;   // máx. 2 h
    }
    ExtAudioFileDispose(file);
    return !stereo.empty();
}

#elif defined(_WIN32)

const char* platformDecoderName() { return "Media Foundation"; }

bool platformDecode(const QString& path, double sampleRate, std::vector<float>& stereo, QString* error) {
    stereo.clear();
    const HRESULT coinit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_LITE))) {
        if (SUCCEEDED(coinit)) CoUninitialize();
        return false;
    }
    IMFSourceReader* reader = nullptr;
    bool ok = false;
    std::vector<float> raw;
    UINT32 channels = 0, rate = 0;
    if (SUCCEEDED(MFCreateSourceReaderFromURL(reinterpret_cast<LPCWSTR>(path.utf16()), nullptr, &reader))) {
        reader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE);
        reader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), TRUE);
        IMFMediaType* type = nullptr;
        if (SUCCEEDED(MFCreateMediaType(&type))) {
            type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
            type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
            reader->SetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), nullptr, type);
            type->Release();
        }
        IMFMediaType* actual = nullptr;
        if (SUCCEEDED(reader->GetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), &actual))) {
            actual->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels);
            actual->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate);
            actual->Release();
        }
        while (channels > 0) {
            DWORD flags = 0;
            IMFSample* sample = nullptr;
            if (FAILED(reader->ReadSample(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), 0, nullptr, &flags, nullptr, &sample))) break;
            if (flags & MF_SOURCE_READERF_ENDOFSTREAM) { if (sample) sample->Release(); break; }
            if (!sample) continue;
            IMFMediaBuffer* mb = nullptr;
            if (SUCCEEDED(sample->ConvertToContiguousBuffer(&mb))) {
                BYTE* data = nullptr;
                DWORD len = 0;
                if (SUCCEEDED(mb->Lock(&data, nullptr, &len))) {
                    const float* f = reinterpret_cast<const float*>(data);
                    raw.insert(raw.end(), f, f + len / sizeof(float));
                    mb->Unlock();
                }
                mb->Release();
            }
            sample->Release();
            if (raw.size() > static_cast<size_t>(rate) * channels * 7200) break;
        }
        reader->Release();
        ok = channels > 0 && rate > 0 && !raw.empty() &&
             convertToStereo(raw.data(), raw.size() / channels, static_cast<int>(channels), static_cast<int>(rate), sampleRate, stereo);
    }
    MFShutdown();
    if (SUCCEEDED(coinit)) CoUninitialize();
    if (!ok && error) *error = QStringLiteral("Media Foundation no pudo decodificar el archivo");
    return ok;
}

#elif defined(JX_HAVE_FFMPEG)

const char* platformDecoderName() { return "FFmpeg"; }

bool platformDecode(const QString& path, double sampleRate, std::vector<float>& stereo, QString* error) {
    stereo.clear();
    AVFormatContext* fmt = nullptr;
    if (avformat_open_input(&fmt, path.toUtf8().constData(), nullptr, nullptr) < 0) {
        if (error) *error = QStringLiteral("FFmpeg no pudo abrir el archivo");
        return false;
    }
    bool ok = false;
    AVCodecContext* ctx = nullptr;
    SwrContext* swr = nullptr;
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    do {
        if (avformat_find_stream_info(fmt, nullptr) < 0) break;
        const AVCodec* codec = nullptr;
        const int si = av_find_best_stream(fmt, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
        if (si < 0 || !codec) break;
        ctx = avcodec_alloc_context3(codec);
        if (!ctx || avcodec_parameters_to_context(ctx, fmt->streams[si]->codecpar) < 0) break;
        if (avcodec_open2(ctx, codec, nullptr) < 0) break;
        AVChannelLayout outLayout = AV_CHANNEL_LAYOUT_STEREO;
        if (swr_alloc_set_opts2(&swr, &outLayout, AV_SAMPLE_FMT_FLT, static_cast<int>(sampleRate), &ctx->ch_layout,
                                ctx->sample_fmt, ctx->sample_rate, 0, nullptr) < 0 || swr_init(swr) < 0)
            break;
        std::vector<float> buf;
        auto drain = [&](AVFrame* f) {
            const int maxOut = swr_get_out_samples(swr, f ? f->nb_samples : 0);
            if (maxOut <= 0) return;
            buf.resize(static_cast<size_t>(maxOut) * 2);
            uint8_t* outPtr = reinterpret_cast<uint8_t*>(buf.data());
            const int got = swr_convert(swr, &outPtr, maxOut, f ? const_cast<const uint8_t**>(f->extended_data) : nullptr,
                                        f ? f->nb_samples : 0);
            if (got > 0) stereo.insert(stereo.end(), buf.begin(), buf.begin() + got * 2);
        };
        while (av_read_frame(fmt, pkt) >= 0) {
            if (pkt->stream_index == si && avcodec_send_packet(ctx, pkt) >= 0) {
                while (avcodec_receive_frame(ctx, frame) >= 0) drain(frame);
            }
            av_packet_unref(pkt);
            if (stereo.size() > static_cast<size_t>(sampleRate * 2 * 7200)) break;
        }
        avcodec_send_packet(ctx, nullptr);
        while (avcodec_receive_frame(ctx, frame) >= 0) drain(frame);
        drain(nullptr);
        ok = !stereo.empty();
    } while (false);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    swr_free(&swr);
    avcodec_free_context(&ctx);
    avformat_close_input(&fmt);
    if (!ok && error) *error = QStringLiteral("FFmpeg no pudo decodificar el archivo");
    return ok;
}

#else

const char* platformDecoderName() { return ""; }
bool platformDecode(const QString&, double, std::vector<float>&, QString* error) {
    if (error) *error = QStringLiteral("Formato no soportado en esta plataforma");
    return false;
}

#endif

} // namespace jx
