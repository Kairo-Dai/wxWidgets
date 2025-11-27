// BorderlessRoundedWindowBase.h
// Base class for creating borderless, rounded-rectangle top level windows
// Cross-platform: wxMSW + wxGTK (X11/Wayland; shape support may vary under Wayland).

#pragma once

#include <wx/frame.h>
#include <wx/graphics.h>

// Base class: a wxFrame with
//   - wxFRAME_SHAPED + wxBORDER_NONE enforced
//   - rounded-rectangle shape updated on resize
//   - optional drag-to-move helpers
//
// Usage notes:
//   * Do NOT pass wxDEFAULT_FRAME_STYLE in extraStyle. Start from 0 and
//     add only what you need (e.g. wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP).
//   * On Linux/Wayland, SetShape() support depends on the compositor.
class BorderlessRoundedWindowBase : public wxFrame
{
public:
    BorderlessRoundedWindowBase(wxWindow* parent,
                                wxWindowID id = wxID_ANY,
                                const wxString& title = wxEmptyString,
                                const wxPoint& pos = wxDefaultPosition,
                                const wxSize& size = wxSize(360, 200),
                                long extraStyle = 0,
                                int cornerRadiusDIP = 12);

    // Change corner radius (in logical/DIP pixels) and recompute shape.
    void SetCornerRadiusDIP(int radiusDIP);

    int GetCornerRadiusDIP() const { return m_cornerRadiusDIP; }

protected:
    // Rebuild rounded-rectangle shape for current window size.
    void UpdateShape();

    // Attach drag handlers to a child window (typically a background panel).
    // After this, pressing and dragging on that child will move the TLW.
    void AttachDragHandlers(wxWindow* win);

    // Event handlers used for drag-to-move and resize.
    void OnSize(wxSizeEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);

private:
    wxPoint m_dragDelta;      // difference between mouse and window origin
    bool    m_dragging;       // currently dragging window by mouse
    int     m_cornerRadiusDIP;

    wxDECLARE_EVENT_TABLE();
};
