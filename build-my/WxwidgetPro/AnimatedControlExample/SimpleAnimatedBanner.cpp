// SimpleAnimatedBanner.cpp
// Implementation of a simple animated banner control.

#include "SimpleAnimatedBanner.h"

#include <algorithm>
#include <memory>

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

wxBEGIN_EVENT_TABLE(SimpleAnimatedBanner, wxControl)
    EVT_PAINT(SimpleAnimatedBanner::OnPaint)
    EVT_TIMER(wxID_ANY, SimpleAnimatedBanner::OnTimer)
wxEND_EVENT_TABLE()

SimpleAnimatedBanner::SimpleAnimatedBanner(wxWindow* parent,
                                           wxWindowID id,
                                           const wxString& text,
                                           const wxPoint& pos,
                                           const wxSize& size)
    : wxControl(parent, id, pos, size, wxBORDER_NONE),
      m_timer(this),
      m_visibleProgress(0.0),
      m_animating(false),
      m_targetVisible(false),
      m_durationMs(250),
      m_intervalMs(16),
      m_slideOffsetDIP(24),
      m_text(text)
{
    // We draw the entire background ourselves.
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void SimpleAnimatedBanner::SetText(const wxString& text)
{
    m_text = text;
    Refresh();
}

void SimpleAnimatedBanner::ShowAnimated(bool show)
{
    m_targetVisible = show;

    if ( show && IsFullyVisible() )
        return;
    if ( !show && IsFullyHidden() )
        return;

    m_animating = true;

    // Ensure the timer is running. If already running, we just change the
    // direction of animation by updating m_targetVisible.
    if ( !m_timer.IsRunning() )
        m_timer.Start(m_intervalMs);
}

double SimpleAnimatedBanner::GetEasedProgress() const
{
    double t = m_visibleProgress;

    if ( t <= 0.0 )
        return 0.0;
    if ( t >= 1.0 )
        return 1.0;

    // Smoothstep: 3t^2 - 2t^3
    return t * t * (3.0 - 2.0 * t);
}

void SimpleAnimatedBanner::OnTimer(wxTimerEvent& WXUNUSED(event))
{
    if ( !m_animating )
    {
        m_timer.Stop();
        return;
    }

    const double delta =
        static_cast<double>(m_intervalMs) /
        static_cast<double>(m_durationMs);

    if ( m_targetVisible )
    {
        m_visibleProgress += delta;
        if ( m_visibleProgress >= 1.0 )
        {
            m_visibleProgress = 1.0;
            m_animating = false;
            m_timer.Stop();
        }
    }
    else
    {
        m_visibleProgress -= delta;
        if ( m_visibleProgress <= 0.0 )
        {
            m_visibleProgress = 0.0;
            m_animating = false;
            m_timer.Stop();
        }
    }

    // Keep progress within [0,1] range.
    m_visibleProgress = std::max(0.0, std::min(1.0, m_visibleProgress));

    Refresh();
}

void SimpleAnimatedBanner::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();

#if wxUSE_GRAPHICS_CONTEXT
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if ( !gc )
        return;

    const wxSize size = GetClientSize();

    const double progress = GetEasedProgress();

    // Slide from slightly above (negative offset) down to 0.
    const double maxOffsetPx = static_cast<double>(FromDIP(m_slideOffsetDIP, this));
    const double offsetY     = (1.0 - progress) * -maxOffsetPx;

    const unsigned char alpha =
        static_cast<unsigned char>(std::round(progress * 255.0));

    gc->Translate(0.0, offsetY);

    wxGraphicsPath path = gc->CreatePath();
    const double radius = static_cast<double>(FromDIP(8, this));
    path.AddRoundedRectangle(0.0, 0.0,
                             static_cast<double>(size.x),
                             static_cast<double>(size.y),
                             radius);

    wxColour bg(30, 144, 255, alpha);   // Dodger blue
    wxColour border(255, 255, 255, alpha);
    wxColour textCol(255, 255, 255, alpha);

    gc->SetBrush(wxBrush(bg));
    gc->SetPen(wxPen(border, 1.5));
    gc->FillPath(path);
    gc->StrokePath(path);

    gc->SetFont(GetFont(), textCol);
    gc->DrawText(m_text,
                 static_cast<double>(FromDIP(16, this)),
                 static_cast<double>(FromDIP(12, this)));
#else
    // Fallback: draw simple text without animation.
    dc.DrawText(m_text, 2, 2);
#endif // wxUSE_GRAPHICS_CONTEXT
}

