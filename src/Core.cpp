#include <Windows.h>
#include "Core.h"
#include "res/loadgroup.h"
#include <memory>
#include <iostream>
#include <filesystem>

static std::wstring loadGroupPath;

static bool Exists(std::wstring dir) {
	return std::filesystem::exists(std::filesystem::path(dir));
}

static void EnsureDirectory(std::wstring dir) {
	std::filesystem::create_directories(std::filesystem::path(dir));
}

static std::wstring GetGameDirectory() {
	wchar_t path[MAX_PATH];
	if (GetModuleFileNameW(NULL, path, MAX_PATH)) {
		std::wstring dir(path);
		size_t pos = dir.find_last_of(L"\\/");
		if (pos != std::wstring::npos) {
			return dir.substr(0, pos);
		}
	}
	return L"";
}

static void CacheLoadGroup() {
	std::unique_ptr<LoadGroup> loadGroup = std::make_unique<LoadGroup>();
	std::wstring gameDir = GetGameDirectory();

	std::wstring tempDir = gameDir + L"\\temp";
	std::wstring skinsDir = gameDir + L"\\skins";
	loadGroupPath = tempDir + L"\\skinselector_cache.p3d";

	if (!Exists(skinsDir)) return;

	EnsureDirectory(tempDir);
	FILE* cacheFile = _wfopen(loadGroupPath.c_str(), L"wb");
	if (cacheFile) {
		loadGroup->Write(cacheFile);
		fclose(cacheFile);
	}
	else
	{
		return;
	}
}

void Core::Initialize() {
	printf("Skin Selector Core Initializing...\n");
	CacheLoadGroup();
}