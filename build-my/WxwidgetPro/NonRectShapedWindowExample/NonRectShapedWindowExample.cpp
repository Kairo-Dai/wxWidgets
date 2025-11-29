// NonRectShapedWindowExample.cpp
// Demo: non-rectangular shaped top-level window using SetShape().

#include <wx/app.h>
#include <wx/frame.h>
#include <wx/graphics.h>
#include <wx/dcbuffer.h>

class ShapedDemoFrame : public wxFrame
{
public:
    ShapedDemoFrame();

private:
    enum ShapeKind
    {
        ShapeCircle = 0,
        ShapeRoundedRect,
        ShapeMax
    };

    ShapeKind m_shape;
    bool      m_dragging;
    wxPoint   m_dragDelta;

    void UpdateShape();

    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);
    void OnLeftDClick(wxMouseEvent& event);

    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(ShapedDemoFrame, wxFrame)
    EVT_PAINT(ShapedDemoFrame::OnPaint)
    EVT_SIZE(ShapedDemoFrame::OnSize)
    EVT_LEFT_DOWN(ShapedDemoFrame::OnLeftDown)
    EVT_MOTION(ShapedDemoFrame::OnMouseMove)
    EVT_LEFT_UP(ShapedDemoFrame::OnLeftUp)
    EVT_LEFT_DCLICK(ShapedDemoFrame::OnLeftDClick)
wxEND_EVENT_TABLE()

ShapedDemoFrame::ShapedDemoFrame()
    : wxFrame(nullptr,
              wxID_ANY,
              "Non-rectangular shaped window",
              wxDefaultPosition,
              wxSize(320, 240),
              wxFRAME_SHAPED | wxBORDER_NONE | wxSTAY_ON_TOP),
      m_shape(ShapeCircle),
      m_dragging(false),
      m_dragDelta(0, 0)
{
    // We fully draw background in OnPaint().
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    UpdateShape();
}

void ShapedDemoFrame::UpdateShape()
{
#if wxUSE_GRAPHICS_CONTEXT
    wxGraphicsRenderer* renderer = wxGraphicsRenderer::GetDefaultRenderer();
    if ( !renderer )
        return;

    const wxSize size = GetClientSize();
    if ( size.x <= 0 || size.y <= 0 )
        return;

    wxGraphicsPath path = renderer->CreatePath();

    switch ( m_shape )
    {
        case ShapeCircle:
        {
            const double radius = std::min(size.x, size.y) / 2.0;
            path.AddCircle(size.x / 2.0, size.y / 2.0, radius);
            break;
        }

        case ShapeRoundedRect:
        {
            const double radius = FromDIP(24, this);
            path.AddRoundedRectangle(0.0, 0.0,
                                     static_cast<double>(size.x),
                                     static_cast<double>(size.y),
                                     radius);
            break;
        }

        case ShapeMax:
        default:
            break;
    }

    if ( m_shape == ShapeCircle || m_shape == ShapeRoundedRect )
        SetShape(path);
    else
        SetShape(wxRegion()); // fall back to normal rectangle
#endif // wxUSE_GRAPHICS_CONTEXT
}

void ShapedDemoFrame::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();

#if wxUSE_GRAPHICS_CONTEXT
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if ( !gc )
        return;

    const wxSize size = GetClientSize();

    // Draw the same basic shape we used for SetShape() for a consistent look.
    wxGraphicsPath path = gc->CreatePath();

    if ( m_shape == ShapeCircle )
    {
        const double radius = std::min(size.x, size.y) / 2.0;
        path.AddCircle(size.x / 2.0, size.y / 2.0, radius);
    }
    else // ShapeRoundedRect
    {
        const double radius = FromDIP(24, this);
        path.AddRoundedRectangle(0.0, 0.0,
                                 static_cast<double>(size.x),
                                 static_cast<double>(size.y),
                                 radius);
    }

    wxColour bg(30, 144, 255, 230);   // slightly transparent blue
    wxColour border(255, 255, 255, 255);

    gc->SetBrush(wxBrush(bg));
    gc->SetPen(wxPen(border, 2.0));
    gc->FillPath(path);
    gc->StrokePath(path);

    gc->SetFont(GetFont(), *wxWHITE);
    gc->DrawText(
        "This is a non-rectangular shaped window.\n"
        "Left-drag to move it.\n"
        "Double-click to toggle shape.",
        FromDIP(18, this),
        FromDIP(18, this)
    );
#endif // wxUSE_GRAPHICS_CONTEXT
}

void ShapedDemoFrame::OnSize(wxSizeEvent& event)
{
    if ( !IsIconized() )
        UpdateShape();

    event.Skip();
}

void ShapedDemoFrame::OnLeftDown(wxMouseEvent& event)
{
    CaptureMouse();

    const wxPoint posScreen = ClientToScreen(event.GetPosition());
    const wxPoint origin    = GetPosition();

    m_dragDelta = wxPoint(posScreen.x - origin.x,
                          posScreen.y - origin.y);
    m_dragging = true;
}

void ShapedDemoFrame::OnMouseMove(wxMouseEvent& event)
{
    if ( m_dragging && event.LeftIsDown() )
    {
        const wxPoint posScreen = ClientToScreen(event.GetPosition());
        Move(wxPoint(posScreen.x - m_dragDelta.x,
                     posScreen.y - m_dragDelta.y));
    }
}

void ShapedDemoFrame::OnLeftUp(wxMouseEvent& WXUNUSED(event))
{
    if ( HasCapture() )
        ReleaseMouse();

    m_dragging = false;
}

void ShapedDemoFrame::OnLeftDClick(wxMouseEvent& WXUNUSED(event))
{
    // Cycle between the available shapes.
    m_shape = static_cast<ShapeKind>((m_shape + 1) % ShapeMax);
    UpdateShape();
    Refresh();
}

class ShapedWindowApp : public wxApp
{
public:
    bool OnInit() override
    {
        if ( !wxApp::OnInit() )
            return false;

        auto* frame = new ShapedDemoFrame();
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP(ShapedWindowApp);

