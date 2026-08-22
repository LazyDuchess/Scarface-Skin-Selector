#pragma once
#include "sf/packageref.h"

class CharacterManager {
public:
	PackageRef** GetMainCharacterPackage();
	void SetMainCharacterPackage(char* packageName, bool unk);
};