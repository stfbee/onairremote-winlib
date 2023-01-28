#include <windows.h>
#include <iostream>
#include <Mmdeviceapi.h>
#include <Audiopolicy.h>
#include <Functiondiscoverykeys_devpkey.h>
#include "micActivityChecker.h"

#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}

BOOL is_microphone_recording()
{
    // #1 Get the audio endpoint associated with the microphone device  
    HRESULT hr = S_OK;
    IMMDeviceEnumerator* p_enumerator = nullptr;
    IAudioSessionManager2* p_session_manager = nullptr;
    BOOL result = FALSE;

    CoInitialize(nullptr);

    // Create the device enumerator.  
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        reinterpret_cast<void**>(&p_enumerator));

    IMMDeviceCollection* d_col = nullptr;
    hr = p_enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &d_col);
    UINT d_count;
    hr = d_col->GetCount(&d_count);
    for (UINT i = 0; i < d_count; i++)
    {
        IMMDevice* p_capture_device = nullptr;
        hr = d_col->Item(i, &p_capture_device);

        IPropertyStore* p_props = nullptr;
        hr = p_capture_device->OpenPropertyStore(
            STGM_READ, &p_props);

        PROPVARIANT varName;
        // Initialize container for property value.  
        PropVariantInit(&varName);

        // Get the endpoint's friendly-name property.  
        hr = p_props->GetValue(PKEY_Device_FriendlyName, &varName);

        std::wstring name_str(varName.pwszVal);

        // #2 Determine whether it is the microphone device you are focusing on  
        std::size_t found = name_str.find(L"Microphone");
        if (found != std::string::npos)
        {
            // Print endpoint friendly name.  
            printf("Endpoint friendly name: \"%S\"\n", varName.pwszVal);

            // Get the session manager.  
            hr = p_capture_device->Activate(
                __uuidof(IAudioSessionManager2), CLSCTX_ALL,
                nullptr, reinterpret_cast<void**>(&p_session_manager));
            break;
        }
    }

    // Get session state  
    if (!p_session_manager)
    {
        return (result = FALSE);
    }

    int cb_session_count = 0;

    IAudioSessionEnumerator* p_session_list = nullptr;
    IAudioSessionControl* p_session_control = nullptr;
    IAudioSessionControl2* p_session_control2 = nullptr;

    // Get the current list of sessions.  
    hr = p_session_manager->GetSessionEnumerator(&p_session_list);

    // Get the session count.  
    hr = p_session_list->GetCount(&cb_session_count);
    printf("Session count: %d\n", cb_session_count);

    for (int index = 0; index < cb_session_count; index++)
    {
        LPWSTR psw_session = nullptr;
        CoTaskMemFree(psw_session);
        SAFE_RELEASE(p_session_control)

        // Get the <n>th session.  
        hr = p_session_list->GetSession(index, &p_session_control);

        hr = p_session_control->QueryInterface(__uuidof(IAudioSessionControl2),
                                               reinterpret_cast<void**>(&p_session_control2));

        // Exclude system sound session  
        hr = p_session_control2->IsSystemSoundsSession();
        if (S_OK == hr)
        {
            printf(" this is a system sound.\n");
            continue;
        }

        // Optional. Determine which application is using Microphone for recording  
        LPWSTR inst_id = nullptr;
        hr = p_session_control2->GetSessionInstanceIdentifier(&inst_id);
        if (S_OK == hr)
        {
            printf("SessionInstanceIdentifier: %ls\n", inst_id);
        }

        AudioSessionState state;
        hr = p_session_control->GetState(&state);
        switch (state)
        {
        case AudioSessionStateInactive:
            printf("Session state: Inactive\n\n");
            break;
        case AudioSessionStateActive:
            // #3 Active state indicates it is recording, otherwise is not recording.  
            printf("Session state: Active\n\n");
            result = TRUE;
            break;
        case AudioSessionStateExpired:
            printf("Session state: Expired\n\n");
            break;
        }
    }

    // Clean up.  
    SAFE_RELEASE(p_session_control)
    SAFE_RELEASE(p_session_list)
    SAFE_RELEASE(p_session_control2)
    SAFE_RELEASE(p_session_manager)
    SAFE_RELEASE(p_enumerator)

    return result;
}
