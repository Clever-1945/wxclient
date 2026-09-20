#pragma once
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/gauge.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <string>
#include "wxErrorsFrame.h"
#include "wxLogsFrame.h"

class wxDebugPanel : public wxPanel {
private:
    wxButton *buttonError = nullptr;
    wxButton *buttonInfo = nullptr;
    wxButton *buttonWarning = nullptr;
    wxButton *buttonRefresh = nullptr;
    int64_t idActionException = 0;
    int64_t idActionLog = 0;
    int64_t idActionWar = 0;
    std::function<void()>* refreshFunction = nullptr;

    void setCountError(int count) {
        setCount(count, "Ошибок", buttonError);
    }

    void setCountInfo(int count) {
        setCount(count, "Логов", buttonInfo);
    }

    void setCountWarning(int count) {
        setCount(count, "Предупреждений", buttonWarning);
    }

    void setCount(int count, std::string prefix, wxButton *button) {
        if (button) {
            button->SetLabel(wxString::FromUTF8(prefix + ": " + std::to_string(count)));
        }
    }

public:
    wxDebugPanel(wxWindow *parent, wxWindowID id = wxID_ANY) : wxPanel(parent, id, wxDefaultPosition, wxSize(-1, 18)) {
        wxStaticLine *separator1 = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL);
        wxStaticLine *separator2 = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL);

        buttonError = new wxButton(this, wxID_ANY, "");
        buttonInfo = new wxButton(this, wxID_ANY, "");
        buttonWarning = new wxButton(this, wxID_ANY, "");
        buttonRefresh = new wxButton(this, wxID_ANY, wxString::FromUTF8("Обновить"));

        wxBoxSizer *rowSizer = new wxBoxSizer(wxHORIZONTAL);

        rowSizer->Add(buttonError, 0, wxALIGN_CENTER_VERTICAL | wxALL, 0);
        rowSizer->Add(separator1, 0, wxEXPAND | wxTOP | wxBOTTOM, 0);

        rowSizer->Add(buttonInfo, 0, wxALIGN_CENTER_VERTICAL | wxALL, 0);
        rowSizer->Add(separator2, 0, wxEXPAND | wxTOP | wxBOTTOM, 0);

        rowSizer->Add(buttonWarning, 0, wxEXPAND | wxTOP | wxBOTTOM, 0);

        rowSizer->AddStretchSpacer(1);

        rowSizer->Add(buttonRefresh, 0, wxALIGN_CENTER_VERTICAL | wxALL, 0);
        setCountError(0);
        setCountInfo(0);
        setCountWarning(0);

        this->SetSizer(rowSizer);

        this->idActionException = assistant::js::exceptions->subscribe([this] {
            this->setCountError(assistant::js::exceptions->count());
        });

        this->idActionLog = assistant::js::logs->subscribe([this] {
            this->setCountInfo(assistant::js::logs->count());
        });

        this->idActionWar = assistant::js::warning->subscribe([this] {
            this->setCountWarning(assistant::js::warning->count());
        });

        buttonError->Bind(wxEVT_BUTTON, [this](wxCommandEvent &event) {
            this->showErrors();
        });
        buttonInfo->Bind(wxEVT_BUTTON, [this](wxCommandEvent &event) {
            auto frame = new wxLogsFrame(wxTheApp->GetMainTopWindow(), "Логи", assistant::js::logs);
            frame->Show();
        });
        buttonWarning->Bind(wxEVT_BUTTON, [this](wxCommandEvent &event) {
            auto frame = new wxLogsFrame(wxTheApp->GetMainTopWindow(), "Предупреждения", assistant::js::warning);
            frame->Show();
        });
    }

    ~wxDebugPanel() {
        assistant::js::exceptions->unsubscribe(this->idActionException);
        assistant::js::logs->unsubscribe(this->idActionLog);
        assistant::js::warning->unsubscribe(this->idActionWar);
        buttonError = nullptr;
        buttonInfo = nullptr;
        buttonWarning = nullptr;
        buttonRefresh = nullptr;
        delete this->refreshFunction;
    }

    void showErrors() {
        auto frame = new wxErrorsFrame(wxTheApp->GetMainTopWindow());
        frame->Show();
    }

    /** Установить функцию, при обновлении приложения */
    void setClickRefresh(const std::function<void()>& fn) {
        if (buttonRefresh) {
            this->refreshFunction = new std::function<void()>(fn);
            buttonRefresh->Bind(wxEVT_BUTTON, [this](wxCommandEvent &event) {
                if (this->refreshFunction) {
                    (*refreshFunction)();
                }
            });
        }
    }

    /** Обновить приложение */
    void refresh() {
        if (this->refreshFunction) {
            (*refreshFunction)();
        }
    }
};