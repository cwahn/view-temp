// #include <math.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <time.h>

#include "efp.hpp"
// #include "imgui.h"
// #include "imgui_impl_glfw.h"
// #include "imgui_impl_opengl3.h"
// #include "implot.h"

using namespace efp;

// #define GL_SILENCE_DEPRECATION
// #if defined(IMGUI_IMPL_OPENGL_ES2)
// #include <GLES2/gl2.h>
// #endif
// #include <GLFW/glfw3.h> // Will drag system OpenGL headers

// // [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// // To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// // Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
// #if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
// #pragma comment(lib, "legacy_stdio_definitions")
// #endif

// // This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
// // #ifdef __EMSCRIPTEN__
// // #include "../libs/emscripten/emscripten_mainloop_stub.h"
// // #endif

// static void glfw_error_callback(int error, const char *description)
// {
//     fprintf(stderr, "GLFW Error %d: %s\n", error, description);
// }

// float lpf_60_05(float x)
// {
//     static float x_1;
//     static float y_1;

//     float y = 0.004149377593361 * x + 0.004149377593361 * x_1 - ((-0.991701244813278) * y_1);
//     x_1 = x;
//     y_1 = y;

//     return y;
// }

// // Main code
// int main(int, char **)
// {
//     glfwSetErrorCallback(glfw_error_callback);
//     if (!glfwInit())
//         return 1;

//         // Decide GL+GLSL versions
// #if defined(IMGUI_IMPL_OPENGL_ES2)
//     // GL ES 2.0 + GLSL 100
//     const char *glsl_version = "#version 100";
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
//     glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
// #elif defined(__APPLE__)
//     // GL 3.2 + GLSL 150
//     const char *glsl_version = "#version 150";
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
//     glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
//     glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac
// #else
//     // GL 3.0 + GLSL 130
//     const char *glsl_version = "#version 130";
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
//     // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
//     // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
// #endif

//     // Create window with graphics context
//     GLFWwindow *window = glfwCreateWindow(1280, 720, "Ars Vivendi BLE Vital", nullptr, nullptr);
//     if (window == nullptr)
//         return 1;
//     glfwMakeContextCurrent(window);
//     glfwSwapInterval(1); // Enable vsync

//     // Setup Dear ImGui context
//     IMGUI_CHECKVERSION();
//     ImGui::CreateContext();
//     ImPlot::CreateContext();

//     ImGuiIO &io = ImGui::GetIO();
//     (void)io;
//     io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
//     io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

//     io.Fonts->AddFontFromFileTTF("/System/Library/Fonts/AppleSDGothicNeo.ttc", 16.0f);
//     io.FontDefault = io.Fonts->Fonts[0];

//     // Setup Dear ImGui style
//     ImGui::StyleColorsDark();
//     // ImGui::StyleColorsLight();

//     // Setup Platform/Renderer backends
//     ImGui_ImplGlfw_InitForOpenGL(window, true);
//     ImGui_ImplOpenGL3_Init(glsl_version);

//     // Our state
//     bool show_demo_window = true;
//     ImVec4 clear_color = ImVec4(36 / 255.f, 39 / 255.f, 45 / 255.f, 1.00f);

//     // Main loop

//     while (!glfwWindowShouldClose(window))
//     {
//         glfwPollEvents();

//         // Start the Dear ImGui frame
//         ImGui_ImplOpenGL3_NewFrame();
//         ImGui_ImplGlfw_NewFrame();
//         ImGui::NewFrame();

//         {
//             static float f = 0.0f;
//             static int counter = 0;

//             ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.
//             ImGui::Text("Graph");          // Display some text (you can use a format strings too)

//             static Vcb<float, 300> ts;
//             static Vcb<float, 300> xs;
//             static Vcb<float, 300> ys;

//             ImVec2 mouse = ImGui::GetMousePos();
//             static float t = 0;
//             t += ImGui::GetIO().DeltaTime;

//             ts.push_back(t);
//             xs.push_back(lpf_60_05(mouse.x * 0.001f));
//             ys.push_back(mouse.y * 0.001f);

//             // static ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_RangeFit;
//             static ImPlotAxisFlags flags = ImPlotAxisFlags_RangeFit;

//             if (ImPlot::BeginPlot("##Scrolling", ImVec2(-1, 500)))
//             {
//                 ImPlot::SetupAxes(nullptr, nullptr, flags, flags);
//                 ImPlot::SetupAxisLimits(ImAxis_X1, t - 5., t, ImGuiCond_Always);
//                 ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);
//                 ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
//                 ImPlot::PlotLine("Mouse X", p_data(ts), p_data(xs), length(xs), 0, 0, sizeof(float));
//                 ImPlot::PlotLine("Mouse Y", p_data(ts), p_data(ys), length(ys), 0, 0, sizeof(float));
//                 ImPlot::EndPlot();
//             }

//             ImGui::SliderFloat("float", &f, 0.0f, 1.0f);             // Edit 1 float using a slider from 0.0f to 1.0f
//             ImGui::ColorEdit4("clear color", (float *)&clear_color); // Edit 3 floats representing a color

//             if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
//                 counter++;
//             ImGui::SameLine();
//             ImGui::Text("counter = %d", counter);

//             ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
//             ImGui::End();
//         }

//         // Rendering
//         ImGui::Render();
//         int display_w, display_h;
//         glfwGetFramebufferSize(window, &display_w, &display_h);
//         glViewport(0, 0, display_w, display_h);
//         glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
//         glClear(GL_COLOR_BUFFER_BIT);
//         ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

//         glfwSwapBuffers(window);
//     }
// #ifdef __EMSCRIPTEN__
//     EMSCRIPTEN_MAINLOOP_END;
// #endif

//     // Cleanup
//     ImGui_ImplOpenGL3_Shutdown();
//     ImGui_ImplGlfw_Shutdown();
//     ImPlot::DestroyContext();
//     ImGui::DestroyContext();

//     glfwDestroyWindow(window);
//     glfwTerminate();

//     return 0;
// }

#include "serialib.h"
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <bitset>
#include <thread>
#include <chrono>

struct IntPair
{
    int a;
    int b;
};

template <typename A>
struct Rs232PacketStream
{
public:
    static std::optional<Rs232PacketStream> initialize(const char *device, const unsigned int baudrate)
    {
        Rs232PacketStream stream{};

        char res = stream.serial_.openDevice(device, baudrate);
        if (res != 1)
        {
            return std::nullopt;
        }

        uint8_t start_byte = 0;
        while (start_byte != 0x55)
        {
            stream.serial_.readBytes(&start_byte, 1);
            printf("start byte: 0x%x \n", start_byte);
        }

        uint8_t rx_buffer[sizeof(A)];
        stream.serial_.readBytes(&rx_buffer, sizeof(A));

        // stream.read_period_ = sizeof(A) / (baudrate / 8.) * 1000;
        stream.read_period_ = 1000;

        return stream;
    }

    ~Rs232PacketStream()
    {
        serial_.closeDevice();
        keep_read_ = false;
    }

    Rs232PacketStream(const Rs232PacketStream &) = delete;
    Rs232PacketStream &operator=(const Rs232PacketStream &) = delete;

    // Define move constructor and move assignment operator
    Rs232PacketStream(Rs232PacketStream &&other) noexcept
        : serial_(std::move(other.serial_)),
          read_period_(other.read_period_),
          keep_read_(other.keep_read_)
    {
        other.keep_read_ = false; // Disable the moved-from object
    }

    template <typename F>
    void begin_read(const F &on_read)
    {
        const auto read_task = [&]()
        {
            while (keep_read_)
            {
                auto start = std::chrono::high_resolution_clock::now();

                std::cout << "is_device_open " << serial_.isDeviceOpen() << std::endl;

                uint8_t start_byte = 0;
                while (start_byte != 0x55)
                {
                    serial_.readBytes(&start_byte, 1);
                    // printf("start byte: 0x%x \n", start_byte);
                }

                uint8_t rx_buffer[sizeof(A)];
                serial_.readBytes(&rx_buffer, sizeof(A));

                on_read(*(A *)(&rx_buffer));

                auto end = std::chrono::high_resolution_clock::now();
                auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

                // Calculate the remaining time until the next read period
                auto remaining_time = std::chrono::milliseconds(read_period_) - elapsed_time;

                if (remaining_time > std::chrono::milliseconds(0))
                {
                    std::this_thread::sleep_for(remaining_time);
                }
            }
        };

        std::thread thread_(read_task);
        read_thread_ = std::move(thread_);
    }

private:
    Rs232PacketStream(){};

    serialib serial_;
    int read_period_;
    bool keep_read_ = true;
    std::thread read_thread_;
};

int main(int argc, char const *argv[])
{
    const char *serial_port = "/dev/cu.usbserial-0001";
    const int baudrate = 115200;

    // serialib serial;
    // char res = serial.openDevice(serial_port, baudrate);
    // if (res != 1)
    // {
    //     return res;
    // }

    // std::cout << "connected" << std::endl;

    // std::cout << "available bytes" << serial.available() << std::endl;

    // uint8_t start_byte = 0;
    // while (start_byte != 0x55)
    // {
    //     serial.readBytes(&start_byte, 1);
    //     printf("start byte: 0x%x \n", start_byte);
    // }

    // uint8_t rx_buffer[1024];
    // int rx_size = serial.readBytes(rx_buffer, 8);

    // std::cout << "receicved " << rx_size << " bytes" << std::endl;

    // IntPair pair = *(IntPair *)(&rx_buffer);
    // std::cout << "a: " << pair.a << ", b: " << pair.b << std::endl;

    // start_byte = 0;
    // while (start_byte != 0x55)
    // {
    //     serial.readBytes(&start_byte, 1);
    //     printf("start byte: 0x%x \n", start_byte);
    // }

    // rx_size = serial.readBytes(rx_buffer, 8);

    // std::cout << "receicved " << rx_size << " bytes" << std::endl;

    // pair = *(IntPair *)(&rx_buffer);
    // std::cout << "a: " << pair.a << ", b: " << pair.b << std::endl;

    // Rs232PacketStream<IntPair> stream =
    //     Rs232PacketStream<IntPair>::initialize(serial_port, baudrate).value();

    // auto on_read = [](const IntPair &data)
    // {
    //     std::cout << "a: " << data.a << ", b: " << data.b << std::endl;
    // };

    // stream.begin_read(on_read);

    ///////////////////

    auto serial_task = [&]()
    {
        serialib serial;
        char res = serial.openDevice(serial_port, baudrate);
        if (res != 1)
        {
            return res;
        }
        std::cout << "connected" << std::endl;

        while (serial.isDeviceOpen())
        {
            uint8_t start_byte = 0;
            while (start_byte != 0x55)
            {
                serial.readBytes(&start_byte, 1);
                std::this_thread::sleep_for(std::chrono::microseconds(8000000 / baudrate));
            }

            int available_byte = serial.available();
            std::cout << "available bytes" << available_byte << std::endl;

            if (available_byte >= sizeof(IntPair))
            {
                uint8_t rx_buffer[1024];
                int rx_size = serial.readBytes(rx_buffer, 8);

                std::cout << "receicved " << rx_size << " bytes" << std::endl;

                IntPair pair = *(IntPair *)(&rx_buffer);
                std::cout << "a: " << pair.a << ", b: " << pair.b << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    };

    std::thread serial_thread{serial_task};

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50000));
    }

    return 0;
}
