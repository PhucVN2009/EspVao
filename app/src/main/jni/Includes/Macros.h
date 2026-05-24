#ifndef ANDROID_MOD_MENU_MACROS_H
#define ANDROID_MOD_MENU_MACROS_H
#include <KittyMemory/KittyMemory.h>
#include <KittyMemory/MemoryPatch.h>
#include <KittyMemory/KittyScanner.h>
#include <KittyMemory/KittyUtils.h>
#include "../And64InlineHook/And64InlineHook.hpp"
#include "TuanMeta/Tools/Dobby/dobby.h"

void hook(void *offset, void* ptr, void **orig)
{
    DobbyHook(offset, ptr, orig);
}

#define HOOK(offset, ptr, orig) DobbyHook((void *)getAbsoluteAddress(oxorany(lib), offset), (void *)ptr, (void **)&orig)
#define HOOK_LIB(lib, offset, ptr, orig) hook((void *)getAbsoluteAddress(oxorany(lib), string2Offset(oxorany(offset))), (void *)ptr, (void **)&orig)
#define HOOK_NO_ORIG(offset, ptr) DobbyHook((void *)getAbsoluteAddress(oxorany(lib), oxorany(offset)), (void *)ptr, NULL)
#define HOOK_LIB_NO_ORIG(lib, offset, ptr) hook((void *)getAbsoluteAddress(oxorany(lib), string2Offset(oxorany(offset))), (void *)ptr, NULL)

#define HOOKSYM(sym, ptr, org) DobbyHook(dlsym(dlopen(targetLibName, 4), OBFUSCATE(sym)), (void *)ptr, (void **)&org)
#define HOOKSYM_LIB(lib, sym, ptr, org) DobbyHook(dlsym(dlopen(OBFUSCATE(lib), 4), OBFUSCATE(sym)), (void *)ptr, (void **)&org)

#define HOOKSYM_NO_ORIG(sym, ptr)  DobbyHook(dlsym(dlopen(targetLibName, 4), OBFUSCATE(sym)), (void *)ptr, NULL)
#define HOOKSYM_LIB_NO_ORIG(lib, sym, ptr) DobbyHook(dlsym(dlopen(OBFUSCATE(lib), 4), OBFUSCATE(sym)), (void *)ptr, NULL)


std::vector<MemoryPatch> memoryPatches;
std::vector<uint64_t> offsetVector;

// Patching a offset without switch.
// Modified patchOffset function
void patchOffset(const char* libName, uintptr_t offset, std::string hex, bool isOn) {
    // Convert library name to KittyMemory::ProcMap
    KittyMemory::ProcMap map;
    // Implement the actual method to fill the map for the given library name
    // For demonstration purposes, we'll assume you have a way to do this
    // Example: map = someFunctionToGetProcMap(libName);

    MemoryPatch patch = MemoryPatch::createWithHex(map, offset, hex);

    // Check if offset exists in the offsetVector
    if (std::find(offsetVector.begin(), offsetVector.end(), offset) != offsetVector.end()) {
        // LOGE(OBFUSCATE("Already exists"));
        std::vector<uint64_t>::iterator itr = std::find(offsetVector.begin(), offsetVector.end(), offset);
        patch = memoryPatches[std::distance(offsetVector.begin(), itr)]; // Get index of memoryPatches vector
    } else {
        memoryPatches.push_back(patch);
        offsetVector.push_back(offset);
        // LOGI(OBFUSCATE("Added"));
    }

    if (!patch.isValid()) {
        LOGE(OBFUSCATE("Failing offset: 0x%llu, please re-check the hex"), offset);
        return;
    }
    if (isOn) {
        if (!patch.Modify()) {
            LOGE(OBFUSCATE("Something went wrong while patching this offset: 0x%llu"), offset);
        }
    } else {
        if (!patch.Restore()) {
            LOGE(OBFUSCATE("Something went wrong while restoring this offset: 0x%llu"), offset);
        }
    }
}

void patchOffsetSym(uintptr_t absolute_address, std::string hexBytes, bool isOn) {
    MemoryPatch patch = MemoryPatch::createWithHex(absolute_address, hexBytes);

    // Check if offset exists in the offsetVector
    if (std::find(offsetVector.begin(), offsetVector.end(), absolute_address) != offsetVector.end()) {
        // LOGE(OBFUSCATE("Already exists"));
        std::vector<uint64_t>::iterator itr = std::find(offsetVector.begin(), offsetVector.end(), absolute_address);
        patch = memoryPatches[std::distance(offsetVector.begin(), itr)]; // Get index of memoryPatches vector
    } else {
        memoryPatches.push_back(patch);
        offsetVector.push_back(absolute_address);
        // LOGI(OBFUSCATE("Added"));
    }

    if (!patch.isValid()) {
        LOGE(OBFUSCATE("Failing offset: 0x%llu, please re-check the hex"), absolute_address);
        return;
    }
    if (isOn) {
        if (!patch.Modify()) {
            LOGE(OBFUSCATE("Something went wrong while patching this offset: 0x%llu"), absolute_address);
        }
    } else {
        if (!patch.Restore()) {
            LOGE(OBFUSCATE("Something went wrong while restoring this offset: 0x%llu"), absolute_address);
        }
    }
}

#define PATCH(offset, hex) patchOffset(targetLibName, offset, OBFUSCATE(hex), true)
#define PATCH_LIB(lib, offset, hex) patchOffset((lib), offset, OBFUSCATE(hex), true)

#define PATCH_SYM(sym, hex) patchOffset(dlsym(dlopen(targetLibName, 4), OBFUSCATE(sym)), OBFUSCATE(hex), true)
#define PATCH_LIB_SYM(lib, sym, hex) patchOffset(dlsym(dlopen(lib, 4), OBFUSCATE(sym)), OBFUSCATE(hex), true)

#define PATCH_SWITCH(offset, hex, boolean) patchOffset(targetLibName, offset, OBFUSCATE(hex), boolean)
#define PATCH_LIB_SWITCH(lib, offset, hex, boolean) patchOffset((lib), offset, OBFUSCATE(hex), boolean)

#define PATCH_SYM_SWITCH(sym, hex, boolean) patchOffsetSym((uintptr_t)dlsym(dlopen(targetLibName, 4), OBFUSCATE(sym)), OBFUSCATE(hex), boolean)
#define PATCH_LIB_SYM_SWITCH(lib, sym, hex, boolean) patchOffsetSym((uintptr_t)dlsym(dlopen(lib, 4), OBFUSCATE(sym)), OBFUSCATE(hex), boolean)

#define RESTORE(offset) patchOffset(targetLibName, offset, "", false)
#define RESTORE_LIB(lib, offset) patchOffset(OBFUSCATE(lib), offset, "", false)

#define RESTORE_SYM(sym) patchOffsetSym((uintptr_t)dlsym(dlopen(targetLibName, 4), OBFUSCATE(sym)), "", false)
#define RESTORE_LIB_SYM(lib, sym) patchOffsetSym((uintptr_t)dlsym(dlopen(lib, 4), OBFUSCATE(sym)), "", false)

#define HOOKAU(dll, ns, claz, method, args, org, rep) do { \
    void *_addr = (void *)(uintptr_t)GetMethodOffset(oxorany(dll), oxorany(ns), oxorany(claz), oxorany(method), args); \
    if (_addr) Tools::Hook(_addr, (void *)org, (void **)&rep); \
    else __android_log_print(ANDROID_LOG_WARN, "HOOKAU", "SKIP: %s.%s::%s not found", dll, claz, method); \
} while(0)

#define HOOKAU_NO_ORIG(dll, ns, claz, method, args, org, rep) do { \
    void *_addr = (void *)(uintptr_t)GetMethodOffset(oxorany(dll), oxorany(ns), oxorany(claz), oxorany(method), args); \
    if (_addr) Tools::Hook(_addr, (void *)org, NULL); \
} while(0)

#define PATCHAU(dll, ns, claz, method, args, hex) do { \
    void *_addr = (void *)(uintptr_t)GetMethodOffset(OBFUSCATE(dll), OBFUSCATE(ns), OBFUSCATE(claz), OBFUSCATE(method), args); \
    if (_addr) MemoryPatch::createWithHex((uintptr_t)_addr, OBFUSCATE(hex)).Modify(); \
} while(0)

#endif //ANDROID_MOD_MENU_MACROS_H
