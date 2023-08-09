#include <iostream>
#include <thread>

#include "efp.hpp"
#include "./plotter.hpp"

using namespace efp;

constexpr const char *target_identifier = "Ars Vivendi BLE";

constexpr int buffer_capacity = 1250;
constexpr float window_size_max_sec = 10.;
constexpr float time_delta_sec = 1 / 120.;

static_assert(buffer_capacity > window_size_max_sec * 1 / time_delta_sec);

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
    Vcb<float, buffer_capacity> time_stamp_secs, ys1, ys2, ys3;
    float max_time_stamp_sec = 0;
    float window_size_sec = 5.;

    auto update_task = [&]()
    {
        int idx = 0;

        while (true)
        {
            float time_ms = idx * time_delta_sec;
            time_stamp_secs.push_back(time_ms);
            max_time_stamp_sec = time_ms;

            ys1.push_back(3 * sin(time_ms / 10 * 2 * M_PI) * sin(time_ms / 0.5 * 2 * M_PI) + 10 * sin(time_ms / 15 * 2 * M_PI));
            ys2.push_back(sin(time_ms / 0.6 * 2 * M_PI));
            ys3.push_back(10 * sin(time_ms / 10 * 2 * M_PI) * cos(time_ms / 0.5 * 2 * M_PI) + 100 * sin(time_ms / 5 * 2 * M_PI));
            idx++;

            std::this_thread::sleep_for(std::chrono::duration<float>(time_delta_sec));
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

        if (ImPlot::BeginPlot("Realtime 1", ImVec2(-1, 500)))
        {
            ImPlot::SetupAxis(ImAxis_X1, "time(s)", ImPlotAxisFlags_RangeFit);
            ImPlot::SetupAxisLimits(ImAxis_X1, max_time_stamp_sec - window_size_sec, max_time_stamp_sec, ImGuiCond_Always);

            const size_t plotting_length = window_size_sec / time_delta_sec;

            VectorView<const float> time_stamp_view{time_stamp_secs.end() - plotting_length, plotting_length};
            VectorView<const float> ys1_view{ys1.end() - plotting_length, plotting_length};
            VectorView<const float> ys2_view{ys2.end() - plotting_length, plotting_length};

            // cout_head("ys1_view", ys1_view, 10);

            ImPlot::SetupAxis(ImAxis_Y1, "y1", ImPlotAxisFlags_RangeFit);
            ImPlot::SetupAxisLimits(ImAxis_Y1, minimum(ys1_view), maximum(ys1_view), ImGuiCond_Always);

            ImPlot::SetupAxis(ImAxis_Y2, "y2", ImPlotAxisFlags_RangeFit);
            ImPlot::SetupAxisLimits(ImAxis_Y2, minimum(ys2_view), maximum(ys2_view), ImGuiCond_Always);

            ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);

            ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
            ImPlot::PlotLine("y1", p_data(time_stamp_view), p_data(ys1_view), plotting_length, 0, 0, sizeof(float));

            ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
            ImPlot::PlotLine("y2", p_data(time_stamp_view), p_data(ys2_view), plotting_length, 0, 0, sizeof(float));

            ImPlot::EndPlot();
        }

        // if (ImPlot::BeginPlot("Realtime 2", ImVec2(-1, 500)))
        // {
        //     ImPlot::SetupAxis(ImAxis_X1, "time(s)", ImPlotAxisFlags_AutoFit);
        //     ImPlot::SetupAxisLimits(ImAxis_X1, maximum(xs) - window_size_sec, maximum(xs), ImGuiCond_Always);
        //     ImPlot::SetupAxis(ImAxis_Y1, "y3", ImPlotAxisFlags_AutoFit);
        //     ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 0, ImPlotAxisFlags_AutoFit);

        //     ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
        //     // todo Further optimize by not plotting out of view point
        //     ImPlot::PlotLine("y3", p_data(xs), p_data(ys3), length(ys3), 0, 0, sizeof(float));

        //     ImPlot::EndPlot();
        // }

        ImGui::SliderFloat("Window size (sec)", &window_size_sec, 0., window_size_max_sec, "%.2f sec");

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    };

    std::thread update_thread{update_task};

    run_window(
        1280,
        720,
        "Ars Vivendi",
        init_task,
        loop_task);
    update_thread.join();

    return 0;
}