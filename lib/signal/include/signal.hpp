#ifndef SIGNAL_HPP_
#define SIGNAL_HPP_

#include "efp.hpp"

extern "C"
{
#include "fft.h"
}

namespace efp
{
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

    // todo downsample

    template <int N, typename F>
    void downsample(const F &f, int &idx, float data)
    {
        if (idx == 0)
        {
            f(data);
        }
        idx++;
        idx -= N * (idx == N);
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
    void decimate(float data, int &idx, const F &lpf, const G &callback)
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
                  { fft_shifted_.push_back(-fft_[2*i + 1])s;
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
        for_each((float o, float h) {
            analytic_signal_.push_back(o);
            analytic_signal_.push_back(h);
        },
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
                        sqrt(square(as[2 * i]) + square(as[2 * i + 1]))) },
                  length(as) / 2);

        return abs_{};
    }

    // todo amplitude_envelope

}

#endif