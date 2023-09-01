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
constexpr int min_sample_resolution_bit = 6;
constexpr int max_sample_resolution_bit = 24;
constexpr float min_sample_amplitude = 0;
constexpr float max_sample_amplitude = 1;
constexpr int min_decimated_rate_hz = 2;
constexpr int max_decimated_rate_hz = audio_sampling_rate_hz / 2.;

constexpr int min_fft_n = 3;
constexpr int max_fft_n = 10;
constexpr double min_fft_stride_ratio = 0;
constexpr double max_fft_stride_ratio = 1;

int sample_resolution_bit = 10;
float sample_amplitude = 0.5;
int decimated_rate_hz = 22050;
// int decimation_ratio = std::round(audio_sampling_rate_hz / decimated_rate_hz);

int audio_sample_idx = 0;
WindowFunctionKind window_funcion_kind;
int fft_n = 8;
float fft_stride_ratio = 0.5;

// Raw data buffer
RtDataFixedRate<600>
    y_1{"data_1", 1 / update_period_sec};
RtDataFixedRate<600> y_2{"data_2", 1 / update_period_sec};
RtDataFixedRate<600> y_3{"data_3", 1 / update_period_sec};
RtPlot3 rt_plot{"rt_plot", -1, -1, &y_1, &y_2, &y_3};

// Audio
// RtDataFixedRate<1024 * 100> raw_audio{"raw audio", audio_sampling_rate_hz};
// RtDataFixedRate<1024 * 100> resampled_audio{"resampled audio", audio_sampling_rate_hz};
RtDataDynamicRate<1024 * 100> raw_audio{"raw audio"};
RtDataDynamicRate<1024 * 100> resampled_audio{"resampled audio"};
RtDataDynamicRate<1024 * 100> decimated_audio{"decimated audio"};
RtPlot3 audio_plot{"audio plot", -1, -1, &raw_audio, &resampled_audio, &decimated_audio};

// Raw FFT
SnapshotData raw_fft{"frequency (hz)", "fft"};
SnapshotPlot raw_fft_plot{"audio FFT plot", -1, -1, &raw_fft};

// Spectrogram
SpectrogramData<1024, 1 << max_fft_n> audio_fft_spectrogram{"frequency (Hz)", "time (sec)"};
SpectrogramPlot audio_fft_spectrogram_plot{"audio FFT spectrogram plot", -1, -1, &audio_fft_spectrogram};

// Main code
int main(int, char **)
{
    auto audio_callback = [&](VectorView<float> xs)
    {
        raw_audio.push_sequence(xs);
        double step = 2 * sample_amplitude / ((1 << sample_resolution_bit) - 1);
        auto resampled_xs = map([&](auto x)
                                { return resample(x, step); },
                                xs);
        resampled_audio.push_sequence(resampled_xs);

        Vector<float> decimated{};
        auto push_decimated = [&](float x)
        { decimated.push_back(x); };

        static float x_1;
        static float y_1;
        decimate(resampled_xs, audio_sampling_rate_hz, decimated_rate_hz, audio_sample_idx, x_1, y_1, push_decimated);

        decimated_audio.push_sequence(decimated);

        auto push_fft = [&](float _)
        {
            decimated_audio.lock();
            auto &as = decimated_audio.as_;
            size_t fft_length_ = fft_length(fft_n);
            VectorView<const double> fft_view{p_data(as) + length(as) - fft_length_, fft_length_};
            decimated_audio.unlock();

            auto detrended_input = normalize_n<float>(detrend<double>(remove_dc<double>(fft_view)));
            auto windowed_input = window_function(detrended_input, window_funcion_kind);
            auto fft_result = complex_abs(fft_real(windowed_input));

            auto fft_freqs = from_function(fft_length_ / 2, [&](int i)
                                           { return (double)i * decimated_rate_hz / (double)(fft_length_); });

            // todo internal mutex
            raw_fft.lock();
            raw_fft.xs_ = fft_freqs;
            raw_fft.ys_ = map([](auto x)
                              { return (double)x; },
                              fft_result);
            raw_fft.unlock();

            audio_fft_spectrogram.push_sequence(map([](auto x)
                                                    { return std::log10(x); },
                                                    fft_result));
            audio_fft_spectrogram.min_y_ = fft_freqs[0];
            audio_fft_spectrogram.max_y_ = fft_freqs[fft_length_ / 2 - 1];
        };

        // FFT
        static int decimated_sample_idx = 0;
        const int fft_stride = std::round(fft_length(fft_n) * fft_stride_ratio);
        for_every_n(push_fft, fft_stride, decimated_sample_idx, decimated);
    };
    AudioStream audio_stream(audio_sampling_rate_hz, 1024, audio_callback);

    auto init_task = [&](ImGuiIO &io)
    {
        using namespace ImGui;
    };

    auto loop_task = [&](ImGuiIO &io)
    {
        using namespace ImGui;
        using namespace ImPlot;

        window("Audio", [&]()
               { audio_plot.plot(); });

        window("FFT", [&]()
               { raw_fft_plot.plot(); });

        window("FFT spectrogram", [&]()
               { audio_fft_spectrogram_plot.plot(); });

        window("Configuration", [&]()
               {
                   Text("application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
                   if (CollapsingHeader("General"))
                   {
                       SeparatorText("Appearance");
                       ImGui::ShowStyleSelector("GUI style");
                       ImPlot::ShowStyleSelector("plot style");
                       ShowColormapSelector("plot colormap");
                   }

                   if (CollapsingHeader("Signal Processing"))
                   {
                       SeparatorText("Sample Resolution");
                       SliderInt("Sample resolution", &sample_resolution_bit, min_sample_resolution_bit, max_sample_resolution_bit);
                       SliderFloat("Sample amplitude", &sample_amplitude, min_sample_amplitude, max_sample_amplitude);

                       SeparatorText("Decimation");
                       SliderInt("Decimated sample rate (hz)", &decimated_rate_hz, min_decimated_rate_hz, max_decimated_rate_hz);

                       SeparatorText("Window Function");
                       const char *window_function_names[] = {
                           "Id",
                           "Hann",
                           "Hamming",
                           "Blackman",
                       };
                       Combo("Window function", (int *) & window_funcion_kind, window_function_names, static_cast<int>(WindowFunctionKind::Count));
                       
                       SeparatorText("FFT");
                       SliderInt("FFT N", &fft_n, min_fft_n, max_fft_n);
                       SliderFloat("FFT stride ratio", &fft_stride_ratio, 0, 1);
                   } });

        ImGui::ShowDemoWindow();
    };

    run_gui(
        1920,
        1080,
        "Ars Vivendi",
        init_task,
        loop_task);

    // fft_thread.join();

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1000));
    }

    return 0;
}