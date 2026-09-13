#pragma once
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/splitter.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include "../assistants/ObservableValue.h"

class wxErrorsFrame : public wxFrame
{
private:
    wxListCtrl* tableError = nullptr;
    wxTextCtrl* textError = nullptr;
    wxButton* buttonClear = nullptr;

    void showErrors()
    {
        tableError->DeleteAllItems();
        auto exceptions = assistant::js::exceptions;
        auto count = exceptions->count();
        for(int i = 0 ; i < count ; i++)
        {
            JSException error = exceptions->get(i);
            JSException* persistentError = new JSException(error);
            tableError->InsertItem(i, wxString::FromUTF8(persistentError->error));
            tableError->SetItemData(i, reinterpret_cast<wxUIntPtr>(persistentError)); 
        }
    }

    void showError(wxListEvent& event)
    {
        long index = event.GetIndex();
        wxUIntPtr data = tableError->GetItemData(index);
        if (data == 0) 
        {
            return;
        }

        JSException* selectedError = reinterpret_cast<JSException*>(data);

        std::string text = "Ошибка: " + selectedError->error + "\r\n" + selectedError->stack;
        textError->SetValue(wxString::FromUTF8(text));
    }

public:
    wxErrorsFrame(wxWindow *parent) : wxFrame(parent, wxID_ANY, wxString::FromUTF8("Список ошибок"), wxDefaultPosition, wxSize(600, 400))
    {
        wxSplitterWindow* splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);
        tableError = new wxListCtrl(splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxBORDER_SUNKEN);
    
        tableError->InsertColumn(0, wxString::FromUTF8("Ошибка"), wxLIST_FORMAT_LEFT);
        tableError->SetColumnWidth(0, tableError->GetClientSize().x);

        textError = new wxTextCtrl(splitter, wxID_ANY, "",  wxDefaultPosition, wxDefaultSize,  wxTE_MULTILINE | wxTE_READONLY | wxBORDER_SUNKEN);

        splitter->SetMinimumPaneSize(50);
        splitter->SplitVertically(tableError, textError, 400);
        buttonClear = new wxButton(this, wxID_ANY, wxString::FromUTF8("Очистить"));
    
        wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
        frameSizer->Add(splitter, 1, wxEXPAND);
        frameSizer->Add(buttonClear, 0, wxEXPAND);

        this->SetSizer(frameSizer);
        this->showErrors();
        tableError->Bind(wxEVT_LIST_ITEM_SELECTED, [this](wxListEvent& event) 
        {
            this->showError(event);
        });

        buttonClear->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) 
        {
            assistant::js::exceptions->clear();
            this->showErrors();
        });
    }
    ~wxErrorsFrame() = default;
};