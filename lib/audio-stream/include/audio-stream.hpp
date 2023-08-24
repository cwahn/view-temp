#ifndef AUDIO_STREAM_HPP_
#define AUDIO_STREAM_HPP_

#include "efp.hpp"
#include "RtAudio.h"

// todo Make output type generic

class AudioStream
{
public:
    template <typename F>
    AudioStream(
        unsigned int sample_rate_hz,
        unsigned int buffer_length,
        const F &callback)
        : buffer_length_(buffer_length)
    //   callback_(callback)

    {
        // RtAudio audio;
        // RtAudio::StreamParameters inputParams;

        if (audio_.getDeviceCount() < 1)
        {
            std::cout << "no audio devices available." << std::endl;
            abort();
        }

        input_params_.deviceId = audio_.getDefaultInputDevice();
        input_params_.nChannels = 1; // Mono input
        input_params_.firstChannel = 0;

        // RtAudio::StreamOptions options;
        options_.flags = RTAUDIO_NONINTERLEAVED;

        auto audio_callback_ = [&](void *outputBuffer, void *inputBuffer, unsigned int nFrames,
                                   double streamTime, RtAudioStreamStatus status, void *userData)
        {
            int16_t *input = static_cast<int16_t *>(inputBuffer);
            // Process the input audio data (e.g., print or analyze)
            for (unsigned int i = 0; i < nFrames; ++i)
            {
                // std::cout << "Sample " << i << ": " << input[i] << std::endl;
                callback(input[i]);
            }
            return 0;
        };

        RtAudioErrorType err;
        err = audio_.openStream(
            nullptr,
            &input_params_,
            RTAUDIO_SINT16,
            sample_rate_hz,
            &buffer_length_,
            efp::to_function_pointer(audio_callback_),
            nullptr,
            &options_);

        if (err)
        {
            std::cout << "rtaudio stream open failed" << std::endl;
            abort();
        }

        err = audio_.startStream();
        if (err)
        {
            std::cout << "rtaudio stream start failed" << std::endl;
            abort();
        }
    }

    ~AudioStream()
    {
        RtAudioErrorType err = audio_.stopStream();
        if (err)
        {
            std::cout << "rtaudio stream stop failed" << std::endl;
            abort();
        }

        audio_.closeStream();
    }

private:
    // int audio_callback_(void *outputBuffer, void *inputBuffer, unsigned int nFrames,
    //                     double streamTime, RtAudioStreamStatus status, void *userData)
    // {
    //     int16_t *input = static_cast<int16_t *>(inputBuffer);
    //     // Process the input audio data (e.g., print or analyze)
    //     for (unsigned int i = 0; i < nFrames; ++i)
    //     {
    //         // std::cout << "Sample " << i << ": " << input[i] << std::endl;
    //         callback_(input[i]);
    //     }
    //     return 0;
    // }

    RtAudio audio_;
    RtAudio::StreamParameters input_params_;
    unsigned int buffer_length_;
    RtAudio::StreamOptions options_;
    // F &callback_;
};

#endif