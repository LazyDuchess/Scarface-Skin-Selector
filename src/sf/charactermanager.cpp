#include "sf/charactermanager.h"

PackageRef** CharacterManager::GetMainCharacterPackage() {
	return *(PackageRef***)((char*)this + 0x18);
}