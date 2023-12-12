// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include <filesystem>
#include <string>

#include <common/logger/logger.h>
#include <common/logger/logger_settings.h>
#include <common/SettingsAPI/settings_objects.h>
#include <common/utils/gpo.h>
#include <common/utils/logger_helper.h>
#include <common/utils/resources.h>
#include <interface/powertoy_module_interface.h>

#include "resource.h"
#include "trace.h"

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD ul_reason_for_call, LPVOID /*lpReserved*/)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        Trace::RegisterProvider();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        Trace::UnregisterProvider();
        break;
    }
    return TRUE;
}

const static wchar_t* MODULE_NAME = L"Command Not Found";
const static wchar_t* MODULE_DESC = L"A module that detects an error thrown by a command in PowerShell and suggests a relevant WinGet package to install, if available.";

inline const std::wstring ModuleKey = L"CmdNotFound";

class CmdNotFound : public PowertoyModuleIface
{
    std::wstring app_name;
    std::wstring app_key;

private:
    bool m_enabled = false;

public:
    CmdNotFound()
    {
        app_name = GET_RESOURCE_STRING(IDS_CMD_NOT_FOUND_NAME);
        app_key = ModuleKey;

        std::filesystem::path logFilePath(PTSettingsHelper::get_module_save_folder_location(this->app_key));
        logFilePath.append(LogSettings::cmdNotFoundLogPath);
        Logger::init(LogSettings::cmdNotFoundLoggerName, logFilePath.wstring(), PTSettingsHelper::get_log_settings_file_location());
        Logger::info("CmdNotFound object is constructing");
    }

    virtual powertoys_gpo::gpo_rule_configured_t gpo_policy_enabled_configuration() override
    {
        return powertoys_gpo::getConfiguredCmdNotFoundEnabledValue();
    }

    virtual void destroy() override
    {
        delete this;
    }

    virtual const wchar_t* get_name() override
    {
        return MODULE_NAME;
    }

    virtual const wchar_t* get_key() override
    {
        return app_key.c_str();
    }

    virtual bool get_config(wchar_t* buffer, int* buffer_size) override
    {
        HINSTANCE hinstance = reinterpret_cast<HINSTANCE>(&__ImageBase);

        PowerToysSettings::Settings settings(hinstance, get_name());
        settings.set_description(MODULE_DESC);

        return settings.serialize_to_buffer(buffer, buffer_size);
    }

    virtual void set_config(const wchar_t* config) override
    {
        try
        {
            PowerToysSettings::PowerToyValues values =
                PowerToysSettings::PowerToyValues::from_json_string(config, get_key());

            values.save_to_settings_file();
        }
        catch (std::exception&)
        {
            // Improper JSON
        }
    }

    virtual void enable()
    {
        Trace::EnableCmdNotFound(true);
        m_enabled = true;
    }

    virtual void disable()
    {
        if (m_enabled)
        {
            Trace::EnableCmdNotFound(false);
            Logger::trace(L"Disabling CmdNotFound...");
        }

        m_enabled = false;
    }

    virtual bool is_enabled() override
    {
        return m_enabled;
    }
};

extern "C" __declspec(dllexport) PowertoyModuleIface* __cdecl powertoy_create()
{
    return new CmdNotFound();
}
