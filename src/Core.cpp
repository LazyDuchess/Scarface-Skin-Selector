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
#include "ini.h"
#include "sf/charactermanager.h"
#include "sf/notificationmanager.h"

typedef void* (__thiscall* CHARACTERMANAGERCTOR)(CharacterManager* self);
typedef void* (__cdecl* SFALLOC)(size_t size, int type);
typedef const char* (__cdecl* FINDPACKAGEPATH)(short resourceId);
typedef void* (__thiscall* SETMAINCHARACTERPACKAGE)(CharacterManager* self, const char* name, bool unk);
typedef const char* (__thiscall* GETMAINCHARACTERPACKAGE)(CharacterManager* self);
typedef void* (__thiscall* RENDERGAME)(void* self);
typedef void* (__thiscall* SHOWNOTIFICATION)(NotificationManager* self, int type, const char* str);

static CHARACTERMANAGERCTOR fpCharacterManagerCtor = NULL;
static SFALLOC fpSF_Alloc = NULL;
static FINDPACKAGEPATH fpFindPackagePath = NULL;
static SETMAINCHARACTERPACKAGE fpSetMainCharacterPackage = NULL;
static GETMAINCHARACTERPACKAGE fpGetMainCharacterPackage = NULL;
static RENDERGAME fpRenderGame = NULL;
static SHOWNOTIFICATION fpShowNotification = NULL;

static bool doAllocHook = false;

static void* playerModelPool;
static CharacterManager* characterManagerInstance;
static NotificationManager* notificationManagerInstance;

static std::string currentPlayerModel = "";
static std::string currentPlayerModelInternalName = "";

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

static std::vector<std::string> vanillaSkins = {
	"MCP_ArmyTony",
	"MCP_Assassin",
	"MCP_BlackSuitTony",
	"MCP_BluePinSuitShadesTony",
	"MCP_BluePinSuitTony",
	"MCP_BlueSuitTony",
	"MCP_Driver",
	"MCP_Enforcer",
	"MCP_GraySuitShadesTony",
	"MCP_GraySuitTony",
	"MCP_HawaiianShadesTony",
	"MCP_HawaiianTony",
	"MCP_SandyShadesTony",
	"MCP_SandyTony",
	"MCP_WhiteSuitTony",
	"MCP_WhiteSuitShadesTony"
};

static std::vector<std::string> customSkins;

// File name -> internal name remaps.
static std::unordered_map<std::string, std::string> remap;

static PackageRef* GetDummyPackageRef() {
	if (playerModelPool == nullptr)
		return nullptr;
	return **(PackageRef***)(playerModelPool);
}

static bool wasKeyJustPressed(int vk)
{
	if (vk == 0) return false;
	static SHORT lastState[256]{};
	SHORT state = GetAsyncKeyState(vk);
	bool pressed = (state & 0x8000) && !(lastState[vk] & 0x8000);
	lastState[vk] = state;
	return pressed;
}

static int prevVanillaSkinKey = 0x74;
static int nextVanillaSkinKey = 0x75;
static int prevCustomSkinKey = 0x76;
static int nextCustomSkinKey = 0x77;

static int GetCurrentCustomSkinIndex() {
	if (currentPlayerModel == "")
		return -1;
	auto it = std::find(customSkins.begin(), customSkins.end(), currentPlayerModel);
	if (it == customSkins.end()) {
		return -1;
	}
	return std::distance(customSkins.begin(), it);
}

static int GetCurrentVanillaSkinIndex() {
	if (currentPlayerModel == "")
		return -1;
	auto it = std::find(vanillaSkins.begin(), vanillaSkins.end(), currentPlayerModel);
	if (it == vanillaSkins.end()) {
		return -1;
	}
	return std::distance(vanillaSkins.begin(), it);
}

static void __fastcall ShowNotification_Hook(NotificationManager* self, void* _, int type, const char* str) {
	notificationManagerInstance = self;
	fpShowNotification(self, type, str);
}

static void __fastcall RenderGame_Hook(void* self, void* _) {
	if (currentPlayerModel != "") {
		if (wasKeyJustPressed(prevVanillaSkinKey)) {
			int currentVanillaSkin = GetCurrentVanillaSkinIndex();
			currentVanillaSkin--;
			if (currentVanillaSkin < 0)
				currentVanillaSkin = vanillaSkins.size() - 1;
			characterManagerInstance->SetMainCharacterPackage(vanillaSkins[currentVanillaSkin].c_str(), false);
			if (notificationManagerInstance != nullptr) {
				notificationManagerInstance->ShowRawNotification(1, currentPlayerModel.c_str());
			}
		}
		if (wasKeyJustPressed(nextVanillaSkinKey)) {
			int currentVanillaSkin = GetCurrentVanillaSkinIndex();
			currentVanillaSkin++;
			if (currentVanillaSkin >= vanillaSkins.size())
				currentVanillaSkin = 0;
			characterManagerInstance->SetMainCharacterPackage(vanillaSkins[currentVanillaSkin].c_str(), false);
			if (notificationManagerInstance != nullptr) {
				notificationManagerInstance->ShowRawNotification(1, currentPlayerModel.c_str());
			}
		}
		if (customSkins.size() > 0) {
			if (wasKeyJustPressed(prevCustomSkinKey)) {
				int currentCustomSkin = GetCurrentCustomSkinIndex();
				currentCustomSkin--;
				if (currentCustomSkin < 0)
					currentCustomSkin = customSkins.size() - 1;
				characterManagerInstance->SetMainCharacterPackage(customSkins[currentCustomSkin].c_str(), false);
				if (notificationManagerInstance != nullptr) {
					notificationManagerInstance->ShowRawNotification(1, currentPlayerModel.c_str());
				}
			}
			if (wasKeyJustPressed(nextCustomSkinKey)) {
				int currentCustomSkin = GetCurrentCustomSkinIndex();
				currentCustomSkin++;
				if (currentCustomSkin >= customSkins.size())
					currentCustomSkin = 0;
				characterManagerInstance->SetMainCharacterPackage(customSkins[currentCustomSkin].c_str(), false);
				if (notificationManagerInstance != nullptr) {
					notificationManagerInstance->ShowRawNotification(1, currentPlayerModel.c_str());
				}
			}
		}
	}
	fpRenderGame(self);
}

static const char* __fastcall GetMainCharacterPackage_Hook(void* self, void* _) {
	return currentPlayerModel.c_str();
}

static void __fastcall SetMainCharacterPackage_Hook(CharacterManager* self, void* _, const char* name, bool unk) {
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
	currentPlayerModelInternalName = name;
	if (remap.find(name) != remap.end()) {
		currentPlayerModelInternalName = remap[name];
	}
	dummyChar->SetName(currentPlayerModelInternalName.c_str());
	fpSetMainCharacterPackage(self, currentPlayerModelInternalName.c_str(), unk);
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

static void* __fastcall CharacterManagerCtor_Hook(CharacterManager* self, void* _) {
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

const std::string iniPrevVanillaSkinKey = "PreviousVanillaSkin";
const std::string iniNextVanillaSkinKey = "NextVanillaSkin";
const std::string iniPrevCustomSkinKey = "PreviousCustomSkin";
const std::string iniNextCustomSkinKey = "NextCustomSkin";

static void CacheSkins() {
	std::wstring gameDir = GetGameDirectory();
	std::wstring skinsDir = gameDir + L"\\skins";
	std::wstring iniDir = gameDir + L"\\SkinSelector.ini";

	if (Exists(iniDir)) {
		mINI::INIFile file(iniDir);
		mINI::INIStructure ini;
		file.read(ini);
		auto keys = ini["Keys"];
		if (keys.has(iniPrevVanillaSkinKey))
		{
			prevVanillaSkinKey = std::stoi(keys.get(iniPrevVanillaSkinKey), nullptr, 0);
		}
		if (keys.has(iniNextVanillaSkinKey))
		{
			nextVanillaSkinKey = std::stoi(keys.get(iniNextVanillaSkinKey), nullptr, 0);
		}
		if (keys.has(iniPrevCustomSkinKey))
		{
			prevCustomSkinKey = std::stoi(keys.get(iniPrevCustomSkinKey), nullptr, 0);
		}
		if (keys.has(iniNextCustomSkinKey))
		{
			nextCustomSkinKey = std::stoi(keys.get(iniNextCustomSkinKey), nullptr, 0);
		}
	}

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
				std::wstring iniPath = (entry.path().parent_path() / entry.path().stem()).wstring() + L".ini";
				if (Exists(iniPath)) {
					mINI::INIFile file(iniPath);
					mINI::INIStructure ini;
					file.read(ini);
					if (ini["general"].has("remap"))
					{
						remap[u8name] = ini["general"]["remap"];
					}
				}
				std::replace(u8Path.begin(), u8Path.end(), '\\', '/');
				skins[u8name] = u8Path;
				if (std::find(vanillaSkins.begin(), vanillaSkins.end(), u8name) == vanillaSkins.end()) {
					customSkins.push_back(u8name);
				}
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
	if (MH_CreateHook(Addresses::CM_GetMainCharacterPackage, &GetMainCharacterPackage_Hook,
		reinterpret_cast<LPVOID*>(&fpGetMainCharacterPackage)) != MH_OK)
	{
		return;
	}
	if (MH_EnableHook(Addresses::CM_GetMainCharacterPackage) != MH_OK)
	{
		return;
	}
	if (MH_CreateHook(Addresses::RenderGame, &RenderGame_Hook,
		reinterpret_cast<LPVOID*>(&fpRenderGame)) != MH_OK)
	{
		return;
	}
	if (MH_EnableHook(Addresses::RenderGame) != MH_OK)
	{
		return;
	}
	if (MH_CreateHook(Addresses::ShowNotification, &ShowNotification_Hook,
		reinterpret_cast<LPVOID*>(&fpShowNotification)) != MH_OK)
	{
		return;
	}
	if (MH_EnableHook(Addresses::ShowNotification) != MH_OK)
	{
		return;
	}
	// Forces SetMainCharacterPackage to reload even if same character address selected.
	Nop((BYTE*)((DWORD)Addresses::CM_SetMainCharacterPackage + 0x63), 6);
	printf("Skin Selector Initialized!\n");
	printf("%i Skins loaded.\n", skins.size());
}