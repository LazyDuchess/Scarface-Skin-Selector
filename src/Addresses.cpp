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

	void* CM_SetMainCharacterPackage;
	static char CM_SetMainCharacterPackageLookup[] = {
		0x51,
		0x57,
		0x8B, 0x7C, 0x24, 0x0C,
		0x85, 0xFF,
		0x89, 0x4C, 0x24, 0x04,
		0x0F, 0x84, 0xE3, 0x00, 0x00, 0x00,
		0x80, 0x3F, 0x00,
		0x0F, 0x84, 0xDA, 0x00, 0x00, 0x00,
		0x55,
		0x8B, 0x69, 0x0C,
		0x56
	};
	static char CM_SetMainCharacterPackageLookupMask[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

	void* FindPackagePath;
	static char FindPackagePathLookup[] = {
		0x8B, 0x44, 0x24, 0x04,
		0x66, 0x3D, 0xFF, 0xFF,
		0x75, 0x03,
		0x33, 0xC0,
		0xC3,
		0x84, 0xE4,
		0x56,
		0x57
	};
	static char FindPackagePathLookupMask[] = "xxxxxxxxxxxxxxxxx";

	void* CM_GetMainCharacterPackage;
	static char CM_GetMainCharacterPackageLookup[] = {
		0x8B, 0x41, 0x18,
		0x85, 0xC0,
		0x74, 0x05,
		0x8B, 0x00,
		0x8B, 0x00,
		0xC3,
		0x33, 0xC0,
		0xC3
	};
	static char CM_GetMainCharacterPackageLookupMask[] = "xxxxxxxxxxxxxxx";

	bool Initialize() {
		HMODULE module = GetModuleHandleA(NULL);
		char* modBase = (char*)module;
		HANDLE proc = GetCurrentProcess();
		MODULEINFO modInfo;
		GetModuleInformation(proc, module, &modInfo, sizeof(MODULEINFO));
		int size = modInfo.SizeOfImage;
		ADDRESS(CharacterManagerCtor, CharacterManagerCtorLookup);
		ADDRESS(ScarfaceAlloc, ScarfaceAllocLookup);
		ADDRESS(CM_SetMainCharacterPackage, CM_SetMainCharacterPackageLookup);
		ADDRESS(FindPackagePath, FindPackagePathLookup);
		ADDRESS(CM_GetMainCharacterPackage, CM_GetMainCharacterPackageLookup);
		return true;
	}
}