#pragma once

class PackageRef {
public:
	const char* GetName();
	short GetResourceId();
	void SetName(const char* name);
};