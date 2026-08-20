#pragma once
#include <vector>
#include <string>
#include <memory>

class LoadObject {
public:
	LoadObject(const std::string& name, const std::string& path);
	void Write(FILE* file);
private:
	std::string m_Name;
	std::string m_Path;
};

class LoadGroup {
public:
	void Write(FILE* file);
private:
	std::vector<std::unique_ptr<LoadObject>> m_LoadObjects;
};