#include <iostream>
#include <thread>
#include <bitset>
#include <map>

#include "efp.hpp"

#include "plotlib.hpp"

#include "server.hpp"

using namespace efp;

constexpr const char *target_identifier = "Ars Vivendi BLE";


/// opt/homebrew/opt/mosquitto/sbin/mosquitto -c /opt/homebrew/etc/mosquitto/mosquitto.conf
// sudo nano /opt/homebrew/etc/mosquitto/mosquitto.conf

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


todo: discon,recon
*/

// 47 home
// 16  ars

auto tp = std::chrono::high_resolution_clock::now();

auto unpack(const flexbuffers::Vector &buffer, const SignalType &stype)
{
    const auto len = buffer.size();
    efp::Vector<double> K;
    K.reserve(len);

#define FLEXBUFFER(TYPE, type)                     \
    for (int j = 0; j < len; j++)                  \
    {                                              \
        K.push_back((double)buffer[j].As##TYPE()); \
    };
    // printf("%f, ", (double)buffer[j].As##TYPE()); printf("\n");

#define FLEXDEBUG_INT()                   \
    for (int j = 0; j < len; j++)                  \
    {                                              \
        printf("%d, ", buffer[j].AsInt32()); \
    }; printf("\n");


    switch (stype)
    {
    case t_bool:
    {
        FLEXBUFFER(Bool, bool);
        break;
    }
    case t_float:
    {
        FLEXBUFFER(Float, float);
        break;
    }
    case t_double:
    {
        FLEXBUFFER(Double, double);
        break;
    }
    case t_int:
    {
        FLEXBUFFER(Int32, int);
        // FLEXDEBUG_INT();
        break;
    }
    case t_int64:
    {
        FLEXBUFFER(Int64, int64_t);
        //FLEXDEBUG_INT();
        break;
    }
    }

    return K;
}

class Gui
{
public:
    Gui()
    {
    }

    ~Gui()
    {
    }

    void start_client()
    {
        if (Cli)
            return;

        // device sent signal list
        // append thread, signal list only.
        // don't care about actual buffer here
        auto nametype_callback = [&](const flexbuffers::Vector &root)
        {
            //std::unique_lock<std::mutex> m(m_buf);

            connected = true;

            const auto tlen = root.size() / 3;
            if (tlen > thread_data.size())
            {
                thread_data.resize(tlen);
                thread_name_c.resize(tlen);
            }

            // full sized
            for (int tid = 0; tid < tlen; ++tid)
            {
                int index = tid * 3;

                auto sname = root[index + 1].AsVector();
                auto stype = root[index + 2].AsVector();
                int slen = sname.size();

                auto &td = thread_data[tid];

                int sdlen = td.signal_name.size();

                // check whether there is new signal or not
                // as table size is never shrink, just compare the size
                if (slen > sdlen)
                {
                    for (int i = sdlen; i < slen; ++i)
                    {
                        td.signal_name.emplace_back(sname[i].AsString().str());
                        td.signal_type.emplace_back(static_cast<SignalType>(stype[i].AsInt32()));
                        td.active_list.emplace_back(static_cast<int8_t>(false));
                    }

                    td.thread_name = root[index].AsString().str();
                    thread_name_c[tid] = td.thread_name.c_str();
                }
            }

            for (int tid = 0; tid < tlen; ++tid)
            {

                printf(thread_name_c[tid]);
                printf("\n\t");

                for (auto c : thread_data[tid].signal_name)
                    printf("%s, ", c.c_str());
                printf("\n");
            }
        };

        // device sent actual signal
        auto signal_callback = [&](const flexbuffers::Vector &root)
        {
            //std::unique_lock<std::mutex> m(m_buf);
             auto now = std::chrono::high_resolution_clock::now();
            auto d = std::chrono::duration_cast<std::chrono::milliseconds>(now - tp);
           
            dt.push_now( d.count());
            tp = now;
            
            
            int16_t in_bitflag = root[0].AsInt32();

            int tid_idx = 0;

            const auto tlen = 1 + std::bitset<16>(in_bitflag).count();

            // not full sized
            for (int t = 1; t < tlen; ++t)
            {
                // determine thread id from in_bitflag (lowe bit first)
                int tid = 32;
                while (tid_idx < 16)
                {
                    if (in_bitflag & 1 << tid_idx)
                    {
                        tid = tid_idx++;
                        break;
                    }
                    ++tid_idx;
                }
                // client and device has defferent table
                if (tid >= thread_data.size())
                    break;

                auto signal = root[t].AsVector();

                auto sid = signal[0].AsVector();
                auto seq_of_signal = signal[1].AsVector();

                // number of signals of thread tid
                const auto snum = sid.size();

                const auto tsnum = thread_data[tid].sbuffer.size();

                for (int i = 0; i < snum; ++i)
                {
                    const int sidx = sid[i].AsInt32();

                    // client and device has defferent table
                    auto search = thread_data[tid].sbuffer.find(sidx);
                    if (search == thread_data[tid].sbuffer.end())
                        continue;

                    const SignalType stype = thread_data[tid].signal_type[sidx];

                    const auto k = seq_of_signal[i].AsVector();

                    // if (sidx == 0) {
                    //     efp::Vector<double> K{1.3,1.3,1.3,1.3,1.3,1.3,1.3};
                    //     search->second->push_sequence(K);
                    // }
                    // else
                    search->second->push_sequence(unpack(k, stype));
                }
            }
        };

        auto disconnected_callback = [&](const bool &con)
        {
            std::unique_lock<std::mutex> m(m_buf);
            connected = false;

            thread_data.clear();
            thread_name_c.clear();
        };

        Cli = std::unique_ptr<ClientViewer<>>(new ClientViewer<>(std::string("c0"), std::string("d0"), nametype_callback, signal_callback, disconnected_callback));
        Cli->init();
    }

    void update_and_publish(int tid, int sid)
    {
        auto &td = thread_data[tid];
        bool state = td.active_list[sid];

        if (state)
        {
            td.sbuffer.emplace(sid, new RtDataDynamicRate<10 * 100 * 20>(td.signal_name[sid]));
            td.splot.emplace(sid, new RtPlot1<RtDataDynamicRate<10 * 100 * 20>>(td.signal_name[sid], -1, -1, td.sbuffer[sid]));
        }
        else
        {
            // first, delete the plot
            delete td.splot[sid];
            td.splot.erase(sid);
            
            // next, delete buffer
            delete td.sbuffer[sid];
            td.sbuffer.erase(sid);
        }

        std::vector<std::vector<SignalID>> ViewList;


        //std::unique_lock<std::mutex> m(m_buf);

        // todo: this is unneccesary stage. see publish_slist()
        const auto tlen = thread_data.size();
        for (int tid = 0; tid < tlen; ++tid)
        {
            std::vector<SignalID> s;
            auto &tdd = thread_data[tid];
            for (int sid = 0; sid < tdd.active_list.size(); ++sid)
            {
                if (tdd.active_list[sid])
                    s.push_back(sid);
            }

            ViewList.push_back(s);
        }

        Cli->publish_slist(ViewList);

        for (int tid = 0; tid < tlen; ++tid)
        {
            for (auto c : thread_data[tid].active_list)
                printf("%d, ", c);
            printf("\n");
        }
    }

    auto get_thread_name()
    {
        // todo: is this safe? const char*
        std::unique_lock<std::mutex> m(m_buf);
        return thread_name_c;
    }
    auto get_signal_name(int tid)
    {
        std::unique_lock<std::mutex> m(m_buf);
        return thread_data[tid].signal_name;
    }

    std::mutex m_buf;

    struct ThreadBuffer
    {
        std::string thread_name;
        std::vector<std::string> signal_name;
        std::vector<SignalType> signal_type;

        std::vector<int8_t> active_list; //

        // not full sized
        // added or removed when update_and_publish called
        std::map<SignalID, RtDataDynamicRate<10 * 100 * 20> *> sbuffer;
        std::map<SignalID, RtPlot1<RtDataDynamicRate<10 * 100 * 20>> *> splot;
    };

    // delta time?
    RtDataDynamicRate<10 * 100 * 20> dt{"delta time"};
    RtPlot1<RtDataDynamicRate<10 * 100 * 20>> dtp{"dt plot", -1, -1, &dt};
    
    std::vector<ThreadBuffer> thread_data;

    // source string exists at std::string thread_name;
    std::vector<const char *> thread_name_c;

    bool connected{false};

private:
    std::unique_ptr<ClientViewer<>> Cli;
};

static int item_current_idx = -1; // Here we store our selection data as an index.

// Main code
int main(int, char **)
{

    Gui guib;
    guib.start_client();

 
    auto init_task = [&](ImGuiIO &io)
    {
        using namespace ImGui;
    };

/*
    스레드 당
        신호들의 이름
        신호들의 타입

        신호들의 체크여부 bool
        체크된 신호들의 버퍼 id로 접근하여 버퍼를 출력 map
        버퍼에 대응하는 plot클래스들
    의 배열

    스레드 이름의 배열


    어떤 신호들이 체크 되었는지 Bool 배열
    위 배열에 따라 실제 rt버퍼 벡터를 조정
    체크 가 눌려 값 변동시 업데이트 호출 (어떤 스레드의 어떤 신호id)
        업데이트가 위 작업 수행 후 리스트를 디바이스에 퍼블리시


*/
#if 1
    auto loop_task = [&](ImGuiIO &io)
    {
        using namespace ImGui;
        using namespace ImPlot;

        {
            Begin("asdasd");

            Text("Broker address / port");

            static char str1[128] = "192.168.0.29";

            SetNextItemWidth(0.5*ImGui::GetContentRegionAvail().x);
            ImGui::InputTextWithHint("##address", "0.0.0.0", str1, IM_ARRAYSIZE(str1));
            ImGui::SameLine();
            SetNextItemWidth(0.25*ImGui::GetContentRegionAvail().x);
            static char buf2[6] = "9884"; ImGui::InputText("##port", buf2, 4, ImGuiInputTextFlags_CharsDecimal);

            static char str2[8] = "d0";
            ImGui::InputTextWithHint("device name", "d0", str2, IM_ARRAYSIZE(str2));

            static int clicked = 0;
            ImGui::Button("fsdfsef");//if ())
                clicked++;

            Text("sdfsdme:");
                
            {
                ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x * 0.3f, ImGui::GetContentRegionAvail().y), false, 0);
                
                ImGui::Text("thread name");
                if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y)))
                {
                    const auto items = guib.get_thread_name();

                    for (int n = 0; n < items.size(); n++)
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
                if (item_current_idx > -1)
                {
                    const auto items = guib.get_signal_name(item_current_idx);

                    for (int n = 0; n < items.size(); n++)
                    {
                        if (ImGui::Checkbox(items[n].c_str(), reinterpret_cast<bool *>(&guib.thread_data[item_current_idx].active_list[n])))
                        {
                            guib.update_and_publish(item_current_idx, n);
                        }
                    }
                }
                ImGui::EndChild();
            }

            End();
        }

        for (int tid = 0; tid < guib.thread_data.size(); ++tid)
        {
            const auto list = guib.thread_data[tid].active_list;
            const auto len = list.size();
            for (int sid = 0; sid < len; ++sid)
            {
                if (list[sid] == 1)
                    window((guib.thread_data[tid].thread_name + "/" + guib.thread_data[tid].signal_name[sid]).c_str(), [&]()
                           { guib.thread_data[tid].splot[sid]->plot(); });
            }
        }

        // debug window
        window("elapsed time", [&]()
                           { guib.dtp.plot(); });
    };

    run_gui(
        1920,
        1080,
        "Ars Vivendi",
        init_task,
        loop_task);
#endif



    // while (true)
    // {
    //     std::this_thread::sleep_for(std::chrono::seconds(1000));
    // }

    return 0;
}