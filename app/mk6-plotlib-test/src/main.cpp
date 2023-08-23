#include <iostream>
#include <thread>

#include "efp.hpp"
// #include "./plotter.hpp"
#include "plotlib.hpp"

using namespace efp;

constexpr const char *target_identifier = "Ars Vivendi BLE";

constexpr int buffer_capacity = 1250;
// constexpr float window_size_max_sec = 10.;
constexpr float update_period_sec = 1 / 60.;

// static_assert(buffer_capacity > window_size_max_sec * 1 / update_period_sec);

template <typename SeqA>
void cout_head(const char *name, const SeqA &as, int n)
{
    VectorView<Element_t<SeqA>> as_view(p_data(as), n);
    std::cout << name << ": ";
    for_each([](const auto &x)
             { std::cout << x << " "; },
             as_view);
    std::cout << std::endl;
}

// Main code
int main(int, char **)
{
    // Vcb<float, buffer_capacity> time_stamp_secs, ys1, ys2, ys3;
    // float max_time_stamp_sec = 0;
    // float window_size_sec = 5.;
    RtPlotData<600> y1{"data_1", 1 / update_period_sec};
    RtPlotData<600> y2{"data_2", 1 / update_period_sec};

    RtPlot2 rt_plot{"rt_plot", -1, 500, &y1, &y2};

    auto update_task = [&]()
    {
        int idx = 0;

        while (true)
        {
            float now_sec_ = now_sec();

            y1.lock();
            y1.ts_.push_back(now_sec_);
            y1.as_.push_back(3 * sin(now_sec_ / 10 * 2 * M_PI) * sin(now_sec_ / 0.5 * 2 * M_PI) + 10 * sin(now_sec_ / 15 * 2 * M_PI));
            y1.unlock();

            y2.lock();
            y2.ts_.push_back(now_sec_);
            y2.as_.push_back(sin(now_sec_ / 0.6 * 2 * M_PI));
            y2.unlock();

            std::this_thread::sleep_for(std::chrono::duration<float>(update_period_sec));
        }
    };

    auto init_task = [&]() {};
    auto loop_task = [&](ImGuiIO &io)
    {
        ImGui::Begin("Real-time Plotter");
        ImGui::Text("Spo2: %f \nHeart rate: %f \ntemperature: %f \n",
                    0.,
                    42.,
                    3.141562);

        ImGui::Text("FFT Graph\n");

        rt_plot.plot();

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    };

    std::thread update_thread{update_task};

    run_window(
        1920,
        1080,
        "Ars Vivendi",
        init_task,
        loop_task);

    update_thread.join();

    return 0;
}