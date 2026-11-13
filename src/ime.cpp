#include "ime.h"

#include <orbis/ImeDialog.h>
#include <orbis/Sysmodule.h>
#include <orbis/UserService.h>

#include <SDL2/SDL.h>

#include <stdlib.h>
#include <wchar.h>
#include <vector>

// Convert the UTF-8 we keep in std::string to the wide text the keyboard uses.
// Only the ASCII range is expected for usernames, emails and passwords.
static std::wstring ToWide(const std::string& in)
{
    std::wstring out;
    out.reserve(in.size());
    for (unsigned char c : in)
        out.push_back((wchar_t)c);
    return out;
}

static std::string ToNarrow(const wchar_t* in)
{
    std::string out;
    for (; *in; ++in)
    {
        if (*in < 0x80)
            out.push_back((char)*in);
    }
    return out;
}

bool EditTextWithKeyboard(const wchar_t* title, bool hidden, std::string& text, int maxLength)
{
    static bool sysmoduleLoaded = false;
    if (!sysmoduleLoaded)
    {
        if (sceSysmoduleLoadModule(ORBIS_SYSMODULE_IME_DIALOG) < 0)
            return false;
        sysmoduleLoaded = true;
    }

    int userId = 0;
    OrbisUserServiceInitializeParams userParams;
    userParams.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
    sceUserServiceInitialize(&userParams);
    if (sceUserServiceGetInitialUser(&userId) < 0)
        return false;

    // Buffer for the keyboard result, plus room for the terminator
    std::vector<wchar_t> buffer(maxLength + 1, L'\0');
    std::wstring initial = ToWide(text);
    wcsncpy(buffer.data(), initial.c_str(), maxLength);

    OrbisImeDialogSetting setting = {};
    setting.userId = userId;
    setting.type = ORBIS_TYPE_BASIC_LATIN;
    setting.enterLabel = ORBIS_BUTTON_LABEL_GO;
    setting.inputMethod = ORBIS__DEFAULT;
    setting.filter = (OrbisTextFilter)0;
    // The SDK headers define no flag for masking input, so the keyboard shows typed text.
    // The form itself still hides the password.
    setting.option = 0;
    (void)hidden;
    setting.maxTextLength = maxLength;
    setting.inputTextBuffer = buffer.data();
    setting.posx = 0;
    setting.posy = 0;
    setting.horizontalAlignment = ORBIS_H_CENTER;
    setting.verticalAlignment = ORBIS_V_CENTER;
    setting.placeholder = hidden ? L"Password" : L"Email";
    setting.title = title;

    if (sceImeDialogInit(&setting, NULL) < 0)
        return false;

    // The keyboard runs as a system overlay; wait for it to close
    while (sceImeDialogGetStatus() == ORBIS_DIALOG_STATUS_RUNNING)
    {
        SDL_PumpEvents();
        SDL_Delay(16);
    }

    OrbisDialogResult result = {};
    if (sceImeDialogGetResult(&result) < 0)
        return false;
    if (result.endstatus != ORBIS_DIALOG_OK)
        return false;

    text = ToNarrow(buffer.data());
    return true;
}
