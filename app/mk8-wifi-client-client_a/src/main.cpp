#include <iostream>
#include <thread>

#include "efp.hpp"

#include "plotlib.hpp"

#include "server.hpp"

using namespace efp;

constexpr const char *target_identifier = "Ars Vivendi BLE";



// Main code
int main(int, char **)
{

    RtDataDynamicRate<1024> raw_audio{"raw audio"};
RtPlot1 audio_plot{"audio plot", -1, -1, &raw_audio};

    Vector<int> K;
    auto audio_callback = [&](int xs)
    {
        raw_audio.push_now(xs*0.05);
  
    };


    
    mainX();
  



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

        ImGui::ShowDemoWindow();
    };

    // run_gui(
    //     1920,
    //     1080,
    //     "Ars Vivendi",
    //     init_task,
    //     loop_task);



    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1000));
    }

    return 0;
}