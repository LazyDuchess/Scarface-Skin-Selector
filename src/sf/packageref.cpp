#include "sf/packageref.h"

const char* PackageRef::GetName() {
	return *(const char**)this;
}

short PackageRef::GetResourceId() {
	return *(short*)((char*)this + 0x14);
}

void PackageRef::SetName(const char* name) {
	*(const char**)this = name;
}