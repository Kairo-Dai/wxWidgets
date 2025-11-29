// SemiTransparentWindowExample.cpp
// Demo: semi-transparent top-level window using SetTransparent().

#include <wx/app.h>
#include <wx/panel.h>
#include <wx/slider.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "BorderlessRoundedWindowBase.h"

class DemoTransparentWindow : public BorderlessRoundedWindowBase
{
public:
    DemoTransparentWindow();

private:
    wxSlider*     m_slider;
    wxStaticText* m_label;

    void OnAlphaChanged(wxCommandEvent& event);
};

DemoTransparentWindow::DemoTransparentWindow()
    : BorderlessRoundedWindowBase(
          nullptr,
          wxID_ANY,
          "Semi-transparent top-level window",
          wxDefaultPosition,
          wxSize(420, 260),
          // Extra style: do not show in taskbar, stay on top.
          wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP,
          14 // corner radius in DIP
      ),
      m_slider(nullptr),
      m_label(nullptr)
{
    // Panel that hosts controls and acts as draggable background.
    wxPanel* panel = new wxPanel(this);

    // Dragging anywhere on the panel moves the window.
    AttachDragHandlers(panel);

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* text = new wxStaticText(
        panel,
        wxID_ANY,
        "Use the slider below to change this window's alpha.\n"
        "Internally this calls wxTopLevelWindow::SetTransparent()."
    );

    const int minAlpha     = 40;
    const int maxAlpha     = 255;
    const int initialAlpha = 180;

    m_slider = new wxSlider(
        panel,
        wxID_ANY,
        initialAlpha,
        minAlpha,
        maxAlpha,
        wxDefaultPosition,
        wxDefaultSize,
        wxSL_HORIZONTAL | wxSL_LABELS
    );

    m_label = new wxStaticText(
        panel,
        wxID_ANY,
        wxString::Format("Alpha: %d", initialAlpha)
    );

    const int margin = FromDIP(12, panel);

    sizer->Add(text,    0, wxEXPAND | wxALL, margin);
    sizer->Add(m_slider,0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, margin);
    sizer->Add(m_label, 0, wxALIGN_RIGHT | wxALL, margin);

    panel->SetSizerAndFit(sizer);

    if ( CanSetTransparent() )
    {
        SetTransparent(static_cast<wxByte>(initialAlpha));

        m_slider->Bind(wxEVT_SLIDER,
                       &DemoTransparentWindow::OnAlphaChanged,
                       this);
    }
    else
    {
        m_slider->Enable(false);
        m_label->SetLabel("Transparency is not supported on this platform.");
    }
}

void DemoTransparentWindow::OnAlphaChanged(wxCommandEvent& event)
{
    const int alpha = event.GetInt();

    m_label->SetLabel(wxString::Format("Alpha: %d", alpha));

    // Clamp to valid byte range before passing to SetTransparent().
    const int clamped = std::max(0, std::min(255, alpha));
    SetTransparent(static_cast<wxByte>(clamped));
}

class SemiTransparentApp : public wxApp
{
public:
    bool OnInit() override
    {
        if ( !wxApp::OnInit() )
            return false;

        auto* win = new DemoTransparentWindow();
        win->Show();
        return true;
    }
};

wxIMPLEMENT_APP(SemiTransparentApp);

