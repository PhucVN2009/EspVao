#pragma once
#include <dlfcn.h>
#include <android/log.h>
#include <stdint.h>
#include "TuanMeta/Tools/Dobby/dobby.h"

// Bypass IL2CPP's g_unmap_open_flag anti-tamper mechanism.
//
// When enable_unmap_file_flag is set server-side, IL2CPP calls
// UnmapOpenFlagRuntime() during startup which sets g_unmap_open_flag = 1.
// With the flag set, IL2CPP refuses to read DLC asset bundles, causing
// "Unable to read header from archive file: dlcconfig.assetbundle" and a
// subsequent game exit.  We must zero the flag after it is set and suppress
// any future calls.

namespace AntiTamper {

static void (*orig_UnmapOpenFlagRuntime)() = nullptr;
static void noop_UnmapOpenFlagRuntime() { /* intentional no-op */ }

// Scan ARM64 instructions starting at fn_ptr for the first ADRP+STR/STRB
// pair (the canonical way the compiler stores an int/bool global).  Returns
// the absolute address of the global, or 0 when not found.
static uintptr_t find_stored_global(void *fn_ptr) {
    if (!fn_ptr) return 0;
    auto *code = reinterpret_cast<const uint32_t *>(fn_ptr);

    int  adrp_rd      = -1;
    uint64_t adrp_page = 0;
    int  full_rd      = -1;   // register after ADRP+ADD
    uint64_t full_addr = 0;

    for (int i = 0; i < 80; i++) {
        uint32_t instr = code[i];

        // ── ADRP Xd, #imm ──────────────────────────────────────────────
        if ((instr & 0x9F000000) == 0x90000000) {
            adrp_rd = (int)(instr & 0x1F);
            uint64_t pc = reinterpret_cast<uint64_t>(&code[i]);
            int64_t immhi = static_cast<int64_t>((instr >> 5)  & 0x7FFFF);
            int64_t immlo = static_cast<int64_t>((instr >> 29) & 0x3);
            int64_t imm   = (immhi << 2) | immlo;
            if (imm & (1LL << 20)) imm |= ~((1LL << 21) - 1); // sign-extend
            adrp_page = static_cast<uint64_t>(
                static_cast<int64_t>(pc & ~0xFFFULL) + (imm << 12));
            full_rd = -1;
            continue;
        }

        // ── ADD Xd, Xn, #imm12  (resolves full address from ADRP page) ─
        if (adrp_rd >= 0 && (instr & 0xFFC00000) == 0x91000000) {
            int rn = (int)((instr >> 5) & 0x1F);
            if (rn == adrp_rd) {
                full_rd   = (int)(instr & 0x1F);
                full_addr = adrp_page + static_cast<uint64_t>((instr >> 10) & 0xFFF);
            }
            continue;
        }

        // ── STR Wt, [Xn, #imm12<<2]  (32-bit store) ────────────────────
        if ((instr & 0xFFC00000) == 0xB9000000) {
            int rn = (int)((instr >> 5) & 0x1F);
            if (adrp_rd >= 0 && rn == adrp_rd)
                return static_cast<uintptr_t>(
                    adrp_page + (static_cast<uint64_t>((instr >> 10) & 0xFFF) << 2));
            if (full_rd >= 0 && rn == full_rd)
                return static_cast<uintptr_t>(
                    full_addr + (static_cast<uint64_t>((instr >> 10) & 0xFFF) << 2));
        }

        // ── STRB Wt, [Xn, #imm12]  (byte store) ────────────────────────
        if ((instr & 0xFFC00000) == 0x39000000) {
            int rn = (int)((instr >> 5) & 0x1F);
            if (adrp_rd >= 0 && rn == adrp_rd)
                return static_cast<uintptr_t>(
                    adrp_page + static_cast<uint64_t>((instr >> 10) & 0xFFF));
            if (full_rd >= 0 && rn == full_rd)
                return static_cast<uintptr_t>(
                    full_addr + static_cast<uint64_t>((instr >> 10) & 0xFFF));
        }

        // ── STR Xt, [Xn, #imm12<<3]  (64-bit, cover all cases) ─────────
        if ((instr & 0xFFC00000) == 0xF9000000) {
            int rn = (int)((instr >> 5) & 0x1F);
            if (adrp_rd >= 0 && rn == adrp_rd)
                return static_cast<uintptr_t>(
                    adrp_page + (static_cast<uint64_t>((instr >> 10) & 0xFFF) << 3));
            if (full_rd >= 0 && rn == full_rd)
                return static_cast<uintptr_t>(
                    full_addr + (static_cast<uint64_t>((instr >> 10) & 0xFFF) << 3));
        }

        // BL resets tracking because callee may clobber the register
        if ((instr & 0xFC000000) == 0x94000000) {
            adrp_rd = -1;
            full_rd = -1;
        }
    }
    return 0;
}

// g_flag_addr is cached so Install() can be called again (from Init_Thread)
// to re-zero the flag without re-hooking.
static uintptr_t s_flag_addr = 0;

inline void Install() {
    // libil2cpp.so is already loaded (its JNI_OnLoad ran before ours),
    // so dlsym(RTLD_DEFAULT) will find the symbol if it is exported.
    void *fn = dlsym(RTLD_DEFAULT, "UnmapOpenFlagRuntime");
    if (!fn) {
        // RTLD_NOLOAD (flag 4) returns a handle only if already loaded.
        void *h = dlopen("libil2cpp.so", 4);
        if (h) fn = dlsym(h, "UnmapOpenFlagRuntime");
    }

    if (!fn) {
        __android_log_print(ANDROID_LOG_WARN, "BYPASS",
            "UnmapOpenFlagRuntime not found — bypass skipped");
        return;
    }

    // Find and zero g_unmap_open_flag
    if (!s_flag_addr) s_flag_addr = find_stored_global(fn);

    if (s_flag_addr) {
        *reinterpret_cast<volatile uint32_t *>(s_flag_addr) = 0;
        __android_log_print(ANDROID_LOG_INFO, "BYPASS",
            "g_unmap_open_flag @ 0x%lx zeroed", static_cast<unsigned long>(s_flag_addr));
    } else {
        __android_log_print(ANDROID_LOG_WARN, "BYPASS",
            "g_unmap_open_flag address not found via ARM64 scan");
    }

    // Hook only once; DobbyHook on an already-hooked address is a no-op or error.
    if (!orig_UnmapOpenFlagRuntime) {
        DobbyHook(fn,
                  reinterpret_cast<void *>(noop_UnmapOpenFlagRuntime),
                  reinterpret_cast<void **>(&orig_UnmapOpenFlagRuntime));
        __android_log_print(ANDROID_LOG_INFO, "BYPASS",
            "UnmapOpenFlagRuntime hooked @ %p", fn);
    }
}

// Call this periodically (e.g. from Init_Thread) to ensure the flag stays 0
// in case the game re-invokes the function through an unhooked path.
inline void EnsureFlagZero() {
    if (s_flag_addr)
        *reinterpret_cast<volatile uint32_t *>(s_flag_addr) = 0;
}

} // namespace AntiTamper
