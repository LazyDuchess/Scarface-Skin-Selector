#pragma once
#include "sf/packageref.h"

class CharacterManager {
public:
	PackageRef** GetMainCharacterPackageRef();
	void SetMainCharacterPackage(const char* packageName, bool unk);
};