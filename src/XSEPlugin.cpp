#include "ShowPlayerInMenus_Hooks.h"
#include "Version.h"
#include "Event.h"
#include "LoadGameFunc.h"

void Init()
{
	auto g_message = SKSE::GetMessagingInterface();

	if (!g_message) {
		logger::error("Messaging Interface Not Found!");
	}

	if (g_message->RegisterListener(ShowPlayerInMenusNamespace::EventCallback)) {
		logger::info("Register Event Call Back!");
	}
}

void InitializeLog()
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		util::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format("{}.log"sv, Plugin::NAME);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

#ifndef NDEBUG
	const auto level = spdlog::level::trace;
#else
	const auto level = spdlog::level::info;
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
	log->set_level(level);
	log->flush_on(level);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%l] %v"s);
}

EXTERN_C [[maybe_unused]] __declspec(dllexport) bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();

	logger::info("ShowPlayerInMenus loaded"sv);

	SKSE::Init(a_skse);

	Init();

	g_plugin_handle = a_skse->GetPluginHandle();
	g_messaging = reinterpret_cast<SKSE::MessagingInterface*>(a_skse->QueryInterface(a_skse->kMessaging));
	
	ShowPlayerInMenus::ShowPlayerInMenusHook::InstallHook();

	return true;
}

EXTERN_C [[maybe_unused]] __declspec(dllexport) constinit auto SKSEPlugin_Version = []() noexcept {
	SKSE::PluginVersionData v;
	v.PluginName("ShowPlayerInMenus");
	v.PluginVersion({ 1, 0, 0, 0 });
	v.UsesAddressLibrary(true);
	v.HasNoStructUse(true);
	return v;
}();

EXTERN_C [[maybe_unused]] __declspec(dllexport) bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface*, SKSE::PluginInfo* pluginInfo)
{
	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;

	return true;
}
