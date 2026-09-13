#include <wx/clntdata.h>
#include <wx/wx.h>

/** Данные привязанные к диалоговому окну */
class wxFrameData : public wxClientData
{
private:
    wxGauge *progressBar = nullptr;
    wxPanel* panel = nullptr;
    wxBoxSizer* panelSizer = nullptr;
    wxDebugPanel* debugPanel = nullptr;
    bool isVisibleProgressBar = false;

public:
    void setProgressBar(wxGauge *progressBar)
    {
        this->progressBar = progressBar;
    }

    void setPanel(wxPanel *panel)
    {
        this->panel = panel;
    }

    void setPanelSizer(wxBoxSizer *panelSizer)
    {
        this->panelSizer = panelSizer;
    }

    void setDebugPanel(wxDebugPanel* debugPanel) 
    {
        this->debugPanel = debugPanel;
    }

    void showLoading(int counterAsync)
    {
        bool isVisibleProgressBar = counterAsync > 0;
        if (this->isVisibleProgressBar == isVisibleProgressBar)
        {
            return;
        }
        this->isVisibleProgressBar = isVisibleProgressBar;
        if (!this->panelSizer || !this->progressBar || !this->panel)
        {
            return;
        }

        if (isVisibleProgressBar)
        {
            this->panelSizer->Insert(0, this->progressBar, 0, wxEXPAND | wxALL, 0);
            this->progressBar->SetValue(1);
            this->progressBar->Pulse();
            this->panel->Enable(false);
        }
        else 
        {
            this->panelSizer->Detach(this->progressBar);                    
            this->progressBar->SetValue(0);
            this->panel->Enable(true);
        }

        this->panel->Layout();
    }

    /** Установить функцию, при обновлении приложения */
    void setClickRefresh(std::function<void()> fn)
    {
        if (this->debugPanel)
        {
            this->debugPanel->setClickRefresh(fn);
        }
    }
};