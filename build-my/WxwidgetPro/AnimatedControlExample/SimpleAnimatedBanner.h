// SimpleAnimatedBanner.h
// A lightweight custom control that slides in/out and fades in/out.

#pragma once

#include <wx/control.h>
#include <wx/timer.h>
#include <wx/string.h>

class SimpleAnimatedBanner : public wxControl
{
public:
    SimpleAnimatedBanner(wxWindow* parent,
                         wxWindowID id = wxID_ANY,
                         const wxString& text = "Animated banner",
                         const wxPoint& pos = wxDefaultPosition,
                         const wxSize& size = wxDefaultSize);

    // Start an animated show/hide sequence.
    void ShowAnimated(bool show);

    bool IsFullyVisible() const { return m_visibleProgress >= 1.0; }
    bool IsFullyHidden() const { return m_visibleProgress <= 0.0; }

    void SetText(const wxString& text);

protected:
    void OnPaint(wxPaintEvent& event);
    void OnTimer(wxTimerEvent& event);

    // Smoothstep easing based on current progress in [0,1].
    double GetEasedProgress() const;

private:
    wxTimer  m_timer;
    double   m_visibleProgress; // 0.0 = fully hidden, 1.0 = fully visible
    bool     m_animating;
    bool     m_targetVisible;
    int      m_durationMs;
    int      m_intervalMs;
    int      m_slideOffsetDIP;
    wxString m_text;

    wxDECLARE_EVENT_TABLE();
};

