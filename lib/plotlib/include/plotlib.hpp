#ifndef PLOTLIB_HPP_
#define PLOTLIB_HPP_

// #define IMGUI_DEFINE_MATH_OPERATORS
// #define IM_VEC2_CLASS_EXTRA
// #define IM_VEC4_CLASS_EXTRA

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

// #include "imgui_dock.h"

#include "signal.hpp"
#include "efp.hpp"

using namespace efp;

#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void set_theme()
{
    ImGuiStyle &style = ImGui::GetStyle();

    style.WindowPadding = ImVec2{8.f, 3.f};
    style.FramePadding = ImVec2{8.f, 3.f};
    style.CellPadding = ImVec2{8.f, 3.f};
    style.ItemSpacing = ImVec2{8.f, 6.f};
    style.ItemInnerSpacing = ImVec2{4.f, 6.f};
    style.TouchExtraPadding = ImVec2{0.f, 0.f};
    style.IndentSpacing = 20.f;
    style.ScrollbarSize = 14.f;
    style.GrabMinSize = 18.f;

    style.WindowBorderSize = 1.f;
    style.ChildBorderSize = 1.f;
    style.PopupBorderSize = 1.f;
    style.FrameBorderSize = 0.f;
    style.TabBorderSize = 0.f;

    style.WindowRounding = 8.f;
    style.ChildRounding = 8.f;
    style.FrameRounding = 4.f;
    style.PopupRounding = 8.f;
    style.ScrollbarRounding = 12.f;
    style.GrabRounding = 18.f;
    style.TabRounding = 4.f;

    style.WindowTitleAlign = ImVec2{0.5f, 0.5f};
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ColorButtonPosition = ImGuiDir_Left;
    style.ButtonTextAlign = ImVec2{0.5f, 0.5f};
    style.SelectableTextAlign = ImVec2{0.f, 0.f};
    style.SeparatorTextBorderSize = 1.f;
    style.SeparatorTextAlign = ImVec2{0.f, 0.5f};
    style.SeparatorTextPadding = ImVec2{8.f, 8.f};
    style.LogSliderDeadzone = 4.f;

    ImVec4 *colors = style.Colors;

    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.09f, 0.09f, 0.94f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.07f, 0.07f, 0.07f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.35f, 0.35f, 0.36f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.27f, 0.27f, 0.27f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.02f, 0.02f, 0.02f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.41f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 0.41f, 0.90f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.41f, 0.41f, 0.41f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 0.41f, 0.90f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.28f, 0.55f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.41f, 0.41f, 0.41f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.41f, 0.90f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.41f, 0.90f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.36f, 0.36f, 0.36f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.00f, 0.41f, 0.90f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.27f, 0.27f, 0.27f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.00f, 0.41f, 0.90f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.00f, 0.31f, 0.68f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.27f, 0.27f, 0.27f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.41f, 0.90f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

template <typename F, typename G>
static void run_gui(int width,
                    int height,
                    const std::string &title,
                    const F &init_task,
                    const G &loop_task,
                    const ImVec4 &clear_color = ImVec4(0.f / 255.f, 0.f / 255.f, 0.f / 255.f, 1.00f))
{
    using namespace ImGui;

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return;

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
    GLFWwindow *window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (window == nullptr)
        return;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.Fonts->AddFontFromFileTTF("/System/Library/Fonts/AppleSDGothicNeo.ttc", 16.0f);
    io.FontDefault = io.Fonts->Fonts[0];

    // Setup Dear ImGui style
    // ImGui::StyleColorsDark();
    set_theme();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    init_task(io);
    // ImGui::InitDock();
    static bool use_work_area = true;
    static ImGuiWindowFlags fullscreen_window_flag =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoMove;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiID dockspace = ImGui::DockSpaceOverViewport();

        // ImGui::SetNextWindowDockID(dockspace);

        // #ifdef IMGUI_HAS_VIEWPORT
        // ImGuiViewport *viewport = ImGui::GetMainViewport();
        // ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
        // ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);
        // ImGui::SetNextWindowViewport(viewport->ID);

        static bool open_main = true;
        // ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::Begin("main", &open_main, fullscreen_window_flag);

        ImGui::Text("Hello");
        // ImGui::ShowStyleSelector("GUI style");
        // ImPlot::ShowStyleSelector("plot style");
        // ImPlot::ShowColormapSelector("plot colormap");
        // Text("application average %.3e ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

        // ImGui::BringWindowToDisplayBack(ImGui::GetCurrentWindow());
        ImGui::End();
        // ImGui::PopStyleVar(1);

        loop_task(io);

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
}

template <typename F>
void window(const char *name, const F &f)
{
    // todo close button
    ImGui::Begin(name);
    // ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
    f();
    ImGui::End();
}

template <typename F>
void child(const char *name, const F &f)
{
    // todo close button
    ImGui::BeginChild(name);
    f();
    ImGui::EndChild();
}

double now_sec()
{
    static auto init_time = std::chrono::high_resolution_clock::now();

    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = now - init_time;

    return elapsed_seconds.count();
}

template <size_t capacity>
class RtDataFixedRate
{
public:
    RtDataFixedRate(std::string name, double sample_rate_hz)
        : name_(name), sample_rate_hz_(sample_rate_hz)
    {
    }

    void setup(ImAxis y_axis, double plot_size_sec)
    {
        ImPlot::SetupAxis(y_axis, name_.c_str(), ImPlotAxisFlags_RangeFit);
        size_t plot_length_ = plot_length(plot_size_sec);
        auto view = as_view(plot_length_);

        ImPlot::SetupAxisLimits(y_axis,
                                minimum(view),
                                maximum(view),
                                ImGuiCond_Always);
    }

    void plot(ImAxis y_axis, double plot_size_sec)
    {
        ImPlot::SetAxes(ImAxis_X1, y_axis);
        size_t plot_length_ = plot_length(plot_size_sec);
        auto ts_view_ = ts_view(plot_length_);
        auto as_view_ = as_view(plot_length_);
        ImPlot::PlotLine(name_.c_str(), p_data(ts_view_), p_data(as_view_), plot_length_, 0, 0, sizeof(double));
    }

    void lock()
    {
        mutex_.lock();
    }

    void unlock()
    {
        mutex_.unlock();
    }

    void push_now(double value)
    {
        lock();
        ts_.push_back(now_sec());
        as_.push_back(value);
        unlock();
    }

    template <typename SeqA>
    void push_sequence(const SeqA &as)
    {
        const double period_sec = 1. / sample_rate_hz_;
        double t = now_sec() - period_sec * length(as);
        // double delta = period_sec ;

        lock();
        for_each([&](double a)
                 { ts_.push_back(t);
                   as_.push_back(a);
                   t += period_sec; },
                 as);
        unlock();
    }

    std::string name()
    {
        return name_;
    }

    double max_plot_sec()
    {
        return (double)capacity / sample_rate_hz_;
    }

    Vcb<double, capacity> ts_;
    Vcb<double, capacity> as_;

private:
    size_t plot_length(double plot_size_sec)
    {
        return std::floor(plot_size_sec * sample_rate_hz_);
    }

    VectorView<const double> ts_view(size_t plot_length)
    {
        return VectorView<const double>{ts_.end() - plot_length, plot_length};
    }

    VectorView<const double> as_view(size_t plot_length)
    {
        return VectorView<const double>{as_.end() - plot_length, plot_length};
    }

    std::string name_;
    double sample_rate_hz_;
    std::mutex mutex_;
};

template <size_t capacity>
class RtDataDynamicRate
{
public:
    RtDataDynamicRate(std::string name)
        : name_(name)
    {
    }

    void setup(ImAxis y_axis, double plot_size_sec)
    {
        ImPlot::SetupAxis(y_axis, name_.c_str(), ImPlotAxisFlags_RangeFit);
        size_t plot_length_ = plot_length(plot_size_sec);
        auto view = as_view(plot_length_);

        ImPlot::SetupAxisLimits(y_axis,
                                minimum(view),
                                maximum(view),
                                ImGuiCond_Always);
    }

    void plot(ImAxis y_axis, double plot_size_sec)
    {
        ImPlot::SetAxes(ImAxis_X1, y_axis);
        size_t plot_length_ = plot_length(plot_size_sec);
        auto ts_view_ = ts_view(plot_length_);
        auto as_view_ = as_view(plot_length_);
        ImPlot::PlotLine(name_.c_str(), p_data(ts_view_), p_data(as_view_), plot_length_, 0, 0, sizeof(double));
    }

    void lock()
    {
        mutex_.lock();
    }

    void unlock()
    {
        mutex_.unlock();
    }

    void push_now(double value)
    {
        lock();
        ts_.push_back(now_sec());
        as_.push_back(value);
        unlock();
    }

    std::string name()
    {
        return name_;
    }

    double max_plot_sec()
    {
        return now_sec() - ts_[0];
    }

    Vcb<double, capacity> ts_;
    Vcb<double, capacity> as_;

private:
    size_t plot_length(double plot_size_sec)
    {
        double min_plot_time = now_sec() - plot_size_sec;
        return capacity - (find_index([&](auto x)
                                      { return x > min_plot_time; },
                                      ts_))
                              .match(
                                  [](int i)
                                  { return i; },
                                  [](Nothing _)
                                  { return 0; });
    }

    VectorView<const double> ts_view(size_t plot_length)
    {
        return VectorView<const double>{ts_.end() - plot_length, plot_length};
    }

    VectorView<const double> as_view(size_t plot_length)
    {
        return VectorView<const double>{as_.end() - plot_length, plot_length};
    }

    std::string name_;
    std::mutex mutex_;
};

template <typename Data1>
class RtPlot1
{
public:
    RtPlot1(std::string name, int width, int height, Data1 *data)
        : name_(name), width_(width), height_(height), data1_(data)
    {
        plot_sec_ = minimum(std::vector<double>{data1_->max_plot_sec()}) / 2.;
    }

    void plot()
    {
        if (ImPlot::BeginPlot(name_.c_str(), ImVec2(width_, height_)))
        {
            ImPlot::SetupAxis(ImAxis_X1, "time(sec)", ImPlotAxisFlags_RangeFit);
            auto now_sec_ = now_sec();
            ImPlot::SetupAxisLimits(ImAxis_X1, now_sec_ - plot_sec_, now_sec_, ImGuiCond_Always);

            data1_->lock();
            data1_->setup(ImAxis_Y1, plot_sec_);

            ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);

            data1_->plot(ImAxis_Y1, plot_sec_);
            data1_->unlock();

            ImPlot::EndPlot();
        }

        ImGui::SliderFloat("plotting window (sec)",
                           &plot_sec_,
                           0.,
                           data1_->max_plot_sec(),
                           "%.3e sec");
    }

    Data1 *data1_;

private:
    float plot_sec_;
    std::string name_;
    int width_;
    int height_;
};

template <typename Data1, typename Data2>
class RtPlot2
{
public:
    RtPlot2(
        std::string name,
        double width,
        double height,
        Data1 *data1,
        Data2 *data2)
        : name_(name), width_(width), height_(height), data1_(data1), data2_(data2)
    {
        plot_sec_ = minimum(std::vector<double>{
                        data1_->max_plot_sec(),
                        data2_->max_plot_sec()}) /
                    2.;
    }

    void plot()
    {
        if (ImPlot::BeginPlot(name_.c_str(), ImVec2(width_, height_)))
        {
            ImPlot::SetupAxis(ImAxis_X1, "time(sec)", ImPlotAxisFlags_RangeFit);
            auto now_sec_ = now_sec();
            ImPlot::SetupAxisLimits(ImAxis_X1, now_sec_ - plot_sec_, now_sec_, ImGuiCond_Always);

            data1_->lock();
            data1_->setup(ImAxis_Y1, plot_sec_);
            data2_->lock();
            data2_->setup(ImAxis_Y2, plot_sec_);

            ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);

            data1_->plot(ImAxis_Y1, plot_sec_);
            data1_->unlock();
            data2_->plot(ImAxis_Y2, plot_sec_);
            data2_->unlock();

            ImPlot::EndPlot();
        }

        ImGui::SliderFloat("plotting window (sec)",
                           &plot_sec_,
                           0.,
                           minimum(std::vector<double>{data1_->max_plot_sec(), data2_->max_plot_sec()}),
                           "%.3e sec");
    }

    Data1 *data1_;
    Data2 *data2_;

private:
    float plot_sec_;
    std::string name_;
    int width_;
    int height_;
};

template <typename Data1, typename Data2, typename Data3>
class RtPlot3
{
public:
    RtPlot3(
        std::string name,
        int width,
        int height,
        Data1 *data1,
        Data2 *data2,
        Data3 *data3)
        : name_(name), width_(width), height_(height), data1_(data1), data2_(data2), data3_(data3)
    {
        plot_sec_ = minimum(std::vector<double>{
                        data1_->max_plot_sec(),
                        data2_->max_plot_sec(),
                        data3_->max_plot_sec()}) /
                    2.;
    }

    void plot()
    {
        if (ImPlot::BeginPlot(name_.c_str(), ImVec2(width_, height_)))
        {
            ImPlot::SetupAxis(ImAxis_X1, "time(sec)", ImPlotAxisFlags_RangeFit);
            auto now_sec_ = now_sec();
            ImPlot::SetupAxisLimits(ImAxis_X1, now_sec_ - plot_sec_, now_sec_, ImGuiCond_Always);

            data1_->lock();
            data1_->setup(ImAxis_Y1, plot_sec_);
            data2_->lock();
            data2_->setup(ImAxis_Y2, plot_sec_);
            data3_->lock();
            data3_->setup(ImAxis_Y3, plot_sec_);

            ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);

            data1_->plot(ImAxis_Y1, plot_sec_);
            data1_->unlock();
            data2_->plot(ImAxis_Y2, plot_sec_);
            data2_->unlock();
            data3_->plot(ImAxis_Y3, plot_sec_);
            data3_->unlock();

            ImPlot::EndPlot();
        }

        ImGui::SliderFloat("plotting window (sec)",
                           &plot_sec_,
                           0.,
                           minimum(std::vector<double>{data1_->max_plot_sec(), data2_->max_plot_sec(), data3_->max_plot_sec()}),
                           "%.3e sec");
    }

    Data1 *data1_;
    Data2 *data2_;
    Data1 *data3_;

private:
    float plot_sec_;
    std::string name_;
    int width_;
    int height_;
};

class SnapshotData
{
public:
    SnapshotData(std::string x_name, std::string y_name)
        : x_name_(x_name), y_name_(y_name)
    {
    }

    void setup(double plot_size_sec)
    {
        using namespace ImPlot;
        // ImPlot::SetupAxis(y_axis, name_.c_str(), ImPlotAxisFlags_RangeFit);
        // size_t plot_length_ = plot_length(plot_size_sec);
        // auto view = as_view(plot_length_);

        // ImPlot::SetupAxisLimits(y_axis,
        //                         minimum(view),
        //                         maximum(view),
        //                         ImGuiCond_Always);

        ImPlotAxisFlags flags = ImPlotAxisFlags_RangeFit;
        SetupAxes(x_name_.c_str(), y_name_.c_str(), flags, flags);
        SetupAxisLimits(ImAxis_X1, minimum(xs_), maximum(xs_), ImGuiCond_Always);
        SetupAxisLimits(ImAxis_Y1, minimum(ys_), maximum(ys_), ImGuiCond_Always);

        SetupAxisFormat(ImAxis_X1, "%.3e");
    }

    void plot(ImAxis y_axis)
    {
        using namespace ImPlot;
        // ImPlot::SetAxes(ImAxis_X1, y_axis);
        // size_t plot_length_ = plot_length(plot_size_sec);
        // auto ts_view_ = ts_view(plot_length_);
        // auto as_view_ = as_view(plot_length_);
        // ImPlot::PlotLine(name_.c_str(), p_data(ts_view_), p_data(as_view_), plot_length_, 0, 0, sizeof(double));

        PlotLine(y_name_.c_str(), p_data(xs_), p_data(ys_), length(ys_), 0, 0, sizeof(double));
    }

    void lock()
    {
        mutex_.lock();
    }

    void unlock()
    {
        mutex_.unlock();
    }

    std::string name()
    {
        return y_name_;
    }

    std::vector<double> xs_;
    std::vector<double> ys_;

private:
    std::string x_name_;
    std::string y_name_;
    std::mutex mutex_;
};

class SnapshotPlot
{
public:
    SnapshotPlot(
        std::string name,
        float width,
        float height,
        SnapshotData *data)
        : name_(name), width_(width), height_(height), data_(data)
    {
    }

    void plot()
    {
        if (ImPlot::BeginPlot(name_.c_str(), ImVec2(width_, height_)))
        {
            ImPlot::SetupAxis(ImAxis_X1, "time(sec)", ImPlotAxisFlags_RangeFit);
            auto now_sec_ = now_sec();
            ImPlot::SetupAxisLimits(ImAxis_X1, minimum(data_->xs_), maximum(data_->ys_), ImGuiCond_Always);

            data_->lock();
            data_->setup(ImAxis_Y1);

            ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);

            data_->plot(ImAxis_Y1);
            data_->unlock();

            ImPlot::EndPlot();
        }
    }

    SnapshotData *data_;

private:
    std::string name_;
    float width_;
    float height_;
};

template <size_t capacity, size_t max_sample_length>
class SpectrogramData
{
public:
    SpectrogramData(std::string x_name, std::string y_name)
        : x_name_(x_name), y_name_(y_name)
    {
    }

    void setup(double plot_size_sec)
    {
        using namespace ImGui;
        using namespace ImPlot;

        auto plot_length_ = plot_length(plot_size_sec);
        auto ass_view_ = ass_view(plot_length_);
        auto ts_view_ = ts_view(plot_length_);

        plot_size_sec_ = plot_size_sec;

        SetupAxes(NULL, NULL, ImPlotAxisFlags_NoTickLabels);

        SetupAxisLimits(ImAxis_X1, ts_view_[0], ts_view_[plot_length_ - 1], ImGuiCond_Always);
        SetupAxisLimits(ImAxis_Y1, 0, 22050, ImGuiCond_Always);

        SetupAxisFormat(ImAxis_X1, "%.3e sec");
        SetupAxisFormat(ImAxis_Y1, "%.3e Hz");
    }

    void plot(double plot_size_sec)
    {
        using namespace ImGui;
        using namespace ImPlot;

        auto plot_length_ = plot_length(plot_size_sec);
        auto ass_view_ = ass_view(plot_length_);
        auto ts_view_ = ts_view(plot_length_);

        auto lims_ = ImPlot::GetPlotLimits();

        // ! Need fix
        PlotHeatmap(
            y_name_.c_str(),
            p_data(ass_view_),
            sample_length_,     // rows on display?? yes
            plot_length_,       // rows on display? yes
            minimum(ass_view_), // color map min?
            maximum(ass_view_), // color map max?
            NULL,               // fmt
            ImPlotPoint{
                ts_view_[0],
                0}, // coordinate of view in unit of limits
            ImPlotPoint{
                ts_view_[plot_length_ - 1],
                22050},                 // coordinate of view in unit of limits
            ImPlotHeatmapFlags_ColMajor // col-major = col-contiguous
        );

        // Text("rows | plot_length_: %d", plot_length_);
        // Text("cols | sample_length_: %d", sample_length_);
        // Text("scale_min | minimum(ass_): %f", minimum(ass_));
        // Text("scale_max | maximum(ass_): %f", maximum(ass_));
        // Text("bound_min | {minimum(ts_view_), 0}: %f, %f", minimum(ts_view_), 0.);
        // Text("bound_max | {maximum(ts_view_), 20000.}: %f, %f", maximum(ts_view_), 20000.);
    }

    void lock()
    {
        mutex_.lock();
    }

    void unlock()
    {
        mutex_.unlock();
    }

    std::string name()
    {
        return y_name_;
    }

    template <typename SeqA>
    void push_sequence(const SeqA &as)
    {
        int as_length = length(as);

        lock();
        if (as_length != sample_length_)
        {
            sample_length_ = as_length;
        }

        // ! Inverse insert
        for_index([&](int i)
                  { ass_.push_back(as[as_length - i - 1]); },
                  sample_length_);

        ts_.push_back(now_sec());
        unlock();
    }

    double max_plot_sec()
    {
        return now_sec() - minimum(ts_);
    }

    Vcb<double, capacity * max_sample_length> ass_;
    Vcb<double, capacity> ts_;

private:
    int max_plot_length()
    {
        return capacity / sample_length_;
    }

    int plot_length(double plot_size_sec)
    {
        // ! need to be fixed
        double min_plot_time_sec = now_sec() - plot_size_sec;
        int min_sample_index = find_index([&](auto x)
                                          { return x > min_plot_time_sec; },
                                          ts_)
                                   .match(
                                       [](int i)
                                       { return i; },
                                       [](Nothing _)
                                       { return 0; });

        return capacity - min_sample_index;
    }

    VectorView<const double> ts_view(size_t plot_length)
    {
        return VectorView<const double>{ts_.end() - plot_length, plot_length};
    }

    VectorView<const double> ass_view(size_t plot_length)
    {
        size_t point_num = plot_length * sample_length_;
        return VectorView<const double>{ass_.end() - point_num, point_num};
    }

    std::string x_name_;
    std::string y_name_;

    double plot_size_sec_;
    int sample_length_;
    int plot_length_;
    // VectorView<const double> ass_view_;
    // VectorView<const double> ts_view_;

    std::mutex mutex_;
};

template <typename D>
class SpectrogramPlot
{
public:
    SpectrogramPlot(
        std::string name,
        int width,
        int height,
        D *data)
        : name_(name), width_(width), height_(height), data_(data)
    {
    }

    void plot()
    {
        using namespace ImGui;
        using namespace ImPlot;

        if (BeginPlot(name_.c_str(), ImVec2(width_, height_)))
        {
            PushColormap(ImPlotColormap_Plasma);

            data_->lock();
            data_->setup(plot_sec_);

            ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);

            data_->plot(plot_sec_);
            data_->unlock();
            PopColormap();

            EndPlot();
        }
        SliderFloat("plotting window (sec)",
                    &plot_sec_,
                    0.,
                    data_->max_plot_sec(),
                    "%.3e sec");
    }

    D *data_;

private:
    std::string name_;
    int width_;
    int height_;

    float plot_sec_;
    int sample_length_;
};

#endif