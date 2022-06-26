#include <Windows.h>
#include <stdio.h>

void patch_bytes(void* dst, void* src, size_t len) {
    DWORD prot;
    VirtualProtect(dst, len, PAGE_READWRITE, &prot);
    memcpy(dst, src, len);
    VirtualProtect(dst, len, prot, &prot);
}

void patch_call(void* target, void* func) {
    BYTE patch[5];
    patch[0] = 0xE8;
    *(DWORD*)(patch + 1) = (DWORD)func - (DWORD)target - 5;
    patch_bytes(target, patch, sizeof(patch));
}

bool iat_hook(LPCWSTR target, LPCSTR dll, LPCSTR func, void* hook) {
    // Get a pointer to the module
    auto pe = (char*)GetModuleHandleW(target);
    if (!pe)
        return false;

    // Get the PE optional header
    auto nt_header = (PIMAGE_NT_HEADERS32)(pe + ((PIMAGE_DOS_HEADER)pe)->e_lfanew);
    auto optional_header = (PIMAGE_OPTIONAL_HEADER32)(&nt_header->OptionalHeader);

    // Find the dll's import descriptor
    bool found = false;
    auto import_desc = (PIMAGE_IMPORT_DESCRIPTOR)(pe + optional_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    for (; import_desc->Name; import_desc++) {
        if (!_stricmp(pe + import_desc->Name, dll)) {
            found = true;
            break;
        }
    }
    if (!found)
        return false;

    // Get the target function's pointer
    auto dll_handle = GetModuleHandleA(dll);
    if (!dll_handle)
        return false;
    auto target_ptr = (DWORD)GetProcAddress(dll_handle, func);
    if (!target_ptr)
        return false;

    // Find and replace the target function with the hook
    auto thunk = (PIMAGE_THUNK_DATA)(pe + import_desc->FirstThunk);
    for (; thunk->u1.Function != NULL; thunk++) {
        if (thunk->u1.Function == target_ptr) {
            printf("Overriding function %s from %s in %S\n", func, dll, target ? target : L"main executable");
            patch_bytes(&thunk->u1.Function, &hook, sizeof(hook));
        }
    }

    return true;
}