/***************************************************************
                            app.cpp

This file handles the wxWidgets initialization.
***************************************************************/

#include "app.h"
#include "Resources/icon_play.h"
#include "Resources/icon_program.h"

wxIMPLEMENT_APP(App);


/*********************************
             Globals
*********************************/

wxCursor cursor_blank = wxNullCursor;
wxBitmap icon_play    = wxNullBitmap;
wxIcon   icon_program = wxNullIcon;


/*==============================
    App (Constructor)
    Initializes the class
==============================*/

App::App()
{

}


/*==============================
    App (Destructor)
    Cleans up the class before deletion
==============================*/

App::~App()
{
    
}


/*==============================
    App::OnInit
    Called when the application is initialized
    @returns Whether the application initialized correctly
==============================*/

bool App::OnInit()
{
    if (!wxApp::OnInit())
        return false;

    // Initialize image handlers
    wxInitAllImageHandlers();

    // Initialize icons
    icon_play = wxBITMAP_PNG_FROM_DATA(icon_play);
    wxBitmap temp = wxBITMAP_PNG_FROM_DATA(icon_program);
    icon_program.CopyFromBitmap(temp);

    // Initialize blank cursor
    wxBitmap img = wxBitmap(1, 1, 1);
    wxBitmap mask = wxBitmap(1, 1, 1);
    img.SetMask(new wxMask(mask));
    cursor_blank = wxCursor(img.ConvertToImage());

    // Show the main window
    this->m_frame1 = new Main();
    this->m_frame1->SetIcon(icon_program);
    this->m_frame1->Show();
    SetTopWindow(this->m_frame1);
    return true;
}