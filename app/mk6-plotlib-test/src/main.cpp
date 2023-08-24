#include <iostream>
#include <thread>

#include "efp.hpp"
#include "RtAudio.h"
#include "audio-stream.hpp"
// #include "./plotter.hpp"
#include "plotlib.hpp"

using namespace efp;

constexpr const char *target_identifier = "Ars Vivendi BLE";

constexpr int buffer_capacity = 1250;
// constexpr float window_size_max_sec = 10.;
constexpr float update_period_sec = 1 / 60.;

// ! temp
int audio_callback(void *outputBuffer, void *inputBuffer, unsigned int nFrames,
                   double streamTime, RtAudioStreamStatus status, void *userData)
{
    int16_t *input = static_cast<int16_t *>(inputBuffer);
    // Process the input audio data (e.g., print or analyze)
    for (unsigned int i = 0; i < nFrames; ++i)
    {
        std::cout << "Sample " << i << ": " << input[i] << std::endl;
    }
    return 0;
}

// Main code
int main(int, char **)
{
    AudioStream audio_stream(44100, 1024, audio_callback);

    std::cout << "Capturing audio. Press Enter to stop..." << std::endl;
    std::cin.get(); // Wait for user to press Enter

    // RtAudio audio;
    // RtAudio::StreamParameters inputParams;

    // if (audio.getDeviceCount() < 1)
    // {
    //     std::cout << "No audio devices available." << std::endl;
    //     return 1;
    // }

    // // Set up input parameters
    // inputParams.deviceId = audio.getDefaultInputDevice();
    // inputParams.nChannels = 1; // Mono input
    // inputParams.firstChannel = 0;
    // unsigned int buffer_size = 256;

    // RtAudio::StreamOptions options;
    // options.flags = RTAUDIO_NONINTERLEAVED;

    // RtAudioErrorType err;
    // err = audio.openStream(
    //     nullptr,
    //     &inputParams,
    //     RTAUDIO_SINT16,
    //     44100,
    //     &buffer_size,
    //     &audio_callback,
    //     nullptr,
    //     &options);

    // if (err)
    // {
    //     std::cout << "" << std::endl;
    //     return 1;
    // }

    // err = audio.startStream();
    // if (err)
    // {
    //     std::cout << "" << std::endl;
    //     return 1;
    // }

    // std::cout << "Capturing audio. Press Enter to stop..." << std::endl;
    // std::cin.get(); // Wait for user to press Enter

    // err = audio.stopStream();
    // if (err)
    // {
    //     std::cout << "" << std::endl;
    //     return 1;
    // }

    // audio.closeStream();

    // RtDataFixedRate<600> y1{"data_1", 1 / update_period_sec};
    // RtDataFixedRate<600> y2{"data_2", 1 / update_period_sec};
    // RtDataFixedRate<600> y3{"data_3", 1 / update_period_sec};
    // RtPlot3 rt_plot{"rt_plot", -1, 500, &y1, &y2, &y3};

    // RtDataDynamicRate<600> mouse_xs{"mouse xs"};
    // RtDataDynamicRate<600> mouse_ys{"mouse ys"};
    // RtPlot2 mouse_rt_plot{"mouse position", -1, 500, &mouse_xs, &mouse_ys};

    // auto sin_update_task = [&]()
    // {
    //     int idx = 0;

    //     while (true)
    //     {
    //         float now_sec_ = now_sec();

    //         y1.push_now(3 * sin(now_sec_ / 10 * 2 * M_PI) * sin(now_sec_ / 0.5 * 2 * M_PI) + 10 * sin(now_sec_ / 15 * 2 * M_PI));
    //         y2.push_now(sin(now_sec_ / 0.6 * 2 * M_PI));
    //         y3.push_now(std::exp(sin(now_sec_ / 5. * 2. * M_PI)) + 0.5 * sin(now_sec_ / 0.04 * 2 * M_PI) * sin(now_sec_ / 0.05 * 2 * M_PI));

    //         std::this_thread::sleep_for(std::chrono::duration<float>(update_period_sec));
    //     }
    // };
    // std::thread update_thread{sin_update_task};

    // auto mouse_update_task = [&]()
    // {
    //     float now_sec_ = now_sec();
    //     ImVec2 mouse = ImGui::GetMousePos();

    //     mouse_xs.push_now(mouse.x);
    //     mouse_ys.push_now(mouse.y);

    //     std::this_thread::sleep_for(std::chrono::duration<float>(update_period_sec));
    // };

    // auto init_task = [&](ImGuiIO &io)
    // {
    //     using namespace ImGui;
    // };

    // auto loop_task = [&](ImGuiIO &io)
    // {
    //     using namespace ImGui;

    //     mouse_update_task();

    //     window("Real-time Generated Data", [&]()
    //            { rt_plot.plot(); });

    //     window("Real-time Mouse Position", [&]()
    //            {   using namespace ImGui;
    //                Text("Realtime mouse position plot");
    //                Text("Sampling rate %.2f Hz", 1. / update_period_sec);
    //                mouse_rt_plot.plot(); });

    //     Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    // };

    // run_gui(
    //     1920,
    //     1080,
    //     "Ars Vivendi",
    //     init_task,
    //     loop_task);

    // update_thread.join();

    return 0;
}