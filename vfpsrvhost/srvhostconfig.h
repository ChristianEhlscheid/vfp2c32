#ifndef _SRVHOSTCONFIG_H_
#define _SRVHOSTCONFIG_H_

#pragma once

#include "VfpSrvHost.h"
#include "XmlLiteConfig.h"

class SrvHostConfig : public XmlLiteConfig
{
public:
	void Release()
	{
		Services.RemoveAll();
	}

	CAutoPtrArray<CService> Services;

	BEGIN_LOAD_CONFIG()
	
		DWORD ServiceId = 0;
		CService* service = 0;
		CAutoPtr<CService> pAutoService;

		BEGIN_ELEMENT_LOOP()

			BEGIN_ON_START_ELEMENT()

				BEGIN_ELEMENT_NAME("ServiceConfig")
					service = new CService();
					if (!service)
					{
						m_hr = E_OUTOFMEMORY;
						return false;
					}
					pAutoService.Attach(service);
					service->ServiceId = ServiceId;
				END_ELEMENT_NAME()

				BEGIN_ELEMENT_NAME("DisplayName")
					READ_ELEMENT_VALUE(service->DisplayName)
				END_ELEMENT_NAME()

				BEGIN_ELEMENT_NAME("ServiceName")
					READ_ELEMENT_VALUE(service->ServiceName)
				END_ELEMENT_NAME()

				BEGIN_ELEMENT_NAME("StartType")
					READ_ELEMENT_VALUE(service->StartType)
				END_ELEMENT_NAME()

				BEGIN_ELEMENT_NAME("ServiceAccount")
					READ_ELEMENT_VALUE(service->ServiceAccount)
				END_ELEMENT_NAME()

				BEGIN_ELEMENT_NAME("ComClass")
					READ_ELEMENT_VALUE(service->ComClass)
				END_ELEMENT_NAME()

				BEGIN_ELEMENT_NAME("StartWaitHint")
					READ_ELEMENT_VALUE(service->Status.dwWaitHint)
				END_ELEMENT_NAME()

				BEGIN_ELEMENT_NAME("StopWaitHint")
					READ_ELEMENT_VALUE(service->StopWaitHint)
				END_ELEMENT_NAME()

				BEGIN_ELEMENT_NAME("PauseWaitHint")
					READ_ELEMENT_VALUE(service->PauseWaitHint)
				END_ELEMENT_NAME()

				BEGIN_ELEMENT_NAME("ContinueWaitHint")
					READ_ELEMENT_VALUE(service->ContinueWaitHint)
				END_ELEMENT_NAME()

				BEGIN_ELEMENT_NAME("Dependencies")
					READ_ELEMENT_VALUE(service->Dependencies)
				END_ELEMENT_NAME()

				BEGIN_ELEMENT_NAME("PowerNotifications")
					READ_ELEMENT_VALUE(service->PowerNotifications);
				END_ELEMENT_NAME()

			END_ON_START_ELEMENT()

			BEGIN_ON_END_ELEMENT()

				BEGIN_ELEMENT_NAME("ServiceConfig")
					Services.Add(pAutoService);
					ServiceId++;
				END_ELEMENT_NAME()

			END_ON_END_ELEMENT()

		END_ELEMENT_LOOP()

	END_LOAD_CONFIG()

};

#endif // _SRVHOSTCONFIG_H_