#pragma once

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/gauge.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/gbsizer.h>
#include <wx/stattext.h>
#include "wxDebugPanel.h"
#include "assistants/assistant_buss.h"
#include <string>

/** Основная панель, расположенная в любом окне приложения */
class wxFrameSizer : public wxBoxSizer {
private:
    wxFrame* frame = nullptr;
    wxPanel* frame_panel = nullptr;
    wxBoxSizer* panel_sizer = nullptr;
    wxGauge* progressBar = nullptr;
    wxGridBagSizer* items_grid = nullptr;
    wxDebugPanel* debug_panel = nullptr;
    int64_t asyncCounterId = 0;
    bool isVisibleProgressBar = false;
    bool isVisibleDebugPanel = false;

    void showLoading(bool isVisibleProgressBar) {
        if (this->isVisibleProgressBar == isVisibleProgressBar) {
            return;
        }
        this->isVisibleProgressBar = isVisibleProgressBar;
        if (!this->progressBar || !this->frame_panel || !this->panel_sizer) {
            return;
        }

        if (isVisibleProgressBar) {
            this->panel_sizer->Insert(0, this->progressBar, 0, wxEXPAND | wxALL, 0);
            this->progressBar->SetValue(1);
            this->progressBar->Pulse();
            this->frame_panel->Enable(false);
        } else {
            this->panel_sizer->Detach(this->progressBar);
            this->progressBar->SetValue(0);
            this->frame_panel->Enable(true);
        }

        this->frame_panel->Layout();
    }

    void showDebugPanel(bool isVisibleDebugPanel) {
        if (this->isVisibleDebugPanel == isVisibleDebugPanel) {
            return;
        }
        this->isVisibleDebugPanel = isVisibleDebugPanel;

        if (this->debug_panel && this->panel_sizer && this->frame_panel) {
            if (isVisibleDebugPanel) {
                this->debug_panel->Show();
                this->panel_sizer->Add(this->debug_panel, 0, wxEXPAND, 0);
                this->frame_panel->Layout();
            } else {
                this->debug_panel->Hide();
                this->panel_sizer->Detach(this->debug_panel);
                this->frame_panel->Layout();
            }
        }
    }
public:
    wxFrameSizer(wxFrame *frame, int gap) : wxBoxSizer(wxVERTICAL) {
        if (!frame) {
            return;
        }

        this->frame = frame;
        this->frame->SetSizer(this);

        this->frame_panel = new wxPanel(frame, wxID_ANY);
        this->Add(this->frame_panel, 1, wxEXPAND, 0);

        this->panel_sizer = new wxBoxSizer(wxVERTICAL);
        this->frame_panel->SetSizer(this->panel_sizer);

        this->progressBar = new wxGauge(this->frame_panel, wxID_ANY, 100,  wxDefaultPosition, wxSize(-1, 3),  wxGA_HORIZONTAL | wxGA_SMOOTH);
        this->panel_sizer->Add(progressBar, 0, wxEXPAND, 0);
//        this->panel_sizer->Detach(this->progressBar);

        this->items_grid = new wxGridBagSizer(gap, gap);
        this->panel_sizer->Add(this->items_grid, 1, wxEXPAND, 0);
        // this->panel_sizer->Add(new wxStaticText(this->frame_panel, wxID_ANY, ""), 1, wxEXPAND, 0);

        this->debug_panel = new wxDebugPanel(this->frame_panel, wxID_ANY);
        this->panel_sizer->Add(this->debug_panel, 0, wxEXPAND, 0);
//        this->panel_sizer->Detach(this->debug_panel);
//        this->asyncCounterId = assistant::buss::asyncCounter->subscribe([this] {
//            this->showLoading(assistant::buss::asyncCounter->get(0) > 0);
//        });
//
         this->frame_panel->Layout();
    }

    ~wxFrameSizer() {
        assistant::buss::asyncCounter->unsubscribe(this->asyncCounterId);
    }

    wxGridBagSizer* getGridItems() {
        return this->items_grid;
    }

    void changeShowDebugPanel() {
        this->showDebugPanel(!this->isVisibleDebugPanel);
    }

    wxPanel* getPanel() {
        return this->frame_panel;
    }
};