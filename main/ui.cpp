#include "ui.h"


#include "cg_logger.h"
#include "cg_window2d.h"
#include "cg_glfw3.h"

#include "imgui/imgui.h"

#include <GLFW/glfw3.h>

#include <cmath>
#include <chrono>
#include <cinttypes>
#include <imgui/imgui_internal.h>

namespace {
    constexpr auto programId = "audio2";

    const ::Data::StateInput *g_inp = nullptr;

    static GLFWwindow*  g_Window = NULL;
    static double       g_Time = 0.0f;
    static bool         g_MousePressed[3] = { false, false, false };
    static float        g_MouseWheel = 0.0f;
    static GLuint       g_FontTexture = 0;

    void ImGui_ImplGlfw_RenderDrawLists(ImDrawData* draw_data)
    {
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        ImGuiIO& io = ImGui::GetIO();
        int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
        int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
        if (fb_width == 0 || fb_height == 0)
            return;
        draw_data->ScaleClipRects(io.DisplayFramebufferScale);

        // We are using the OpenGL fixed pipeline to make the example code simpler to read!
        // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers.
        GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
        GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
        GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
        glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_SCISSOR_TEST);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glEnable(GL_TEXTURE_2D);
        //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound

        // Setup viewport, orthographic projection matrix
        glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, -1.0f, +1.0f);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // Render command lists
#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
            const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;
            glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + OFFSETOF(ImDrawVert, pos)));
            glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + OFFSETOF(ImDrawVert, uv)));
            glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + OFFSETOF(ImDrawVert, col)));

            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->UserCallback)
                {
                    pcmd->UserCallback(cmd_list, pcmd);
                }
                else
                {
                    glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                    glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                    glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer);
                }
                idx_buffer += pcmd->ElemCount;
            }
        }
#undef OFFSETOF

        // Restore modified state
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glBindTexture(GL_TEXTURE_2D, (GLuint)last_texture);
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glPopAttrib();
        glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
        glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
    }

    static const char* ImGui_ImplGlfw_GetClipboardText(void* user_data)
    {
        return glfwGetClipboardString((GLFWwindow*)user_data);
    }

    static void ImGui_ImplGlfw_SetClipboardText(void* user_data, const char* text)
    {
        glfwSetClipboardString((GLFWwindow*)user_data, text);
    }

    void ImGui_ImplGlfw_MouseButtonCallback(GLFWwindow*, int button, int action, int /*mods*/)
    {
        if (action == GLFW_PRESS && button >= 0 && button < 3)
            g_MousePressed[button] = true;
    }

    void ImGui_ImplGlfw_ScrollCallback(GLFWwindow*, double /*xoffset*/, double yoffset)
    {
        g_MouseWheel += (float)yoffset; // Use fractional mouse wheel, 1.0 unit 5 lines.
    }

    void ImGui_ImplGlFw_KeyCallback(GLFWwindow*, int key, int, int action, int mods)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (action == GLFW_PRESS)
            io.KeysDown[key] = true;
        if (action == GLFW_RELEASE)
            io.KeysDown[key] = false;

        (void)mods; // Modifiers are not reliable across systems
        io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
        io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
        io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
        io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
    }

    void ImGui_ImplGlfw_CharCallback(GLFWwindow*, unsigned int c)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (c > 0 && c < 0x10000)
            io.AddInputCharacter((unsigned short)c);
    }

    bool    ImGui_ImplGlfw_Init(GLFWwindow* window, bool install_callbacks)
    {
        g_Window = window;

        ImGuiIO& io = ImGui::GetIO();
        io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;                     // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
        io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
        io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
        io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
        io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
        io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
        io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
        io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
        io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
        io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
        io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
        io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
        io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
        io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
        io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
        io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
        io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
        io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
        io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

        io.RenderDrawListsFn = ImGui_ImplGlfw_RenderDrawLists;      // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
        io.SetClipboardTextFn = ImGui_ImplGlfw_SetClipboardText;
        io.GetClipboardTextFn = ImGui_ImplGlfw_GetClipboardText;
        io.ClipboardUserData = g_Window;

        if (install_callbacks)
        {
            glfwSetMouseButtonCallback(window, ImGui_ImplGlfw_MouseButtonCallback);
            glfwSetScrollCallback(window, ImGui_ImplGlfw_ScrollCallback);
            glfwSetKeyCallback(window, ImGui_ImplGlFw_KeyCallback);
            glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
        }

        return true;
    }

    bool ImGui_ImplGlfw_CreateDeviceObjects()
    {
        // Build texture atlas
        ImGuiIO& io = ImGui::GetIO();
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

        // Upload texture to graphics system
        GLint last_texture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
        glGenTextures(1, &g_FontTexture);
        glBindTexture(GL_TEXTURE_2D, g_FontTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        // Store our identifier
        io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

        // Restore state
        glBindTexture(GL_TEXTURE_2D, last_texture);

        return true;
    }

    void ImGui_ImplGlfw_NewFrame()
    {
        if (!g_FontTexture)
            ImGui_ImplGlfw_CreateDeviceObjects();

        ImGuiIO& io = ImGui::GetIO();

        // Setup display size (every frame to accommodate for window resizing)
        int w, h;
        int display_w, display_h;
        glfwGetWindowSize(g_Window, &w, &h);
        glfwGetFramebufferSize(g_Window, &display_w, &display_h);
        io.DisplaySize = ImVec2((float)w, (float)h);
        io.DisplayFramebufferScale = ImVec2(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

        // Setup time step
        double current_time =  glfwGetTime();
        io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f/60.0f);
        g_Time = current_time;

        // Setup inputs
        // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
        if (glfwGetWindowAttrib(g_Window, GLFW_FOCUSED))
        {
            double mouse_x, mouse_y;
            glfwGetCursorPos(g_Window, &mouse_x, &mouse_y);
            io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);   // Mouse position in screen coordinates (set to -1,-1 if no mouse / on another screen, etc.)
        }
        else
        {
            io.MousePos = ImVec2(-1,-1);
        }

        for (int i = 0; i < 3; i++)
        {
            io.MouseDown[i] = g_MousePressed[i] || glfwGetMouseButton(g_Window, i) != 0;    // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
            g_MousePressed[i] = false;
        }

        io.MouseWheel = g_MouseWheel;
        g_MouseWheel = 0.0f;

        // Hide OS mouse cursor if ImGui is drawing it
        glfwSetInputMode(g_Window, GLFW_CURSOR, io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

        // Start the frame
        ImGui::NewFrame();
    }

    void    ImGui_ImplGlfw_InvalidateDeviceObjects()
    {
        if (g_FontTexture)
        {
            glDeleteTextures(1, &g_FontTexture);
            ImGui::GetIO().Fonts->TexID = 0;
            g_FontTexture = 0;
        }
    }

    void ImGui_ImplGlfw_Shutdown(ImGuiContext* context)
    {
        ImGui_ImplGlfw_InvalidateDeviceObjects();
        ImGui::Shutdown(context);
    }

}

struct UI::Data {
    Data() {
    }

    int frametime_ms = 0;

    void *imGuiTexID = nullptr;
    ImGuiContext *imGuiContext = nullptr;
    CG::Window2D *window = nullptr;

    //ImGuiWindowFlags windowFlags = ImGuiWindowFlags_ShowBorders;
    ImGuiWindowFlags windowFlags = 0;

    std::shared_ptr<::Data::StateInput> stateInput;
    std::weak_ptr<::Data::StateData> stateData;
    std::shared_ptr<::Data::StateData> stateDataLocked;

    std::map<Event, std::function<void()>> callbacks;
};

UI::UI() : _data(new Data) {
    CG_INFO(0, "Creating UI object\n");

    if (CG::GLFW3::getInstance().init() == false) {
        CG_FATAL(0, "Error initializing GLFW!\n");
    }
}

UI::~UI() {
    CG_INFO(0, "Destroying UI object\n");

    CG::GLFW3::getInstance().terminate();
}

bool UI::init(std::weak_ptr<CG::Window2D> window) {
    if (window.expired()) {
        CG_FATAL(0, "Cannot initialize UI with null window\n");
        return false;
    }

    _data->imGuiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(_data->imGuiContext);

    // Build texture atlas
    ImGuiIO &io = ImGui::GetIO();
    unsigned char *pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width,
                                 &height);   // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

    // Upload texture to graphics system
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures(1, &g_FontTexture);
    glBindTexture(GL_TEXTURE_2D, g_FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->TexID = (void *) (intptr_t) g_FontTexture;

    // Restore state
    glBindTexture(GL_TEXTURE_2D, last_texture);


    _data->imGuiTexID = ImGui::GetIO().Fonts->TexID;

    ImGui::StyleColorsDark();

    if (auto w = window.lock()) {
        _data->window = w.get();
    } else {
        return false;
    }

    if (auto &c = _data->callbacks[BUTTON_INIT]) c();

    return true;
}

void UI::pollEvents() {
    CG::GLFW3::getInstance().poll();
}

void UI::processKeyboard() {
    if (ImGui::GetIO().KeysDown[GLFW_KEY_ESCAPE]) {
        if (auto &c = _data->callbacks[KEY_ESCAPE]) c();
    }

}


void UI::update() {
    ImGui::SetCurrentContext(_data->imGuiContext);
    ImGui_ImplGlfw_Init(_data->window->getGLFWWindow(), true);
    ImGui::GetIO().Fonts->TexID = _data->imGuiTexID;

    pollEvents();
    processKeyboard();
}

void UI::render() const {
    ImGui_ImplGlfw_NewFrame();

    auto tStart = std::chrono::high_resolution_clock::now();

    if (!(_data->stateDataLocked = _data->stateData.lock())) {
       // CG_WARN(0, "Missing state data\n");
    }

    renderMainMenuBar();
    renderWindowControls();
    renderWindowInput();
    //renderWindowOutput();

    auto tEnd = std::chrono::high_resolution_clock::now();
    _data->frametime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart).count();

    ImGui::Render();
}



void UI::terminate() {
    CG_INFO(0, "Terminating UI\n");

    ImGui::SetCurrentContext(_data->imGuiContext);
    ImGui_ImplGlfw_Shutdown(_data->imGuiContext);
    ImGui::DestroyContext(_data->imGuiContext);
}

void UI::setStateData(std::weak_ptr<::Data::StateData> stateData) {
    _data->stateData = stateData;
}

std::weak_ptr<::Data::StateInput> UI::getStateInput() const {
    return _data->stateInput;
}

void UI::setEventCallback(Event event, std::function<void()> &&callback) {
    _data->callbacks[event] = std::move(callback);
}

void UI::renderMainMenuBar() const {
    static bool showStyleEditorWindow = false;
    static bool showMetricsWindow = true;
    static bool showDebugWindow = true;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit", "Escape")) {
                if (auto &c = _data->callbacks[KEY_ESCAPE]) c();
            }
            ImGui::EndMenu();
        }


        if (ImGui::BeginMenu("Windows")) {
            ImGui::MenuItem("Debug") && (showDebugWindow = !showDebugWindow);
            ImGui::MenuItem("Metrics") && (showMetricsWindow = !showMetricsWindow);
            ImGui::MenuItem("Style Editor") && (showStyleEditorWindow = !showStyleEditorWindow);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }


    if (showMetricsWindow) {
        ImGui::ShowMetricsWindow(&showMetricsWindow);
    }

}

void UI::renderWindowControls() const {

}

void UI::renderWindowInput() const {

}

void UI::renderWindowOutput() const {

}