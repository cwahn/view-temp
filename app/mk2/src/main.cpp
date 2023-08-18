#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>

#include "efp.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

#include "ble-vital-client.hpp"

using namespace efp;

#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

constexpr const char *target_identifier = "Ars Vivendi BLE";

struct BleFrame
{
    int spo2;
    int heart_rate;
    int temperature;
    int fft[125];
};

struct BleFrameFloat
{
    float spo2;
    float heart_rate;
    float temperature;
    float fft[125];

    // BleFrameFloat(const &BleFrame frame)
    // {
    //     spo2 = frame.spo2 / 1000000.;
    //     heart_rate = frame.heart_rate / 1000000.;
    //     temperature = frame.temperature / 1000000.;
    //     for_each([](auto x)
    //              { return (float)(x) / 1000000.; },
    //              frame.fft);
    // }
};

static void
glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

ImVec4 clear_color = ImVec4(36 / 255.f, 39 / 255.f, 45 / 255.f, 1.00f);

std::mutex ble_frame_mutex{};
BleFrameFloat ble_frame{};
float ble_read_duration_ms{};
bool is_new_frame = false;
bool is_ble_connected = false;
SimpleBLE::Peripheral found_peripheral;

// Main code
int main(int, char **)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

        // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char *glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char *glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow *window = glfwCreateWindow(1280, 720, "Ars Vivendi BLE Vital", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.Fonts->AddFontFromFileTTF("/System/Library/Fonts/AppleSDGothicNeo.ttc", 16.0f);
    io.FontDefault = io.Fonts->Fonts[0];

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our states

    const auto setup_task = [&](SimpleBLE::Adapter &adp)
    {
        std::cout << "Setting up GATT client" << std::endl;
    };

    const GapCallback gap_callback = {
        [&](SimpleBLE::Adapter &adp)
        {
            std::cout << "Starting scan" << std::endl;
        },
        [&](SimpleBLE::Adapter &adp)
        {
            std::cout << "Scan stopped" << std::endl;
        },
        [&](SimpleBLE::Adapter &adp, SimpleBLE::Peripheral p)
        {
            const auto id = p.identifier();
            std::cout << "Found device: " << id << " [" << p.address() << "] " << p.rssi() << " dBm" << std::endl;

            if (id == target_identifier)
            {
                std::cout << "Target device found" << std::endl;
                // found_peripheral = p;
                adp.scan_stop();

                auto read_task = [&](SimpleBLE::Peripheral p)
                {
                    std::cout << "Connecting to the device" << std::endl;
                    while (!p.is_connected())
                    {
                        try
                        {
                            p.connect();
                        }
                        catch (...)
                        {
                            std::cout << "Connecting failed with exception. Retrying" << std::endl;
                            // p.connect();
                        }
                    }

                    std::cout << "Connected. MTU: " << p.mtu() << std::endl;
                    is_ble_connected = true;

                    auto service_uuid = p.services()[0].uuid();
                    auto characteristic_uuid = p.services()[0].characteristics()[0].uuid();

                    while (p.is_connected())
                    {
                        try
                        {
                            const auto read_start = std::chrono::high_resolution_clock::now();
                            SimpleBLE::ByteArray rx_data = p.read(service_uuid, characteristic_uuid);
                            const auto read_end = std::chrono::high_resolution_clock::now();
                            const auto read_duration = std::chrono::duration_cast<std::chrono::milliseconds>(read_end - read_start);

                            {
                                ble_frame_mutex.lock();
                                BleFrame *p_ble_frame = (BleFrame *)rx_data.c_str();

                                ArrayView<float, 125> fft_view{(float *)&(p_ble_frame->fft)};
                                for_each_with_index([](int i, auto x)
                                                    { ble_frame.fft[i] = (x >= 0 ? x : -x) / (128. * 1000000.); },
                                                    fft_view);
                                ble_frame.spo2 = (float)p_ble_frame->spo2 / 1000000.;
                                ble_frame.heart_rate = (float)p_ble_frame->heart_rate / 1000000.;
                                ble_frame.temperature = (float)p_ble_frame->temperature / 1000000.;
                                ble_read_duration_ms = read_duration.count();
                                is_new_frame = true;
                                ble_frame_mutex.unlock();
                            }

                            std::cout << "Received " << rx_data.size() << " bytes ";
                            std::cout << "in " << std::dec << read_duration.count() << " ms" << std::endl;
                        }
                        catch (SimpleBLE::Exception::OperationFailed &ex)
                        {
                            if (!p.is_connected())
                            {
                                std::cout << "Peripheral disconnected with exception" << std::endl;
                                is_ble_connected = false;
                                adp.scan_start();
                            }
                        }
                    }

                    std::cout << "Peripheral disconnected" << std::endl;
                    is_ble_connected = false;
                    adp.scan_start();
                };

                std::thread read_thread(read_task, p);
                read_thread.detach();
            }
        },
        [&](SimpleBLE::Adapter &adp, SimpleBLE::Peripheral p) {

        }};

    // std::thread gui_thread(gui_task);

    ClientConfig config{setup_task, gap_callback};
    Client client{config};

    static Vector<float> freqs = from_function(125, [](int i)
                                               { return i * 1024.f / 128.f / 2.f; });

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        static BleFrameFloat ble_frame_snapshot;
        static float ble_read_duration_snapshot_ms;

        {
            ble_frame_mutex.lock();
            if (is_new_frame)
            {
                is_new_frame = false;
                ble_frame_snapshot = ble_frame;
                ble_read_duration_snapshot_ms = ble_read_duration_ms;
            }
            ble_frame_mutex.unlock();
        }

        ImGui::Begin("BLE Vital Real-time Monitor");

        if (is_ble_connected)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
            ImGui::Text("BLE connected");
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
            ImGui::Text("BLE disconnected");
            ImGui::PopStyleColor();
        }

        ImGui::Text("\nSpo2: %f \nHeart rate: %f \ntemperature: %f \nBLE read duration (ms): %f\n ",
                    ble_frame_snapshot.spo2 / 1000000.,
                    ble_frame_snapshot.heart_rate / 1000000.,
                    ble_frame_snapshot.temperature / 1000000.,
                    ble_read_duration_ms);

        // ImGui::Text("Normalized FFT");

        static ImPlotAxisFlags flags = ImPlotAxisFlags_RangeFit;

        // auto fft_float = map([](auto x)
        //                      { return (float)(x); },
        //                      ble_frame_snapshot.fft)

        if (ImPlot::BeginPlot("Normalized FFT", ImVec2(-1, 500)))
        {
            ImPlot::SetupAxes("frequency (Hz)", "normalized fft", flags, flags);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0, 124 * 1024.f / 128.f / 2.f, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -1, maximum(ble_frame.fft) < 20 ? 20 : maximum(ble_frame.fft) * 1.1, ImGuiCond_Always);
            ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
            ImPlot::PlotLine("FFT", p_data(freqs), p_data(ble_frame.fft), 125, 0, 0, sizeof(float));
            ImPlot::EndPlot();
        }

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
