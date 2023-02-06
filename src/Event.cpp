#include "Event.h"
#include "ShowPlayerInMenus_Hooks.h"

#define SMOOTHCAM_API_COMMONLIB
#include "SmoothCamAPI.h"

constexpr auto MATH_PI = 3.14159265358979323846f;
constexpr auto TWO_PI = 6.2831853071795865f;

bool m_thirdForced = false;
bool m_enteredTweenFromFirst = false;
bool m_inMenu = false;
bool m_rotatedInTweenMenu = false;
bool m_shouldDisableAnimCam = false;
bool m_fixCameraZoom = false;
bool m_rotatedPlayer = false;
bool m_inSitState = false;
uint32_t m_camStateId;
float m_playerRotation;
RE::NiPoint2 g_freeRotation;
RE::NiPoint2 m_finalFreeRotation;
float m_finalPlayerAngleX;
float m_finalPlayerAngleZ;
float fRotationAmount = 0.10f;
float fTurnSensitivity = 3.0f;
RE::Setting* fOverShoulderCombatPosX;
RE::Setting* fOverShoulderCombatAddY;
RE::Setting* fOverShoulderCombatPosZ;
RE::Setting* fOverShoulderPosX;
RE::Setting* fOverShoulderPosZ;
RE::Setting* fAutoVanityModeDelay;
RE::Setting* fVanityModeMinDist;
RE::Setting* fVanityModeMaxDist;
RE::Setting* fMouseWheelZoomSpeed;

constexpr auto defaultSettingsPath = L"Data/MCM/Config/ShowPlayerInMenus/settings.ini";
constexpr auto mcmSettingsPath = L"Data/MCM/Settings/ShowPlayerInMenus.ini";
constexpr auto altCamPath = L"Data/SKSE/Plugins/AlternateConversationCamera.ini";

CSimpleIniA mcmDefault;
CSimpleIniA mcmOverride;
CSimpleIniA altCam;

// TDM Helpers
float NormalRelativeAngle(float a_angle)
{
	while (a_angle > MATH_PI)
		a_angle -= TWO_PI;
	while (a_angle < -MATH_PI)
		a_angle += TWO_PI;
	return a_angle;

	//return fmod(a_angle, TWO_PI) >= 0 ? (a_angle < PI) ? a_angle : a_angle - TWO_PI : (a_angle >= -PI) ? a_angle : a_angle + TWO_PI;
}

float GetAngle(RE::NiPoint2& a, RE::NiPoint2& b)
{
	return atan2(a.Cross(b), a.Dot(b));
}

RE::NiPoint3 Project(const RE::NiPoint3& A, const RE::NiPoint3& B)
{
	return (B * ((A.x * B.x + A.y * B.y + A.z * B.z) / (B.x * B.x + B.y * B.y + B.z * B.z)));
}

RE::NiPoint2 Vec2Rotate(const RE::NiPoint2& vec, float angle)
{
	RE::NiPoint2 ret;
	ret.x = vec.x * cos(angle) - vec.y * sin(angle);
	ret.y = vec.x * sin(angle) + vec.y * cos(angle);
	return ret;
}

RE::NiPoint3 GetCameraPos()
{
	auto player = RE::PlayerCharacter::GetSingleton();
	auto playerCamera = RE::PlayerCamera::GetSingleton();
	RE::NiPoint3 ret;

	if (playerCamera->currentState == playerCamera->cameraStates[RE::CameraStates::kFirstPerson] ||
		playerCamera->currentState == playerCamera->cameraStates[RE::CameraStates::kThirdPerson] ||
		playerCamera->currentState == playerCamera->cameraStates[RE::CameraStates::kMount]) {
		RE::NiNode* root = playerCamera->cameraRoot.get();
		if (root) {
			ret.x = root->world.translate.x;
			ret.y = root->world.translate.y;
			ret.z = root->world.translate.z;
		}
	} else {
		RE::NiPoint3 playerPos = player->GetLookingAtLocation();

		ret.z = playerPos.z;
		ret.x = player->GetPositionX();
		ret.y = player->GetPositionY();
	}

	return ret;
}

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
			if (!m_inMenu) {// && (state->id != RE::CameraState::kTween)) {
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

				// disable animcam before moving in menu
				if (camera && buttonEvent && m_shouldDisableAnimCam && !ui->GameIsPaused()) {
					auto idEvent = static_cast<RE::ButtonEvent*>(event);
					auto userEvents = RE::UserEvents::GetSingleton();
					bool bHasMovementInput = idEvent->userEvent == userEvents->forward ||
											idEvent->userEvent == userEvents->strafeLeft ||
											idEvent->userEvent == userEvents->strafeRight ||
											idEvent->userEvent == userEvents->back;

					if (bHasMovementInput) {
						auto thirdPersonState = static_cast<RE::ThirdPersonState*>(camera->currentState.get());
						if (player->AsActorState()->IsWeaponDrawn()) {
							thirdPersonState->freeRotation = m_finalFreeRotation;
							player->data.angle.x = m_finalPlayerAngleX;
							player->data.angle.z = m_finalPlayerAngleZ;
							player->Update3DPosition(true);
							camera->Update();
						}
						thirdPersonState->toggleAnimCam = false;

						if (m_fixCameraZoom) {
							fVanityModeMinDist->data.f -= (17.0f + (66.6f * MenuOpenCloseEventHandler::fPitch));
							fVanityModeMaxDist->data.f -= (17.0f + (66.6f * MenuOpenCloseEventHandler::fPitch));
							m_fixCameraZoom = false;
						}

						m_shouldDisableAnimCam = false;
					}
				}

				if (!buttonEvent || !buttonEvent->IsHeld()) {
					allowRotation = false;
					continue;
				}

				switch (inputDevice) {
				case RE::INPUT_DEVICE::kGamepad:
					{
						if (MenuOpenCloseEventHandler::iGamepadTurnMethod == 1 && MenuOpenCloseEventHandler::bGamepadRotating) {
							const auto gamepadButton = static_cast<ControllerButton>(buttonEvent->GetIDCode());

							if (gamepadButton == 9) {
								if (const auto playerCamera = RE::PlayerCamera::GetSingleton()) {
									player->SetRotationZ(player->data.angle.z + (1 * fRotationAmount));
									auto thirdPersonState = static_cast<RE::ThirdPersonState*>(playerCamera->currentState.get());
									thirdPersonState->freeRotation.x -= (1 * fRotationAmount);
									player->Update3DPosition(true);
								}
								continue;
							} else if (gamepadButton == 10) {
								if (const auto playerCamera = RE::PlayerCamera::GetSingleton()) {
									player->SetRotationZ(player->data.angle.z + (-1 * fRotationAmount));
									auto thirdPersonState = static_cast<RE::ThirdPersonState*>(playerCamera->currentState.get());
									thirdPersonState->freeRotation.x -= (-1 * fRotationAmount);
									player->Update3DPosition(true);
								}
								continue;
							}
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
					if (const auto playerCamera = RE::PlayerCamera::GetSingleton()) {
						bool bMoving = player->AsActorState()->actorState1.movingBack ||
									   player->AsActorState()->actorState1.movingForward ||
									   player->AsActorState()->actorState1.movingRight ||
									   player->AsActorState()->actorState1.movingLeft;

						if (!bMoving) {
							auto thirdPersonState = static_cast<RE::ThirdPersonState*>(playerCamera->currentState.get());
							if (player->AsActorState()->IsWeaponDrawn()) {
								if (!m_shouldDisableAnimCam && m_rotatedPlayer) {
									thirdPersonState->toggleAnimCam = true;
									thirdPersonState->freeRotationEnabled = true;
									thirdPersonState->freeRotation.x = m_finalFreeRotation.x;
									player->data.angle.x = m_finalPlayerAngleX;
									player->data.angle.z = m_finalPlayerAngleZ;
									playerCamera->Update();
									m_shouldDisableAnimCam = true;
								}
							}
							player->SetRotationZ(player->data.angle.z + ((mouseEvent->mouseInputX > 0 ? -1 : 1) * fRotationAmount));
							thirdPersonState->freeRotation.x -= ((mouseEvent->mouseInputX > 0 ? -1 : 1) * fRotationAmount);
							player->Update3DPosition(true);
						}
					}
				}
			}
			continue;
			case RE::INPUT_EVENT_TYPE::kThumbstick:
			{
				// disable animcam before moving in menu
				if (camera && m_shouldDisableAnimCam && !ui->GameIsPaused()) {
					auto idEvent = static_cast<RE::ButtonEvent*>(event);
					auto userEvents = RE::UserEvents::GetSingleton();
					if (idEvent->userEvent == userEvents->move) {
						auto thirdPersonState = static_cast<RE::ThirdPersonState*>(camera->currentState.get());
						if (player->AsActorState()->IsWeaponDrawn()) {
							thirdPersonState->freeRotation = m_finalFreeRotation;
							player->data.angle.x = m_finalPlayerAngleX;
							player->data.angle.z = m_finalPlayerAngleZ;
							player->Update3DPosition(true);
							camera->Update();
							thirdPersonState->freeRotationEnabled = true;
						}
						thirdPersonState->toggleAnimCam = false;

						// turning off AnimCam messes with zoom for some reason, so modify it again
						if (m_fixCameraZoom) {
							fVanityModeMinDist->data.f -= (17.0f + (66.6f * MenuOpenCloseEventHandler::fPitch));
							fVanityModeMaxDist->data.f -= (17.0f + (66.6f * MenuOpenCloseEventHandler::fPitch));
							m_fixCameraZoom = false;
						}

						m_shouldDisableAnimCam = false;
					}
				}
				// don't rotate if item preview is maximized
				RE::Inventory3DManager* inventory3DManager = RE::Inventory3DManager::GetSingleton();

				if (inventory3DManager->GetRuntimeData().zoomProgress == 0.0f) {
					if (MenuOpenCloseEventHandler::iGamepadTurnMethod == 0 && MenuOpenCloseEventHandler::bGamepadRotating) {
						auto stickEvent = reinterpret_cast<RE::ThumbstickEvent*>(event->AsIDEvent());
						auto playerCamera = RE::PlayerCamera::GetSingleton();

						if (stickEvent && playerCamera && stickEvent->IsRight()) {
							bool bMoving = player->AsActorState()->actorState1.movingBack ||
										   player->AsActorState()->actorState1.movingForward ||
										   player->AsActorState()->actorState1.movingRight ||
										   player->AsActorState()->actorState1.movingLeft;

							if (!bMoving) {
								auto thirdPersonState = static_cast<RE::ThirdPersonState*>(playerCamera->currentState.get());
								if (abs(stickEvent->yValue) > (2 * fTurnSensitivity))
									continue;
								if (player->AsActorState()->IsWeaponDrawn()) {
									if (!m_shouldDisableAnimCam && m_rotatedPlayer) {
										thirdPersonState->toggleAnimCam = true;
										thirdPersonState->freeRotationEnabled = true;
										thirdPersonState->freeRotation.x = m_finalFreeRotation.x;
										player->data.angle.x = m_finalPlayerAngleX;
										player->data.angle.z = m_finalPlayerAngleZ;
										playerCamera->Update();
										m_shouldDisableAnimCam = true;
									}
								}
								player->SetRotationZ(player->data.angle.z + ((stickEvent->xValue > 0 ? -1 : 1) * fRotationAmount));
								thirdPersonState->freeRotation.x -= ((stickEvent->xValue > 0 ? -1 : 1) * fRotationAmount);
								player->Update3DPosition(true);

								// change pitch with thumbstick
								/*if (!(abs(stickEvent->xValue) > 0.1f)) {
								player->data.angle.x += ((stickEvent->yValue > 0 ? -0.015f : 0.015f));
								}*/
							}
						}
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

	// collect original values for later
	m_playerAngleX = player->data.angle.x;
	m_freeRotation = thirdState->freeRotation;
	g_freeRotation = m_freeRotation;
	m_posOffsetExpected = thirdState->posOffsetExpected;
	m_radialBlurStrength = mod->radialBlur.strength;
	m_blurRadius = mod->blurRadius->floatValue;

	camera->SetState(thirdState);
	//camera->UpdateThirdPerson(player->AsActorState()->IsWeaponDrawn());

	// set over the shoulder camera values for when player has weapon drawn and unpaused menu(s) in order to prevent camera from snapping
	fOverShoulderCombatPosX = ini->GetSetting("fOverShoulderCombatPosX:Camera");
	fOverShoulderCombatAddY = ini->GetSetting("fOverShoulderCombatAddY:Camera");
	fOverShoulderCombatPosZ = ini->GetSetting("fOverShoulderCombatPosZ:Camera");
	fAutoVanityModeDelay = ini->GetSetting("fAutoVanityModeDelay:Camera");
	fOverShoulderPosX = ini->GetSetting("fOverShoulderPosX:Camera");
	fOverShoulderPosZ = ini->GetSetting("fOverShoulderPosZ:Camera");
	fVanityModeMinDist = ini->GetSetting("fVanityModeMinDist:Camera");
	fVanityModeMaxDist = ini->GetSetting("fVanityModeMaxDist:Camera");
	fMouseWheelZoomSpeed = ini->GetSetting("fMouseWheelZoomSpeed:Camera");
	m_fOverShoulderCombatPosX = fOverShoulderCombatPosX->GetFloat();
	m_fOverShoulderCombatAddY = fOverShoulderCombatAddY->GetFloat();
	m_fOverShoulderCombatPosZ = fOverShoulderCombatPosZ->GetFloat();
	m_fOverShoulderPosX = fOverShoulderPosX->GetFloat();
	m_fOverShoulderPosZ = fOverShoulderPosZ->GetFloat();
	m_fAutoVanityModeDelay = fAutoVanityModeDelay->GetFloat();
	m_fVanityModeMinDist = fVanityModeMinDist->GetFloat();
	m_fVanityModeMaxDist = fVanityModeMaxDist->GetFloat();
	m_fMouseWheelZoomSpeed = fMouseWheelZoomSpeed->GetFloat();
	m_worldFOV = camera->worldFOV;
	player->GetGraphVariableBool("IsNPC", m_playerHeadtrackingEnabled);

	// disable blur before opening menu so character is not obscured
	mod->radialBlur.strength = 0;
	mod->blurRadius->floatValue = 0;

	// temporarily disable headtracking if enabled
	player->SetGraphVariableBool("IsNPC", false);

	// toggle anim cam which unshackles camera and lets it move in front of player with their weapon drawn, necessary if not using TDM
	thirdState->toggleAnimCam = true;
	thirdState->freeRotationEnabled = true;

	// Check latest settings
	ReadFloatSetting(mcmOverride, "PositionSettings", "fXOffset", fXOffset);
	ReadFloatSetting(mcmOverride, "PositionSettings", "fYOffset", fYOffset);
	ReadFloatSetting(mcmOverride, "PositionSettings", "fZOffset", fZOffset);
	ReadFloatSetting(mcmOverride, "PositionSettings", "fPitch", fPitch);
	ReadFloatSetting(mcmOverride, "PositionSettings", "fRotation", fRotation);

	fAutoVanityModeDelay->data.f = 10800.0f;  // 3 hours

	m_fNewOverShoulderCombatPosX = -fXOffset - 75.0f;
	m_fNewOverShoulderCombatAddY = 0.f;
	m_fNewOverShoulderCombatPosZ = fZOffset - 50.0f;

	thirdState->freeRotation.x = MATH_PI + fRotation - 0.5f;

	// account for camera freeRotation settings getting pushed into player's pitch (x) values when weapon drawn
	thirdState->freeRotation.y = 0.0f;
	if (!player->AsActorState()->IsWeaponDrawn())
		player->data.angle.x = 0.2f + fPitch;
	else {
		player->data.angle.x -= player->data.angle.x - fPitch - 0.2f;
	}

	fOverShoulderCombatPosX->data.f = m_fNewOverShoulderCombatPosX;
	fOverShoulderCombatAddY->data.f = m_fNewOverShoulderCombatAddY;
	fOverShoulderCombatPosZ->data.f = m_fNewOverShoulderCombatPosZ;

	fOverShoulderPosX->data.f = m_fNewOverShoulderCombatPosX;
	fOverShoulderPosZ->data.f = m_fNewOverShoulderCombatPosZ;

	fVanityModeMinDist->data.f = 155.0f - fYOffset;
	fVanityModeMaxDist->data.f = 155.0f - fYOffset;

	// Skyrim Souls RE has the potential to mess with camera distance in menus after exiting, so when we restore it later on, make the transition suitably instant
	fMouseWheelZoomSpeed->data.f = 10000.0f;

	// make final camera distance not depend on camera pitch (looking down at player)
	thirdState->pitchZoomOffset = 0.1f;

	thirdState->posOffsetExpected = thirdState->posOffsetActual = RE::NiPoint3(m_fNewOverShoulderCombatPosX, m_fNewOverShoulderCombatAddY, m_fNewOverShoulderCombatPosZ);

	camera->worldFOV = 90.f;

	camera->Update();
}

void MenuOpenCloseEventHandler::RotatePlayer()
{
	auto camera = RE::PlayerCamera::GetSingleton();
	auto ini = RE::INISettingCollection::GetSingleton();
	auto player = RE::PlayerCharacter::GetSingleton();

	auto thirdState = (RE::ThirdPersonState*)camera->cameraStates[RE::CameraState::kThirdPerson].get();
	auto mod = RE::TESForm::LookupByID<RE::TESImageSpaceModifier>(0x000434BB);	// Image Space Adapter - ISMTween

	// collect original values for later
	m_playerAngleX = player->data.angle.x;
	m_targetZoomOffset = thirdState->targetZoomOffset;
	m_freeRotation = thirdState->freeRotation;
	g_freeRotation = m_freeRotation;
	m_posOffsetExpected = thirdState->posOffsetExpected;
	m_radialBlurStrength = mod->radialBlur.strength;
	m_blurRadius = mod->blurRadius->floatValue;

	camera->SetState(thirdState);
	//camera->UpdateThirdPerson(player->AsActorState()->IsWeaponDrawn());

	// set over the shoulder camera values for when player has weapon drawn and unpaused menu(s) in order to prevent camera from snapping
	fOverShoulderCombatPosX = ini->GetSetting("fOverShoulderCombatPosX:Camera");
	fOverShoulderCombatAddY = ini->GetSetting("fOverShoulderCombatAddY:Camera");
	fOverShoulderCombatPosZ = ini->GetSetting("fOverShoulderCombatPosZ:Camera");
	fAutoVanityModeDelay = ini->GetSetting("fAutoVanityModeDelay:Camera");
	fOverShoulderPosX = ini->GetSetting("fOverShoulderPosX:Camera");
	fOverShoulderPosZ = ini->GetSetting("fOverShoulderPosZ:Camera");
	fVanityModeMinDist = ini->GetSetting("fVanityModeMinDist:Camera");
	fVanityModeMaxDist = ini->GetSetting("fVanityModeMaxDist:Camera");
	fMouseWheelZoomSpeed = ini->GetSetting("fMouseWheelZoomSpeed:Camera");
	m_fOverShoulderCombatPosX = fOverShoulderCombatPosX->GetFloat();
	m_fOverShoulderCombatAddY = fOverShoulderCombatAddY->GetFloat();
	m_fOverShoulderCombatPosZ = fOverShoulderCombatPosZ->GetFloat();
	m_fOverShoulderPosX = fOverShoulderPosX->GetFloat();
	m_fOverShoulderPosZ = fOverShoulderPosZ->GetFloat();
	m_fAutoVanityModeDelay = fAutoVanityModeDelay->GetFloat();
	m_fVanityModeMinDist = fVanityModeMinDist->GetFloat();
	m_fVanityModeMaxDist = fVanityModeMaxDist->GetFloat();
	m_fMouseWheelZoomSpeed = fMouseWheelZoomSpeed->GetFloat();
	m_worldFOV = camera->worldFOV;
	player->GetGraphVariableBool("IsNPC", m_playerHeadtrackingEnabled);

	// disable blur before opening menu so character is not obscured
	mod->radialBlur.strength = 0;
	mod->blurRadius->floatValue = 0;

	// temporarily disable headtracking if enabled
	player->SetGraphVariableBool("IsNPC", false);

	// toggle anim cam which unshackles camera and lets it move in front of player with their weapon drawn, necessary if not using TDM
	thirdState->toggleAnimCam = true;
	m_shouldDisableAnimCam = true;
	thirdState->freeRotationEnabled = true;

	// Check latest settings
	ReadFloatSetting(mcmOverride, "PositionSettings", "fXOffset", fXOffset);
	ReadFloatSetting(mcmOverride, "PositionSettings", "fYOffset", fYOffset);
	ReadFloatSetting(mcmOverride, "PositionSettings", "fZOffset", fZOffset);
	ReadFloatSetting(mcmOverride, "PositionSettings", "fPitch", fPitch);
	ReadFloatSetting(mcmOverride, "PositionSettings", "fRotation", fRotation);

	// TDM Voodoo Magic
	auto playerPos = player->GetPosition();
	RE::NiPoint3 cameraPos = GetCameraPos();
	float currentCharacterYaw = player->data.angle.z;
	RE::NiPoint3 targetPos = player->GetLookingAtLocation();
	RE::NiPoint3 playerToTarget = RE::NiPoint3(targetPos.x - playerPos.x, targetPos.y - playerPos.y, targetPos.z - playerPos.z);
	RE::NiPoint3 playerDirectionToTarget = playerToTarget;
	playerDirectionToTarget.Unitize();
	RE::NiPoint3 cameraToPlayer = RE::NiPoint3(playerPos.x - cameraPos.x, playerPos.y - cameraPos.y, playerPos.z - cameraPos.z);
	RE::NiPoint3 projected = Project(cameraToPlayer, playerDirectionToTarget);
	RE::NiPoint3 projectedPos = RE::NiPoint3(projected.x + cameraPos.x, projected.y + cameraPos.y, projected.z + cameraPos.z);
	RE::NiPoint3 projectedDirectionToTarget = RE::NiPoint3(targetPos.x - projectedPos.x, targetPos.y - projectedPos.y, targetPos.z - projectedPos.z);
	projectedDirectionToTarget.Unitize();
	RE::NiPoint2 forwardVector(0.f, 1.f);
	RE::NiPoint2 currentCameraDirection = Vec2Rotate(forwardVector, currentCharacterYaw);
	RE::NiPoint2 projectedDirectionToTargetXY(-projectedDirectionToTarget.x, projectedDirectionToTarget.y);
	float angleDelta = GetAngle(currentCameraDirection, projectedDirectionToTargetXY);
	angleDelta = NormalRelativeAngle(angleDelta);
	player->data.angle.z += angleDelta;

	auto finalRotation = fRotation;

	if (m_thirdForced) {
		finalRotation -= 0.3f;
	} else {
		if (player->AsActorState()->IsWeaponDrawn()) {
			finalRotation += 0.1f;
			fYOffset -= 5.0f;
		}
	}
	float angleChange = MATH_PI + (finalRotation - 0.1f) - 0.4f;

	player->SetRotationZ(player->data.angle.z -= angleChange);

	// adding to freeRotation.x moves the camera to face more left
	// subtracting to freeRotation.x moves the camera to face more right

	// finesse the final camera rotation values
	thirdState->freeRotation.x = angleChange + 0.1f;
	if (!m_thirdForced && player->AsActorState()->IsWeaponDrawn()) {
		thirdState->freeRotation.x -= 0.2f;
	}

	if (m_thirdForced) {
		auto savCam = ShowPlayerInMenus::savedCameraState;
		player->data.angle.z = savCam.ConsumeX() - angleChange - 0.4f;
		thirdState->freeRotation.x += 0.24f;
	//	if (player->AsActorState()->IsWeaponDrawn()) {
	//		thirdState->freeRotation.x += 0.26f;
	//	}
		//fXOffset -= 20.0f;
		//fYOffset += 5.0f;
	}

	player->data.angle.x = 0.27f + fPitch;

	fAutoVanityModeDelay->data.f = 10800.0f;  // 3 hours

	m_fNewOverShoulderCombatPosX = -fXOffset - 75.0f;
	m_fNewOverShoulderCombatAddY = 0.0f;
	m_fNewOverShoulderCombatPosZ = fZOffset - 50.0f;

	fOverShoulderCombatPosX->data.f = m_fNewOverShoulderCombatPosX;
	fOverShoulderCombatAddY->data.f = m_fNewOverShoulderCombatAddY;
	fOverShoulderCombatPosZ->data.f = m_fNewOverShoulderCombatPosZ;

	fOverShoulderPosX->data.f = m_fNewOverShoulderCombatPosX;
	fOverShoulderPosZ->data.f = m_fNewOverShoulderCombatPosZ;

	fVanityModeMinDist->data.f = 150.0f - fYOffset;
	fVanityModeMaxDist->data.f = 150.0f - fYOffset;

	// Skyrim Souls RE has the potential to mess with camera distance in menus after exiting, so when we restore it later on, make the transition suitably instant
	fMouseWheelZoomSpeed->data.f = 10000.0f;

	// make final camera distance not depend on camera pitch (looking down at player)
	thirdState->freeRotation.y = 0.0f;
	thirdState->pitchZoomOffset = 0.1f;

	thirdState->posOffsetExpected = thirdState->posOffsetActual = RE::NiPoint3(m_fNewOverShoulderCombatPosX, m_fNewOverShoulderCombatAddY, m_fNewOverShoulderCombatPosZ);

	camera->worldFOV = 90.f;

	m_finalFreeRotation = thirdState->freeRotation;
	m_finalPlayerAngleX = player->data.angle.x;
	m_finalPlayerAngleZ = player->data.angle.z;
	m_rotatedPlayer = true;

	camera->Update();
	player->Update3DPosition(true);
}

void MenuOpenCloseEventHandler::ResetCamera()
{
	auto player = RE::PlayerCharacter::GetSingleton();
	
	auto camera = RE::PlayerCamera::GetSingleton();
	auto thirdState = (RE::ThirdPersonState*)camera->cameraStates[RE::CameraState::kThirdPerson].get();
	auto mod = RE::TESForm::LookupByID<RE::TESImageSpaceModifier>(0x000434BB);

	if (m_thirdForced) {
		//to cameraState = (RE::TESCameraState*)camera->cameraStates[m_camStateId].get();
		const auto firstPersonState = static_cast<RE::FirstPersonState*>(camera->cameraStates[RE::CameraState::kFirstPerson].get());
		camera->SetState(firstPersonState);
	}

	// restore original values
	player->data.angle.x = m_playerAngleX;
	player->data.angle.z = m_playerRotation;
	fAutoVanityModeDelay->data.f = m_fAutoVanityModeDelay;
	thirdState->toggleAnimCam = false;
	thirdState->targetZoomOffset = m_targetZoomOffset;
	thirdState->freeRotation = m_freeRotation;
	fVanityModeMinDist->data.f = m_fVanityModeMinDist;
	fVanityModeMaxDist->data.f = m_fVanityModeMaxDist;
	camera->worldFOV = m_worldFOV;
	thirdState->posOffsetExpected = thirdState->posOffsetActual = m_posOffsetExpected;
	fOverShoulderCombatPosX->data.f = m_fOverShoulderCombatPosX;
	fOverShoulderCombatAddY->data.f = m_fOverShoulderCombatAddY;
	fOverShoulderCombatPosZ->data.f = m_fOverShoulderCombatPosZ;
	fOverShoulderPosX->data.f = m_fOverShoulderPosX;
	fOverShoulderPosZ->data.f = m_fOverShoulderPosZ;
	mod->radialBlur.strength = m_radialBlurStrength;
	mod->blurRadius->floatValue = m_blurRadius;
	player->SetGraphVariableBool("IsNPC", m_playerHeadtrackingEnabled);

	m_thirdForced = false;

	camera->Update();

	// camera->Update() function uses this value, so restore it after we've updated the camera
	fMouseWheelZoomSpeed->data.f = m_fMouseWheelZoomSpeed;

	m_rotatedPlayer = false;
	m_inSitState = false;
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

	// set rotation settings
	auto player = RE::PlayerCharacter::GetSingleton();
	m_playerRotation = player->data.angle.z;
	ReadFloatSetting(mcmOverride, "PositionSettings", "fRotationAmount", fRotationAmount);
	ReadFloatSetting(mcmOverride, "PositionSettings", "fTurnSensitivity", fTurnSensitivity);
	ReadBoolSetting(mcmOverride, "GamepadSettings", "bEnableRotating", bGamepadRotating);
	ReadUint32Setting(mcmOverride, "GamepadSettings", "iGamepadTurnMethod", iGamepadTurnMethod);

	// SmoothCam compatibility
	if (g_SmoothCam && g_SmoothCam->IsCameraEnabled())
	{
		const auto result = g_SmoothCam->RequestCameraControl(g_pluginHandle);
		if (result == SmoothCamAPI::APIResult::OK || result == SmoothCamAPI::APIResult::AlreadyGiven)
		{
			g_SmoothCam->RequestInterpolatorUpdates(g_pluginHandle, true);
		}
	}

	ReadBoolSetting(mcmOverride, "ModeSettings", "bRotateCamera", bRotateCamera);
	ReadBoolSetting(mcmOverride, "ModeSettings", "bRotatePlayer", bRotatePlayer);
	m_fixCameraZoom = true;
	if (bRotatePlayer && !m_inSitState) {
		RotatePlayer();
	} else {
		RotateCamera();
	}
}

void MenuOpenCloseEventHandler::OnInventoryClose()
{
	if (!m_inMenu)
		return;

	// SmoothCam compatibility
	if (g_SmoothCam && g_SmoothCam->IsCameraEnabled())
	{
		//g_SmoothCam->RequestInterpolatorUpdates(g_pluginHandle, false);
		g_SmoothCam->ReleaseCameraControl(g_pluginHandle);
	}

	ResetCamera();

	m_inMenu = false;
	m_rotatedInTweenMenu = false;
	m_shouldDisableAnimCam = false;

	auto camera = RE::PlayerCamera::GetSingleton();
	auto thirdPersonState = static_cast<RE::ThirdPersonState*>(camera->currentState.get());
	if (thirdPersonState->toggleAnimCam)
		logger::info("OnInventoryClose(): thirdPersonState did not finish properly resetting.");
}

auto MenuOpenCloseEventHandler::ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	-> EventResult
{
	auto uiStr = RE::InterfaceStrings::GetSingleton();
	if (uiStr) {
		mcmOverride.SetUnicode();
		mcmOverride.LoadFile(mcmSettingsPath);

		auto& name = a_event->menuName;

		if (name == uiStr->inventoryMenu)
		{
			g_uiName = name;
			ReadBoolSetting(mcmOverride, "GeneralSettings", "bEnableInInventoryMenu", bEnableInInventoryMenu);

			if (a_event->opening) {
				if (!m_rotatedInTweenMenu && bEnableInInventoryMenu) {
					m_inMenu = true;
					OnInventoryOpen();
				} else if (m_rotatedInTweenMenu && !bEnableInInventoryMenu) {
					OnInventoryClose();
				}
			} else if (!a_event->opening && bEnableInInventoryMenu) {
				OnInventoryClose();
			}
		} else if (name == uiStr->containerMenu)
		{
			g_uiName = name;
			ReadBoolSetting(mcmOverride, "GeneralSettings", "bEnableInContainerMenu", bEnableInContainerMenu);

			if (a_event->opening) {
				if (!m_rotatedInTweenMenu && bEnableInContainerMenu) {
					m_inMenu = true;
					OnInventoryOpen();
				} else if (m_rotatedInTweenMenu && !bEnableInContainerMenu) {
					OnInventoryClose();
				}
			} else if (!a_event->opening && bEnableInContainerMenu) {
				OnInventoryClose();
			}
		} else if (name == uiStr->barterMenu) {
			g_uiName = name;
			ReadBoolSetting(mcmOverride, "GeneralSettings", "bEnableInBarterMenu", bEnableInBarterMenu);

			if (bEnableInBarterMenu) {
				altCam.SetUnicode();
				altCam.LoadFile(altCamPath);
				ReadBoolSetting(altCam, "Settings", "bSwitchTarget", bAltCamSwitchTarget);
				if (!bAltCamSwitchTarget) {
					if (a_event->opening) {
						m_inMenu = true;
						OnInventoryOpen();
					} else
						OnInventoryClose();
				}
			}
		} else if (name == uiStr->magicMenu)
		{
			g_uiName = name;
			ReadBoolSetting(mcmOverride, "GeneralSettings", "bEnableInMagicMenu", bEnableInMagicMenu);

			if (a_event->opening) {
				if (!m_rotatedInTweenMenu && bEnableInMagicMenu) {
					m_inMenu = true;
					OnInventoryOpen();
				} else if (m_rotatedInTweenMenu && !bEnableInMagicMenu) {
					OnInventoryClose();
				}
			} else if (!a_event->opening && bEnableInMagicMenu) {
				OnInventoryClose();
			}
		} else if (name == uiStr->tweenMenu) {
			if (m_camStateId == RE::CameraState::kFirstPerson) {
				m_thirdForced = true;
			}
			g_uiName = name;
			ReadBoolSetting(mcmOverride, "GeneralSettings", "bEnableInTweenMenu", bEnableInTweenMenu);

			if (bEnableInTweenMenu) {
				m_inMenu = true;

				if (a_event->opening) {
					m_rotatedInTweenMenu = true;
					OnInventoryOpen();
				} else {
					OnInventoryClose();
				}
			}
		} 

	}
	return EventResult::kContinue;
}

bool MenuOpenCloseEventHandler::CheckOptions()
{
	auto player = RE::PlayerCharacter::GetSingleton();
	auto camera = RE::PlayerCamera::GetSingleton();

	auto sitSleepState = player->AsActorState()->GetSitSleepState();

	// check if sitting
	if (sitSleepState >= RE::SIT_SLEEP_STATE::kWantToSit &&
		sitSleepState <= RE::SIT_SLEEP_STATE::kWantToStand) {
		m_inSitState = true;
		ReadBoolSetting(mcmOverride, "ConditionalSettings", "bEnableWhileSitting", bEnableSitting);
		if (!bEnableSitting) {
			m_inMenu = false;
			return true;
		}
	}

	// disable while in first person
	if (m_camStateId == RE::CameraState::kFirstPerson || m_thirdForced) {
		ReadBoolSetting(mcmOverride, "ConditionalSettings", "bEnableFirstPerson", bEnableFirstPerson);
		if (!bEnableFirstPerson) {
			m_inMenu = false;
			return true;
		}
		m_thirdForced = true;
	}

	// disable while sleeping
	if (sitSleepState >= RE::SIT_SLEEP_STATE::kWantToSleep) {
		m_inMenu = false;
		return true;
	}
	
	// disable on mount
	if (player->IsOnMount()) {
		m_inMenu = false;
		return true;
	}

	// disable in combat
	if (player->IsInCombat()) {
		ReadBoolSetting(mcmOverride, "ConditionalSettings", "bEnableDuringCombat", bEnableCombat);
		if (!bEnableCombat) {
			m_inMenu = false;
			return true;
		}
	}

	// disable while moving
	bool bMoving = player->AsActorState()->actorState1.movingBack ||
				   player->AsActorState()->actorState1.movingForward ||
				   player->AsActorState()->actorState1.movingRight ||
				   player->AsActorState()->actorState1.movingLeft;

	if (bMoving) {
		ReadBoolSetting(mcmOverride, "ConditionalSettings", "bEnableWhileMoving", bEnableMoving);
		if (!bEnableMoving) {
			m_inMenu = false;
			return true;
		}
	}

	// disable while auto moving
	auto controls = RE::PlayerControls::GetSingleton();
	if (controls->data.autoMove) {
		ReadBoolSetting(mcmOverride, "ConditionalSettings", "bEnableWhileAutoMoving", bEnableAutoMoving);
		if (!bEnableAutoMoving) {
			m_inMenu = false;
			return true;
		}
	}

	// disable in free camera mode (TFC)
	if (camera->IsInFreeCameraMode()) {
		m_inMenu = false;
		return true;
	}

	return false;
}

void MenuOpenCloseEventHandler::ReadDefaultMCMSettings()
{
	mcmDefault.SetUnicode();
	mcmDefault.LoadFile(defaultSettingsPath);

	ReadBoolSetting(mcmDefault, "GeneralSettings", "bEnableInInventoryMenu", bEnableInInventoryMenu);
	ReadBoolSetting(mcmDefault, "GeneralSettings", "bEnableInContainerMenu", bEnableInContainerMenu);
	ReadBoolSetting(mcmDefault, "GeneralSettings", "bEnableInBarterMenu", bEnableInBarterMenu);
	ReadBoolSetting(mcmDefault, "GeneralSettings", "bEnableInMagicMenu", bEnableInMagicMenu);
	ReadBoolSetting(mcmDefault, "GeneralSettings", "bEnableInTweenMenu", bEnableInTweenMenu);

	ReadBoolSetting(mcmDefault, "ModeSettings", "bRotateCamera", bRotateCamera);
	ReadBoolSetting(mcmDefault, "ModeSettings", "bRotatePlayer", bRotatePlayer);

	ReadBoolSetting(mcmDefault, "ConditionalSettings", "bEnableFirstPerson", bEnableFirstPerson);
	ReadBoolSetting(mcmDefault, "ConditionalSettings", "bEnableWhileMoving", bEnableMoving);
	ReadBoolSetting(mcmDefault, "ConditionalSettings", "bEnableWhileSitting", bEnableSitting);
	ReadBoolSetting(mcmDefault, "ConditionalSettings", "bEnableDuringCombat", bEnableCombat);
	ReadBoolSetting(mcmDefault, "ConditionalSettings", "bEnableWhileAutoMoving", bEnableAutoMoving);

	ReadFloatSetting(mcmDefault, "PositionSettings", "fXOffset", fXOffset);
	ReadFloatSetting(mcmDefault, "PositionSettings", "fYOffset", fYOffset);
	ReadFloatSetting(mcmDefault, "PositionSettings", "fZOffset", fZOffset);
	ReadFloatSetting(mcmDefault, "PositionSettings", "fPitch", fPitch);
	ReadFloatSetting(mcmDefault, "PositionSettings", "fRotation", fRotation);
	ReadFloatSetting(mcmDefault, "PositionSettings", "fRotationAmount", fRotationAmount);
	ReadFloatSetting(mcmDefault, "PositionSettings", "fTurnSensitivity", fTurnSensitivity);

	ReadBoolSetting(mcmDefault, "GamepadSettings", "bEnableRotating", bGamepadRotating);
	ReadUint32Setting(mcmDefault, "GamepadSettings", "iGamepadTurnMethod", iGamepadTurnMethod);
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

RE::BSEventNotifyControl Item3DControls::ProcessEvent_Hook(RE::InputEvent** a_event, RE::BSTEventSource<RE::InputEvent*>* a_source)
{
	// Disable rotating items when controller turning is enabled
	if (a_event && *a_event && MenuOpenCloseEventHandler::bGamepadRotating) {
		for (RE::InputEvent* evn = *a_event; evn; evn = evn->next) {
			if (evn && evn->HasIDCode()) {
				RE::UserEvents* userEvents = RE::UserEvents::GetSingleton();
				RE::IDEvent* idEvent = static_cast<RE::ButtonEvent*>(evn);

				RE::Inventory3DManager* inventory3DManager = RE::Inventory3DManager::GetSingleton();
				if (m_inMenu && idEvent && idEvent->userEvent == userEvents->rotate && inventory3DManager && inventory3DManager->GetRuntimeData().zoomProgress == 0.0f) {
					idEvent->userEvent = "";
				}
			}
		}
	}

	return _ProcessEvent(this, a_event, a_source);
}

void Item3DControls::InstallHook()
{
	REL::Relocation<std::uintptr_t> MenuControlsVtbl{ RE::VTABLE_MenuControls[0] };
	_ProcessEvent = MenuControlsVtbl.write_vfunc(0x1, &ProcessEvent_Hook);
}

//constexpr uint32_t hash(const std::string_view data) noexcept {
//	uint32_t hash = 5385;
//
//	for (const auto &e : data)
//		hash = ((hash << 5) + hash) + e;
//
//	return hash;
//}
