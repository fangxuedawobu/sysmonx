#include "common.h"
#include "callbacks_etw_event_ids.h"

bool EventCollectorETW::SetupCallbackPowershellEventsHandler()
{
	bool ret = false;

	if (m_config.IsInitialized() && m_powershellProvider)
	{
		m_powershellProvider->add_on_event_callback([&](const EVENT_RECORD &record)
		{
			UINT32 currentEventID = SysmonXDefs::INVALID_EVENT_ID;
			UINT32 currentPID = SysmonXDefs::INVALID_PROCESS_ID;
			UINT32 currentOPCode = SysmonXDefs::DEFAULT_OPCODE;
			UINT32 schemaEventID = SysmonXDefs::INVALID_EVENT_ID;

			try
			{
				// Grabbing event schema
				krabs::schema schema(record);

				// Getting event data for evaluation
				schemaEventID = schema.event_id();

				//Event 1001 - SECURITY_EVENT_ID_SYSMONX_POWERSHELL_START_COMMAND
				if (schemaEventID == ETW_POWERSHELL_PROVIDER::ETW_POWERSHELL_PROVIDER_SCRIPTBLOCK_LOGGING_EVENT)
				{
					krabs::parser parser(schema);

					//Assigning working data
					currentEventID = EventID::SECURITY_EVENT_ID_SYSMONX_POWERSHELL_START_COMMAND;
					currentPID = schema.process_id();
					currentOPCode = schema.event_opcode();

					//Creating new security event and transfering ownership
					SysmonXTypes::EventObject newEvent = SysmonXEvents::GetNewSecurityEvent(EventID::SECURITY_EVENT_ID_SYSMONX_POWERSHELL_START_COMMAND);
										
					//Filling up Common Section of event
					newEvent->EventVersion = SysmonXDefs::CURRENT_EVENT_VERSION;
					newEvent->EventID = currentEventID;
					newEvent->EventCollectorTechID = EventCollectorTechID::EVENT_COLLECTOR_TECH_ETW;
					newEvent->EventCollectorVectorID = EventCollectorVectorID::EVENT_SUBTYPE_ETW_USER_CUSTOM;
					newEvent->EventCreationTimestamp = schema.timestamp();
					newEvent->EventETWProviderPID = currentPID;
					newEvent->EventETWProviderName.assign(ETWProvidersName::POWERSHELL_PROVIDER_NAME);
					newEvent->ProcessId = currentPID;

					//Filling up event-specific data
					if (!(parser.try_parse<std::wstring>(L"ScriptBlockText", newEvent->FreeText)))
					{
						m_logger.Error("EventCollectorETW::SetupCallbackPowershellEventsHandler - There was a problem parsing property {} at Event ID {}", "FreeText", currentEventID);
					}
					
					m_eventsProcessor.DispatchEvent(newEvent);
				}
			}
			catch (...)
			{
				m_logger.Error("EventCollectorETW::SetupCallbackPowershellEventsHandler - There was a problem parsing Event ID {}", currentEventID);
			}
		});

		ret = true;
	}

	return ret;
}