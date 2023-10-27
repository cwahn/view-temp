#include <iostream>
#include <thread>



#include "plotlib.hpp"




// Main code
int main(int, char **)
{



    auto init_task = [&](ImGuiIO &io)
    {
        using namespace ImGui;
    };

    auto loop_task = [&](ImGuiIO &io)
    {
        using namespace ImGui;
        using namespace ImPlot;

        Begin("asdasd");

         ImGui::Text("signal name:");
        {
            ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x * 0.25f, ImGui::GetContentRegionAvail().y), false, 0);
            const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIII", "JJJJ", "KKKK", "LLLLLLL", "MMMM", "OOOOOOO" };
            static int item_current_idx = 0; // Here we store our selection data as an index.
            ImGui::Text("thread name");
            if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y)))
            {
                for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                {
                    const bool is_selected = (item_current_idx == n);
                    if (ImGui::Selectable(items[n], is_selected))
                        item_current_idx = n;

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndListBox();
            }

            ImGui::EndChild();
        }
      

        ImGui::SameLine();

        {

            ImGui::BeginChild("ChildR", ImVec2(0, ImGui::GetContentRegionAvail().y), false, 0);
            ImGui::Text("signal name:");

            static ImGuiSliderFlags flags = 0;
            for (int n = 0; n < 42; n++)
            {
                ImGui::CheckboxFlags("ImGuiSliderFlags_AlwaysClamp", &flags, 0);
            }
            ImGui::EndChild();
        }

        End();
        // ImGui::ShowDemoWindow();
    };

    run_gui(
        1920,
        1080,
        "Ars Vivendi",
        init_task,
        loop_task);



    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1000));
    }

    return 0;
}