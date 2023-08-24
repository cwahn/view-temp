#include <iostream>
#include <thread>

#include "efp.hpp"
#include "RtAudio.h"
#include "audio-stream.hpp"
// #include "./plotter.hpp"
#include "plotlib.hpp"
#include "signal.hpp"

using namespace efp;

constexpr const char *target_identifier = "Ars Vivendi BLE";

constexpr int buffer_capacity = 1250;
// constexpr float window_size_max_sec = 10.;
constexpr float update_period_sec = 1 / 60.;

constexpr int audio_sampling_rate_hz = 44100; // 20480 is not supported
constexpr int decimated_rate_hz = 2048;
constexpr int decimation_ratio = audio_sampling_rate_hz / decimated_rate_hz;
int audio_sample_idx = 0;

RtDataFixedRate<600> y_1{"data_1", 1 / update_period_sec};
RtDataFixedRate<600> y_2{"data_2", 1 / update_period_sec};
RtDataFixedRate<600> y_3{"data_3", 1 / update_period_sec};
RtPlot3 rt_plot{"rt_plot", -1, 500, &y_1, &y_2, &y_3};

RtDataDynamicRate<600> mouse_xs{"mouse xs"};
RtDataDynamicRate<600> mouse_ys{"mouse ys"};
RtPlot2 mouse_rt_plot{"mouse position", -1, 500, &mouse_xs, &mouse_ys};

RtDataFixedRate<1024 * 100> raw_audio{"raw audio", audio_sampling_rate_hz};
RtDataFixedRate<1024 * 100> decimated_audio{"decimated audio", decimated_rate_hz};
RtPlot2 audio_plot{"audio plot", -1, 500, &raw_audio, &decimated_audio};

// Main code
int main(int, char **)
{

    auto sin_update_task = [&]()
    {
        int idx = 0;

        while (true)
        {
            float now_sec_ = now_sec();

            y_1.push_now(3 * sin(now_sec_ / 10 * 2 * M_PI) * sin(now_sec_ / 0.5 * 2 * M_PI) + 10 * sin(now_sec_ / 15 * 2 * M_PI));
            y_2.push_now(sin(now_sec_ / 0.6 * 2 * M_PI));
            y_3.push_now(std::exp(sin(now_sec_ / 5. * 2. * M_PI)) + 0.5 * sin(now_sec_ / 0.04 * 2 * M_PI) * sin(now_sec_ / 0.05 * 2 * M_PI));

            std::this_thread::sleep_for(std::chrono::duration<float>(update_period_sec));
        }
    };
    std::thread update_thread{sin_update_task};

    auto audio_callback = [&](VectorView<float> xs)
    {
        raw_audio.push_sequence(xs);

        Vector<float> decimated{};
        auto push_decimated = [&](float x)
        {
            decimated.push_back(x);
        };

        for_each([&](float x)
                 { decimate<decimation_ratio>(x, &audio_sample_idx, lpf_1024_44100, push_decimated); },
                 xs);

        decimated_audio.push_sequence(decimated);
    };
    AudioStream audio_stream(audio_sampling_rate_hz, 1024, audio_callback);

    auto mouse_update_task = [&]()
    {
        float now_sec_ = now_sec();
        ImVec2 mouse = ImGui::GetMousePos();

        mouse_xs.push_now(mouse.x);
        mouse_ys.push_now(mouse.y);

        std::this_thread::sleep_for(std::chrono::duration<float>(update_period_sec));
    };

    auto init_task = [&](ImGuiIO &io)
    {
        using namespace ImGui;
    };

    auto loop_task = [&](ImGuiIO &io)
    {
        using namespace ImGui;

        mouse_update_task();

        window("Real-time Generated Data", [&]()
               { rt_plot.plot(); });

        window("Real-time Mouse Position", [&]()
               {   using namespace ImGui;
                   Text("Realtime mouse position plot");
                   Text("Sampling rate %.2f Hz", 1. / update_period_sec);
                   mouse_rt_plot.plot(); });

        window("Audio", [&]()
               {
                   using namespace ImGui;
                   Text("Raw Audio");
                   Text("Sampling rate %d Hz", audio_sampling_rate_hz);
                   audio_plot.plot(); });

        Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    };

    run_gui(
        1920,
        1080,
        "Ars Vivendi",
        init_task,
        loop_task);

    update_thread.join();

    return 0;
}