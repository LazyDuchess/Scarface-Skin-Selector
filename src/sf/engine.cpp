#include "sf/engine.h"
#include "Addresses.h"

void* SF_Alloc(size_t size, int type) {
	return ((void*(__cdecl*)(size_t, int))Addresses::ScarfaceAlloc)(size, type);
}