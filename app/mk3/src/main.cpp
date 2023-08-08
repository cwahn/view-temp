#include <iostream>

#include "efp.hpp"
#include "./plotter.hpp"

using namespace efp;

constexpr const char *target_identifier = "Ars Vivendi BLE";

// Main code
int main(int, char **)
{
    const auto xs = from_function(125, id<float>);
    const auto sin_wave = from_function(125,
                                        [](int x) -> float
                                        { return sin(x / 125. * 2 * M_PI); });

    auto init_task = [&]() {};
    auto loop_task = [&](ImGuiIO &io)
    {
        ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.
        ImGui::Text("Spo2: %f \nHeart rate: %f \ntemperature: %f \n",
                    0.,
                    42.,
                    3.141562);

        ImGui::Text("FFT Graph");

        static ImPlotAxisFlags flags = ImPlotAxisFlags_RangeFit;

        if (ImPlot::BeginPlot("##Scrolling", ImVec2(-1, 500)))
        {
            ImPlot::SetupAxes(nullptr, nullptr, flags, flags);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0, 125, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);
            ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
            ImPlot::PlotLine("FFT", p_data(xs), p_data(sin_wave), 125, 0, 0, sizeof(float));
            ImPlot::EndPlot();
        }

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    };

    run_window(
        1280,
        720,
        "Ars Vivendi",
        init_task,
        loop_task);

    return 0;
}
