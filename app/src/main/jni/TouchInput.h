#pragma once

#include "UnityResolve.h"
#include "ImGui/Call_ImGui.h"

namespace TouchInput {

enum class TouchPhase { Began, Moved, Stationary, Ended, Canceled };
enum class TouchType { Direct, Indirect, Stylus };

struct Touch {
    int m_FingerId{};
    Vector2 m_Position;
    Vector2 m_RawPosition;
    Vector2 m_PositionDelta;
    float m_TimeDelta{};
    int m_TapCount{};
    TouchPhase m_Phase;
    TouchType m_Type;
    float m_Pressure{};
    float m_maximumPossiblePressure{};
    float m_Radius{};
    float m_RadiusVariance{};
    float m_AltitudeAngle{};
    float m_AzimuthAngle{};
};

static Touch (*Input_GetTouch)(int index) = nullptr;
static int (*Input_get_touchCount)() = nullptr;
static bool is_init = false;

inline void Init() {
    // Try multiple possible image names
    const char* images[] = {
        "UnityEngine.dll",
        "UnityEngine.CoreModule.dll",
        "UnityEngine.InputLegacyModule.dll",
        nullptr
    };

    for (int i = 0; images[i] != nullptr; i++) {
        Input_GetTouch = (Touch (*)(int)) GetMethodOffset(images[i], "UnityEngine", "Input", "GetTouch", 1);
        Input_get_touchCount = (int (*)()) GetMethodOffset(images[i], "UnityEngine", "Input", "get_touchCount", 0);
        if (Input_GetTouch && Input_get_touchCount) break;
    }

    is_init = (Input_GetTouch != nullptr && Input_get_touchCount != nullptr);
    __android_log_print(ANDROID_LOG_INFO, "TouchInput", "Init: GetTouch=%p touchCount=%p init=%d", Input_GetTouch, Input_get_touchCount, is_init);
}

inline void Update() {
    if (!is_init) return;

    if (ImGui::GetCurrentContext() == nullptr) return;
    ImGuiIO& io = ImGui::GetIO();
    if (io.DisplaySize.x <= 0 || io.DisplaySize.y <= 0) return;

    int touchCount = Input_get_touchCount();
    if (touchCount < 0 || touchCount > 10) touchCount = 0;

    for (int idx = 0; idx < touchCount && idx < 3; ++idx) {
        Touch touch = Input_GetTouch(idx);
        float x = touch.m_Position.x;
        float y = round(io.DisplaySize.y) - touch.m_Position.y;

        switch (touch.m_Phase) {
            case TouchPhase::Began:
                io.MousePos = ImVec2(x, y);
                io.MouseDown[idx] = true;
                break;
            case TouchPhase::Moved:
            case TouchPhase::Stationary:
                io.MousePos = ImVec2(x, y);
                break;
            case TouchPhase::Ended:
            case TouchPhase::Canceled:
                io.MouseDown[idx] = false;
                break;
            default: break;
        }
    }
    for (int idx = touchCount; idx < 3; ++idx) {
        io.MouseDown[idx] = false;
    }
}

} // namespace TouchInput
