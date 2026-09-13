#pragma once
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/splitter.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include "../assistants/ObservableValue.h"

class wxLogsFrame : public wxFrame
{
private:
    wxListCtrl* tableLog = nullptr;
    wxButton* buttonClear = nullptr;
    ObservableValue<std::string> *store = nullptr;

    void showLogs()
    {
        tableLog->DeleteAllItems();
        auto count = store->count();
        for(int i = 0 ; i < count ; i++)
        {
            std::string text = store->get(i);
            tableLog->InsertItem(i, wxString::FromUTF8(text));
        }
    }

public:
    wxLogsFrame(wxWindow *parent, std::string title, ObservableValue<std::string> *store) : wxFrame(parent, wxID_ANY, wxString::FromUTF8(title), wxDefaultPosition, wxSize(600, 400))
    {
        this->store = store;
    
        tableLog = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxBORDER_SUNKEN);
        tableLog->InsertColumn(0, wxString::FromUTF8(title), wxLIST_FORMAT_LEFT);
        tableLog->SetColumnWidth(0, tableLog->GetClientSize().x);
        buttonClear = new wxButton(this, wxID_ANY, wxString::FromUTF8("Очистить"));
    
        wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
        frameSizer->Add(tableLog, 1, wxEXPAND);
        frameSizer->Add(buttonClear, 0, wxEXPAND);

        this->SetSizer(frameSizer);
        this->showLogs();

        buttonClear->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) 
        {
            this->store->clear();
            this->showLogs();
        });
    }
};