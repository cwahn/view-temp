
#ifndef SIGNAL_PROCESSING
#define SIGNAL_PROCESSING

#include <tuple>

#include "efp.hpp"
#include "signal_processing.hpp"

extern "C"
{
#include "fft.h"
}

using namespace efp;

// template <typename SeqA>
// void logi_head(const char *tag, const SeqA &as, const char *delimeter, int n)
// {
//     size_t print_len = n < length(as) ? n : length(as);
//     VectorView<const Element_t<SeqA>> as_view{p_data(as), print_len};

//     std::string str;
//     for_each([&](float x)
//              {
//                 char buffer [10];
//                 sprintf(buffer, "%.2f%s", x, delimeter);
//                 str += buffer; },
//              as_view);

//     ESP_LOGI(tag, "%s", str.c_str());
// }

template <typename SeqA>
Vector<float> fft_real(SeqA &as)
{
    Vector<float> bs(length(as));

    fft_config_t *fft_config = fft_init(
        length(as),
        FFT_REAL,
        FFT_FORWARD,
        as.data(),
        bs.data());
    fft_execute(fft_config);
    fft_destroy(fft_config);

    return bs;
}

// ! Inplace mutating function

template <typename SeqA>
Vector<float> ifft_real(SeqA &as)
{
    Vector<float> result(length(as));

    fft_config_t *fft_config = fft_init(
        length(as),
        FFT_REAL,
        FFT_BACKWARD,
        as.data(),
        result.data());
    fft_execute(fft_config);
    fft_destroy(fft_config);

    return result;
}

template <typename Seq>
float heart_rate(Seq &irs, int sampling_frequency_hz)
{
    // Heartrate would be not faster than 120 and No slower than 30
    // 120 Hz -> Need 0.5 sec, 50 sample or lag
    // 30 Hz -> 2 seconds of data
    // Drop first 50 data.
    constexpr int num_drop = 50;

    auto autocorrelation_function =
        from_function(length(irs) - num_drop, [&](int i)
                      { return autocorrelation<float>(irs, i + num_drop); });

    auto maybe_max_lag = elem_index(
        maximum(autocorrelation_function),
        autocorrelation_function);

    int max_lag = maybe_max_lag.match(
        [](Nothing)
        {
            // ESP_LOGE("heart_rate", "Nothing");
            return 0;
        },
        [](int i)
        { return i + num_drop; });

    return 60. /
           ((float)max_lag / (float)sampling_frequency_hz);
}

template <typename Seq>
float heart_rate_fft(Seq &irs, int sampling_frequency_hz)
{
    // auto fft_result = fft_real(irs);
    // Element_t<Seq> max_elem = 0;
    // int max_idx = 0;

    // for_each_with_index([&](int i, const Element_t<Seq> &x)
    //                     { if (x > max_elem)
    //                         max_idx = i; },
    //                     fft_abs);

    auto normalized_abs_fft = map([](const Element_t<Seq> &x)
                                  { return x > 0 ? x / 254 : -x / 254; },
                                  fft_real(irs));

    ArrayView<const Element_t<Seq>, 256 - 40> fft_view{p_data(normalized_abs_fft) + 40};

    auto max_idx = elem_index(maximum(fft_view), fft_view).value() + 40;
    // logi_head("fft_view", fft_view, " ", 10);
    ESP_LOGI("heart_rate_fft", "max index: %d", max_idx);

    // return zero if heart rate is smaller than 30
    return max_idx > 40
               ? (float)max_idx * (float)sampling_frequency_hz / (float)length(irs) / 2. * 60.
               : 0;
}

float spo2(float red_ac, float red_dc, float ir_ac, float ir_dc)
{
    const float r = ((red_ac / red_dc) / (ir_ac / ir_dc));
    return (r >= 0.4 && r < 3.4) ? 104 - 17 * r : 0;
}
 
// Heart rate => 30 ~ 120 /min -> Max 150 / min
// Max is 2.5 Hz
// Sampling > 5 Hz -> closest: 200/32 = 6.25 Hz
// Max = 6.25 * 60 / 2 = 187.5 Hz
// Resolution = 187.5 Hz / 256 = 0.73 bpm
// Period = 1000/6.25 = 160 ms 1 new sample per period.

template <typename SeqA, typename SeqB>
std::tuple<float, float, float> heart_rate_spo2(SeqA &reds, SeqB &irs, int sampling_frequency_hz)
{
    auto dc_removed_reds = remove_dc<float>(reds);
    auto dc_removed_irs = remove_dc<float>(irs);

    auto detrended_reds = detrend<float>(dc_removed_reds);
    auto detrended_irs = detrend<float>(dc_removed_irs);

    constexpr int correlation_calc_len = 16;
    ArrayView<const float, correlation_calc_len> reds_corr_view{p_data(detrended_reds) + length(reds) - correlation_calc_len};
    ArrayView<const float, correlation_calc_len> irs_corr_view{p_data(detrended_irs) + length(irs) - correlation_calc_len};

    // float correlation_value = correlation<float>(detrended_reds, detrended_irs);
    float correlation_value = correlation<float>(reds_corr_view, irs_corr_view);

    float heart_rate_value = 0;
    float spo2_value = 0;

    // // ! debugging
    // if (correlation_value < -1.1 || correlation_value > 1.1)
    // {
    //     ESP_LOGE("heart_rate_spo2", "Invalid correlation %f", correlation_value);
    //     logi_head("reds", reds, ", ", length(reds));
    //     logi_head("irs", irs, ", ", length(irs));
    // }

    heart_rate_value = heart_rate_fft(detrended_irs, sampling_frequency_hz);
    // heart_rate_value = heart_rate(detrended_irs, sampling_frequency_hz);
    spo2_value = spo2(rms<float>(detrended_reds), mean<float>(reds), rms<float>(detrended_irs), mean<float>(irs));

    // if (correlation_value > 0.7)
    // {
    //     heart_rate_value = heart_rate(detrended_irs, sampling_frequency_hz);
    //     spo2_value = spo2(rms<float>(detrended_reds), mean<float>(reds), rms<float>(detrended_irs), mean<float>(irs));
    // }

    return std::make_tuple(correlation_value, heart_rate_value, spo2_value);
}

float sin_wave(float freq_hz, int samping_freq_hz, int index)
{
    float t_s = index / (float)samping_freq_hz;
    float period_s = 1. / freq_hz;
    return std::sin(2. * M_PI * t_s / period_s);
}

// template <typename F>
// void logi_duration(const F &f)
// {
//     int64_t t0 = esp_timer_get_time();
//     f();
//     int64_t t1 = esp_timer_get_time();

//     // ESP_LOGI("logi_duration", "Execution time: %lld us", t1 - t0);
// }

float lpf(float x)
{
    static float x_1;
    static float y_1;

    float y = 0.012345679012346 * x + 0.012345679012346 * x_1 - ((-0.975308641975309) * y_1);
    x_1 = x;
    y_1 = y;

    return y;
}

#endif