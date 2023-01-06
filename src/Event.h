#pragma once

#define SMOOTHCAM_API_COMMONLIB
#include "SmoothCamAPI.h"
#include <Simpleini.h>
#include <RE/N/NiFloatInterpolator.h>

RE::BSFixedString g_uiName;
SmoothCamAPI::IVSmoothCam2* g_SmoothCam = nullptr;
SKSE::MessagingInterface* g_Messaging = nullptr;
SKSE::PluginHandle g_pluginHandle = SKSE::kInvalidPluginHandle;

class InputEventHandler : public RE::BSTEventSink<RE::InputEvent*>
{
public:
	using EventResult = RE::BSEventNotifyControl;

	static InputEventHandler* GetSingleton()
	{
		static InputEventHandler singleton;
		return std::addressof(singleton);
	}

	static void Enable()
	{
		auto ui = RE::BSInputDeviceManager::GetSingleton();
		if (ui) {
			ui->AddEventSink(GetSingleton());
		}
	}

	static void Disable()
	{
		auto ui = RE::BSInputDeviceManager::GetSingleton();
		if (ui) {
			ui->RemoveEventSink(GetSingleton());
		}
	}

	virtual EventResult ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) override;

private:
	InputEventHandler() = default;
	InputEventHandler(const InputEventHandler&) = delete;
	InputEventHandler(InputEventHandler&&) = delete;

	inline ~InputEventHandler() { Disable(); }

	InputEventHandler& operator=(const InputEventHandler&) = delete;
	InputEventHandler& operator=(InputEventHandler&&) = delete;
};

class MenuOpenCloseEventHandler : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
public:
	using EventResult = RE::BSEventNotifyControl;

	static MenuOpenCloseEventHandler* GetSingleton()
	{
		static MenuOpenCloseEventHandler singleton;
		return std::addressof(singleton);
	}

	static void Enable()
	{
		auto ui = RE::UI::GetSingleton();
		if (ui) {
			ui->AddEventSink(GetSingleton());
		}
	}

	static void Disable()
	{
		auto ui = RE::UI::GetSingleton();
		if (ui) {
			ui->RemoveEventSink(GetSingleton());
		}
	}

	void RotateCamera();
	void ResetCamera();

	void OnInventoryOpen();
	void OnInventoryClose();

	static void ReadBoolSetting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, bool& a_setting);
	static void ReadFloatSetting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, float& a_setting);

	static inline bool bEnableInInventoryMenu = true;
	static inline bool bEnableInContainerMenu = false;
	static inline bool bEnableInBarterMenu = false;
	static inline bool bEnableInMagicMenu = false;
	static inline bool bEnableInCombat = false;
	static inline float fXOffset = 0.0;
	static inline float fYOffset = 0.0;
	static inline float fZOffset = 0.0;
	static inline float fRotation = 0.0;

	virtual EventResult ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

private:
	MenuOpenCloseEventHandler() = default;
	MenuOpenCloseEventHandler(const MenuOpenCloseEventHandler&) = delete;
	MenuOpenCloseEventHandler(MenuOpenCloseEventHandler&&) = delete;

	inline ~MenuOpenCloseEventHandler() { Disable(); }

	MenuOpenCloseEventHandler& operator=(const MenuOpenCloseEventHandler&) = delete;
	MenuOpenCloseEventHandler& operator=(MenuOpenCloseEventHandler&&) = delete;

	bool									m_thirdForced;
	bool									m_bDoRadialBlur;
	float									m_targetZoomOffset;
	float									m_fOverShoulderCombatPosX;
	float									m_fOverShoulderCombatAddY;
	float									m_fOverShoulderCombatPosZ;
	RE::NiPoint2							m_freeRotation;
	RE::NiPoint3							m_posOffsetExpected;
	RE::NiPointer<RE::NiFloatInterpolator>	m_radialBlurStrength;
	float									m_blurRadius;
};

