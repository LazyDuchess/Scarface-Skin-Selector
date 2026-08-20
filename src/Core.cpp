#include <Windows.h>
#include "Core.h"
#include "res/loadgroup.h"
#include <memory>
#include <iostream>
#include <filesystem>
#include "MinHook.h"
#include <unordered_map>
#include <algorithm>
#include "Addresses.h"
#include "sf/engine.h"

static std::wstring loadGroupPath;
static std::wstring originalLoadGroupPath;
static int timesCalled = 0;
static int skinCount = 0;

typedef HANDLE(__stdcall* CREATEFILEW)(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
typedef void* (__thiscall* CHARACTERMANAGERCTOR)(void* self);
typedef void* (__cdecl* SFALLOC)(size_t size, int type);
static CREATEFILEW fpCreateFileW = NULL;
static CHARACTERMANAGERCTOR fpCharacterManagerCtor = NULL;
static SFALLOC fpSF_Alloc = NULL;

static bool doAllocHook = false;

static void* __cdecl SF_Alloc_Hook(size_t size, int type) {
	if (doAllocHook) {
		doAllocHook = false;
		size = skinCount * 4;
		return malloc(size);
	}
	return fpSF_Alloc(size, type);
}

static void* __fastcall CharacterManagerCtor_Hook(void* self, void* _) {
	doAllocHook = true;
	void* ret = fpCharacterManagerCtor(self);
	size_t skinAllocSize = skinCount * 4;
	void* skinAlloc = (*(void**)((DWORD)ret + 0xC));
	(*(void**)((DWORD)ret + 0x10)) = (void*)((DWORD)skinAlloc + skinAllocSize);
	return ret;
}

// Hook whenever the game tries to call the vanilla loadgroups
static HANDLE __stdcall CreateFileW_Hook(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
	if (timesCalled < 2) {
		if (dwDesiredAccess == GENERIC_READ && dwCreationDisposition == OPEN_EXISTING && wcscmp(lpFileName, originalLoadGroupPath.c_str()) == 0)
		{
			timesCalled++;
			return fpCreateFileW(loadGroupPath.c_str(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
		}
	}
	return fpCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

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
	originalLoadGroupPath = gameDir + L"\\art\\characters\\pc\\pc_loadgroup.p3d";

	if (!Exists(skinsDir)) return;

	std::unordered_map<std::string, LoadObject> vanillaSkins = {
		{"MCP_ArmyTony", LoadObject("MCP_ArmyTony", "packages/characters/pc/MCP_ArmyTony.p3d")},
		{"MCP_Assassin", LoadObject("MCP_Assassin", "packages/characters/pc/MCP_Assassin.p3d")},
		{"MCP_BlackSuitTony", LoadObject("MCP_BlackSuitTony", "packages/characters/pc/MCP_BlackSuitTony.p3d")},
		{"MCP_BluePinSuitShadesTony", LoadObject("MCP_BluePinSuitShadesTony", "packages/characters/pc/MCP_BluePinSuitShadesTony.p3d")},
		{"MCP_BluePinSuitTony", LoadObject("MCP_BluePinSuitTony", "packages/characters/pc/MCP_BluePinSuitTony.p3d")},
		{"MCP_BlueSuitTony", LoadObject("MCP_BlueSuitTony", "packages/characters/pc/MCP_BlueSuitTony.p3d")},
		{"MCP_Driver", LoadObject("MCP_Driver", "packages/characters/pc/MCP_Driver.p3d")},
		{"MCP_Enforcer", LoadObject("MCP_Enforcer", "packages/characters/pc/MCP_Enforcer.p3d")},
		{"MCP_GraySuitShadesTony", LoadObject("MCP_GraySuitShadesTony", "packages/characters/pc/MCP_GraySuitShadesTony.p3d")},
		{"MCP_GraySuitTony", LoadObject("MCP_GraySuitTony", "packages/characters/pc/MCP_GraySuitTony.p3d")},
		{"MCP_HawaiianShadesTony", LoadObject("MCP_HawaiianShadesTony", "packages/characters/pc/MCP_HawaiianShadesTony.p3d")},
		{"MCP_HawaiianTony", LoadObject("MCP_HawaiianTony", "packages/characters/pc/MCP_HawaiianTony.p3d")},
		{"MCP_SandyShadesTony", LoadObject("MCP_SandyShadesTony", "packages/characters/pc/MCP_SandyShadesTony.p3d")},
		{"MCP_SandyTony", LoadObject("MCP_SandyTony", "packages/characters/pc/MCP_SandyTony.p3d")},
		{"MCP_WhiteSuitTony", LoadObject("MCP_WhiteSuitTony", "packages/characters/pc/MCP_WhiteSuitTony.p3d")},
		{"MCP_WhiteSuitShadesTony", LoadObject("MCP_WhiteSuitShadesTony", "packages/characters/pc/MCP_WhiteSuitShadesTony.p3d")}
	};

	std::filesystem::path skinPath = std::filesystem::path(skinsDir);
	for (const auto& entry : std::filesystem::recursive_directory_iterator(skinPath)) {
		if (entry.is_regular_file()) {
			std::wstring fext = entry.path().extension().wstring();
			std::transform(fext.begin(), fext.end(), fext.begin(), ::tolower);
			if (fext == L".p3d")
			{
				std::string u8name = entry.path().stem().u8string();
				std::string u8Path = std::filesystem::relative(entry.path(), std::filesystem::path(gameDir)).u8string();
				std::replace(u8Path.begin(), u8Path.end(), '\\', '/');
				if (vanillaSkins.find(u8name) != vanillaSkins.end()) {
					vanillaSkins.erase(u8name);
				}
				loadGroup->m_LoadObjects.emplace_back(u8name, u8Path);
			}
		}
	}

	for (const auto& [name, loadObject] : vanillaSkins) {
		loadGroup->m_LoadObjects.push_back(loadObject);
	}
	skinCount = loadGroup->m_LoadObjects.size();
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
	CacheLoadGroup();
	if (!Addresses::Initialize())
	{
		return;
	}
	if (MH_Initialize() != MH_OK)
		return;
	HMODULE kernelBase = GetModuleHandle("KERNELBASE.DLL");
	FARPROC fileCreateProc = GetProcAddress(kernelBase, "CreateFileW");
	if (MH_CreateHook(fileCreateProc, &CreateFileW_Hook,
		reinterpret_cast<LPVOID*>(&fpCreateFileW)) != MH_OK)
	{
		return;
	}
	if (MH_EnableHook(fileCreateProc) != MH_OK)
	{
		return;
	}
	if (MH_CreateHook(Addresses::CharacterManagerCtor, &CharacterManagerCtor_Hook,
		reinterpret_cast<LPVOID*>(&fpCharacterManagerCtor)) != MH_OK)
	{
		return;
	}
	if (MH_EnableHook(Addresses::CharacterManagerCtor) != MH_OK)
	{
		return;
	}
	if (MH_CreateHook(Addresses::ScarfaceAlloc, &SF_Alloc_Hook,
		reinterpret_cast<LPVOID*>(&fpSF_Alloc)) != MH_OK)
	{
		return;
	}
	if (MH_EnableHook(Addresses::ScarfaceAlloc) != MH_OK)
	{
		return;
	}
	printf("Skin Selector Initialized!\n");
	printf("%i Skins loaded.\n", skinCount);
}