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
        F &callback)
        : buffer_length_(buffer_length)
    //   callback_(callback)

    {
        // RtAudio audio;
        // RtAudio::StreamParameters inputParams;
        auto err_callback_ = [](RtAudioErrorType type, const std::string &errorText)
        {
            switch(type)
            {
                case(RTAUDIO_NO_ERROR):      /*!< No error. */
                break;
                case(RTAUDIO_WARNING):           /*!< A non-critical error. */
                break;
                case(RTAUDIO_UNKNOWN_ERROR):     /*!< An unspecified error type. */
                break;
                case(RTAUDIO_DEVICE_DISCONNECT): /*!< A device in use was disconnected. */
                break;

                
            }
            std::cout << errorText << std::endl;
        };
        audio_.setErrorCallback(err_callback_);

        if (audio_.getDeviceCount() < 1)
        {
            std::cout << "no audio devices available." << std::endl;
            abort();
        }
        int NN = audio_.getDeviceCount();
        std::cout << audio_.getDeviceCount() << std::endl;

        auto ids = audio_.getDeviceIds();
        bool available = false;
        for (auto i : ids) {
            RtAudio::DeviceInfo info = audio_.getDeviceInfo(i);
            std::cout << info.name << " : " << info.inputChannels << " , " << info.duplexChannels << " , " << info.isDefaultInput << std::endl;
            available |= info.inputChannels > 0 || info.duplexChannels  > 0;
            if (info.inputChannels > 0)
                input_params_.deviceId = info.ID;
        }

        // if (!available)
        // {
        //     while(!available)
        //     {

        //     }
        // }
        //input_params_.deviceId = audio_.getDefaultInputDevice();
        input_params_.nChannels = 1; // Mono input
        input_params_.firstChannel = 0;

        // RtAudio::StreamOptions options;
        options_.flags = RTAUDIO_NONINTERLEAVED;

        auto audio_callback_ = [&](void *outputBuffer, void *inputBuffer, unsigned int nFrames,
                                   double streamTime, RtAudioStreamStatus status, void *userData)
        {
            float *input = static_cast<float *>(inputBuffer);
            efp::VectorView<float> inputs{input, static_cast<const int>(nFrames), static_cast<const int>(nFrames)};

            // for (unsigned int i = 0; i < nFrames; ++i)
            // {
            //      std::cout << "Sample " << i << ": " << input[i] << "\n" ;
            //     //callback(input[i]);
            // }
            //   std::cout <<   std::endl;
            callback(inputs);

            return 0;
        };


        RtAudioErrorType err;
        err = audio_.openStream(
            nullptr,
            &input_params_,
            RTAUDIO_FLOAT32,
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

    RtAudio audio_{};
    RtAudio::StreamParameters input_params_;
    unsigned int buffer_length_;
    RtAudio::StreamOptions options_;
    // F &callback_;
};

#endif