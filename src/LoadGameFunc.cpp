#pragma once

#include "LoadGameFunc.h"
#include "Event.h"

#define SMOOTHCAM_API_COMMONLIB
#include "SmoothCamAPI.h"

namespace ShowPlayerInMenusNamespace
{
	void EventCallback(SKSE::MessagingInterface::Message* msg)
	{
		switch (msg->type) {
			case SKSE::MessagingInterface::kPostLoad:
				if (!SmoothCamAPI::RegisterInterfaceLoaderCallback(
					g_Messaging,
					[](void* interfaceInstance, SmoothCamAPI::InterfaceVersion interfaceVersion) {
						if (interfaceVersion == SmoothCamAPI::InterfaceVersion::V2) {
							g_SmoothCam = reinterpret_cast<SmoothCamAPI::IVSmoothCam2*>(interfaceInstance);
							_MESSAGE("Obtained SmoothCamAPI");
						}
						else
							_MESSAGE("Unable to aquire requested SmoothCamAPI interface version");
					}
				)) _MESSAGE("SmoothCamAPI::RegisterInterfaceLoaderCallback reported an error");
				break;
			case SKSE::MessagingInterface::kPostPostLoad:
				if (!SmoothCamAPI::RequestInterface(
					g_Messaging,
					SmoothCamAPI::InterfaceVersion::V2
				)) _MESSAGE("SmoothCamAPI::RequestInterface reported an error");
				break;
			case SKSE::MessagingInterface::kInputLoaded:
				_MESSAGE("Input Load CallBack Trigger!");

				InputEventHandler::Enable();
				MenuOpenCloseEventHandler::Enable();
				break;
			default:
				break;
		}
	}
}
