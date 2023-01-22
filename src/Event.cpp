#include "Event.h"

#define SMOOTHCAM_API_COMMONLIB
#include "SmoothCamAPI.h"

constexpr auto MATH_PI = 3.14159265358979323846f;

bool m_thirdForced = false;
bool m_inMenu = false;
uint32_t m_camStateId;
float m_playerRotation;
float fRotationAmount = 0.10f;
float fTurnSensitivity = 3.0f;
RE::Setting* fOverShoulderCombatPosX;
RE::Setting* fOverShoulderCombatAddY;
RE::Setting* fOverShoulderCombatPosZ;
RE::Setting* fOverShoulderPosX;
RE::Setting* fOverShoulderPosZ;
RE::Setting* fAutoVanityModeDelay;
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

	auto player = RE::PlayerCharacter::GetSingleton();
	if (!player || !player->Is3DLoaded())
		return EventResult::kContinue;

	auto ui = RE::UI::GetSingleton();

	if (!ui || !m_inMenu)
		return EventResult::kContinue;

	for (auto event = *a_event; event; event = event->next) {
		auto inputDevice = event->GetDevice();
		
		switch (event->GetEventType()) {
		case RE::INPUT_EVENT_TYPE::kButton:
			{
				auto buttonEvent = event->AsButtonEvent();
				if (!buttonEvent || !buttonEvent->IsHeld()) {
					allowRotation = false;
					continue;
				}

				switch (inputDevice) {
				case RE::INPUT_DEVICE::kGamepad:
					{
						const auto gamepadButton = static_cast<ControllerButton>(buttonEvent->GetIDCode());
						if (gamepadButton == MenuOpenCloseEventHandler::iGamepadLeftTurnButton) {
							if (const auto playerCamera = RE::PlayerCamera::GetSingleton()) {
								player->SetRotationZ(player->data.angle.z + (1 * fRotationAmount));
								auto thirdPersonState = static_cast<RE::ThirdPersonState*>(playerCamera->currentState.get());
								thirdPersonState->freeRotation.x -= (1 * fRotationAmount);
								player->Update3DPosition(true);
							}
							continue;
						} else if (gamepadButton == MenuOpenCloseEventHandler::iGamepadRightTurnButton) {
							if (const auto playerCamera = RE::PlayerCamera::GetSingleton()) {
								player->SetRotationZ(player->data.angle.z + (-1 * fRotationAmount));
								auto thirdPersonState = static_cast<RE::ThirdPersonState*>(playerCamera->currentState.get());
								thirdPersonState->freeRotation.x -= (-1 * fRotationAmount);
								player->Update3DPosition(true);
							}
							continue;
						}
					}
					break;
				case RE::INPUT_DEVICE::kMouse:
					if (const auto mouseButton = static_cast<MouseButton>(buttonEvent->GetIDCode()); mouseButton == (257 - 0x100)) {
						allowRotation = true;
						continue;
					}
					break;
				}
			}
			continue;
		case RE::INPUT_EVENT_TYPE::kMouseMove:
			{
				if (allowRotation) {
					auto mouseEvent = reinterpret_cast<RE::MouseMoveEvent*>(event->AsIDEvent());
					if (abs(mouseEvent->mouseInputX) < fTurnSensitivity)
						continue;
					auto playerCamera = RE::PlayerCamera::GetSingleton();
					if (playerCamera) {
						player->SetRotationZ(player->data.angle.z + ((mouseEvent->mouseInputX > 0 ? -1 : 1) * fRotationAmount));
						auto thirdPersonState = static_cast<RE::ThirdPersonState*>(playerCamera->currentState.get());
						thirdPersonState->freeRotation.x -= ((mouseEvent->mouseInputX > 0 ? -1 : 1) * fRotationAmount);
						player->Update3DPosition(true);
						//player->DoReset3D(!player->AsActorState()->IsWeaponDrawn());
					}
				}
			}
			break;
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
	m_playerAngleX = player->data.angle.x;
	m_targetZoomOffset = thirdState->targetZoomOffset;
	m_freeRotation = thirdState->freeRotation;
	m_posOffsetExpected = thirdState->posOffsetExpected;
	m_radialBlurStrength = mod->radialBlur.strength;
	m_blurRadius = mod->blurRadius->floatValue;

	camera->SetState(thirdState);
	camera->UpdateThirdPerson(player->AsActorState()->IsWeaponDrawn());

	// set over the shoulder camera values for when player has weapon drawn and unpaused menu(s) in order to prevent camera from snapping
	fOverShoulderCombatPosX = ini->GetSetting("fOverShoulderCombatPosX:Camera");
	fOverShoulderCombatAddY = ini->GetSetting("fOverShoulderCombatAddY:Camera");
	fOverShoulderCombatPosZ = ini->GetSetting("fOverShoulderCombatPosZ:Camera");
	fAutoVanityModeDelay = ini->GetSetting("fAutoVanityModeDelay:Camera");
	fOverShoulderPosX = ini->GetSetting("fOverShoulderPosX:Camera");
	fOverShoulderPosZ = ini->GetSetting("fOverShoulderPosZ:Camera");
	m_fOverShoulderCombatPosX = fOverShoulderCombatPosX->GetFloat();
	m_fOverShoulderCombatAddY = fOverShoulderCombatAddY->GetFloat();
	m_fOverShoulderCombatPosZ = fOverShoulderCombatPosZ->GetFloat();
	m_fOverShoulderPosX = fOverShoulderPosX->GetFloat();
	m_fOverShoulderPosZ = fOverShoulderPosZ->GetFloat();
	m_fAutoVanityModeDelay = fAutoVanityModeDelay->GetFloat();

	// disable blur before opening menu so character is not obscured
	mod->radialBlur.strength = 0;
	mod->blurRadius->floatValue = 0;

	// toggle anim cam which unshackles camera and lets it move in front of player with their weapon drawn, necessary if not using TDM
	thirdState->toggleAnimCam = true;
	//thirdState->freeRotationEnabled = true;

	// set gamepad control maps;
	ReadUint32Setting(mcm, "GamepadSettings", "iGamepadLeftTurnButton", iGamepadLeftTurnButton);
	ReadUint32Setting(mcm, "GamepadSettings", "iGamepadRightTurnButton", iGamepadRightTurnButton);

	// Check latest settings
	ReadFloatSetting(mcm, "PositionSettings", "fXOffset", fXOffset);
	ReadFloatSetting(mcm, "PositionSettings", "fYOffset", fYOffset);
	ReadFloatSetting(mcm, "PositionSettings", "fZOffset", fZOffset);
	ReadFloatSetting(mcm, "PositionSettings", "fPitch", fPitch);
	ReadFloatSetting(mcm, "PositionSettings", "fRotation", fRotation);

	fAutoVanityModeDelay->data.f = 10800.0f;	// 3 hours

	m_fNewOverShoulderCombatPosX = -fXOffset - 75.0f;
	m_fNewOverShoulderCombatAddY = fYOffset - 50.0f;
	m_fNewOverShoulderCombatPosZ = fZOffset - 50.0f;

	// rotate and move camera
	thirdState->targetZoomOffset = -0.1f;
	thirdState->freeRotation.x = MATH_PI + fRotation - 0.5f;

	// account for camera freeRotation settings getting pushed into player's pitch (x) values when weapon drawn
	thirdState->freeRotation.y = 0.0f;
	if (!player->AsActorState()->IsWeaponDrawn())
		player->data.angle.x = 0.2f + fPitch;
	else {
		player->data.angle.x -= player->data.angle.x - fPitch - 0.2f;
	}

	fOverShoulderCombatPosX->data.f = fOverShoulderPosX->data.f = m_fNewOverShoulderCombatPosX;
	fOverShoulderCombatAddY->data.f = m_fNewOverShoulderCombatAddY;
	fOverShoulderCombatPosZ->data.f = fOverShoulderPosZ->data.f = m_fNewOverShoulderCombatPosZ;

	thirdState->posOffsetExpected = thirdState->posOffsetActual = RE::NiPoint3(m_fNewOverShoulderCombatPosX, m_fNewOverShoulderCombatAddY, m_fNewOverShoulderCombatPosZ);

	camera->Update();
}

void MenuOpenCloseEventHandler::ResetCamera()
{
	auto player = RE::PlayerCharacter::GetSingleton();
	
	auto camera = RE::PlayerCamera::GetSingleton();
	auto thirdState = (RE::ThirdPersonState*)camera->cameraStates[RE::CameraState::kThirdPerson].get();
	auto mod = RE::TESForm::LookupByID<RE::TESImageSpaceModifier>(0x000434BB);

	// restore original values
	player->data.angle.x = m_playerAngleX;
	player->data.angle.z = m_playerRotation;
	fAutoVanityModeDelay->data.f = m_fAutoVanityModeDelay;
	thirdState->toggleAnimCam = false;
	thirdState->targetZoomOffset = m_targetZoomOffset;
	thirdState->freeRotation = m_freeRotation;
	thirdState->posOffsetExpected = thirdState->posOffsetActual = m_posOffsetExpected;
	fOverShoulderCombatPosX->data.f = m_fOverShoulderCombatPosX;
	fOverShoulderCombatAddY->data.f = m_fOverShoulderCombatAddY;
	fOverShoulderCombatPosZ->data.f = m_fOverShoulderCombatPosZ;
	fOverShoulderPosX->data.f = m_fOverShoulderPosX;
	fOverShoulderPosZ->data.f = m_fOverShoulderPosZ;
	mod->radialBlur.strength = m_radialBlurStrength;
	mod->blurRadius->floatValue = m_blurRadius;

	if (m_thirdForced) {
		auto cameraState = (RE::TESCameraState*)camera->cameraStates[m_camStateId].get();
		camera->SetState(cameraState);
	}

	thirdState->toggleAnimCam = false;
	m_thirdForced = false;
	camera->Update();
}

void MenuOpenCloseEventHandler::OnInventoryOpen()
{
	auto camera = RE::PlayerCamera::GetSingleton();
	if (!camera) {
		return;
	}

	if (CheckOptions()) {
		m_inMenu = false;
		return;
	}

	// SmoothCam compatibility
	if (g_smooth_cam && g_smooth_cam->IsCameraEnabled())
	{
		const auto result = g_smooth_cam->RequestCameraControl(g_plugin_handle);
		if (result == SmoothCamAPI::APIResult::OK || result == SmoothCamAPI::APIResult::AlreadyGiven)
		{
			g_smooth_cam->RequestInterpolatorUpdates(g_plugin_handle, true);
		}
	}

	RotateCamera();
}

void MenuOpenCloseEventHandler::OnInventoryClose()
{
	if (CheckOptions()) {
		m_inMenu = false;
		return;
	}

	// SmoothCam compatibility
	if (g_smooth_cam && g_smooth_cam->IsCameraEnabled())
	{
		//g_SmoothCam->RequestInterpolatorUpdates(g_pluginHandle, false);
		g_smooth_cam->ReleaseCameraControl(g_plugin_handle);
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
			g_ui_name = name;
			ReadBoolSetting(mcm, "GeneralSettings", "bEnableInInventoryMenu", bEnableInInventoryMenu);

			if (bEnableInInventoryMenu) {
				m_inMenu = true;

				if (a_event->opening) {
					auto player = RE::PlayerCharacter::GetSingleton();
					m_playerRotation = player->data.angle.z;
					ReadFloatSetting(mcm, "PositionSettings", "fRotationAmount", fRotationAmount);
					ReadFloatSetting(mcm, "PositionSettings", "fTurnSensitivity", fTurnSensitivity);
					OnInventoryOpen();
				} else
					OnInventoryClose();
			}
		} else if (name == uiStr->containerMenu)
		{
			g_ui_name = name;
			ReadBoolSetting(mcm, "GeneralSettings", "bEnableInContainerMenu", bEnableInContainerMenu);

			if (bEnableInContainerMenu) {
				m_inMenu = true;

				if (a_event->opening) {
					auto player = RE::PlayerCharacter::GetSingleton();
					m_playerRotation = player->data.angle.z;
					ReadFloatSetting(mcm, "PositionSettings", "fRotationAmount", fRotationAmount);
					ReadFloatSetting(mcm, "PositionSettings", "fTurnSensitivity", fTurnSensitivity);
					OnInventoryOpen();
				} else
					OnInventoryClose();
			}
		} else if (name == uiStr->barterMenu) {
			g_ui_name = name;
			ReadBoolSetting(mcm, "GeneralSettings", "bEnableInBarterMenu", bEnableInBarterMenu);

			if (bEnableInBarterMenu) {
				m_inMenu = true;

				if (a_event->opening) {
					auto player = RE::PlayerCharacter::GetSingleton();
					m_playerRotation = player->data.angle.z;
					ReadFloatSetting(mcm, "PositionSettings", "fRotationAmount", fRotationAmount);
					ReadFloatSetting(mcm, "PositionSettings", "fTurnSensitivity", fTurnSensitivity);
					OnInventoryOpen();
				} else
					OnInventoryClose();
			}
		} else if (name == uiStr->magicMenu)
		{
			g_ui_name = name;
			ReadBoolSetting(mcm, "GeneralSettings", "bEnableInMagicMenu", bEnableInMagicMenu);

			if (bEnableInMagicMenu) {
				m_inMenu = true;

				if (a_event->opening) {
					auto player = RE::PlayerCharacter::GetSingleton();
					m_playerRotation = player->data.angle.z;
					ReadFloatSetting(mcm, "PositionSettings", "fRotationAmount", fRotationAmount);
					ReadFloatSetting(mcm, "PositionSettings", "fTurnSensitivity", fTurnSensitivity);
					OnInventoryOpen();
				} else
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

void MenuOpenCloseEventHandler::ReadUint32Setting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, uint32_t& a_setting)
{
	const char* bFound = nullptr;
	bFound = a_ini.GetValue(a_sectionName, a_settingName);
	if (bFound) {
		a_setting = a_ini.GetLongValue(a_sectionName, a_settingName);
	}
}

bool MenuOpenCloseEventHandler::CheckOptions()
{
	auto player = RE::PlayerCharacter::GetSingleton();
	auto camera = RE::PlayerCamera::GetSingleton();
	auto controls = RE::PlayerControls::GetSingleton();

	if (m_camStateId < RE::CameraState::kThirdPerson) {
		ReadBoolSetting(mcm, "ConditionalSettings", "bEnableFirstPerson", bEnableFirstPerson);
		if (!bEnableFirstPerson) {
			return true;
		}
		m_thirdForced = true;
	}

	if (player->IsOnMount()) {
		m_inMenu = false;
		return true;
	}

	if (player->IsInCombat()) {
		ReadBoolSetting(mcm, "ConditionalSettings", "bEnableDuringCombat", bEnableCombat);
		if (!bEnableCombat) {
			m_inMenu = false;
			return true;
		}
	}

	if (controls->data.autoMove) {
		ReadBoolSetting(mcm, "ConditionalSettings", "bEnableWhileAutoMoving", bEnableAutoMoving);
		if (!bEnableAutoMoving) {
			m_inMenu = false;
			return true;
		}
	}

	if (camera->IsInFreeCameraMode()) {
		m_inMenu = false;
		return true;
	}

	return false;
}

//constexpr uint32_t hash(const std::string_view data) noexcept {
//	uint32_t hash = 5385;
//
//	for (const auto &e : data)
//		hash = ((hash << 5) + hash) + e;
//
//	return hash;
//}
