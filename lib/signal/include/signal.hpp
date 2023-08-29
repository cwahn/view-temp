#ifndef SIGNAL_HPP_
#define SIGNAL_HPP_

#include "efp.hpp"
#include "./fixed_point.hpp"

extern "C"
{
#include "fft.h"
}

namespace efp
{
    template <typename F, typename G>
    void periodic_loop(const F &f, const G &period)
    {
        using namespace std::chrono;

        while (true)
        {
            auto start_time = high_resolution_clock::now();
            f();
            auto end_time = high_resolution_clock::now();
            auto elapsed_time = duration_cast<milliseconds>(end_time - start_time);

            auto period_ = period();
            if (elapsed_time < period_)
                std::this_thread::sleep_for(period_ - elapsed_time);
        }
    }

    template <typename A, typename SeqA>
    Vector<A> normalize_n(const SeqA &as)
    {
        return map([&](auto x)
                   { return (A)(x / (Element_t<SeqA>)length(as)); },
                   as);
    }

    // ! partial fucntion. length should be power of two, otherwise abort
    template <typename SeqA>
    Vector<float> fft_complex(SeqA &as)
    {
        Vector<float> bs(length(as));

        fft_config_t *fft_config = fft_init(
            length(as),
            FFT_COMPLEX,
            FFT_FORWARD,
            as.data(),
            bs.data());
        fft_execute(fft_config);
        fft_destroy(fft_config);

        return bs;
    }

    // ! partial fucntion. length should be power of two, otherwise abort
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
    // ! partial fucntion. length should be power of two, otherwise abort
    template <typename SeqA>
    Vector<float> ifft_complex(SeqA &as)
    {
        Vector<float> result(length(as));

        fft_config_t *fft_config = fft_init(
            length(as),
            FFT_COMPLEX,
            FFT_BACKWARD,
            as.data(),
            result.data());
        fft_execute(fft_config);
        fft_destroy(fft_config);

        return result;
    }

    // ! Inplace mutating function
    // ! partial fucntion. length should be power of two, otherwise abort
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

    size_t fft_length(int n)
    {
        return 1 << n;
    }

    // todo lpf (f_c: 1024 Hz, delta_t: 1/20480)
    float lpf_1024_20480(float x)
    {
        static float x_1;
        static float y_1;

        float y =
            0.135755248163633 * x + 0.135755248163633 * x_1 - ((-0.728489503672734) * y_1);
        x_1 = x;
        y_1 = y;

        return y;
    }

    // todo lpf (f_c: 1024 Hz, delta_t: 1/20480)
    float lpf_1024_44100(float x)
    {

        // num =
        //     {[0.067988069774338 0.067988069774338]}

        // den =
        // { [1 - 0.864023860451324] }

        // 44100 / 2048 = 21.53

        static float x_1;
        static float y_1;

        float y =
            0.067988069774338 * x + 0.067988069774338 * x_1 - ((-0.864023860451324) * y_1);
        x_1 = x;
        y_1 = y;

        return y;
    }

    // todo downsample

    template <int N, typename F>
    void downsample(const F &f, int *idx, float data)
    {
        if (*idx == 0)
        {
            f(data);
        }
        (*idx)++;
        (*idx) -= N * ((*idx) == N);
    }

    // for_each([&](uint16_t data)
    //          {
    //         auto filtered = lpf(data);
    //         if (idx == 0)
    //         {
    //              adc_samples.push_back(filtered);
    //         }
    //         sample_count ++;
    //         idx ++;
    //         idx -= 20 * (idx == 20); },
    //          datas);

    // todo decimate 2048 Hz

    template <int N, typename F, typename G>
    void decimate(float data, int *idx, const F &lpf, const G &callback)
    {
        float filtered = lpf(data);
        downsample<N>(callback, idx, filtered);
    }

    // todo hibert
    // ! partial fucntion. length should be power of two, otherwise abort
    template <typename SeqA>
    Vector<float> hilbert(const SeqA &as)
    {
        auto fft_ = fft_real(as);

        // Shift phase by +pi/2
        Vector<float> fft_shifted_{};
        for_index([&](int i)
                  { fft_shifted_.push_back(-fft_[2*i + 1]);
                    fft_shifted_.push_back(fft_[2*i]); },
                  length(as));

        return ifft_complex(fft_shifted_);
    }

    // todo analytic_signal
    // ! partial fucntion. length should be power of two, otherwise abort
    template <typename SeqA>
    Vector<float> analytic_signal(const SeqA &as)
    {
        Vector<float> analytic_signal_{};
        for_each([&](float o, float h)
                 {analytic_signal_.push_back(o);
                 analytic_signal_.push_back(h); },
                 as, hilbert(as));

        return analytic_signal_;
    }

    // ! partial fucntion. length should be power of two, otherwise abort
    template <typename SeqA>
    Vector<float> complex_abs(const SeqA &as)
    {
        Vector<float> abs_{};
        for_index([&](int i)
                  { abs_.push_back(
                        sqrt(square(as[2 * i]) + square(as[2 * i + 1]))); },
                  length(as) / 2);

        return abs_;
    }

    // todo amplitude_envelope

}

#endif