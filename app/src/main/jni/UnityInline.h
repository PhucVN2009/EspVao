#pragma once
// UnityInline.h — Auto-update bridge using UnitySDK.h
// UnitySDK.h provides a standalone implementation that finds libil2cpp.so
// via dl_iterate_phdr, parses ELF sections and global-metadata.dat directly,
// without requiring the IL2CPP runtime API (no dlopen/dlsym needed).

#include "KittyMemory/KittyMemory.h"
#include "UnitySDK.h"
