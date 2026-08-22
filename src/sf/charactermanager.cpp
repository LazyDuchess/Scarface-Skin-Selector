#include "sf/charactermanager.h"
#include "Addresses.h"

PackageRef** CharacterManager::GetMainCharacterPackage() {
	return *(PackageRef***)((char*)this + 0x18);
}

void CharacterManager::SetMainCharacterPackage(char* packageName, bool unk) {
	return ((void* (__thiscall*)(CharacterManager*,char*,bool))Addresses::CM_SetMainCharacterPackage)(this, packageName, unk);
}