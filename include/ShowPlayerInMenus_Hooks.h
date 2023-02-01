#pragma once

#include "Event.h"

#ifdef _MSC_VER
#undef GetObject
#endif

namespace ShowPlayerInMenus
{
	struct SavedCamera
	{
		enum class RotationType : uint8_t
		{
			kNone,
			kFirstPerson,
			kThirdPerson,
			kTween
		};
		RotationType rotationType = RotationType::kNone;
		RE::NiPoint2 rotation{ 0.f, 0.f };

		void SaveX(float a_x, RotationType a_rotationType)
		{
			rotation.x = a_x;
			rotationType = a_rotationType;
		}

		float ConsumeX()
		{
			rotationType = RotationType::kNone;
			return rotation.x;
		}
	} savedCameraState;

	class ShowPlayerInMenusHook
	{
	public:
		static void InstallHook()
		{
			REL::Relocation<std::uintptr_t> func{ REL::VariantID(50884, 51757, 50884) };	// UpdateItem3D
			//REL::Relocation<std::uintptr_t> func2{ REL::VariantID(50885, 51758, 50885) };	// UpdateMagic3D
			//REL::Relocation<std::uintptr_t> hook2{ RELOCATION_ID(36365, 37356) };  // 5D87F0, 5FD7E0 SetRotationZ

			auto& trampoline = SKSE::GetTrampoline();
			trampoline.write_branch<5>(func.address(), &UpdateItem3D);
			//OriginalSetRotationZ = trampoline.write_call<5>(hook2.address() + RELOCATION_OFFSET(0x9C7, 0xA87), Actor_SetRotationZ);	 // 5D91B7
			//trampoline.write_branch<5>(func2.address(), &UpdateMagic3D);
		}

		/* void UpdateMagic3D(RE::TESForm* a_form, std::uint32_t a_arg2)
		{
			constexpr auto mcmSettingsPath = L"Data/MCM/Settings/ShowPlayerInMenus.ini";
			
			CSimpleIniA mcm;
			mcm.SetUnicode();
			mcm.LoadFile(mcmSettingsPath);

			ReadBoolSetting(mcm, "MagicMenuSettings", "bHideSpell3D", bHideSpell3D);
			if (bHideSpell3D) {
				return;
			}

			_UpdateMagic3D(RE::TESForm * a_form, std::uint32_t a_arg2);
			//if (a_form->GetFormType() == RE::FormType::Spell) {

			//}
		}*/

		/* void Actor_SetRotationZ(RE::Actor* a_this, float a_angle)
		{
			if (a_this->IsPlayerRef()) {
				auto thirdPersonState = static_cast<RE::ThirdPersonState*>(RE::PlayerCamera::GetSingleton()->cameraStates[RE::CameraState::kThirdPerson].get());
				if (RE::PlayerCamera::GetSingleton()->currentState.get() == thirdPersonState && thirdPersonState->freeRotationEnabled) {
					float angleDelta = a_angle - a_this->data.angle.z;
					thirdPersonState->freeRotation.x -= angleDelta;
				}
			}

			_Actor_SetRotationZ(a_this, a_angle);
		}*/

		void UpdateItem3D(RE::InventoryEntryData* a_objDesc)
		{
			if (!a_objDesc)
				return;

			auto obj = a_objDesc->GetObject();
			if (!obj)
				return;

			auto manager = RE::Inventory3DManager::GetSingleton();

			constexpr auto defaultSettingsPath = L"Data/MCM/Config/ShowPlayerInMenus/settings.ini";
			constexpr auto mcmSettingsPath = L"Data/MCM/Settings/ShowPlayerInMenus.ini";

			CSimpleIniA mcmDefault;
			CSimpleIniA mcmOverride;
			mcmDefault.SetUnicode();
			mcmDefault.LoadFile(defaultSettingsPath);
			mcmOverride.SetUnicode();
			mcmOverride.LoadFile(mcmSettingsPath);

			if (RE::BSFixedString("InventoryMenu") == g_uiName)
			{
				switch (obj->GetFormType())
				{
					case RE::FormType::AlchemyItem:
						ReadBoolSetting(mcmDefault, "InventoryMenuSettings", "bHideAlchemyItem3D", bInventoryHideAlchemyItem3D);
						ReadBoolSetting(mcmOverride, "InventoryMenuSettings", "bHideAlchemyItem3D", bInventoryHideAlchemyItem3D);
						if (bInventoryHideAlchemyItem3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
					case RE::FormType::Armor:
						ReadBoolSetting(mcmDefault, "InventoryMenuSettings", "bHideArmor3D", bInventoryHideArmor3D);
						ReadBoolSetting(mcmOverride, "InventoryMenuSettings", "bHideArmor3D", bInventoryHideArmor3D);
						if (bInventoryHideArmor3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
					case RE::FormType::Ammo:
						ReadBoolSetting(mcmDefault, "InventoryMenuSettings", "bHideAmmo3D", bInventoryHideAmmo3D);
						ReadBoolSetting(mcmOverride, "InventoryMenuSettings", "bHideAmmo3D", bInventoryHideAmmo3D);
						if (bInventoryHideAmmo3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
					case RE::FormType::Book:
					case RE::FormType::Scroll:
						ReadBoolSetting(mcmDefault, "InventoryMenuSettings", "bHideBooksScrolls3D", bInventoryHideBooksScrolls3D);
						ReadBoolSetting(mcmOverride, "InventoryMenuSettings", "bHideBooksScrolls3D", bInventoryHideBooksScrolls3D);
						if (bInventoryHideBooksScrolls3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
					case RE::FormType::Light:
					case RE::FormType::Misc:
						ReadBoolSetting(mcmDefault, "InventoryMenuSettings", "bHideMisc3D", bInventoryHideMisc3D);
						ReadBoolSetting(mcmOverride, "InventoryMenuSettings", "bHideMisc3D", bInventoryHideMisc3D);
						if (bInventoryHideMisc3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
					case RE::FormType::Weapon:
						ReadBoolSetting(mcmDefault, "InventoryMenuSettings", "bHideWeapon3D", bInventoryHideWeapon3D);
						ReadBoolSetting(mcmOverride, "InventoryMenuSettings", "bHideWeapon3D", bInventoryHideWeapon3D);
						if (bInventoryHideWeapon3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
					default:
						manager->UpdateMagic3D(obj, 0);
						break;
				}
			} else if (RE::BSFixedString("ContainerMenu") == g_uiName)
			{
				switch (obj->GetFormType()) {
				case RE::FormType::AlchemyItem:
						ReadBoolSetting(mcmDefault, "ContainerMenuSettings", "bHideAlchemyItem3D", bContainerHideAlchemyItem3D);
						ReadBoolSetting(mcmOverride, "ContainerMenuSettings", "bHideAlchemyItem3D", bContainerHideAlchemyItem3D);
						if (bContainerHideAlchemyItem3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				case RE::FormType::Armor:
						ReadBoolSetting(mcmDefault, "ContainerMenuSettings", "bHideArmor3D", bContainerHideArmor3D);
						ReadBoolSetting(mcmOverride, "ContainerMenuSettings", "bHideArmor3D", bContainerHideArmor3D);
						if (bContainerHideArmor3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				case RE::FormType::Ammo:
						ReadBoolSetting(mcmDefault, "ContainerMenuSettings", "bHideAmmo3D", bContainerHideAmmo3D);
						ReadBoolSetting(mcmOverride, "ContainerMenuSettings", "bHideAmmo3D", bContainerHideAmmo3D);
						if (bContainerHideAmmo3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				case RE::FormType::Book:
				case RE::FormType::Scroll:
						ReadBoolSetting(mcmDefault, "ContainerMenuSettings", "bHideBooksScrolls3D", bContainerHideBooksScrolls3D);
						ReadBoolSetting(mcmOverride, "ContainerMenuSettings", "bHideBooksScrolls3D", bContainerHideBooksScrolls3D);
						if (bContainerHideBooksScrolls3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				case RE::FormType::Light:
				case RE::FormType::Misc:
						ReadBoolSetting(mcmDefault, "ContainerMenuSettings", "bHideMisc3D", bContainerHideMisc3D);
						ReadBoolSetting(mcmOverride, "ContainerMenuSettings", "bHideMisc3D", bContainerHideMisc3D);
						if (bContainerHideMisc3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				case RE::FormType::Weapon:
						ReadBoolSetting(mcmDefault, "ContainerMenuSettings", "bHideWeapon3D", bContainerHideWeapon3D);
						ReadBoolSetting(mcmOverride, "ContainerMenuSettings", "bHideWeapon3D", bContainerHideWeapon3D);
						if (bContainerHideWeapon3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				default:
						manager->UpdateMagic3D(obj, 0);
						break;
				}
			} else if (RE::BSFixedString("BarterMenu") == g_uiName) {
				switch (obj->GetFormType()) {
				case RE::FormType::AlchemyItem:
						ReadBoolSetting(mcmDefault, "BarterMenuSettings", "bHideAlchemyItem3D", bBarterHideAlchemyItem3D);
						ReadBoolSetting(mcmOverride, "BarterMenuSettings", "bHideAlchemyItem3D", bBarterHideAlchemyItem3D);
						if (bBarterHideAlchemyItem3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				case RE::FormType::Armor:
						ReadBoolSetting(mcmDefault, "BarterMenuSettings", "bHideArmor3D", bBarterHideArmor3D);
						ReadBoolSetting(mcmOverride, "BarterMenuSettings", "bHideArmor3D", bBarterHideArmor3D);
						if (bBarterHideArmor3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				case RE::FormType::Ammo:
						ReadBoolSetting(mcmDefault, "BarterMenuSettings", "bHideAmmo3D", bBarterHideAmmo3D);
						ReadBoolSetting(mcmOverride, "BarterMenuSettings", "bHideAmmo3D", bBarterHideAmmo3D);
						if (bBarterHideAmmo3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				case RE::FormType::Book:
				case RE::FormType::Scroll:
						ReadBoolSetting(mcmDefault, "BarterMenuSettings", "bHideBooksScrolls3D", bBarterHideBooksScrolls3D);
						ReadBoolSetting(mcmOverride, "BarterMenuSettings", "bHideBooksScrolls3D", bBarterHideBooksScrolls3D);
						if (bBarterHideBooksScrolls3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				case RE::FormType::Light:
				case RE::FormType::Misc:
						ReadBoolSetting(mcmDefault, "BarterMenuSettings", "bHideMisc3D", bBarterHideMisc3D);
						ReadBoolSetting(mcmOverride, "BarterMenuSettings", "bHideMisc3D", bBarterHideMisc3D);
						if (bBarterHideMisc3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				case RE::FormType::Weapon:
						ReadBoolSetting(mcmDefault, "BarterMenuSettings", "bHideWeapon3D", bBarterHideWeapon3D);
						ReadBoolSetting(mcmOverride, "BarterMenuSettings", "bHideWeapon3D", bBarterHideWeapon3D);
						if (bBarterHideWeapon3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				default:
						manager->UpdateMagic3D(obj, 0);
						break;
				}
			} else
			{
				manager->UpdateMagic3D(obj, 0);
			}

		}

		void ReadBoolSetting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, bool& a_setting)
		{
			const char* bFound = nullptr;
			bFound = a_ini.GetValue(a_sectionName, a_settingName);
			if (bFound) {
				a_setting = a_ini.GetBoolValue(a_sectionName, a_settingName);
			}
		}

		static inline bool bInventoryHideAlchemyItem3D = false;
		static inline bool bInventoryHideArmor3D = false;
		static inline bool bInventoryHideAmmo3D = false;
		static inline bool bInventoryHideBooksScrolls3D = false;
		static inline bool bInventoryHideMisc3D = false;
		static inline bool bInventoryHideWeapon3D = false;
		static inline bool bContainerHideAlchemyItem3D = false;
		static inline bool bContainerHideArmor3D = false;
		static inline bool bContainerHideAmmo3D = false;
		static inline bool bContainerHideBooksScrolls3D = false;
		static inline bool bContainerHideMisc3D = false;
		static inline bool bContainerHideWeapon3D = false;
		static inline bool bBarterHideAlchemyItem3D = false;
		static inline bool bBarterHideArmor3D = false;
		static inline bool bBarterHideAmmo3D = false;
		static inline bool bBarterHideBooksScrolls3D = false;
		static inline bool bBarterHideMisc3D = false;
		static inline bool bBarterHideWeapon3D = false;
		static inline bool bHideSpell3D = false;
	};

	class FirstPersonStateHook
	{
	public:
		static void InstallHook()
		{
			REL::Relocation<std::uintptr_t> FirstPersonStateVtbl{ RE::VTABLE_FirstPersonState[0] };
			_OnExitState = FirstPersonStateVtbl.write_vfunc(0x2, OnExitState);
		}

		static void OnExitState(RE::FirstPersonState* a_this)
		{
			auto playerCharacter = RE::PlayerCharacter::GetSingleton();

			if (playerCharacter) {
				savedCameraState.SaveX(playerCharacter->data.angle.z, SavedCamera::RotationType::kFirstPerson);
			}

			_OnExitState(a_this);
		}

	private:
		static inline REL::Relocation<decltype(OnExitState)> _OnExitState;
	};

	class ThirdPersonStateHook
	{
	public:
		static void InstallHook()
		{
			REL::Relocation<std::uintptr_t> ThirdPersonStateVtbl{ RE::VTABLE_ThirdPersonState[0] };
			_SetFreeRotationMode = ThirdPersonStateVtbl.write_vfunc(0xD, SetFreeRotationMode);
		}

		static void SetFreeRotationMode(RE::ThirdPersonState* a_this, bool a_weaponSheathed)
		{
			//auto directionalMovementHandler = DirectionalMovementHandler::GetSingleton();

			//directionalMovementHandler->Update();

			//if (directionalMovementHandler->IsFreeCamera()) {
				RE::Actor* cameraTarget = nullptr;
				cameraTarget = static_cast<RE::PlayerCamera*>(a_this->camera)->cameraTarget.get().get();

				//bool bHasTargetLocked = directionalMovementHandler->HasTargetLocked();

				if (cameraTarget) {
						a_this->freeRotationEnabled = true;
						//directionalMovementHandler->UpdateAIProcessRotationSpeed(cameraTarget);	 // because the game is skipping the original call while in freecam

						//if (!bHasTargetLocked) {
							auto actorState = cameraTarget->AsActorState();

							float pitchDelta = -a_this->freeRotation.y;

							bool bMoving = actorState->actorState1.movingBack ||
										   actorState->actorState1.movingForward ||
										   actorState->actorState1.movingRight ||
										   actorState->actorState1.movingLeft;

							if (bMoving || !a_weaponSheathed) {
								//cameraTarget->SetRotationX(cameraTarget->data.angle.x + pitchDelta);
								cameraTarget->data.angle.x += pitchDelta;
								a_this->freeRotation.y += pitchDelta;
							}
						//}
				}
			//} else {
				_SetFreeRotationMode(a_this, a_weaponSheathed);
			//}
		}

	private:
		static inline REL::Relocation<decltype(SetFreeRotationMode)> _SetFreeRotationMode;
	};

	class Actor_SetRotationHook
	{
	public:
		static void InstallHook()
		{
			auto& trampoline = SKSE::GetTrampoline();
			REL::Relocation<uintptr_t> hook1{ RELOCATION_ID(32042, 32796) };  // 4EC300, 504B30  // synchronized anims
			REL::Relocation<uintptr_t> hook2{ RELOCATION_ID(36365, 37356) };  // 5D87F0, 5FD7E0

			_Actor_SetRotationX = trampoline.write_call<5>(hook1.address() + RELOCATION_OFFSET(0x4DC, 0x667), Actor_SetRotationX);	// 4EC7DC
			_Actor_SetRotationZ = trampoline.write_call<5>(hook2.address() + RELOCATION_OFFSET(0x9C7, 0xA87), Actor_SetRotationZ);	// 5D91B7
		}

		static void Actor_SetRotationX(RE::Actor* a_this, float a_angle)
		{
			if (a_this->IsPlayerRef()) {
				auto thirdPersonState = static_cast<RE::ThirdPersonState*>(RE::PlayerCamera::GetSingleton()->cameraStates[RE::CameraState::kThirdPerson].get());
				if (RE::PlayerCamera::GetSingleton()->currentState.get() == thirdPersonState && thirdPersonState->freeRotationEnabled) {
						float angleDelta = a_angle - a_this->data.angle.x;
						thirdPersonState->freeRotation.y += angleDelta;
				}
			}

			_Actor_SetRotationX(a_this, a_angle);
		}

		static void Actor_SetRotationZ(RE::Actor* a_this, float a_angle)
		{
			if (a_this->IsPlayerRef()) {
				auto thirdPersonState = static_cast<RE::ThirdPersonState*>(RE::PlayerCamera::GetSingleton()->cameraStates[RE::CameraState::kThirdPerson].get());
				if (RE::PlayerCamera::GetSingleton()->currentState.get() == thirdPersonState && thirdPersonState->freeRotationEnabled) {
						float angleDelta = a_angle - a_this->data.angle.z;
						thirdPersonState->freeRotation.x -= angleDelta;
				}
			}

			_Actor_SetRotationZ(a_this, a_angle);
		}

	private:
		static inline REL::Relocation<decltype(Actor_SetRotationX)> _Actor_SetRotationX;
		static inline REL::Relocation<decltype(Actor_SetRotationZ)> _Actor_SetRotationZ;
	};
}
