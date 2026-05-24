#pragma once

monoString *CreateMonoString(const char *str) {
    monoString *(*String_CreateString)(void *instance, const char *str) = (monoString *(*)(void *, const char *)) (uintptr_t) GetMethodOffset(OBFUSCATE("mscorlib.dll"), OBFUSCATE("System"), OBFUSCATE("String"), OBFUSCATE("CreateString"), 1);
    return String_CreateString(NULL, str);
}