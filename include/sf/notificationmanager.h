#pragma once

class NotificationManager {
public:
	// Vanilla
	void ShowNotification(int type, const char* str);
	// Custom
	void ShowRawNotification(int type, const char* str);
};