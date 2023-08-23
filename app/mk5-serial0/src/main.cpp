#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <bitset>
#include <thread>
#include <chrono>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

#include "efp.hpp"
#include "rs232-stream.hpp"
#include "signal_processing.hpp"

#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

struct Max30102SerialFrame
{
    int red;
    int ir;
    int temp;
};

struct SerialFrame
{
    Max30102SerialFrame max30102_data;
    int audio[32];
};

static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

float now_sec()
{
    static auto init_time = std::chrono::high_resolution_clock::now();

    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed_seconds = now - init_time;

    return elapsed_seconds.count();
}

// Global constant

// constexpr int buffer_capacity = 1250;
constexpr float max_window_size_sec = 10.;
constexpr int audio_buffer_capacity = 2048 * (int)max_window_size_sec;
constexpr int max30102_buffer_capacity = 50 * (int)max_window_size_sec;

constexpr int min_audio_fft_power = 3;
constexpr int max_audio_fft_power = 11;

constexpr int min_max30102_fft_power = 3;
constexpr int max_max30102_fft_power = 8;

// constexpr float time_delta_sec = 1 / 120.;

// Global variables

ImVec4 clear_color = ImVec4(36 / 255.f, 39 / 255.f, 45 / 255.f, 1.00f);

// std::mutex serial_frame_mutex{};
// SerialFrame serial_frame{};
// bool is_new_frame = false;

int audio_fft_power = 3;
int max30102_fft_power = 3;

float max_time_stamp_sec = 0;
float window_size_sec = 5.;

float max30102_acf_length_sec = 1.;
float max30102_pearson_correlation_length_sec = 1.;
float max30102_spo2_length_sec = 1.;

// display buffer

std::mutex display_buffer_mutex{};

Vcb<float, audio_buffer_capacity> audio_time_secs, audio;
Vcb<float, max30102_buffer_capacity> max30102_time_secs, reds, irs, temps;
std::vector<float> audio_fft_normalized{};
std::vector<float> max30102_fft_normalized{};
std::vector<float> max30102_acf{};
Vcb<float, max30102_buffer_capacity> pearson_correlations{};
Vcb<float, max30102_buffer_capacity> spo2_values{};

Maybe<Unit> glfw_init()
{
    glfwSetErrorCallback(glfw_error_callback);
    if (glfwInit())
        return Unit{};
    else
        return Nothing{};
}

Maybe<const char *> glsl_version()
{
    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char *glsl_version_ = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    return glsl_version_;
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char *glsl_version_ = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac
    return glsl_version_;
#else
    // GL 3.0 + GLSL 130
    const char *glsl_version_ = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
    return glsl_version_;
#endif
    return Nothing{};
}

Maybe<GLFWwindow *> glfw_window(int width, int height, const char *title)
{
    GLFWwindow *window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window == nullptr)
        return Nothing{};
    return window;
}

Maybe<GLFWwindow *> glfw_current_window(int width, int height, const char *title)
{
    // todo Use match
    auto maybe_window = glfw_window(width, height, title);
    if (maybe_window.has_value())
    {
        auto window = maybe_window.value();
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
        return window;
    }
    else
    {
        return Nothing{};
    }
}

// Main function
int main(int, char **)
{
    glfw_init().value();

    auto window = glfw_current_window(1280, 720, "Ars Vivendi BLE Vital Serial Monitor").value();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    // (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.Fonts->AddFontFromFileTTF("/System/Library/Fonts/AppleSDGothicNeo.ttc", 16.0f);
    io.FontDefault = io.Fonts->Fonts[0];

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version().value());

    const char *serial_port = "/dev/cu.usbserial-0001";
    const int baudrate = 115200;

    auto on_read = [](const SerialFrame &data)
    {
        // Pushing data to processing buffer
        float now_value_sec = now_sec();

        // todo Single data push abstraction
        max30102_time_secs.push_back(now_value_sec);
        reds.push_back((float)data.max30102_data.red / (float)1000.);
        irs.push_back((float)data.max30102_data.ir / (float)1000.);
        temps.push_back((float)data.max30102_data.temp / (float)1000.);

        // todo Multiple data push abstraction
        for_each([&](auto x)
                 { audio.push_back(x); },
                 data.audio);
        float last_value_sec = p_data(audio_time_secs)[audio_buffer_capacity];
        float time_tick = (now_value_sec - last_value_sec) / length(data.audio);

        for_index([&](int i)
                  { audio_time_secs.push_back(last_value_sec + time_tick * (i + 1)); },
                  length(data.audio));

        // FFT audio
        size_t audio_fft_length = 1 << audio_fft_power;

        auto normalized_abs = [&](auto x)
        { float normailzed_x = x/(float)audio_fft_length;
        return normailzed_x >= 0 ? normailzed_x : -normailzed_x; };

        VectorView<const float> audio_fft_input{p_data(audio) + audio_buffer_capacity - audio_fft_length, audio_fft_length};
        auto audio_fft_raw = fft_real(detrend<float>(remove_dc<float>(audio_fft_input)));
        audio_fft_normalized = map(normalized_abs, audio_fft_raw);

        // todo FFT max30102
        size_t max30102_fft_length = 1 << max30102_fft_power;
        VectorView<const float> max30102_fft_view{p_data(irs) + max30102_buffer_capacity - max30102_fft_length, max30102_fft_length};
        auto dc_removed_ir = remove_dc<float>(max30102_fft_view);
        auto detrended_ir = detrend<float>(dc_removed_ir);
        auto max30102_fft_raw = fft_real(detrended_ir);
        max30102_fft_normalized = map(normalized_abs, max30102_fft_raw);

        // todo ACF max30102
        size_t max30102_acf_length = (int)(max30102_acf_length_sec * 50);
        VectorView<const float> max30102_acf_view{p_data(irs) + max30102_buffer_capacity - max30102_acf_length, max30102_acf_length};
        auto afc_dc_removed_ir = remove_dc<float>(max30102_acf_view);
        auto afc_detrended_ir = detrend<float>(afc_dc_removed_ir);
        max30102_acf = from_function(length(afc_detrended_ir), [&](int i)
                                     { return autocorrelation<float>(irs, i); });

        // todo pearson correlation
        size_t max30102_pearson_correlation_length = (int)(max30102_pearson_correlation_length_sec * 50);
        VectorView<const float> pearson_correlation_reds_view{p_data(reds) + max30102_buffer_capacity - max30102_pearson_correlation_length, max30102_pearson_correlation_length};
        VectorView<const float> pearson_correlation_irs_view{p_data(irs) + max30102_buffer_capacity - max30102_pearson_correlation_length, max30102_pearson_correlation_length};
        pearson_correlations.push_back(
            correlation<float>(
                detrend<float>(remove_dc<float>(pearson_correlation_reds_view)),
                detrend<float>(remove_dc<float>(pearson_correlation_irs_view))));

        // todo last
        // todo SPO2 and other intermediates
        size_t max30102_spo2_length = (int)(max30102_spo2_length_sec * 50);
        VectorView<const float> spo2_reds_view{p_data(reds) + max30102_buffer_capacity - max30102_spo2_length, max30102_spo2_length};
        VectorView<const float> spo2_irs_view{p_data(irs) + max30102_buffer_capacity - max30102_spo2_length, max30102_spo2_length};

        spo2_values.push_back(spo2(rms<float>(detrend<float>(remove_dc<float>(spo2_reds_view))),
                                   mean<float>(spo2_reds_view),
                                   rms<float>(detrend<float>(remove_dc<float>(spo2_irs_view))),
                                   mean<float>(spo2_irs_view)));
    };

    Rs232Stream<SerialFrame, decltype(on_read)>
        serial{serial_port, baudrate, on_read};

    static Vector<float> freqs = from_function(125, [](int i)
                                               { return i * 1024.f / 128.f / 2.f; });

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // static BleFrameFloat ble_frame_snapshot;
        // static float ble_read_duration_snapshot_ms;

        // todo fetch data

        {
            display_buffer_mutex.lock();
            std::vector<float> audio_fft_normalized_ = audio_fft_normalized;
            std::vector<float> max30102_fft_normalized_ = max30102_fft_normalized;
            std::vector<float> max30102_acf_ = max30102_acf;
            Vcb<float, max30102_buffer_capacity> pearson_correlations_ = pearson_correlations;
            Vcb<float, max30102_buffer_capacity> spo2_values_ = spo2_values;
            display_buffer_mutex.unlock();
        }

        // todo Variable window raw data time view
        {
            ImGui::Begin("BLE Vital Raw Data Real-time Monitor");

            if (serial.is_connected())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
                ImGui::Text("RS232 serial connected");
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
                ImGui::Text("RS232 serial disconnected");
                ImGui::PopStyleColor();
            }

            static ImPlotAxisFlags flags = ImPlotAxisFlags_RangeFit;

            // if (ImPlot::BeginPlot("Normalized FFT", ImVec2(-1, 500)))
            // {
            //     ImPlot::SetupAxes("frequency (Hz)", "normalized fft", flags, flags);
            //     ImPlot::SetupAxisLimits(ImAxis_X1, 0, 124 * 1024.f / 128.f / 2.f, ImGuiCond_Always);
            //     ImPlot::SetupAxisLimits(ImAxis_Y1, -1, maximum(ble_frame.fft) < 20 ? 20 : maximum(ble_frame.fft) * 1.1, ImGuiCond_Always);
            //     ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
            //     ImPlot::PlotLine("FFT", p_data(freqs), p_data(ble_frame.fft), 125, 0, 0, sizeof(float));
            //     ImPlot::EndPlot();
            // }

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // ImGui::Text("\nSpo2: %f \nHeart rate: %f \ntemperature: %f \nBLE read duration (ms): %f\n ",
        //             ble_frame_snapshot.spo2,
        //             ble_frame_snapshot.heart_rate,
        //             ble_frame_snapshot.temperature,
        //             ble_read_duration_ms);

        // todo FFT of audio and ir, ACF of ir

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
