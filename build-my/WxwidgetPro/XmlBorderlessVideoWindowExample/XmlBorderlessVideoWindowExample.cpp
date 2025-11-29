// XmlBorderlessVideoWindowExample.cpp
// Demo implementing the window requirements described in build-my docs,
// using an XRC-described UI.
//
// Requirements:
//  1) Mode 1: full-screen, top-most, borderless window with only a video
//     rendering control.
//  2) Mode 2: non-fullscreen borderless window (initial size 800x600) with
//     video rendering control, a custom title bar (right side has minimize and
//     maximize/fullscreen buttons) and a taskbar icon.
//
// UI layout (title bar, buttons, video area) is described in
//  rc/video_window.xrc and loaded via wxXmlResource at runtime.

#include <memory>

#include <wx/app.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/xrc/xmlres.h>
#include <wx/artprov.h>
#include <wx/log.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>

#include "BorderlessRoundedWindowBase.h"

// ---------------------------------------------------------------------------
// VideoRenderPanel: placeholder for the real video rendering control
// ---------------------------------------------------------------------------
//
// This control is instantiated from XRC via subclass="VideoRenderPanel" in
// rc/video_window.xrc. It currently draws a simple placeholder background and
// text; real projects can replace its OnPaint implementation with actual
// video rendering logic.

class VideoRenderPanel : public wxPanel
{
public:
    wxDECLARE_DYNAMIC_CLASS(VideoRenderPanel);
    wxDECLARE_EVENT_TABLE();

    VideoRenderPanel();
    VideoRenderPanel(wxWindow* parent,
                     wxWindowID id,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxBORDER_NONE,
                     const wxString& name = wxPanelNameStr);

private:
    void OnPaint(wxPaintEvent& event);
};

wxIMPLEMENT_DYNAMIC_CLASS(VideoRenderPanel, wxPanel);

wxBEGIN_EVENT_TABLE(VideoRenderPanel, wxPanel)
    EVT_PAINT(VideoRenderPanel::OnPaint)
wxEND_EVENT_TABLE()

VideoRenderPanel::VideoRenderPanel()
    : wxPanel()
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

VideoRenderPanel::VideoRenderPanel(wxWindow* parent,
                                   wxWindowID id,
                                   const wxPoint& pos,
                                   const wxSize& size,
                                   long style,
                                   const wxString& name)
    : wxPanel(parent, id, pos, size, style, name)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void VideoRenderPanel::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();

#if wxUSE_GRAPHICS_CONTEXT
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if ( !gc )
        return;

    const wxSize size = GetClientSize();

    // Dark background to emulate a video surface.
    wxColour bg(10, 10, 10);
    gc->SetBrush(wxBrush(bg));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawRectangle(0, 0, size.x, size.y);

    // Simple placeholder text in the center.
    gc->SetFont(GetFont(), *wxWHITE);

    const wxString text = "Video render area";
    wxDouble tw = 0.0;
    wxDouble th = 0.0;
    gc->GetTextExtent(text, &tw, &th);

    const wxDouble x = (size.x - tw) / 2.0;
    const wxDouble y = (size.y - th) / 2.0;

    gc->DrawText(text, x, y);
#else
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
    dc.SetTextForeground(*wxWHITE);
    dc.DrawText("Video render area", 10, 10);
#endif // wxUSE_GRAPHICS_CONTEXT
}

// ---------------------------------------------------------------------------
// VideoWindowFrame: borderless rounded window with two modes
// ---------------------------------------------------------------------------

class VideoWindowFrame : public BorderlessRoundedWindowBase
{
public:
    VideoWindowFrame();

private:
    wxPanel*          m_rootPanel;
    wxPanel*          m_titleBarPanel;
    VideoRenderPanel* m_videoPanel;
    wxButton*         m_btnMinimize;
    wxButton*         m_btnToggleFullscreen;

    bool              m_isFullscreen;
    int               m_savedCornerRadius;

    void InitializeFromXrc();

    void EnterFullscreenMode();
    void ExitFullscreenMode();
    void ToggleFullscreenMode();

    void OnMinimize(wxCommandEvent& event);
    void OnToggleFullscreen(wxCommandEvent& event);
    void OnKeyDown(wxKeyEvent& event);
};

VideoWindowFrame::VideoWindowFrame()
    : BorderlessRoundedWindowBase(
          nullptr,
          wxID_ANY,
          "Borderless video window",
          wxDefaultPosition,
          wxSize(800, 600),          // Mode 2 initial size
          wxSTAY_ON_TOP,             // Always-on-top for both modes
          14                         // corner radius in DIP for windowed mode
      ),
      m_rootPanel(nullptr),
      m_titleBarPanel(nullptr),
      m_videoPanel(nullptr),
      m_btnMinimize(nullptr),
      m_btnToggleFullscreen(nullptr),
      m_isFullscreen(false),
      m_savedCornerRadius(GetCornerRadiusDIP())
{
    // Provide a frame icon so the window has a taskbar icon in mode 2.
    SetIcon(wxArtProvider::GetIcon(wxART_INFORMATION, wxART_FRAME_ICON));

    InitializeFromXrc();
}

void VideoWindowFrame::InitializeFromXrc()
{
    // Load the root panel defined in rc/video_window.xrc.
    m_rootPanel = wxXmlResource::Get()->LoadPanel(this, "video_window_root");
    if ( !m_rootPanel )
    {
        wxLogError("Failed to load XRC panel 'video_window_root'.");
        return;
    }

    auto* frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(m_rootPanel, 1, wxEXPAND);
    SetSizer(frameSizer);
    Layout();

    // Fetch child controls by their XRC names.
    m_titleBarPanel = XRCCTRL(*this, "title_bar_panel", wxPanel);
    m_videoPanel = XRCCTRL(*this, "video_render_panel", VideoRenderPanel);
    m_btnMinimize = XRCCTRL(*this, "btn_minimize", wxButton);
    m_btnToggleFullscreen = XRCCTRL(*this, "btn_toggle_fullscreen", wxButton);

    // Dragging on the title bar moves the window.
    if ( m_titleBarPanel )
        AttachDragHandlers(m_titleBarPanel);

    if ( m_btnMinimize )
    {
        m_btnMinimize->Bind(wxEVT_BUTTON,
                            &VideoWindowFrame::OnMinimize,
                            this);
    }

    if ( m_btnToggleFullscreen )
    {
        m_btnToggleFullscreen->Bind(wxEVT_BUTTON,
                                    &VideoWindowFrame::OnToggleFullscreen,
                                    this);
    }

    // Keyboard shortcut: F11 toggles fullscreen, Esc exits fullscreen.
    Bind(wxEVT_KEY_DOWN, &VideoWindowFrame::OnKeyDown, this);
}

void VideoWindowFrame::EnterFullscreenMode()
{
    if ( m_isFullscreen )
        return;

    m_isFullscreen = true;

    if ( m_titleBarPanel )
        m_titleBarPanel->Hide();

    // In fullscreen we want a pure rectangle without rounded corners to
    // cover the whole screen.
    m_savedCornerRadius = GetCornerRadiusDIP();
    SetCornerRadiusDIP(0);

    ShowFullScreen(true);

    Layout();
}

void VideoWindowFrame::ExitFullscreenMode()
{
    if ( !m_isFullscreen )
        return;

    m_isFullscreen = false;

    ShowFullScreen(false);

    if ( m_titleBarPanel )
        m_titleBarPanel->Show();

    // Restore rounded corners for non-fullscreen mode.
    SetCornerRadiusDIP(m_savedCornerRadius);

    Layout();
}

void VideoWindowFrame::ToggleFullscreenMode()
{
    if ( m_isFullscreen )
        ExitFullscreenMode();
    else
        EnterFullscreenMode();
}

void VideoWindowFrame::OnMinimize(wxCommandEvent& WXUNUSED(event))
{
    Iconize(true);
}

void VideoWindowFrame::OnToggleFullscreen(wxCommandEvent& WXUNUSED(event))
{
    ToggleFullscreenMode();
}

void VideoWindowFrame::OnKeyDown(wxKeyEvent& event)
{
    const int key = event.GetKeyCode();

    if ( key == WXK_F11 )
    {
        ToggleFullscreenMode();
    }
    else if ( key == WXK_ESCAPE && m_isFullscreen )
    {
        ExitFullscreenMode();
    }
    else
    {
        event.Skip();
    }
}

// ---------------------------------------------------------------------------
// Application entry point
// ---------------------------------------------------------------------------

class VideoWindowApp : public wxApp
{
public:
    bool OnInit() override;
};

wxIMPLEMENT_APP(VideoWindowApp);

bool VideoWindowApp::OnInit()
{
    if ( !wxApp::OnInit() )
        return false;

    // Initialize standard XRC handlers and enable locale/envvar support.
    wxXmlResource::Get()->InitAllHandlers();
    wxXmlResource::Get()->SetFlags(wxXRC_USE_LOCALE | wxXRC_USE_ENVVARS);

    // Load all XRC resources from the "rc" directory located next to the
    // executable. Using an absolute path avoids accidentally picking up the
    // wxWidgets library rc/ directory when the working directory differs.
    const wxString exeDir =
        wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();
    const wxString rcDir = exeDir + wxFILE_SEP_PATH + "rc";

    if ( !wxXmlResource::Get()->LoadAllFiles(rcDir) )
    {
        wxLogError("Failed to load XRC resources from '%s'.", rcDir);
        return false;
    }

    auto* frame = new VideoWindowFrame();
    frame->Show();
    return true;
}
