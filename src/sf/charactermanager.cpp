#include "sf/charactermanager.h"
#include "Addresses.h"

PackageRef** CharacterManager::GetMainCharacterPackageRef() {
	return *(PackageRef***)((char*)this + 0x18);
}

void CharacterManager::SetMainCharacterPackage(const char* packageName, bool unk) {
	((void* (__thiscall*)(CharacterManager*,const char*,bool))Addresses::CM_SetMainCharacterPackage)(this, packageName, unk);
}