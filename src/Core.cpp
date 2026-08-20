#include <Windows.h>
#include "Core.h"
#include "res/loadgroup.h"
#include <memory>
#include <iostream>

void Core::Initialize() {
	printf("Skin Selector Core Initializing...\n");
	std::unique_ptr<LoadGroup> loadGroup = std::make_unique<LoadGroup>();
}