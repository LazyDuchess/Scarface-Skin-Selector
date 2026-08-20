#pragma once
#include <vector>
#include <string>

// Represents a player model package.
class LoadObject {
public:
	LoadObject(const std::string& name, const std::string& path);
	void Write(FILE* file);
private:
	std::string m_Name;
	std::string m_Path;
};

// Represents a load group for player models.
class LoadGroup {
public:
	void Write(FILE* file);
	std::vector<LoadObject> m_LoadObjects;
};