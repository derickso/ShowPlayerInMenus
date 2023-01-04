#pragma once

#include "Event.h"

#ifdef _MSC_VER
#undef GetObject
#endif

namespace ShowPlayerInMenus
{
	class ShowPlayerInMenusHook
	{
	public:
		static void InstallHook()
		{
			SKSE::AllocTrampoline(1 << 4);

			REL::Relocation<std::uintptr_t> func{ REL::VariantID(50884, 51757, 50884) };	// UpdateItem3D
			//REL::Relocation<std::uintptr_t> func2{ REL::VariantID(50885, 51758, 50885) };	// UpdateMagic3D

			auto& trampoline = SKSE::GetTrampoline();
			trampoline.write_branch<5>(func.address(), &UpdateItem3D);
			//trampoline.write_branch<5>(func2.address(), &UpdateMagic3D);
		}

//		void UpdateMagic3D(RE::TESForm* a_form, std::uint32_t a_arg2)
//		{
//			if (a_form->GetFormType() == RE::FormType::Spell) {
//
//			}
//		}

		void UpdateItem3D(RE::InventoryEntryData* a_objDesc)
		{
			if (!a_objDesc)
				return;

			auto obj = a_objDesc->GetObject();
			if (!obj)
				return;

			auto manager = RE::Inventory3DManager::GetSingleton();

			constexpr auto mcmSettingsPath = L"Data/MCM/Settings/ShowPlayerInMenus.ini";

			CSimpleIniA mcm;
			mcm.SetUnicode();
			mcm.LoadFile(mcmSettingsPath);

			if (RE::BSFixedString("InventoryMenu") == g_uiName)
			{
				switch (obj->GetFormType())
				{
					case RE::FormType::AlchemyItem:
						ReadBoolSetting(mcm, "InventoryMenuSettings", "bHideAlchemyItem3D", bInventoryHideAlchemyItem3D);
						if (bInventoryHideAlchemyItem3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
					case RE::FormType::Armor:
						ReadBoolSetting(mcm, "InventoryMenuSettings", "bHideArmor3D", bInventoryHideArmor3D);
						if (bInventoryHideArmor3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
					case RE::FormType::Ammo:
						ReadBoolSetting(mcm, "InventoryMenuSettings", "bHideAmmo3D", bInventoryHideAmmo3D);
						if (bInventoryHideAmmo3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
					case RE::FormType::Book:
					case RE::FormType::Scroll:
						ReadBoolSetting(mcm, "InventoryMenuSettings", "bHideBooksScrolls3D", bInventoryHideBooksScrolls3D);
						if (bInventoryHideBooksScrolls3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
					case RE::FormType::Misc:
						ReadBoolSetting(mcm, "InventoryMenuSettings", "bHideMisc3D", bInventoryHideMisc3D);
						if (bInventoryHideMisc3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
					case RE::FormType::Weapon:
						ReadBoolSetting(mcm, "InventoryMenuSettings", "bHideWeapon3D", bInventoryHideWeapon3D);
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
						ReadBoolSetting(mcm, "ContainerMenuSettings", "bHideAlchemyItem3D", bContainerHideAlchemyItem3D);
						if (bContainerHideAlchemyItem3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				case RE::FormType::Armor:
						ReadBoolSetting(mcm, "ContainerMenuSettings", "bHideArmor3D", bContainerHideArmor3D);
						if (bContainerHideArmor3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				case RE::FormType::Ammo:
						ReadBoolSetting(mcm, "ContainerMenuSettings", "bHideAmmo3D", bContainerHideAmmo3D);
						if (bContainerHideAmmo3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				case RE::FormType::Book:
				case RE::FormType::Scroll:
						ReadBoolSetting(mcm, "ContainerMenuSettings", "bHideBooksScrolls3D", bContainerHideBooksScrolls3D);
						if (bContainerHideBooksScrolls3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				case RE::FormType::Misc:
						ReadBoolSetting(mcm, "ContainerMenuSettings", "bHideMisc3D", bContainerHideMisc3D);
						if (bContainerHideMisc3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				case RE::FormType::Weapon:
						ReadBoolSetting(mcm, "ContainerMenuSettings", "bHideWeapon3D", bContainerHideWeapon3D);
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
						ReadBoolSetting(mcm, "BarterMenuSettings", "bHideAlchemyItem3D", bBarterHideAlchemyItem3D);
						if (bBarterHideAlchemyItem3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				case RE::FormType::Armor:
						ReadBoolSetting(mcm, "BarterMenuSettings", "bHideArmor3D", bBarterHideArmor3D);
						if (bBarterHideArmor3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				case RE::FormType::Ammo:
						ReadBoolSetting(mcm, "BarterMenuSettings", "bHideAmmo3D", bBarterHideAmmo3D);
						if (bBarterHideAmmo3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				case RE::FormType::Book:
				case RE::FormType::Scroll:
						ReadBoolSetting(mcm, "BarterMenuSettings", "bHideBooksScrolls3D", bBarterHideBooksScrolls3D);
						if (bBarterHideBooksScrolls3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				case RE::FormType::Misc:
						ReadBoolSetting(mcm, "BarterMenuSettings", "bHideMisc3D", bBarterHideMisc3D);
						if (bBarterHideMisc3D) {
							manager->Clear3D();
							break;
						}
						manager->UpdateMagic3D(obj, 0);
						break;
				case RE::FormType::Weapon:
						ReadBoolSetting(mcm, "BarterMenuSettings", "bHideWeapon3D", bBarterHideWeapon3D);
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

		static inline bool bInventoryHideAlchemyItem3D = true;
		static inline bool bInventoryHideArmor3D = true;
		static inline bool bInventoryHideAmmo3D = true;
		static inline bool bInventoryHideBooksScrolls3D = true;
		static inline bool bInventoryHideMisc3D = true;
		static inline bool bInventoryHideWeapon3D = true;
		static inline bool bContainerHideAlchemyItem3D = true;
		static inline bool bContainerHideArmor3D = true;
		static inline bool bContainerHideAmmo3D = true;
		static inline bool bContainerHideBooksScrolls3D = true;
		static inline bool bContainerHideMisc3D = true;
		static inline bool bContainerHideWeapon3D = true;
		static inline bool bBarterHideAlchemyItem3D = true;
		static inline bool bBarterHideArmor3D = true;
		static inline bool bBarterHideAmmo3D = true;
		static inline bool bBarterHideBooksScrolls3D = true;
		static inline bool bBarterHideMisc3D = true;
		static inline bool bBarterHideWeapon3D = true;
	};


}
