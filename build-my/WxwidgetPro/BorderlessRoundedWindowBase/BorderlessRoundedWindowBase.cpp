// BorderlessRoundedWindowBase.cpp

#include "BorderlessRoundedWindowBase.h"

#include <wx/event.h>

wxBEGIN_EVENT_TABLE(BorderlessRoundedWindowBase, wxFrame)
    EVT_SIZE(BorderlessRoundedWindowBase::OnSize)
wxEND_EVENT_TABLE()

BorderlessRoundedWindowBase::BorderlessRoundedWindowBase(wxWindow* parent,
                                                         wxWindowID id,
                                                         const wxString& title,
                                                         const wxPoint& pos,
                                                         const wxSize& size,
                                                         long extraStyle,
                                                         int cornerRadiusDIP)
    : wxFrame(parent,
              id,
              title,
              pos,
              size,
              // Enforce shaped + borderless; caller supplies only extra flags.
              (extraStyle | wxFRAME_SHAPED | wxBORDER_NONE)),
      m_dragDelta(0, 0),
      m_dragging(false),
      m_cornerRadiusDIP(cornerRadiusDIP)
{
    // Initial shape.
    UpdateShape();

    // Default: allow dragging by clicking on the frame's client area.
    // This gives basic drag-to-move behaviour even if the caller does not
    // explicitly call AttachDragHandlers() on a child window.
    AttachDragHandlers(this);

    // Callers can still attach drag handlers to specific children (e.g. a
    // background panel) via AttachDragHandlers(child) if they want to limit
    // the draggable region.
}

void BorderlessRoundedWindowBase::SetCornerRadiusDIP(int radiusDIP)
{
    m_cornerRadiusDIP = radiusDIP;
    UpdateShape();
}

void BorderlessRoundedWindowBase::UpdateShape()
{
#if wxUSE_GRAPHICS_CONTEXT
    const wxSize size = GetSize();
    if (size.x <= 0 || size.y <= 0)
        return;

    wxGraphicsRenderer* renderer = wxGraphicsRenderer::GetDefaultRenderer();
    if (!renderer)
        return;

    wxGraphicsPath path = renderer->CreatePath();

    double radius = static_cast<double>(FromDIP(m_cornerRadiusDIP, this));
    const double maxR = (std::min(size.x, size.y) / 2.0) - 1.0;
    if (radius > maxR && maxR > 0.0)
        radius = maxR;
    if (radius < 0.0)
        radius = 0.0;

    path.AddRoundedRectangle(0.0, 0.0,
                             static_cast<double>(size.x),
                             static_cast<double>(size.y),
                             radius);

    SetShape(path);
#endif // wxUSE_GRAPHICS_CONTEXT
}

void BorderlessRoundedWindowBase::AttachDragHandlers(wxWindow* win)
{
    if (!win)
        return;

    win->Bind(wxEVT_LEFT_DOWN, &BorderlessRoundedWindowBase::OnLeftDown, this);
    win->Bind(wxEVT_MOTION,    &BorderlessRoundedWindowBase::OnMouseMove, this);
    win->Bind(wxEVT_LEFT_UP,   &BorderlessRoundedWindowBase::OnLeftUp,   this);
}

void BorderlessRoundedWindowBase::OnSize(wxSizeEvent& event)
{
    if (!IsIconized())
        UpdateShape();

    event.Skip();
}

void BorderlessRoundedWindowBase::OnLeftDown(wxMouseEvent& event)
{
    CaptureMouse();

    wxPoint posScreen;
    if (auto* src = wxDynamicCast(event.GetEventObject(), wxWindow))
    {
        // Convert from the source window's client coords to screen.
        posScreen = src->ClientToScreen(event.GetPosition());
    }
    else
    {
        posScreen = ClientToScreen(event.GetPosition());
    }

    const wxPoint origin    = GetPosition();

    m_dragDelta = wxPoint(posScreen.x - origin.x, posScreen.y - origin.y);
    m_dragging  = true;
}

void BorderlessRoundedWindowBase::OnMouseMove(wxMouseEvent& event)
{
    if (m_dragging && event.LeftIsDown())
    {
        wxPoint posScreen;
        if (auto* src = wxDynamicCast(event.GetEventObject(), wxWindow))
        {
            posScreen = src->ClientToScreen(event.GetPosition());
        }
        else
        {
            posScreen = ClientToScreen(event.GetPosition());
        }

        Move(wxPoint(posScreen.x - m_dragDelta.x,
                     posScreen.y - m_dragDelta.y));
    }
}

void BorderlessRoundedWindowBase::OnLeftUp(wxMouseEvent& event)
{
    if (HasCapture())
        ReleaseMouse();

    m_dragging = false;
}
