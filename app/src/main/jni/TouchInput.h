#pragma once

#include <atomic>
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
// atomic with release on write / acquire on read prevents ARM64 store reordering
// from making is_init visible before the function pointers are visible
static std::atomic<bool> is_init{false};

inline void Init() {
    const char* images[] = {
        "UnityEngine.dll",
        "UnityEngine.CoreModule.dll",
        "UnityEngine.InputLegacyModule.dll",
        nullptr
    };

    Touch (*tmp_touch)(int) = nullptr;
    int (*tmp_count)() = nullptr;

    for (int i = 0; images[i] != nullptr; i++) {
        tmp_touch = (Touch (*)(int)) GetMethodOffset(images[i], "UnityEngine", "Input", "GetTouch", 1);
        tmp_count = (int (*)()) GetMethodOffset(images[i], "UnityEngine", "Input", "get_touchCount", 0);
        if (tmp_touch && tmp_count) break;
    }

    // write function pointers before releasing is_init to the render thread
    Input_GetTouch = tmp_touch;
    Input_get_touchCount = tmp_count;
    // release store: all prior writes (Input_GetTouch, Input_get_touchCount) are
    // guaranteed visible to any thread that does an acquire load on is_init
    is_init.store(tmp_touch != nullptr && tmp_count != nullptr, std::memory_order_release);
    __android_log_print(ANDROID_LOG_INFO, "TouchInput", "Init: GetTouch=%p touchCount=%p init=%d",
        Input_GetTouch, Input_get_touchCount, (int)is_init.load());
}

inline void Update() {
    // acquire load pairs with the release store in Init(), ensuring we see all
    // writes made before is_init was set true
    if (!is_init.load(std::memory_order_acquire)) return;
    // belt-and-suspenders: never call through a null pointer
    if (!Input_GetTouch || !Input_get_touchCount) return;

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
