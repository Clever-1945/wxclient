#pragma once

#include <wx/msw/window.h>
#include <wx/clntdata.h>
#include "JSValueClientData.h"

/** JS данные привязанные к контролу */
struct wxEventControlJsData {
    /** Контрол, который сгенирировал событие */
    wxWindow *control = nullptr;
    /** Данные привязанные к контролу */
    wxClientData *clientDataVoid = nullptr;
    /** Данные из JS */
    JSValueClientData *clientData = nullptr;
};