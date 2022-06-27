// Touhou 10: Mountain of Faith v1.00a

#include "patch_util.h"
#include "limiter.h"
#include "d3d9_hook.h"
#include "touhou10.h"

int __stdcall update_window_hook(void* self) {
	limiter_tick();
	return CWindowManager__Update(self);
}

void th10_install_patches() {
	{
		// Skip the original frame limiter
		BYTE patch[] = { 0xEB };
		patch_bytes((void*)0x004393C5, patch, sizeof(patch));
	}
	{
		// Don't sleep in the main loop
		BYTE patch[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
		patch_bytes((void*)0x004393B7, patch, sizeof(patch));
	}
	{
		// Hook window update
		patch_call((void*)0x00438D31, update_window_hook);
	}
	{
		// Redirect Direct3DCreate9 call
		// IAT is being hooked, but thcrap also hooks Direct3DCreate9 in the same place
		// This should force our Direct3DCreate9 hook to be loaded no matter what
		patch_call((void*)0x00438BC3, Direct3DCreate9_hook);
	}
}