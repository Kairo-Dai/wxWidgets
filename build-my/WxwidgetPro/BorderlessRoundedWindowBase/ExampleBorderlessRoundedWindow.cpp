// ExampleBorderlessRoundedWindow.cpp
// Minimal example showing how to use BorderlessRoundedWindowBase.

#include <wx/app.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>

#include "BorderlessRoundedWindowBase.h"

class MyRoundedWindow : public BorderlessRoundedWindowBase
{
public:
    MyRoundedWindow()
        : BorderlessRoundedWindowBase(
              nullptr,
              wxID_ANY,
              "Borderless rounded window",
              wxDefaultPosition,
              wxSize(420, 240),
              // Extra style: do not show in taskbar, stay on top.
              wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP,
              14) // corner radius in DIP
    {
        // Create a background panel that fills the client area.
        wxPanel* panel = new wxPanel(this);

        // Attach drag handlers so dragging on the panel moves the window.
        AttachDragHandlers(panel);

        // Simple content: text + a close button.
        auto* sizer = new wxBoxSizer(wxVERTICAL);

        auto* label = new wxStaticText(panel, wxID_ANY,
            "This is a borderless, rounded-rectangle window.\n"
            "Drag anywhere on the background to move it."
        );

        auto* buttonClose = new wxButton(panel, wxID_ANY, "Close");
        buttonClose->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ Close(); });

        sizer->Add(label, 0, wxALIGN_CENTER | wxALL, FromDIP(16, panel));
        sizer->AddStretchSpacer();
        sizer->Add(buttonClose, 0,
                   wxALIGN_CENTER | wxBOTTOM, FromDIP(16, panel));

        panel->SetSizer(sizer);

        // Optional: you can change the radius at runtime.
        // SetCornerRadiusDIP(20);
    }
};

class MyApp : public wxApp
{
public:
    bool OnInit() override
    {
        if ( !wxApp::OnInit() )
            return false;

        MyRoundedWindow* win = new MyRoundedWindow();
        win->Show();
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);
