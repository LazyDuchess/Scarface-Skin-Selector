#include "sf/notificationmanager.h"
#include "Addresses.h"
#include <string>

void NotificationManager::ShowNotification(int type, const char* str) {
	((void* (__thiscall*)(NotificationManager*, int, const char*))Addresses::ShowNotification)(this, type, str);
}

void NotificationManager::ShowRawNotification(int type, const char* str) {
	ShowNotification(type, ("[NOKEY]" + std::string(str) + "[NOKEY]").c_str());
}