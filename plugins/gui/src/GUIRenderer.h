/*
 * GUIRenderer.h
 *
 * Copyright (C) 2006 - 2008 by Universitaet Stuttgart (VIS).
 * Alle Rechte vorbehalten.
 */

#ifndef MEGAMOL_GUI_GUIRENDERER_H_INCLUDED
#define MEGAMOL_GUI_GUIRENDERER_H_INCLUDED
#if (defined(_MSC_VER) && (_MSC_VER > 1000))
#    pragma once
#endif /* (defined(_MSC_VER) && (_MSC_VER > 1000)) */
#if defined(_WIN32) && defined(_MANAGED)
#    pragma managed(push, off)
#endif /* defined(_WIN32) && defined(_MANAGED) */


#include "mmcore/CallerSlot.h"
#include "mmcore/CoreInstance.h"
#include "mmcore/Module.h"
#include "mmcore/param/BoolParam.h"
#include "mmcore/param/ButtonParam.h"
#include "mmcore/param/ColorParam.h"
#include "mmcore/param/EnumParam.h"
#include "mmcore/param/FilePathParam.h"
#include "mmcore/param/FlexEnumParam.h"
#include "mmcore/param/FloatParam.h"
#include "mmcore/param/IntParam.h"
#include "mmcore/param/ParamSlot.h"
#include "mmcore/param/StringParam.h"
#include "mmcore/param/TernaryParam.h"
#include "mmcore/param/Vector2fParam.h"
#include "mmcore/param/Vector3fParam.h"
#include "mmcore/param/Vector4fParam.h"
#include "mmcore/view/Renderer2DModule.h"
#include "mmcore/view/Renderer3DModule.h"

#include "vislib/UTF8Encoder.h"

#include <iomanip> // setprecision
#include <sstream> // stringstream

#include <imgui.h>
#include "imgui_impl_opengl3.h"


namespace megamol {
namespace gui {

template <class M, class C> class GUIRenderer : public M {
public:
    /**
     * Answer the name of this module.
     *
     * @return The name of this module.
     */
    static const char* ClassName(void);

    /**
     * Answer a human readable description of this module.
     *
     * @return A human readable description of this module.
     */
    static inline const char* Description(void) { return "Graphical user interface renderer"; }

    /**
     * Answers whether this module is available on the current system.
     *
     * @return 'true' if the module is available, 'false' otherwise.
     */
    static inline bool IsAvailable(void) { return true; }

    /**
     * Initialises a new instance.
     */
    GUIRenderer();

    /**
     * Finalises an instance.
     */
    virtual ~GUIRenderer();

protected:
    virtual bool create() override;

    virtual void release() override;

    virtual bool OnKey(core::view::Key key, core::view::KeyAction action, core::view::Modifiers mods) override;

    virtual bool OnChar(unsigned int codePoint) override;

    virtual bool OnMouseButton(
        core::view::MouseButton button, core::view::MouseButtonAction action, core::view::Modifiers mods) override;

    virtual bool OnMouseMove(double x, double y) override;

    virtual bool OnMouseScroll(double dx, double dy) override;

    virtual bool GetExtents(C& call) override;

    virtual bool Render(C& call) override;


private:
    // TYPES, ENUMS -----------------------------------------------------------

    // Type for
    typedef void (GUIRenderer<M, C>::*GuiFunc)(void);

    /** Type for holding window definition. */
    typedef struct _gui_window {
        std::string label;      // window label
        bool open;              // open/close state of window
        core::view::Key hotkey; // hotkey for opening/closing window
        std::string hotkey_str; // human readable text for hotkey
        ImGuiWindowFlags flags; // imgui window flags
        GuiFunc func;           // pointer to function drawing window content
    } GUIWindow;

    // (Arbitrary) ImGui key map assignment for text manipulation hotkeys (< 512)
    enum TextModHotkeys { CTRL_A = 506, CTRL_C = 507, CTRL_V = 508, CTRL_X = 509, CTRL_Y = 510, CTRL_Z = 511 };

    // VARIABLES --------------------------------------------------------------

    /** The decorated renderer caller slot */
    core::CallerSlot decorated_renderer_slot;

    /** Current fps as string. */
    std::string fps_string;

    /** Current time delay since last time fps have been updated. */
    float fps_delay;

    /** Spacing of parameter name and hotkey in hotkey window. */
    float hotkey_spacing;

    /** Array holding window states. */
    std::list<GUIWindow> windows;

    // FUNCTIONS --------------------------------------------------------------

    /**
     * Callback for drawing the parameter window.
     */
    void drawMainWindowCallback(void);

    /**
     * Callback for drawing hotkey window.
     */
    void drawHotkeyWindowCallback(void);

    /**
     * Callback for drawing font selection window.
     */
    void drawFontSelectionWindowCallback(void);

    /**
     * Draws the menu bar.
     */
    void drawWindow(GUIWindow& win);

    /**
     * Draws the menu bar.
     */
    void drawMenu(void);

    /**
     * Draws a parameter for the parameter window.
     */
    void drawParameter(const core::Module& mod, core::param::ParamSlot& slot);

    /**
     * Shutdown megmol core.
     */
    void shutdown(void);

    /**
     * Convert vislib::sys::KeyCode to megamol::core::view::Key as integer without modifiers.
     */
    int getNoModKey(vislib::sys::KeyCode keycode);

    // ------------------------------------------------------------------------
};


typedef GUIRenderer<core::view::Renderer2DModule, core::view::CallRender2D> GUIRenderer2D;
typedef GUIRenderer<core::view::Renderer3DModule, core::view::CallRender3D> GUIRenderer3D;


/**
 * GUIRenderer<core::view::Renderer2DModule, core::view::CallRender2D>::
 */
template <>
inline GUIRenderer<core::view::Renderer2DModule, core::view::CallRender2D>::GUIRenderer()
    : decorated_renderer_slot("decoratedRenderer", "Connects to another 2D Renderer being decorated") {

    this->decorated_renderer_slot.SetCompatibleCall<core::view::CallRender2DDescription>();
    this->MakeSlotAvailable(&this->decorated_renderer_slot);
}


/**
 * GUIRenderer<core::view::Renderer3DModule, core::view::CallRender3D>::GUIRenderer
 */
template <>
inline GUIRenderer<core::view::Renderer3DModule, core::view::CallRender3D>::GUIRenderer()
    : decorated_renderer_slot("decoratedRenderer", "Connects to another 3D Renderer being decorated") {

    this->decorated_renderer_slot.SetCompatibleCall<core::view::CallRender3DDescription>();
    this->MakeSlotAvailable(&this->decorated_renderer_slot);
}


/**
 * GUIRenderer<core::view::Renderer2DModule, core::view::CallRender2D>::ClassName
 */
template <> inline const char* GUIRenderer<core::view::Renderer2DModule, core::view::CallRender2D>::ClassName(void) {

    return "GUIRenderer2D";
}


/**
 * GUIRenderer<core::view::Renderer3DModule, core::view::CallRender3D>::ClassName
 */
template <> inline const char* GUIRenderer<core::view::Renderer3DModule, core::view::CallRender3D>::ClassName(void) {

    return "GUIRenderer3D";
}


/**
 * GUIRenderer<M, C>::~GUIRenderer
 */
template <class M, class C> GUIRenderer<M, C>::~GUIRenderer() { this->Release(); }


/**
 * GUIRenderer<M, C>::create
 */
template <class M, class C> bool GUIRenderer<M, C>::create() {

    // Window definitions
    this->windows.clear();
    GUIWindow tmp_win;
    // Main Window ------------------------------------------------------------
    tmp_win.label = "MegaMol";
    tmp_win.open = true;
    tmp_win.hotkey = core::view::Key::KEY_F12;
    tmp_win.hotkey_str = "F12";
    tmp_win.flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar;
    tmp_win.func = &GUIRenderer<M, C>::drawMainWindowCallback;
    this->windows.push_back(tmp_win);
    // Hotkey Window ----------------------------------------------------------
    tmp_win.label = "Hotkeys";
    tmp_win.open = false;
    tmp_win.hotkey = core::view::Key::KEY_F11;
    tmp_win.hotkey_str = "F11";
    tmp_win.flags = ImGuiWindowFlags_AlwaysAutoResize;
    tmp_win.func = &GUIRenderer<M, C>::drawHotkeyWindowCallback;
    this->windows.push_back(tmp_win);
    // Font Selection Window --------------------------------------------------
    tmp_win.label = "Font Selection";
    tmp_win.open = false;
    tmp_win.hotkey = core::view::Key::KEY_F10;
    tmp_win.hotkey_str = "F10";
    tmp_win.flags = ImGuiWindowFlags_AlwaysAutoResize;
    tmp_win.func = &GUIRenderer<M, C>::drawFontSelectionWindowCallback;
    this->windows.push_back(tmp_win);

    // Other state variables ---------------------------------------------------
    this->hotkey_spacing = 0.0f;
    this->fps_delay = 0.0f;
    this->fps_string = "";

    // Create ImGui context ---------------------------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // IO settings ------------------------------------------------------------
    ImGuiIO& io = ImGui::GetIO();
    io.IniSavingRate = 5.0f; //  in seconds
    io.IniFilename = "imgui.ini";
    io.LogFilename = "imgui_log.txt";
    io.Fonts->AddFontDefault();

    // ImGui Key Map
    io.KeyMap[ImGuiKey_Tab] = static_cast<int>(core::view::Key::KEY_TAB);
    io.KeyMap[ImGuiKey_LeftArrow] = static_cast<int>(core::view::Key::KEY_LEFT);
    io.KeyMap[ImGuiKey_RightArrow] = static_cast<int>(core::view::Key::KEY_RIGHT);
    io.KeyMap[ImGuiKey_UpArrow] = static_cast<int>(core::view::Key::KEY_UP);
    io.KeyMap[ImGuiKey_DownArrow] = static_cast<int>(core::view::Key::KEY_DOWN);
    io.KeyMap[ImGuiKey_PageUp] = static_cast<int>(core::view::Key::KEY_PAGE_UP);
    io.KeyMap[ImGuiKey_PageDown] = static_cast<int>(core::view::Key::KEY_PAGE_DOWN);
    io.KeyMap[ImGuiKey_Home] = static_cast<int>(core::view::Key::KEY_HOME);
    io.KeyMap[ImGuiKey_End] = static_cast<int>(core::view::Key::KEY_END);
    io.KeyMap[ImGuiKey_Insert] = static_cast<int>(core::view::Key::KEY_INSERT);
    io.KeyMap[ImGuiKey_Delete] = static_cast<int>(core::view::Key::KEY_DELETE);
    io.KeyMap[ImGuiKey_Backspace] = static_cast<int>(core::view::Key::KEY_BACKSPACE);
    io.KeyMap[ImGuiKey_Space] = static_cast<int>(core::view::Key::KEY_SPACE);
    io.KeyMap[ImGuiKey_Enter] = static_cast<int>(core::view::Key::KEY_ENTER);
    io.KeyMap[ImGuiKey_Escape] = static_cast<int>(core::view::Key::KEY_ESCAPE);
    io.KeyMap[ImGuiKey_A] = static_cast<int>(TextModHotkeys::CTRL_A);
    io.KeyMap[ImGuiKey_C] = static_cast<int>(TextModHotkeys::CTRL_C);
    io.KeyMap[ImGuiKey_V] = static_cast<int>(TextModHotkeys::CTRL_V);
    io.KeyMap[ImGuiKey_X] = static_cast<int>(TextModHotkeys::CTRL_X);
    io.KeyMap[ImGuiKey_Y] = static_cast<int>(TextModHotkeys::CTRL_Y);
    io.KeyMap[ImGuiKey_Z] = static_cast<int>(TextModHotkeys::CTRL_Z);

    // Style settings ---------------------------------------------------------
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 3.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.AntiAliasedLines = true;
    style.AntiAliasedFill = true;
    style.DisplaySafeAreaPadding = ImVec2(5.0f, 5.0f);

    // Init OpenGL for ImGui --------------------------------------------------
    const char* glsl_version = "#version 150";
    ImGui_ImplOpenGL3_Init(glsl_version);

    return true;
}


/**
 * GUIRenderer<M, C>::release
 */
template <class M, class C> void GUIRenderer<M, C>::release() {

    this->windows.clear();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}


/**
 * GUIRenderer<M, C>::OnKey
 */
template <class M, class C>
bool GUIRenderer<M, C>::OnKey(core::view::Key key, core::view::KeyAction action, core::view::Modifiers mods) {

    ImGuiIO& io = ImGui::GetIO();
    auto keyIndex = static_cast<size_t>(key);
    switch (action) {
    case core::view::KeyAction::PRESS:
        io.KeysDown[keyIndex] = true;
        break;
    case core::view::KeyAction::RELEASE:
        io.KeysDown[keyIndex] = false;
        break;
    default:
        break;
    }
    io.KeyCtrl = mods.test(core::view::Modifier::CTRL);
    io.KeyShift = mods.test(core::view::Modifier::SHIFT);
    io.KeyAlt = mods.test(core::view::Modifier::ALT);
    io.KeySuper = mods.test(core::view::Modifier::SUPER);

    // Check for additional text modification hotkeys
    if (action == core::view::KeyAction::RELEASE) {
        io.KeysDown[static_cast<size_t>(TextModHotkeys::CTRL_A)] = false;
        io.KeysDown[static_cast<size_t>(TextModHotkeys::CTRL_C)] = false;
        io.KeysDown[static_cast<size_t>(TextModHotkeys::CTRL_V)] = false;
        io.KeysDown[static_cast<size_t>(TextModHotkeys::CTRL_X)] = false;
        io.KeysDown[static_cast<size_t>(TextModHotkeys::CTRL_Y)] = false;
        io.KeysDown[static_cast<size_t>(TextModHotkeys::CTRL_Z)] = false;
    }
    bool hotkeyPressed = true;
    if (io.KeyCtrl && ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_A))) {
        keyIndex = static_cast<size_t>(TextModHotkeys::CTRL_A);
    } else if (io.KeyCtrl && ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_C))) {
        keyIndex = static_cast<size_t>(TextModHotkeys::CTRL_C);
    } else if (io.KeyCtrl && ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_V))) {
        keyIndex = static_cast<size_t>(TextModHotkeys::CTRL_V);
    } else if (io.KeyCtrl && ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_X))) {
        keyIndex = static_cast<size_t>(TextModHotkeys::CTRL_X);
    } else if (io.KeyCtrl && ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_Y))) {
        keyIndex = static_cast<size_t>(TextModHotkeys::CTRL_Y);
    } else if (io.KeyCtrl && ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_Z))) {
        keyIndex = static_cast<size_t>(TextModHotkeys::CTRL_Z);
    } else {
        hotkeyPressed = false;
    }
    if (hotkeyPressed && (action == core::view::KeyAction::PRESS)) {
        io.KeysDown[keyIndex] = true;
    }

    // Exit megamol
    bool exit = (ImGui::IsKeyDown(io.KeyMap[ImGuiKey_Escape])) ||                               // Escape
                (ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_Q))) ||                 // 'q'
                ((io.KeyAlt) && (ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_F4)))); // Alt + F4
    if (exit) {
        this->shutdown();
        return true;
    }

    // Window hotkeys
    for (auto& win : this->windows) {
        if ((ImGui::IsKeyDown(static_cast<int>(win.hotkey)))) {
            win.open = !win.open;
        }
    }

    // Check for pressed arameter hotkeys
    hotkeyPressed = false;
    this->GetCoreInstance()->EnumParameters([&, this](const auto& mod, auto& slot) {
        auto param = slot.Parameter();
        if (!param.IsNull()) {
            if (auto* p = slot.Param<core::param::ButtonParam>()) {
                auto keyCode = p->GetKeyCode();
                auto noModKey = this->getNoModKey(keyCode);
                hotkeyPressed = (ImGui::IsKeyDown(noModKey)) && (keyCode.IsAltMod() == io.KeyAlt) &&
                                (keyCode.IsCtrlMod() == io.KeyCtrl) && (keyCode.IsShiftMod() == io.KeyShift);
                if (hotkeyPressed) {
                    p->setDirty();
                }
            }
        }
    });
    if (hotkeyPressed) return true;


    auto* cr = this->decorated_renderer_slot.template CallAs<C>();
    if (cr == nullptr) return false;

    core::view::InputEvent evt;
    evt.tag = core::view::InputEvent::Tag::Key;
    evt.keyData.key = key;
    evt.keyData.action = action;
    evt.keyData.mods = mods;
    cr->SetInputEvent(evt);
    if (!(*cr)(core::view::InputCall::FnOnKey)) return false;

    return false;
}


/**
 * GUIRenderer<M, C>::OnChar
 */
template <class M, class C> bool GUIRenderer<M, C>::OnChar(unsigned int codePoint) {

    ImGuiIO& io = ImGui::GetIO();
    io.ClearInputCharacters();
    if (codePoint > 0 && codePoint < 0x10000) io.AddInputCharacter((unsigned short)codePoint);

    auto* cr = this->decorated_renderer_slot.template CallAs<C>();
    if (cr) {
        core::view::InputEvent evt;
        evt.tag = core::view::InputEvent::Tag::Char;
        evt.charData.codePoint = codePoint;
        cr->SetInputEvent(evt);
        if ((*cr)(core::view::InputCall::FnOnChar)) return true;
    }

    return true;
}


/**
 * GUIRenderer<M, C>::OnMouseMove
 */
template <class M, class C> bool GUIRenderer<M, C>::OnMouseMove(double x, double y) {

    ImGuiIO& io = ImGui::GetIO();
    io.MousePos =
        ImVec2((float)x, (float)y); // TODO: This is broken, since x and y are transformed by View2D class
                                    // => will be fixed when screen2world transformation is available in CallRender.

    auto hoverFlags =
        ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_AllowWhenBlockedByPopup;
    if (!ImGui::IsWindowHovered(hoverFlags)) {
        auto* cr = this->decorated_renderer_slot.template CallAs<C>();
        if (cr == NULL) return false;

        core::view::InputEvent evt;
        evt.tag = core::view::InputEvent::Tag::MouseMove;
        evt.mouseMoveData.x = x;
        evt.mouseMoveData.y = y;
        cr->SetInputEvent(evt);
        if (!(*cr)(core::view::InputCall::FnOnMouseMove)) return false;
    }

    return true;
}


/**
 * GUIRenderer<M, C>::OnMouseButton
 */
template <class M, class C>
bool GUIRenderer<M, C>::OnMouseButton(
    core::view::MouseButton button, core::view::MouseButtonAction action, core::view::Modifiers mods) {

    bool down = (action == core::view::MouseButtonAction::PRESS);
    auto buttonIndex = static_cast<size_t>(button);
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[buttonIndex] = down;

    auto hoverFlags =
        ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_AllowWhenBlockedByPopup;
    if (!ImGui::IsWindowHovered(hoverFlags)) {
        auto* cr = this->decorated_renderer_slot.template CallAs<C>();
        if (cr == NULL) return false;

        core::view::InputEvent evt;
        evt.tag = core::view::InputEvent::Tag::MouseButton;
        evt.mouseButtonData.button = button;
        evt.mouseButtonData.action = action;
        evt.mouseButtonData.mods = mods;
        cr->SetInputEvent(evt);
        if (!(*cr)(core::view::InputCall::FnOnMouseButton)) return false;
    }

    return true;
}


/**
 * GUIRenderer<M, C>::template <class M, class C> bool GUIRenderer<M, C>::OnMouseScroll(double dx, double dy) {

 */
template <class M, class C> bool GUIRenderer<M, C>::OnMouseScroll(double dx, double dy) {

    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheelH += (float)dx;
    io.MouseWheel += (float)dy;

    auto hoverFlags =
        ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_AllowWhenBlockedByPopup;
    if (!ImGui::IsWindowHovered(hoverFlags)) {
        auto* cr = this->decorated_renderer_slot.template CallAs<C>();
        if (cr == NULL) return false;

        core::view::InputEvent evt;
        evt.tag = core::view::InputEvent::Tag::MouseScroll;
        evt.mouseScrollData.dx = dx;
        evt.mouseScrollData.dy = dy;
        cr->SetInputEvent(evt);
        if (!(*cr)(core::view::InputCall::FnOnMouseScroll)) return false;
    }

    return true;
}


/**
 * GUIRenderer<core::view::Renderer2DModule, core::view::CallRender2D>::GetExtents
 */
template <>
inline bool GUIRenderer<core::view::Renderer2DModule, core::view::CallRender2D>::GetExtents(
    core::view::CallRender2D& call) {

    auto* cr = this->decorated_renderer_slot.CallAs<core::view::CallRender2D>();
    if (cr != NULL) {
        (*cr) = call;
        if ((*cr)(core::view::AbstractCallRender::FnGetExtents)) {
            call = (*cr);
        }
    } else {
        call.SetBoundingBox(vislib::math::Rectangle<float>(0, 1, 1, 0));
    }

    return true;
}


/**
 * GUIRenderer<core::view::Renderer3DModule, core::view::CallRender3D>::GetExtents
 */
template <>
inline bool GUIRenderer<core::view::Renderer3DModule, core::view::CallRender3D>::GetExtents(
    core::view::CallRender3D& call) {

    auto* cr = this->decorated_renderer_slot.CallAs<core::view::CallRender3D>();
    if (cr != NULL) {
        (*cr) = call;
        if ((*cr)(core::view::AbstractCallRender::FnGetExtents)) {
            call = (*cr);
        }
    } else {
        call.AccessBoundingBoxes().Clear();
        call.AccessBoundingBoxes().SetWorldSpaceBBox(
            vislib::math::Cuboid<float>(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f));
    }

    return true;
}


/**
 * GUIRenderer<M, C>::Render
 */
template <class M, class C> bool GUIRenderer<M, C>::Render(C& call) {

    auto* cr = this->decorated_renderer_slot.template CallAs<C>();
    if (cr != NULL) {
        (*cr) = call;
        if ((*cr)(core::view::AbstractCallRender::FnRender)) {
            call = (*cr);
        }
    }

    auto viewportWidth = call.GetViewport().Width();
    auto viewportHeight = call.GetViewport().Height();

    // Set IO stuff
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)viewportWidth, (float)viewportHeight);
    io.DisplayFramebufferScale = ImVec2(1.0, 1.0);
    io.DeltaTime = static_cast<float>(call.LastFrameTime() / 1000.0); // in milliseconds

    // Start the frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // Construct frame
    for (auto& win : this->windows) {
        this->drawWindow(win);
    }

    // Render the frame
    glViewport(0, 0, viewportWidth, viewportHeight);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return true;
}


/**
 * GUIRenderer<M, C>::drawMainWindowCallback
 */
template <class M, class C> void GUIRenderer<M, C>::drawMainWindowCallback(void) {

    // Menu -------------------------------------------------------------------
    if (ImGui::BeginMenuBar()) {
        this->drawMenu();
        ImGui::EndMenuBar();
    }

    // Parameters -------------------------------------------------------------
    ImGui::Text("Parameters by Modules: ");

    int overrideState = -1;
    ImGui::SameLine(180.0f);
    if (ImGui::Button("Expand All")) {
        overrideState = 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Collapse All")) {
        overrideState = 0;
    }

    const core::Module* currentMod = nullptr;
    bool currentModOpen = false;
    this->GetCoreInstance()->EnumParameters([&, this](const auto& mod, auto& slot) {
        if (currentMod != &mod) {
            currentMod = &mod;

            auto headerId = ImGui::GetID(mod.FullName());
            auto headerState = overrideState;
            if (headerState == -1) {
                headerState = ImGui::GetStateStorage()->GetInt(headerId, 0); // 0=close 1=open
            }
            ImGui::GetStateStorage()->SetInt(headerId, headerState);
            currentModOpen = ImGui::CollapsingHeader(mod.FullName());
        }
        if (currentModOpen) {
            this->drawParameter(mod, slot);
        }
    });
}


/**
 * GUIRenderer<M, C>::drawHotkeyWindowCallback
 */
template <class M, class C> void GUIRenderer<M, C>::drawHotkeyWindowCallback(void) {

    this->GetCoreInstance()->EnumParameters([&, this](const auto& mod, auto& slot) {
        auto param = slot.Parameter();
        if (!param.IsNull()) {
            if (auto* p = slot.Param<core::param::ButtonParam>()) {
                auto label = std::string(slot.Name().PeekBuffer());
                auto keycode = std::string(p->GetKeyCode().ToStringA().PeekBuffer());

                ImGui::Text(label.data());
                // Adapt spacing between label and hotkey string
                auto labelLength = ImGui::GetFontSize() * (float)label.length() * 0.8f;
                this->hotkey_spacing = (labelLength > this->hotkey_spacing) ? (labelLength) : (this->hotkey_spacing);
                ImGui::SameLine(this->hotkey_spacing);

                ImGui::Text(keycode.data());
                ImGui::Separator();
            }
        }
    });
}


/**
 * GUIRenderer<M, C>::drawFontSelectionWindowCallback
 */
template <class M, class C> void GUIRenderer<M, C>::drawFontSelectionWindowCallback(void) {

    char buffer[4096];
    if (ImGui::InputText("Filename", buffer, IM_ARRAYSIZE(buffer))) {
    }
    if (ImGui::Button("Load Font")) {
    }


    // ImFontConfig config;
    // config.OversampleH = 5;
    // config.OversampleV = 1;
    // float font_size = 15.0f;
    //// ASSERT if file is not found!
    // ImFont* font = io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", font_size, &config);
    // ImFont* font = io.Fonts->AddFontFromFileTTF("Karla-Regular.ttf", font_size, &config);
    // ImFont* font = io.Fonts->AddFontFromFileTTF("DroidSans.ttf", font_size, &config);
    // ImFont* font = io.Fonts->AddFontFromFileTTF("Cousine-Regular.ttf", font_size, &config);
    // ImFont* font = io.Fonts->AddFontFromFileTTF("ProggyTiny.ttf", font_size, &config);
    // ImFont* font = io.Fonts->AddFontFromFileTTF("ProggyClean.ttf", font_size, &config);
    // ImGui::PushFont(font);
}


/**
 * GUIRenderer<M, C>::drawWindow
 */
template <class M, class C> void GUIRenderer<M, C>::drawWindow(GUIWindow& win) {

    if (win.open) {
        ImGui::SetNextWindowPos(ImVec2(5.0f, 5.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin(win.label.data(), &win.open, win.flags);
        (this->*win.func)();
        ImGui::End();
    }
}


/**
 * GUIRenderer<M, C>::drawMenu
 */
template <class M, class C> void GUIRenderer<M, C>::drawMenu(void) {

    ImGuiIO& io = ImGui::GetIO();

    // fps
    const float delay = 1.0f; // only update every second
    this->fps_delay += io.DeltaTime;
    if (this->fps_delay >= delay) {
        std::stringstream stream;
        stream << std::fixed << std::setprecision(2) //<< std::setw(7)
               << io.Framerate;
        this->fps_string = stream.str();
        this->fps_delay = 0.0f;
    }

    // File
    if (ImGui::BeginMenu("File")) {
        ImGui::Text(this->fps_string.data());
        ImGui::SameLine();
        std::string hint = "Copy to Clipboard";
        if (ImGui::MenuItem("fps", hint.data())) {
            ImGui::SetClipboardText(fps_string.data());
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit", "Alt + F4, 'q', Esc")) {
            this->shutdown();
        }
        ImGui::EndMenu();
    }

    // Windows
    if (ImGui::BeginMenu("Windows")) {
        for (auto& win : this->windows) {
            if (ImGui::MenuItem(win.label.data(), win.hotkey_str.data(), &win.open)) {
                win.open = !win.open;
            }
        }
        ImGui::EndMenu();
    }

    // Help
    bool open_popup = false;
    if (ImGui::BeginMenu("Help")) {
        const std::string gitLink = "https://github.com/UniStuttgart-VISUS/megamol";
        const std::string mmLink = "https://megamol.org/";
        const std::string helpLink = "https://github.com/UniStuttgart-VISUS/megamol/blob/master/Readme.md";
        const std::string hint = "Copy Link to Clipboard";
        if (ImGui::MenuItem("GitHub", hint.data())) {
            ImGui::SetClipboardText(gitLink.data());
        }
        if (ImGui::MenuItem("Readme", hint.data())) {
            ImGui::SetClipboardText(helpLink.data());
        }
        if (ImGui::MenuItem("Web Page", hint.data())) {
            ImGui::SetClipboardText(mmLink.data());
        }
        ImGui::Separator();
        if (ImGui::MenuItem("About...")) {
            open_popup = true;
        }
        ImGui::EndMenu();
    }

    // PopUp
    std::stringstream stream;
    stream << "MegaMol is GREAT!" << std::endl << "Using Dear ImGui " << IMGUI_VERSION << std::endl;
    std::string about = stream.str();
    if (open_popup) {
        ImGui::OpenPopup("About");
    }
    if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text(about.data());
        ImGui::Separator();
        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::EndPopup();
    }
}


/**
 * GUIRenderer<M, C>::drawParameter
 */
template <class M, class C>
void GUIRenderer<M, C>::drawParameter(const core::Module& mod, core::param::ParamSlot& slot) {

    auto param = slot.Parameter();
    if (!param.IsNull()) {
        auto label = slot.Name().PeekBuffer();
        if (auto* p = slot.Param<core::param::BoolParam>()) {
            auto value = p->Value();
            if (ImGui::Checkbox(label, &value)) {
                p->SetValue(value);
            }
        } else if (auto* p = slot.Param<core::param::ButtonParam>()) {
            std::string hotkeyLabel(label);
            hotkeyLabel.append(" (");
            hotkeyLabel.append(p->GetKeyCode().ToStringA().PeekBuffer());
            hotkeyLabel.append(")");

            if (ImGui::Button(hotkeyLabel.data())) {
                p->setDirty();
            }
        } else if (auto* p = slot.Param<core::param::ColorParam>()) {
            core::param::ColorParam::Type value;
            std::memcpy(value, p->Value(), sizeof(core::param::ColorParam::Type));
            if (ImGui::ColorEdit4(label, value)) {
                p->SetValue(value);
            }
        } else if (auto* p = slot.Param<core::param::EnumParam>()) {
            // XXX: no UTF8 fanciness required here?
            auto map = p->getMap();
            auto key = p->Value();
            if (ImGui::BeginCombo(label, map[key].PeekBuffer())) {
                auto iter = map.GetConstIterator();
                while (iter.HasNext()) {
                    auto pair = iter.Next();
                    bool isSelected = (pair.Key() == key);
                    if (ImGui::Selectable(pair.Value().PeekBuffer(), isSelected)) {
                        p->SetValue(pair.Key());
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        } else if (auto* p = slot.Param<core::param::FlexEnumParam>()) {
            // XXX: no UTF8 fanciness required here?
            auto value = p->Value();
            if (ImGui::BeginCombo(label, value.c_str())) {
                for (auto valueOption : p->getStorage()) {
                    bool isSelected = (valueOption == value);
                    if (ImGui::Selectable(valueOption.c_str(), isSelected)) {
                        p->SetValue(valueOption);
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        } else if (auto* p = slot.Param<core::param::FloatParam>()) {
            auto value = p->Value();
            if (ImGui::InputFloat(label, &value)) {
                p->SetValue(value);
            }
        } else if (auto* p = slot.Param<core::param::IntParam>()) {
            auto value = p->Value();
            if (ImGui::InputInt(label, &value)) {
                p->SetValue(value);
            }
        } else if (auto* p = slot.Param<core::param::Vector2fParam>()) {
            auto value = p->Value();
            if (ImGui::InputFloat2(label, value.PeekComponents())) {
                p->SetValue(value);
            }
        } else if (auto* p = slot.Param<core::param::Vector3fParam>()) {
            auto value = p->Value();
            if (ImGui::InputFloat3(label, value.PeekComponents())) {
                p->SetValue(value);
            }
        } else if (auto* p = slot.Param<core::param::Vector4fParam>()) {
            auto value = p->Value();
            if (ImGui::InputFloat4(label, value.PeekComponents())) {
                p->SetValue(value);
            }
        } else { // if (auto* p = slot.Param<core::param::StringParam>()) {
            // XXX: UTF8 conversion and allocation every frame is horrific inefficient.
            vislib::StringA valueString;
            vislib::UTF8Encoder::Encode(valueString, param->ValueString());

            size_t bufferLength = std::min(4096, (valueString.Length() + 1) * 2);
            char* buffer = new char[bufferLength];
            memcpy(buffer, valueString, valueString.Length() + 1);

            if (ImGui::InputText(slot.Name().PeekBuffer(), buffer, bufferLength)) {
                vislib::UTF8Encoder::Decode(valueString, vislib::StringA(buffer));
                param->ParseValue(valueString);
            }

            delete[] buffer;
        }
    }
}


/**
 * GUIRenderer<M, C>::shutdown
 */
template <class M, class C> void GUIRenderer<M, C>::shutdown(void) {

    vislib::sys::Log::DefaultLog.WriteInfo(">>>>> GUIRenderer: Initialising megamol core instance shutdown.");
    this->GetCoreInstance()->Shutdown();
}


/**
 * GUIRenderer<M, C>::getNoModKey
 */
template <class M, class C> int GUIRenderer<M, C>::getNoModKey(vislib::sys::KeyCode keycode) {

    auto key = static_cast<int>(keycode.NoModKeys());

    switch (key) {
    case (97):
        return static_cast<int>(core::view::Key::KEY_A);
    case (98):
        return static_cast<int>(core::view::Key::KEY_B);
    case (99):
        return static_cast<int>(core::view::Key::KEY_C);
    case (100):
        return static_cast<int>(core::view::Key::KEY_D);
    case (101):
        return static_cast<int>(core::view::Key::KEY_E);
    case (102):
        return static_cast<int>(core::view::Key::KEY_F);
    case (103):
        return static_cast<int>(core::view::Key::KEY_G);
    case (104):
        return static_cast<int>(core::view::Key::KEY_H);
    case (105):
        return static_cast<int>(core::view::Key::KEY_I);
    case (106):
        return static_cast<int>(core::view::Key::KEY_J);
    case (107):
        return static_cast<int>(core::view::Key::KEY_K);
    case (108):
        return static_cast<int>(core::view::Key::KEY_L);
    case (109):
        return static_cast<int>(core::view::Key::KEY_M);
    case (110):
        return static_cast<int>(core::view::Key::KEY_N);
    case (111):
        return static_cast<int>(core::view::Key::KEY_O);
    case (112):
        return static_cast<int>(core::view::Key::KEY_P);
    case (113):
        return static_cast<int>(core::view::Key::KEY_Q);
    case (114):
        return static_cast<int>(core::view::Key::KEY_R);
    case (115):
        return static_cast<int>(core::view::Key::KEY_S);
    case (116):
        return static_cast<int>(core::view::Key::KEY_T);
    case (117):
        return static_cast<int>(core::view::Key::KEY_U);
    case (118):
        return static_cast<int>(core::view::Key::KEY_V);
    case (119):
        return static_cast<int>(core::view::Key::KEY_W);
    case (120):
        return static_cast<int>(core::view::Key::KEY_X);
    case (121):
        return static_cast<int>(core::view::Key::KEY_Y);
    case (122):
        return static_cast<int>(core::view::Key::KEY_Z);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_ESC)):
        return static_cast<int>(core::view::Key::KEY_ESCAPE);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_ENTER)):
        return static_cast<int>(core::view::Key::KEY_ENTER);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_TAB)):
        return static_cast<int>(core::view::Key::KEY_TAB);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_BACKSPACE)):
        return static_cast<int>(core::view::Key::KEY_BACKSPACE);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_INSERT)):
        return static_cast<int>(core::view::Key::KEY_INSERT);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_DELETE)):
        return static_cast<int>(core::view::Key::KEY_DELETE);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_RIGHT)):
        return static_cast<int>(core::view::Key::KEY_RIGHT);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_LEFT)):
        return static_cast<int>(core::view::Key::KEY_LEFT);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_DOWN)):
        return static_cast<int>(core::view::Key::KEY_DOWN);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_UP)):
        return static_cast<int>(core::view::Key::KEY_UP);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_PAGE_UP)):
        return static_cast<int>(core::view::Key::KEY_PAGE_UP);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_PAGE_DOWN)):
        return static_cast<int>(core::view::Key::KEY_PAGE_DOWN);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_HOME)):
        return static_cast<int>(core::view::Key::KEY_HOME);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_END)):
        return static_cast<int>(core::view::Key::KEY_END);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_F1)):
        return static_cast<int>(core::view::Key::KEY_F1);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_F2)):
        return static_cast<int>(core::view::Key::KEY_F2);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_F3)):
        return static_cast<int>(core::view::Key::KEY_F3);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_F4)):
        return static_cast<int>(core::view::Key::KEY_F4);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_F5)):
        return static_cast<int>(core::view::Key::KEY_F5);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_F6)):
        return static_cast<int>(core::view::Key::KEY_F6);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_F7)):
        return static_cast<int>(core::view::Key::KEY_F7);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_F8)):
        return static_cast<int>(core::view::Key::KEY_F8);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_F9)):
        return static_cast<int>(core::view::Key::KEY_F9);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_F10)):
        return static_cast<int>(core::view::Key::KEY_F10);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_F11)):
        return static_cast<int>(core::view::Key::KEY_F11);
    case (static_cast<int>(vislib::sys::KeyCode::KEY_F12)):
        return static_cast<int>(core::view::Key::KEY_F12);
    }
    return key;
}

} // end namespace gui
} // end namespace megamol

#endif // MEGAMOL_GUI_GUIRENDERER_H_INCLUDED