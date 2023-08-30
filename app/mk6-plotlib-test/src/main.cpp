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
// constexpr double window_size_max_sec = 10.;
constexpr double update_period_sec = 1 / 60.;

constexpr int audio_sampling_rate_hz = 44100; // 20480 is not supported
constexpr int decimated_rate_hz = 44100;
constexpr int decimation_ratio = audio_sampling_rate_hz / decimated_rate_hz;
constexpr int min_fft_n = 3;
constexpr int max_fft_n = 10;
constexpr double min_fft_stride_ratio = 0;
constexpr double max_fft_stride_ratio = 1;

int audio_sample_idx = 0;
int fft_n = 8;
float fft_stride_ratio = 0.5;

RtDataFixedRate<600>
    y_1{"data_1", 1 / update_period_sec};
RtDataFixedRate<600> y_2{"data_2", 1 / update_period_sec};
RtDataFixedRate<600> y_3{"data_3", 1 / update_period_sec};
RtPlot3 rt_plot{"rt_plot", -1, -1, &y_1, &y_2, &y_3};

RtDataDynamicRate<600> mouse_xs{"mouse xs"};
RtDataDynamicRate<600> mouse_ys{"mouse ys"};
RtPlot2 mouse_rt_plot{"mouse position", -1, -1, &mouse_xs, &mouse_ys};

RtDataFixedRate<1024 * 100> raw_audio{"raw audio", audio_sampling_rate_hz};
RtDataFixedRate<1024 * 100> decimated_audio{"decimated audio", decimated_rate_hz};
RtPlot2 audio_plot{"audio plot", -1, -1, &raw_audio, &decimated_audio};

SnapshotData raw_fft{"frequency (hz)", "fft"};
SnapshotPlot raw_fft_plot{"audio FFT plot", -1, -1, &raw_fft};

SpectrogramData<1024, 1 << max_fft_n> audio_fft_spectrogram{"frequency (Hz)", "time (sec)"};
SpectrogramPlot audio_fft_spectrogram_plot{"audio FFT spectrogram plot", -1, -1, &audio_fft_spectrogram};

// Main code
int main(int, char **)
{
    auto audio_callback = [&](VectorView<float> xs)
    {
        raw_audio.push_sequence(xs);

        Vector<float> decimated{};
        auto push_decimated = [&](float x)
        {
            decimated.push_back(x);
        };

        for_each([&](float x)
                 { 
                    // decimate<decimation_ratio>(x, &audio_sample_idx, lpf_1024_44100, push_decimated); 
                    decimate<decimation_ratio>(x, &audio_sample_idx, id<float>, push_decimated); },
                 xs);
        decimated_audio.push_sequence(decimated);
    };
    AudioStream audio_stream(audio_sampling_rate_hz, 1024, audio_callback);

    auto fft_task = [&]()
    {
        // todo Should be every certain number of sample
        periodic_loop(
            [&]()
            {
                decimated_audio.lock();
                auto &as = decimated_audio.as_;
                size_t fft_length_ = fft_length(fft_n);
                VectorView<const double> fft_view{p_data(as) + length(as) - fft_length_, fft_length_};
                decimated_audio.unlock();

                auto detrended_input = normalize_n<float>(detrend<double>(remove_dc<double>(fft_view)));
                auto fft_result = complex_abs(fft_real(detrended_input));

                // todo internal mutex
                raw_fft.lock();
                raw_fft.xs_ = from_function(fft_length_ / 2, [&](int i)
                                            { return (double)i * decimated_rate_hz / (double)(fft_length_); });
                raw_fft.ys_ = map([](auto x)
                                  { return (double)x; },
                                  fft_result);
                raw_fft.unlock();

                audio_fft_spectrogram.push_sequence(map([](auto x)
                                                        { return std::log10(x); },
                                                        fft_result));
            },
            [&]()
            {
                return std::chrono::microseconds(
                    (int)((float)fft_length(fft_n) / (float)decimated_rate_hz * 1000000 * fft_stride_ratio));
            });
    };
    std::thread fft_thread{fft_task};

    auto init_task = [&](ImGuiIO &io)
    {
        using namespace ImGui;
    };

    auto loop_task = [&](ImGuiIO &io)
    {
        using namespace ImGui;
        using namespace ImPlot;

        window("Audio", [&]()
               {   Text("Raw Audio");
                   Text("Sampling rate %d Hz", audio_sampling_rate_hz);
                   audio_plot.plot(); });

        window("FFT & MFCC", [&]()
               {    raw_fft_plot.plot();
                    SliderInt("FFT N", &fft_n, min_fft_n, max_fft_n);
                    SliderFloat("FFT stride ratio", &fft_stride_ratio, 0, 1); });

        window("FFT Spectrogram", [&]()
               { audio_fft_spectrogram_plot.plot(); });

        window("Configuration", [&]()
               {
                   ImGui::ShowStyleSelector("GUI style");
                   ImPlot::ShowStyleSelector("plot style");
                   ShowColormapSelector("plot colormap");
                   Text("application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
               });

        ImGui::ShowDemoWindow();
    };

    run_gui(
        1920,
        1080,
        "Ars Vivendi",
        init_task,
        loop_task);

    fft_thread.join();

    return 0;
}