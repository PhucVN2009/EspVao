#pragma once
// UnityInline.h — Bridge for UnityResolve.h
// Unity.h, Vector2/3, Quaternion, VInt3 are already included via
// TuanMeta/Call_Me.h -> Call_IL2CppSDKGenerator.h -> Unity.h
// DO NOT re-include them here — Unity.h has no include guards.

#include "KittyMemory/KittyMemory.h"
#include "TuanMeta/IL2CppSDKGenerator/Il2Cpp.h"

namespace Unity {

    // Init: attach Il2Cpp metadata cache (called once from InitUnityResolve)
    inline void get_cached_unity() {
        Il2Cpp::Attach("libil2cpp.so");
    }

    // Method address — Il2Cpp already returns absolute address; base unused
    inline void* GetMethodAddress(uintptr_t base,
                                   const char* image,
                                   const char* namespaze,
                                   const char* clazz,
                                   const char* method,
                                   int args) {
        (void)base;
        return Il2Cpp::GetMethodOffset(image, namespaze, clazz, method, args);
    }

    // Field offset within class object
    inline uintptr_t GetFieldOffset(const char* image,
                                     const char* namespaze,
                                     const char* clazz,
                                     const char* field) {
        return Il2Cpp::GetFieldOffset(image, namespaze, clazz, field);
    }

} // namespace Unity
