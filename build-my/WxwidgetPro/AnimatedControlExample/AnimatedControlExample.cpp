// AnimatedControlExample.cpp
// Demo using SimpleAnimatedBanner: smooth slide in/out + fade in/out.

#include <wx/app.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/sizer.h>

#include "SimpleAnimatedBanner.h"

class AnimatedControlFrame : public wxFrame
{
public:
    AnimatedControlFrame();

private:
    SimpleAnimatedBanner* m_banner;
    bool                  m_bannerShown;

    void OnShow(wxCommandEvent& event);
    void OnHide(wxCommandEvent& event);
    void OnToggle(wxCommandEvent& event);
};

AnimatedControlFrame::AnimatedControlFrame()
    : wxFrame(nullptr,
              wxID_ANY,
              "Animated control example",
              wxDefaultPosition,
              wxSize(600, 400)),
      m_banner(nullptr),
      m_bannerShown(false)
{
    wxPanel* panel = new wxPanel(this);

    auto* topSizer = new wxBoxSizer(wxVERTICAL);

    m_banner = new SimpleAnimatedBanner(
        panel,
        wxID_ANY,
        "This banner slides in from the top and fades in/out.\n"
        "Use the buttons below to control it."
    );
    // Give it a reasonable height so the effect is visible.
    m_banner->SetMinSize(wxSize(-1, FromDIP(80, panel)));

    auto* btnShow   = new wxButton(panel, wxID_ANY, "Show");
    auto* btnHide   = new wxButton(panel, wxID_ANY, "Hide");
    auto* btnToggle = new wxButton(panel, wxID_ANY, "Toggle");

    auto* buttons = new wxBoxSizer(wxHORIZONTAL);
    const int gap = FromDIP(8, panel);
    buttons->Add(btnShow,   0, wxRIGHT, gap);
    buttons->Add(btnHide,   0, wxRIGHT, gap);
    buttons->Add(btnToggle, 0);

    topSizer->Add(m_banner, 0, wxEXPAND | wxALL, FromDIP(10, panel));
    topSizer->AddStretchSpacer();
    topSizer->Add(buttons, 0, wxALIGN_CENTER | wxALL, FromDIP(10, panel));

    panel->SetSizerAndFit(topSizer);

    btnShow->Bind(wxEVT_BUTTON, &AnimatedControlFrame::OnShow,   this);
    btnHide->Bind(wxEVT_BUTTON, &AnimatedControlFrame::OnHide,   this);
    btnToggle->Bind(wxEVT_BUTTON, &AnimatedControlFrame::OnToggle, this);
}

void AnimatedControlFrame::OnShow(wxCommandEvent& WXUNUSED(event))
{
    m_bannerShown = true;
    m_banner->ShowAnimated(true);
}

void AnimatedControlFrame::OnHide(wxCommandEvent& WXUNUSED(event))
{
    m_bannerShown = false;
    m_banner->ShowAnimated(false);
}

void AnimatedControlFrame::OnToggle(wxCommandEvent& WXUNUSED(event))
{
    m_bannerShown = !m_bannerShown;
    m_banner->ShowAnimated(m_bannerShown);
}

class AnimatedControlApp : public wxApp
{
public:
    bool OnInit() override
    {
        if ( !wxApp::OnInit() )
            return false;

        auto* frame = new AnimatedControlFrame();
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP(AnimatedControlApp);

