LOCAL_PATH := $(call my-dir)
MAIN_LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE            := libdobby
LOCAL_SRC_FILES         := TuanMeta/Tools/Dobby/libraries/$(TARGET_ARCH_ABI)/libdobby.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/TuanMeta/Tools/Dobby/
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE            := libssl
LOCAL_SRC_FILES         := TuanMeta/Curl/openssl-android-$(TARGET_ARCH_ABI)/lib/libssl.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE            := libcrypto
LOCAL_SRC_FILES         := TuanMeta/Curl/openssl-android-$(TARGET_ARCH_ABI)/lib/libcrypto.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE            := libcurl
LOCAL_SRC_FILES         := TuanMeta/Curl/curl-android-$(TARGET_ARCH_ABI)/lib/libcurl.a
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := libfoxcheats
LOCAL_SRC_FILES := TuanMeta/Tools/foxcheats/libs/$(TARGET_ARCH_ABI)/libfoxcheats.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE           := tgpa

LOCAL_CFLAGS           := -Wno-error=format-security -fvisibility=hidden -ffunction-sections -fdata-sections -w
LOCAL_CFLAGS           += -fno-rtti -fno-exceptions -fpermissive
LOCAL_CPPFLAGS         := -Wno-error=format-security -fvisibility=hidden -ffunction-sections -fdata-sections -w -Werror -s -std=c++17
LOCAL_CPPFLAGS         += -Wno-error=c++11-narrowing -fms-extensions -fno-rtti -fno-exceptions -fpermissive
LOCAL_LDFLAGS          += -Wl,--gc-sections,--strip-all, -llog
LOCAL_ARM_MODE         := arm
LOCAL_LDLIBS           := -llog -landroid -lEGL -lGLESv3 -lGLESv2 -lGLESv1_CM -lz

LOCAL_C_INCLUDES       += $(LOCAL_PATH)
LOCAL_C_INCLUDES       += $(LOCAL_PATH)/ImGui
LOCAL_C_INCLUDES       += $(LOCAL_PATH)/ImGui/backends
LOCAL_C_INCLUDES       += $(LOCAL_PATH)/TuanMeta
LOCAL_C_INCLUDES       += $(LOCAL_PATH)/TuanMeta/Tools/libzip
LOCAL_C_INCLUDES       := $(LOCAL_PATH)/TuanMeta/Curl/curl-android-$(TARGET_ARCH_ABI)/include
LOCAL_C_INCLUDES       += $(LOCAL_PATH)/TuanMeta/Curl/openssl-android-$(TARGET_ARCH_ABI)/include


LOCAL_SRC_FILES := Main.cpp \
    ImGui/imgui.cpp \
    ImGui/imgui_draw.cpp \
    ImGui/imgui_tables.cpp \
    ImGui/imgui_widgets.cpp \
    ImGui/imgui_stdlib.cpp \
    ImGui/ImguiPP.cpp \
    Image/stb_image.cpp \
    ImGui/backends/imgui_impl_opengl3.cpp \
    ImGui/backends/imgui_impl_android.cpp \
    KittyMemory/KittyMemory.cpp \
    KittyMemory/MemoryPatch.cpp \
    KittyMemory/MemoryBackup.cpp \
    KittyMemory/KittyUtils.cpp \
    KittyMemory/KittyScanner.cpp \
    KittyMemory/KittyArm64.cpp \
    TuanMeta/Tools/base64.cpp \
    TuanMeta/Tools/Tools.cpp \
    TuanMeta/IL2CppSDKGenerator/Il2Cpp.cpp \
    Includes/oxorany/oxorany.cpp \
    TuanMeta/Tools/Substrate/hde64.c \
    TuanMeta/Tools/Substrate/SubstrateDebug.cpp \
    TuanMeta/Tools/Substrate/SubstrateHook.cpp \
    TuanMeta/Tools/Substrate/SubstratePosixMemory.cpp \
    TuanMeta/Tools/Substrate/SymbolFinder.cpp \
    And64InlineHook/And64InlineHook.cpp \
    #Zygisk.hpp \

LOCAL_STATIC_LIBRARIES := libdobby libcurl libssl libcrypto libfoxcheats
LOCAL_CPP_FEATURES     := exceptions

include $(BUILD_SHARED_LIBRARY)
# ============================================================================

