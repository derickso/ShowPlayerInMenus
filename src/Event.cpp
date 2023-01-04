#include "Event.h"

#define SMOOTHCAM_API_COMMONLIB
#include "SmoothCamAPI.h"

constexpr auto MATH_PI = 3.14159265358979323846f;

bool m_inMenu = false;
uint32_t m_camStateId;
RE::Setting* fOverShoulderCombatPosX;
RE::Setting* fOverShoulderCombatAddY;
RE::Setting* fOverShoulderCombatPosZ;

//constexpr auto defaultSettingsPath = L"Data/MCM/Config/ShowPlayerInMenus/settings.ini";
constexpr auto mcmSettingsPath = L"Data/MCM/Settings/ShowPlayerInMenus.ini";

CSimpleIniA mcm;

// Input

auto InputEventHandler::ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*)
	-> EventResult
{
	if (!a_event)
		return EventResult::kContinue;

	auto camera = RE::PlayerCamera::GetSingleton();
	if (camera) {
		auto& state = camera->currentState;
		if (state) {
			if (!m_inMenu && (state->id != RE::CameraState::kTween)) {
				m_camStateId = state->id;
			}
		}
	}

	return EventResult::kContinue;
}

// Menu

void MenuOpenCloseEventHandler::RotateCamera()
{
	auto camera = RE::PlayerCamera::GetSingleton();
	auto ini = RE::INISettingCollection::GetSingleton();
	auto player = RE::PlayerCharacter::GetSingleton();

	if (m_camStateId < RE::CameraState::kThirdPerson) {
		m_thirdForced = true;
	}

	auto thirdState = (RE::ThirdPersonState*)camera->cameraStates[RE::CameraState::kThirdPerson].get();
	auto mod = RE::TESForm::LookupByID<RE::TESImageSpaceModifier>(0x000434BB);	// Image Space Adapter - ISMTween

	/* Camera collision code, doesn't seem to work at the moment for some reason, forms are properly deleted but camera collisions remain
	auto col_Camera = RE::TESForm::LookupByID<RE::BGSCollisionLayer>(0x00088788);
	auto col_Static = RE::TESForm::LookupByID<RE::BGSCollisionLayer>(0x00088762);
	auto col_AnimStatic = RE::TESForm::LookupByID<RE::BGSCollisionLayer>(0x00088763);
	auto col_Trees = RE::TESForm::LookupByID<RE::BGSCollisionLayer>(0x0008876A);

	auto end = std::remove(std::begin(col_Camera->collidesWith), std::end(col_Camera->collidesWith), col_Static);
	if (!end) col_Camera->collidesWith.erase(end);
	auto end2 = std::remove(std::begin(col_Camera->collidesWith), std::end(col_Camera->collidesWith), col_AnimStatic);
	if (!end2) col_Camera->collidesWith.erase(end2);
	auto end3 = std::remove(std::begin(col_Camera->collidesWith), std::end(col_Camera->collidesWith), col_Trees);
	if (!end3) col_Camera->collidesWith.erase(end3);

	logger::info("");
	for (unsigned int i = 0; i < col_Camera->collidesWith.size(); i++) {
		logger::info("collidesWith before: {}"sv, col_Camera->collidesWith[i]->name);
	}

	camera->Update();
	*/

	// collect original values for later
	m_targetZoomOffset = thirdState->targetZoomOffset;
	m_freeRotation = thirdState->freeRotation;
	m_posOffsetExpected = thirdState->posOffsetExpected;
	m_toggleAnimCam = thirdState->toggleAnimCam;
	m_radialBlurStrength = mod->radialBlur.strength;
	m_blurRadius = mod->blurRadius->floatValue;

	camera->SetState(thirdState);
	camera->UpdateThirdPerson(player->AsActorState()->IsWeaponDrawn());

	// set over the shoulder camera values for when player has weapon drawn and unpaused menu(s) in order to prevent camera from snapping
	fOverShoulderCombatPosX = ini->GetSetting("fOverShoulderCombatPosX:Camera");
	fOverShoulderCombatAddY = ini->GetSetting("fOverShoulderCombatAddY:Camera");
	fOverShoulderCombatPosZ = ini->GetSetting("fOverShoulderCombatPosZ:Camera");
	m_fOverShoulderCombatPosX = fOverShoulderCombatPosX->GetFloat();
	m_fOverShoulderCombatAddY = fOverShoulderCombatAddY->GetFloat();
	m_fOverShoulderCombatPosZ = fOverShoulderCombatPosZ->GetFloat();

	// disable blur before opening menu so character is not obscured
	mod->radialBlur.strength = 0;
	mod->blurRadius->floatValue = 0;

	// toggle anim cam which unshackles camera and lets it move in front of player with their weapon drawn, necessary if not using TDM
	thirdState->toggleAnimCam = true;

	// Check latest settings
	ReadFloatSetting(mcm, "PositionSettings", "fXOffset", fXOffset);
	ReadFloatSetting(mcm, "PositionSettings", "fYOffset", fYOffset);
	ReadFloatSetting(mcm, "PositionSettings", "fZOffset", fZOffset);
	ReadFloatSetting(mcm, "PositionSettings", "fRotation", fRotation);

	// rotate and move camera
	thirdState->targetZoomOffset = -0.1f;
	thirdState->freeRotation.x = MATH_PI + fRotation - 0.5f;
	fOverShoulderCombatPosX->data.f = fXOffset - 75.0f;
	fOverShoulderCombatAddY->data.f = fYOffset - 50.0f;
	fOverShoulderCombatPosZ->data.f = fZOffset - 50.0f;
	thirdState->posOffsetExpected = thirdState->posOffsetActual = RE::NiPoint3(-fXOffset-75.0f, fYOffset-50.0f, fZOffset-50.0f);

	camera->Update();
}

void MenuOpenCloseEventHandler::ResetCamera()
{
	auto camera = RE::PlayerCamera::GetSingleton();
	auto thirdState = (RE::ThirdPersonState*)camera->cameraStates[RE::CameraState::kThirdPerson].get();
	auto mod = RE::TESForm::LookupByID<RE::TESImageSpaceModifier>(0x000434BB);

	// restore original values
	thirdState->targetZoomOffset = m_targetZoomOffset;
	thirdState->freeRotation = m_freeRotation;
	thirdState->posOffsetExpected = thirdState->posOffsetActual = m_posOffsetExpected;
	thirdState->toggleAnimCam = m_toggleAnimCam;
	fOverShoulderCombatPosX->data.f = m_fOverShoulderCombatPosX;
	fOverShoulderCombatAddY->data.f = m_fOverShoulderCombatAddY;
	fOverShoulderCombatPosZ->data.f = m_fOverShoulderCombatPosZ;
	mod->radialBlur.strength = m_radialBlurStrength;
	mod->blurRadius->floatValue = m_blurRadius;

	if (m_thirdForced) {
		auto cameraState = (RE::TESCameraState*)camera->cameraStates[m_camStateId].get();
		camera->SetState(cameraState);
	}

	camera->Update();
}

void MenuOpenCloseEventHandler::OnInventoryOpen()
{
	auto camera = RE::PlayerCamera::GetSingleton();
	if (!camera) {
		return;
	}

	// SmoothCam compatibility
	if (g_SmoothCam && g_SmoothCam->IsCameraEnabled())
	{
		const auto result = g_SmoothCam->RequestCameraControl(g_pluginHandle);
		if (result == SmoothCamAPI::APIResult::OK || result == SmoothCamAPI::APIResult::AlreadyGiven)
		{
			g_SmoothCam->RequestInterpolatorUpdates(g_pluginHandle, true);
		}
	}

	m_thirdForced = false;
	RotateCamera();
}

void MenuOpenCloseEventHandler::OnInventoryClose()
{
	// SmoothCam compatibility
	if (g_SmoothCam && g_SmoothCam->IsCameraEnabled())
	{
		//g_SmoothCam->RequestInterpolatorUpdates(g_pluginHandle, false);
		g_SmoothCam->ReleaseCameraControl(g_pluginHandle);
	}

	ResetCamera();
	m_inMenu = false;
}

auto MenuOpenCloseEventHandler::ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	-> EventResult
{
	mcm.SetUnicode();
	mcm.LoadFile(mcmSettingsPath);

	auto uiStr = RE::InterfaceStrings::GetSingleton();
	if (uiStr) {
		auto& name = a_event->menuName;

		if (name == uiStr->inventoryMenu)
		{
			g_uiName = name;
			ReadBoolSetting(mcm, "GeneralSettings", "bEnableInInventoryMenu", bEnableInInventoryMenu);

			if (bEnableInInventoryMenu) {
				m_inMenu = true;

				if (a_event->opening)
					OnInventoryOpen();
				else
					OnInventoryClose();
			}
		} else if (name == uiStr->containerMenu)
		{
			g_uiName = name;
			ReadBoolSetting(mcm, "GeneralSettings", "bEnableInContainerMenu", bEnableInContainerMenu);

			if (bEnableInContainerMenu) {
				m_inMenu = true;

				if (a_event->opening)
					OnInventoryOpen();
				else
					OnInventoryClose();
			}
		} else if (name == uiStr->barterMenu) {
			g_uiName = name;
			ReadBoolSetting(mcm, "GeneralSettings", "bEnableInBarterMenu", bEnableInBarterMenu);

			if (bEnableInBarterMenu) {
				m_inMenu = true;

				if (a_event->opening)
					OnInventoryOpen();
				else
					OnInventoryClose();
			}
		} else if (name == uiStr->magicMenu)
		{
			g_uiName = name;
			ReadBoolSetting(mcm, "GeneralSettings", "bEnableInMagicMenu", bEnableInMagicMenu);

			if (bEnableInMagicMenu) {
				m_inMenu = true;

				if (a_event->opening)
					OnInventoryOpen();
				else
					OnInventoryClose();
			}
		} 

	}
	return EventResult::kContinue;
}

void MenuOpenCloseEventHandler::ReadBoolSetting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, bool& a_setting)
{
	const char* bFound = nullptr;
	bFound = a_ini.GetValue(a_sectionName, a_settingName);
	if (bFound) {
		a_setting = a_ini.GetBoolValue(a_sectionName, a_settingName);
	}
}

void MenuOpenCloseEventHandler::ReadFloatSetting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, float& a_setting)
{
	const char* bFound = nullptr;
	bFound = a_ini.GetValue(a_sectionName, a_settingName);
	if (bFound) {
		a_setting = static_cast<float>(a_ini.GetDoubleValue(a_sectionName, a_settingName));
	}
}

//constexpr uint32_t hash(const std::string_view data) noexcept {
//	uint32_t hash = 5385;
//
//	for (const auto &e : data)
//		hash = ((hash << 5) + hash) + e;
//
//	return hash;
//}
