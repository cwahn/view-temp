#ifndef PLOTLIB_HPP_
#define PLOTLIB_HPP_

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#include "efp.hpp"

using namespace efp;

static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

template <typename F, typename G>
static void run_gui(int width,
                    int height,
                    const std::string &title,
                    const F &init_task,
                    const G &loop_task,
                    const ImVec4 &clear_color = ImVec4(36 / 255.f, 39 / 255.f, 45 / 255.f, 1.00f))
{
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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.Fonts->AddFontFromFileTTF("/System/Library/Fonts/AppleSDGothicNeo.ttc", 16.0f);
    io.FontDefault = io.Fonts->Fonts[0];

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    init_task(io);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

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
    f();
    ImGui::End();
}

float now_sec()
{
    static auto init_time = std::chrono::high_resolution_clock::now();

    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed_seconds = now - init_time;

    return elapsed_seconds.count();
}

template <size_t Capacity>
class RtDataFixedRate
{
public:
    RtDataFixedRate(std::string name, float sample_rate_hz)
        : name_(name), sample_rate_hz_(sample_rate_hz)
    {
    }

    void setup(ImAxis y_axis, float plot_size_sec)
    {
        ImPlot::SetupAxis(y_axis, name_.c_str(), ImPlotAxisFlags_RangeFit);
        size_t plot_length_ = plot_length(plot_size_sec);
        auto view = as_view(plot_length_);

        ImPlot::SetupAxisLimits(y_axis,
                                minimum(view),
                                maximum(view),
                                ImGuiCond_Always);
    }

    void plot(ImAxis y_axis, float plot_size_sec)
    {
        ImPlot::SetAxes(ImAxis_X1, y_axis);
        size_t plot_length_ = plot_length(plot_size_sec);
        auto ts_view_ = ts_view(plot_length_);
        auto as_view_ = as_view(plot_length_);
        ImPlot::PlotLine(name_.c_str(), p_data(ts_view_), p_data(as_view_), plot_length_, 0, 0, sizeof(float));
    }

    void lock()
    {
        mutex_.lock();
    }

    void unlock()
    {
        mutex_.unlock();
    }

    void push_now(float value)
    {
        lock();
        ts_.push_back(now_sec());
        as_.push_back(value);
        unlock();
    }

    template <typename SeqA>
    void push_sequence(const SeqA &as)
    {
        const float period_sec = 1. / sample_rate_hz_;
        float t = now_sec() - period_sec * length(as);
        // float delta = period_sec ;

        lock();
        for_each([&](float a)
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

    float max_plot_sec()
    {
        return (float)Capacity / sample_rate_hz_;
    }

    Vcb<float, Capacity> ts_;
    Vcb<float, Capacity> as_;

private:
    size_t plot_length(float plot_size_sec)
    {
        return std::floor(plot_size_sec * sample_rate_hz_);
    }

    VectorView<const float> ts_view(size_t plot_length)
    {
        return VectorView<const float>{ts_.end() - plot_length, plot_length};
    }

    VectorView<const float> as_view(size_t plot_length)
    {
        return VectorView<const float>{as_.end() - plot_length, plot_length};
    }

    std::string name_;
    float sample_rate_hz_;
    std::mutex mutex_;
};

template <size_t Capacity>
class RtDataDynamicRate
{
public:
    RtDataDynamicRate(std::string name)
        : name_(name)
    {
    }

    void setup(ImAxis y_axis, float plot_size_sec)
    {
        ImPlot::SetupAxis(y_axis, name_.c_str(), ImPlotAxisFlags_RangeFit);
        size_t plot_length_ = plot_length(plot_size_sec);
        auto view = as_view(plot_length_);

        ImPlot::SetupAxisLimits(y_axis,
                                minimum(view),
                                maximum(view),
                                ImGuiCond_Always);
    }

    void plot(ImAxis y_axis, float plot_size_sec)
    {
        ImPlot::SetAxes(ImAxis_X1, y_axis);
        size_t plot_length_ = plot_length(plot_size_sec);
        auto ts_view_ = ts_view(plot_length_);
        auto as_view_ = as_view(plot_length_);
        ImPlot::PlotLine(name_.c_str(), p_data(ts_view_), p_data(as_view_), plot_length_, 0, 0, sizeof(float));
    }

    void lock()
    {
        mutex_.lock();
    }

    void unlock()
    {
        mutex_.unlock();
    }

    void push_now(float value)
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

    float max_plot_sec()
    {
        return now_sec() - ts_[0];
    }

    Vcb<float, Capacity> ts_;
    Vcb<float, Capacity> as_;

private:
    size_t plot_length(float plot_size_sec)
    {
        float min_plot_time = now_sec() - plot_size_sec;
        return Capacity - (find_index([&](auto x)
                                      { return x > min_plot_time; },
                                      ts_))
                              .match(
                                  [](int i)
                                  { return i; },
                                  [](Nothing _)
                                  { return 0; });
    }

    VectorView<const float> ts_view(size_t plot_length)
    {
        return VectorView<const float>{ts_.end() - plot_length, plot_length};
    }

    VectorView<const float> as_view(size_t plot_length)
    {
        return VectorView<const float>{as_.end() - plot_length, plot_length};
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
        plot_sec_ = minimum(std::vector<float>{data1_->max_plot_sec()}) / 2.;
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
                           "%.3f sec");
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
        float width,
        float height,
        Data1 *data1,
        Data2 *data2)
        : name_(name), width_(width), height_(height), data1_(data1), data2_(data2)
    {
        plot_sec_ = minimum(std::vector<float>{
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
                           minimum(std::vector<float>{data1_->max_plot_sec(), data2_->max_plot_sec()}),
                           "%.3f sec");
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
        plot_sec_ = minimum(std::vector<float>{
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
                           minimum(std::vector<float>{data1_->max_plot_sec(), data2_->max_plot_sec(), data3_->max_plot_sec()}),
                           "%.3f sec");
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

    void setup(float plot_size_sec)
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
    }

    void plot(ImAxis y_axis)
    {
        using namespace ImPlot;
        // ImPlot::SetAxes(ImAxis_X1, y_axis);
        // size_t plot_length_ = plot_length(plot_size_sec);
        // auto ts_view_ = ts_view(plot_length_);
        // auto as_view_ = as_view(plot_length_);
        // ImPlot::PlotLine(name_.c_str(), p_data(ts_view_), p_data(as_view_), plot_length_, 0, 0, sizeof(float));

        PlotLine(y_name_.c_str(), p_data(xs_), p_data(ys_), length(ys_), 0, 0, sizeof(float));
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

    std::vector<float> xs_;
    std::vector<float> ys_;

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
        int width,
        int height,
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
    int width_;
    int height_;
};

template <size_t capacity>
class SpectrogramData
{
public:
    SpectrogramData(std::string x_name, std::string y_name)
        : x_name_(x_name), y_name_(y_name)
    {
    }

    void setup(float plot_size_sec)
    {
        using namespace ImPlot;

        // ImPlotAxisFlags flags = ImPlotAxisFlags_RangeFit;
        // SetupAxes(x_name_.c_str(), y_name_.c_str(), flags, flags);
        // SetupAxisLimits(ImAxis_X1, minimum(xs_), maximum(xs_), ImGuiCond_Always);
        // SetupAxisLimits(ImAxis_Y1, minimum(ys_), maximum(ys_), ImGuiCond_Always);

        SetupAxes(NULL, NULL, ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_Lock);
        SetupAxisLimits(ImAxis_X1, maximum(ts_) - plot_size_sec, maximum(ts_), ImGuiCond_Always);
        SetupAxisLimits(ImAxis_Y1, 0, 1024, ImGuiCond_Always);
        SetupAxisFormat(ImAxis_X1, "%f sec");
        SetupAxisFormat(ImAxis_Y1, "%f Hz");
    }

    void plot(float plot_size_sec)
    {
        using namespace ImGui;
        using namespace ImPlot;

        // PlotLine(y_name_.c_str(), p_data(xs_), p_data(ys_), length(ys_), 0, 0, sizeof(float));
        PlotHeatmap(
            y_name_.c_str(),
            p_data(ass_),
            plot_length(plot_size_sec),
            sample_length_,
            minimum(ass_),
            // 0,
            maximum(ass_),
            // 1024,
            NULL,
            // todo ts_ last
            {maximum(ts_) - plot_size_sec, 0},
            {maximum(ts_), 1024},
            ImPlotHeatmapFlags_ColMajor);

        Text("rows | plot_length: %d", plot_length(plot_size_sec));
        Text("cols | sample_length: %d", sample_length_);
        Text("scale_min | minimum(ass_): %f", minimum(ass_));
        Text("scale_max | maximum(ass_): %f", maximum(ass_));
        Text("bound_min | {maximum(ts_) - plot_size_sec, 0}: %f, %f", maximum(ts_) - plot_size_sec, 0);
        Text("bound_max | {maximum(ts_), maximum(ass_)}: %f, %f", maximum(ts_), maximum(ass_));
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
            // todo If length is different clear buffer
            sample_length_ = as_length;
        }
        for_each([&](auto x)
                 { ass_.push_back(x); },
                 as);
        ts_.push_back(now_sec());
        unlock();
    }

    float max_plot_sec()
    {
        return now_sec() - minimum(ts_);
    }

    Vcb<float, capacity> ass_;
    Vcb<float, capacity> ts_;

private:
    int max_plot_length()
    {
        return capacity / sample_length_;
    }

    int plot_length(float plot_size_sec)
    {
        // float min_plot_time = now_sec() - plot_size_sec;
        // return Capacity - (find_index([&](auto x)
        //                               { return x > min_plot_time; },
        //                               ts_))
        //                       .match(
        //                           [](int i)
        //                           { return i; },
        //                           [](Nothing _)
        //                           { return 0; });

        float min_plot_time_sec = now_sec() - plot_size_sec;
        int min_sample_index = find_index([&](auto x)
                                          { return x > min_plot_time_sec; },
                                          ts_)
                                   .match(
                                       [](int i)
                                       { return i; },
                                       [](Nothing _)
                                       { return 0; });

        // return max_plot_length() - min_sample_index;
        return 100;
    }

    std::string x_name_;
    std::string y_name_;
    int sample_length_;
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
        if (ImPlot::BeginPlot(name_.c_str(), ImVec2(width_, height_)))
        {
            // ImPlot::SetupAxis(ImAxis_X1, "time(sec)", ImPlotAxisFlags_RangeFit);
            // auto now_sec_ = now_sec();
            // ImPlot::SetupAxisLimits(ImAxis_X1, minimum(data_->xs_), maximum(data_->ys_), ImGuiCond_Always);

            data_->lock();
            data_->setup(plot_sec_);

            ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);

            data_->plot(plot_sec_);
            data_->unlock();

            ImPlot::EndPlot();
        }

        ImGui::SliderFloat("plotting window (sec)",
                           &plot_sec_,
                           0.,
                           data_->max_plot_sec(),
                           "%.3f sec");
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