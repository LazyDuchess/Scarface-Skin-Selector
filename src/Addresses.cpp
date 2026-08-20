#include "Addresses.h"
#include "Windows.h"
#include "Psapi.h"
#include "scan.h"
#include <iostream>

#define ADDRESS(name, lookup) \
name = ScanInternal(lookup, lookup##Mask, modBase, size);\
if (name == nullptr) {\
	return false;\
}\
else\
{\
	printf("Found %s at %p\n", #name, name);\
}\

namespace Addresses {
	void* CharacterManagerCtor;
	static char CharacterManagerCtorLookup[] = {
		0x6A, 0xFF,
		0x68, 0x3A, 0x01, 0x71, 0x00,
		0x64, 0xA1, 0x00, 0x00, 0x00, 0x00,
		0x50,
		0x64, 0x89, 0x25, 0x00, 0x00, 0x00, 0x00,
		0x51,
		0x53,
		0x56,
		0x8B, 0xF1,
		0x89, 0x74, 0x24, 0x08,
		0xC7, 0x46, 0x04, 0x78, 0xBE, 0x73, 0x00,
		0x6A, 0x40
	};
	static char CharacterManagerCtorLookupMask[] = "xxx????xxxxxxxxxxxxxxxxxxxxxxxxxx????xx";

	void* ScarfaceAlloc;
	static char ScarfaceAllocLookup[] = {
		0x53,
		0x8B, 0x5C, 0x24, 0x0C,
		0x56,
		0x57,
		0x8B, 0x7C, 0x24, 0x10,
		0x8B, 0xC7,
		0xC1, 0xE0, 0x04,
		0x8B
	};
	static char ScarfaceAllocLookupMask[] = "xxxxxxxxxxxxxxxxx";

	bool Initialize() {
		HMODULE module = GetModuleHandleA(NULL);
		char* modBase = (char*)module;
		HANDLE proc = GetCurrentProcess();
		MODULEINFO modInfo;
		GetModuleInformation(proc, module, &modInfo, sizeof(MODULEINFO));
		int size = modInfo.SizeOfImage;
		ADDRESS(CharacterManagerCtor, CharacterManagerCtorLookup);
		ADDRESS(ScarfaceAlloc, ScarfaceAllocLookup);
		return true;
	}
}