#include <Windows.h>
#include "Core.h"
#include <memory>
#include <iostream>
#include <filesystem>
#include "MinHook.h"
#include <unordered_map>
#include <algorithm>
#include "Addresses.h"
#include "sf/engine.h"
#include "scan.h"
#include "sf/packageref.h"

typedef void* (__thiscall* CHARACTERMANAGERCTOR)(void* self);
typedef void* (__cdecl* SFALLOC)(size_t size, int type);
typedef const char* (__cdecl* FINDPACKAGEPATH)(short resourceId);
typedef void* (__thiscall* SETMAINCHARACTERPACKAGE)(void* self, char* name, bool unk);

static CHARACTERMANAGERCTOR fpCharacterManagerCtor = NULL;
static SFALLOC fpSF_Alloc = NULL;
static FINDPACKAGEPATH fpFindPackagePath = NULL;
static SETMAINCHARACTERPACKAGE fpSetMainCharacterPackage = NULL;

static bool doAllocHook = false;

static void* playerModelPool;
static void* characterManagerInstance;

static std::string currentPlayerModel = "";

static std::unordered_map<std::string, std::string> skins = {
		{"MCP_ArmyTony", "packages/characters/pc/MCP_ArmyTony.p3d"},
		{"MCP_Assassin", "packages/characters/pc/MCP_Assassin.p3d"},
		{"MCP_BlackSuitTony", "packages/characters/pc/MCP_BlackSuitTony.p3d"},
		{"MCP_BluePinSuitShadesTony", "packages/characters/pc/MCP_BluePinSuitShadesTony.p3d"},
		{"MCP_BluePinSuitTony", "packages/characters/pc/MCP_BluePinSuitTony.p3d"},
		{"MCP_BlueSuitTony", "packages/characters/pc/MCP_BlueSuitTony.p3d"},
		{"MCP_Driver", "packages/characters/pc/MCP_Driver.p3d"},
		{"MCP_Enforcer", "packages/characters/pc/MCP_Enforcer.p3d"},
		{"MCP_GraySuitShadesTony", "packages/characters/pc/MCP_GraySuitShadesTony.p3d"},
		{"MCP_GraySuitTony", "packages/characters/pc/MCP_GraySuitTony.p3d"},
		{"MCP_HawaiianShadesTony", "packages/characters/pc/MCP_HawaiianShadesTony.p3d"},
		{"MCP_HawaiianTony", "packages/characters/pc/MCP_HawaiianTony.p3d"},
		{"MCP_SandyShadesTony", "packages/characters/pc/MCP_SandyShadesTony.p3d"},
		{"MCP_SandyTony", "packages/characters/pc/MCP_SandyTony.p3d"},
		{"MCP_WhiteSuitTony", "packages/characters/pc/MCP_WhiteSuitTony.p3d"},
		{"MCP_WhiteSuitShadesTony", "packages/characters/pc/MCP_WhiteSuitShadesTony.p3d"}
};

static PackageRef* GetDummyPackageRef() {
	if (playerModelPool == nullptr)
		return nullptr;
	return **(PackageRef***)(playerModelPool);
}

static void __fastcall SetMainCharacterPackage_Hook(void* self, void* _, char* name, bool unk) {
	if (currentPlayerModel != "")
	{
		if (name == currentPlayerModel) {
			return;
		}
	}
	if (skins.find(name) == skins.end()) {
		return;
	}
	PackageRef* dummyChar = GetDummyPackageRef();
	currentPlayerModel = name;
	dummyChar->SetName(currentPlayerModel.c_str());
	fpSetMainCharacterPackage(self, name, unk);
}

static const char* __cdecl FindPackagePath_Hook(short resourceId) {
	const char* path = fpFindPackagePath(resourceId);
	if (currentPlayerModel == "")
		return path;
	PackageRef* dummyChar = GetDummyPackageRef();
	if (dummyChar != nullptr) {
		if (resourceId == dummyChar->GetResourceId()) {
			return skins[currentPlayerModel].c_str();
		}
	}
	return path;
}

static void* __cdecl SF_Alloc_Hook(size_t size, int type) {
	void* ret = fpSF_Alloc(size, type);
	if (doAllocHook) {
		doAllocHook = false;
		playerModelPool = ret;
	}
	return ret;
}

static void* __fastcall CharacterManagerCtor_Hook(void* self, void* _) {
	characterManagerInstance = self;
	doAllocHook = true;
	return fpCharacterManagerCtor(self);
}

static bool Exists(std::wstring dir) {
	return std::filesystem::exists(std::filesystem::path(dir));
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

static void CacheSkins() {
	std::wstring gameDir = GetGameDirectory();
	std::wstring skinsDir = gameDir + L"\\skins";

	if (!Exists(skinsDir)) return;

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
				skins[u8name] = u8Path;
			}
		}
	}
}

void Core::Initialize() {
	CacheSkins();
	if (!Addresses::Initialize())
	{
		return;
	}
	if (MH_Initialize() != MH_OK)
		return;
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
	if (MH_CreateHook(Addresses::FindPackagePath, &FindPackagePath_Hook,
		reinterpret_cast<LPVOID*>(&fpFindPackagePath)) != MH_OK)
	{
		return;
	}
	if (MH_EnableHook(Addresses::FindPackagePath) != MH_OK)
	{
		return;
	}
	if (MH_CreateHook(Addresses::CM_SetMainCharacterPackage, &SetMainCharacterPackage_Hook,
		reinterpret_cast<LPVOID*>(&fpSetMainCharacterPackage)) != MH_OK)
	{
		return;
	}
	if (MH_EnableHook(Addresses::CM_SetMainCharacterPackage) != MH_OK)
	{
		return;
	}
	// Forces SetMainCharacterPackage to reload even if same character address selected.
	Nop((BYTE*)((DWORD)Addresses::CM_SetMainCharacterPackage + 0x63), 6);
	printf("Skin Selector Initialized!\n");
	printf("%i Skins loaded.\n", skins.size());
}