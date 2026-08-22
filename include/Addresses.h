#pragma once

// My pretty hook targets
namespace Addresses {
	extern void* CharacterManagerCtor;
	extern void* ScarfaceAlloc;
	extern void* CM_SetMainCharacterPackage;
	extern void* FindPackagePath;
	extern void* CM_GetMainCharacterPackage;
	extern void* RenderGame;
	bool Initialize();
}