#pragma once

// My pretty hook targets
namespace Addresses {
	extern void* CharacterManagerCtor;
	extern void* ScarfaceAlloc;
	extern void* CM_SetMainCharacterPackage;
	extern void* FindPackagePath;
	bool Initialize();
}