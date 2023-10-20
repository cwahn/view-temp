#include <iostream>
#include <thread>

#include "efp.hpp"

#include "plotlib.hpp"

#include "server.hpp"

using namespace efp;

constexpr const char *target_identifier = "Ars Vivendi BLE";



/*
global buffer

start network
    ..
    manipulate buffer using callback

start imgui
    while true
        show signal list
        if signal_list updaated
            update signal list
            notify network to publish new list

        for num_of_signal
            if signal[i] is checked on list
                show signal_i_graph



*/

std::vector<std::string> sname;
std::vector<SignalType> stype;
std::vector<RtDataFixedRate<120*10>> Data{};


RtDataDynamicRate<120*10> ddd{"as"};
RtPlot1 rt_plot{"rt_plot", -1, -1, &ddd};


// Main code
int main(int, char **)
{

   

    auto nametype_callback = [](const std::vector<std::string> &sname_, const std::vector<SignalType> &stype_)
    {
        sname = sname_;
        stype = stype_;
    };

    auto signal_callback = [](const flexbuffers::Vector &root)
    {


				// #define FLEXBUFFER(TYPE, type) \
				// 	for (int j = 0; j < vectorn.size(); j++) {\
				// 		const auto a = vectorn[j].AsTYPE(); \
				// 		printf("%d, ", a); \
				// 	} \
				// 	printf("\n"); \


				// auto result = root[0].AsInt32();
				// int index = 0;
				// for(int i = 0; i < 32; ++i)
				// {
				// 	if (result & 1 << i)// && out_flag & 1 << sid)
				// 	{
				// 		auto vectorn = root[index++].AsVector();

				// 		switch (stype)
				// 		{
				// 			case t_bool:
				// 			{
				// 				FLEXBUFFER(Bool, bool);
				// 				break;
				// 			}
				// 			case t_float:
				// 			{
				// 				FLEXBUFFER(Float, float);
				// 				break;
				// 			}
				// 			case t_double:
				// 			{
				// 				FLEXBUFFER(Double, double);
				// 				break;
				// 			}
				// 			case t_int:
				// 			{
				// 				FLEXBUFFER(Int32, int);
				// 				break;
				// 			}
				// 		}
						
				// 	}
				// }


        auto out_flag = root[0].AsInt32();
        auto vectorn = root[1].AsVector();
        efp::Vector<int> K;
        for (size_t j = 0; j < vectorn.size()/10; j++) {
            //printf("%d, ", vectorn[j*10].AsInt32());
            K.push_back(vectorn[j*10].AsInt32());
        }
        ddd.push_sequence(K);
        //printf("\n");
    };

//     RtDataDynamicRate<1024> raw_audio{"raw audio"};

// RtPlot1 audio_plot{"audio plot", -1, -1, &raw_audio};






// RtPlot1 xd_plot{"x plot", -1, -1, &x_plot};


//     Vector<int> K;
//     auto audio_callback = [&](int xs)
//     {
//         x_plot.push_now(xs);
  
//     };



    ClientViewer C(std::string("c0"), std::string("d0"), nametype_callback, signal_callback);
    C.init();
  



    auto init_task = [&](ImGuiIO &io)
    {
        using namespace ImGui;
    };

    auto loop_task = [&](ImGuiIO &io)
    {
        using namespace ImGui;
        using namespace ImPlot;

        window("Audio", [&]()
               { rt_plot.plot(); });

        ImGui::ShowDemoWindow();
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