#include "main.h"
#include <wx/textfile.h>
#include <wx/tokenzr.h>
#include <wx/dir.h>


/*********************************
              Macros
*********************************/

#define PROGRAM_FRAMERATE    60.0f
#define ANIMATION_FRAMERATE  30.0f
#define DEFAULT_PLAYBACK     (ANIMATION_FRAMERATE/PROGRAM_FRAMERATE)


/*********************************
             Globals
*********************************/

bool settings_showgrid         = true;
bool settings_showorigin       = true;
bool settings_showlighting     = true;
bool settings_showmeshroots    = false;
bool settings_yaxisup          = false;
bool settings_showhighlight    = true;
bool settings_animating        = true;
float settings_playbackspeed   = DEFAULT_PLAYBACK;
bool settings_animatingreverse = false;


/*********************************
            Main Class
*********************************/

/*==============================
    Main (Constructor)
    Initializes the class
==============================*/

Main::Main() : wxFrame(nullptr, wxID_ANY, PROGRAM_NAME, wxPoint(0, 0), wxSize(640, 480))
{
    this->m_Sausage64Model = NULL;
    this->SetSizeHints(wxDefaultSize, wxDefaultSize);

    // Menu bar 'File'
    this->m_MenuBar = new wxMenuBar(0);
    this->m_Menu_File = new wxMenu();
    wxMenuItem* m_MenuItem_FileImport;
    m_MenuItem_FileImport = new wxMenuItem(this->m_Menu_File, wxID_ANY, wxString(wxT("Import S64 Model")) + wxT('\t') + wxT("Ctrl+I"), wxEmptyString, wxITEM_NORMAL);
    this->m_Menu_File->Append(m_MenuItem_FileImport);
    this->m_MenuItem_FileTextureImport = new wxMenuItem(this->m_Menu_File, wxID_ANY, wxString(wxT("Import Texture Definition")) + wxT('\t') + wxT("Ctrl+T"), wxEmptyString, wxITEM_NORMAL);
    this->m_Menu_File->Append(this->m_MenuItem_FileTextureImport);
    this->m_Menu_File->AppendSeparator();
    this->m_MenuItem_FileTextureExport = new wxMenuItem(this->m_Menu_File, wxID_ANY, wxString(wxT("Export Texture Definition")) + wxT('\t') + wxT("Ctrl+E"), wxEmptyString, wxITEM_NORMAL);
    this->m_Menu_File->Append(this->m_MenuItem_FileTextureExport);
    this->m_MenuBar->Append(this->m_Menu_File, wxT("File"));
    this->m_MenuItem_FileTextureImport->Enable(false);
    this->m_MenuItem_FileTextureExport->Enable(false);

    // Menu bar 'Animation'
    this->m_Menu_Animation = new wxMenu();
    this->m_MenuItem_AnimationPlayToggle = new wxMenuItem(this->m_Menu_Animation, wxID_ANY, wxString(wxT("Play Animation")) + wxT('\t') + wxT("Ctrl+P"), wxEmptyString, wxITEM_CHECK);
    this->m_Menu_Animation->Append(this->m_MenuItem_AnimationPlayToggle);
    this->m_MenuItem_AnimationPlayToggle->Check(settings_animating);
    this->m_MenuItem_AnimationReverse = new wxMenuItem(this->m_Menu_Animation, wxID_ANY, wxString(wxT("Reverse")) + wxT('\t') + wxT("Ctrl+R"), wxEmptyString, wxITEM_CHECK);
    this->m_Menu_Animation->Append(this->m_MenuItem_AnimationReverse);
    this->m_MenuItem_AnimationReverse->Check(settings_animatingreverse);
    this->m_Menu_Animation->AppendSeparator();
    this->m_SubMenu_AnimationSpeed = new wxMenu();
    wxMenuItem* m_SubMenu_AnimationSpeedItem = new wxMenuItem(this->m_Menu_Animation, wxID_ANY, wxT("Playback Speed"), wxEmptyString, wxITEM_NORMAL, this->m_SubMenu_AnimationSpeed);
    this->m_MenuItem_AnimationFaster = new wxMenuItem(this->m_SubMenu_AnimationSpeed, wxID_ANY, wxString(wxT("Faster")) + wxT('\t') + wxT("Ctrl+Up"), wxEmptyString, wxITEM_NORMAL);
    this->m_SubMenu_AnimationSpeed->Append(this->m_MenuItem_AnimationFaster);
    this->m_MenuItem_AnimationNormal = new wxMenuItem(this->m_SubMenu_AnimationSpeed, wxID_ANY, wxString(wxT("Normal")) + wxT('\t') + wxT("Ctrl+Right"), wxEmptyString, wxITEM_NORMAL);
    this->m_SubMenu_AnimationSpeed->Append(this->m_MenuItem_AnimationNormal);
    this->m_MenuItem_AnimationSlower = new wxMenuItem(this->m_SubMenu_AnimationSpeed, wxID_ANY, wxString(wxT("Slower")) + wxT('\t') + wxT("Ctrl+Down"), wxEmptyString, wxITEM_NORMAL);
    this->m_SubMenu_AnimationSpeed->Append(this->m_MenuItem_AnimationSlower);
    this->m_Menu_Animation->Append(m_SubMenu_AnimationSpeedItem);
    this->m_MenuBar->Append(this->m_Menu_Animation, wxT("Animation"));

    // Menu Bar 'View'
    this->m_Menu_View = new wxMenu();
    wxMenuItem* m_MenuItem_ViewGrid;
    m_MenuItem_ViewGrid = new wxMenuItem(this->m_Menu_View, wxID_ANY, wxString(wxT("Grid")), wxEmptyString, wxITEM_CHECK);
    this->m_Menu_View->Append(m_MenuItem_ViewGrid);
    m_MenuItem_ViewGrid->Check(settings_showgrid);
    wxMenuItem* m_MenuItem_ViewOrigin;
    m_MenuItem_ViewOrigin = new wxMenuItem(this->m_Menu_View, wxID_ANY, wxString(wxT("Origin")), wxEmptyString, wxITEM_CHECK);
    this->m_Menu_View->Append(m_MenuItem_ViewOrigin);
    m_MenuItem_ViewOrigin->Check(settings_showorigin);
    wxMenuItem* m_MenuItem_ViewLighting;
    m_MenuItem_ViewLighting = new wxMenuItem(this->m_Menu_View, wxID_ANY, wxString(wxT("Lighting")), wxEmptyString, wxITEM_CHECK);
    this->m_Menu_View->Append(m_MenuItem_ViewLighting);
    m_MenuItem_ViewLighting->Check(settings_showlighting);
    wxMenuItem* m_MenuItem_ViewMeshRoots;
    m_MenuItem_ViewMeshRoots = new wxMenuItem(this->m_Menu_View, wxID_ANY, wxString(wxT("Mesh Roots")), wxEmptyString, wxITEM_CHECK);
    this->m_Menu_View->Append(m_MenuItem_ViewMeshRoots);
    m_MenuItem_ViewMeshRoots->Check(settings_showmeshroots);
    m_Menu_View->AppendSeparator();
    wxMenuItem* m_MenuItem_ViewYUp;
    m_MenuItem_ViewYUp = new wxMenuItem(this->m_Menu_View, wxID_ANY, wxString(wxT("Y Axis Up")), wxEmptyString, wxITEM_CHECK);
    this->m_Menu_View->Append(m_MenuItem_ViewYUp);
    m_MenuItem_ViewYUp->Check(settings_yaxisup);
    wxMenuItem* m_MenuItem_ViewHighlight;
    m_MenuItem_ViewHighlight = new wxMenuItem(this->m_Menu_View, wxID_ANY, wxString(wxT("Highlight Selected")), wxEmptyString, wxITEM_CHECK);
    this->m_Menu_View->Append(m_MenuItem_ViewHighlight);
    m_MenuItem_ViewHighlight->Check(settings_showhighlight);
    this->m_MenuBar->Append(this->m_Menu_View, wxT("View"));

    // Create the menu bar
    this->SetMenuBar(this->m_MenuBar);

    // Create the main sizer
    wxBoxSizer* m_Sizer_Main;
    m_Sizer_Main = new wxBoxSizer(wxVERTICAL);

    // Create the horizontal split
    this->m_Splitter_Horizontal = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE);
    this->m_Splitter_Horizontal->SetSashGravity(1);
    this->m_Splitter_Horizontal->SetMinimumPaneSize(1);
    this->m_Splitter_Horizontal->Connect(wxEVT_IDLE, wxIdleEventHandler(Main::m_Splitter_HorizontalOnIdle), NULL, this);

    // Make the top pane
    this->m_Panel_Top = new wxPanel(this->m_Splitter_Horizontal, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    wxBoxSizer* m_Sizer_Top;
    m_Sizer_Top = new wxBoxSizer(wxVERTICAL);

    // Split the top pane vertically
    this->m_Splitter_Vertical = new wxSplitterWindow(this->m_Panel_Top, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_3DSASH|wxSP_LIVE_UPDATE);
    this->m_Splitter_Vertical->SetSashGravity(1);
    this->m_Splitter_Vertical->SetMinimumPaneSize(1);
    this->m_Splitter_Vertical->Connect(wxEVT_IDLE, wxIdleEventHandler(Main::m_Splitter_VerticalOnIdle), NULL, this);

    // Create our OpenGL canvas
	wxGLAttributes args;
	args.PlatformDefaults().Depth(24).RGBA().DoubleBuffer().EndList();
    this->m_Model_Canvas = new ModelCanvas(this->m_Splitter_Vertical, args, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER, "");
    this->m_Model_Canvas->SetApp(this);

    // Create the panel to hold our TreeCtrl
    this->m_Panel_TreeCtrl = new wxPanel(this->m_Splitter_Vertical, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN|wxTAB_TRAVERSAL);
    wxBoxSizer* m_Sizer_TreeCtrl;
    m_Sizer_TreeCtrl = new wxBoxSizer(wxHORIZONTAL);

    // Create the model data TreeCtrl
    this->m_TreeCtrl_ModelData = new wxTreeCtrl(this->m_Panel_TreeCtrl, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE|wxTR_SINGLE|wxHSCROLL|wxVSCROLL);
    m_Sizer_TreeCtrl->Add(this->m_TreeCtrl_ModelData, 1, wxEXPAND, 5);
    this->m_Panel_TreeCtrl->SetSizer(m_Sizer_TreeCtrl);
    this->m_Panel_TreeCtrl->Layout();
    m_Sizer_TreeCtrl->Fit(this->m_Panel_TreeCtrl);

    // Actually perform the vertical window splitting
    this->m_Splitter_Vertical->SplitVertically(this->m_Model_Canvas, this->m_Panel_TreeCtrl, 400);
    m_Sizer_Top->Add(m_Splitter_Vertical, 1, wxEXPAND, 5);

    // Adjust the top panel contents
    this->m_Panel_Top->SetSizer(m_Sizer_Top);
    this->m_Panel_Top->Layout();
    m_Sizer_Top->Fit(this->m_Panel_Top);

    // Create the horizontal splitter
    this->m_Panel_Bottom = new wxPanel(this->m_Splitter_Horizontal, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

    this->m_Sizer_Bottom = new wxBoxSizer(wxVERTICAL);
    this->m_Sizer_Bottom_Mesh = new wxBoxSizer(wxVERTICAL);
    this->m_CheckBox_Mesh_Billboard = new wxCheckBox(this->m_Panel_Bottom, wxID_ANY, wxT("Billboard"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_Sizer_Bottom_Mesh->Add(this->m_CheckBox_Mesh_Billboard, 0, wxALL, 5);
    this->m_Sizer_Bottom->Add(this->m_Sizer_Bottom_Mesh, 1, 0, 5); 
    this->m_Sizer_Bottom_Texture = new wxFlexGridSizer(0, 1, 0, 0);
    this->m_Sizer_Bottom_Texture->SetFlexibleDirection(wxBOTH);
    this->m_Sizer_Bottom_Texture->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);
    wxBoxSizer* m_Sizer_Bottom_Texture_Label;
    m_Sizer_Bottom_Texture_Label = new wxBoxSizer(wxVERTICAL);
    wxStaticText* m_Label_TextureType = new wxStaticText(this->m_Panel_Bottom, wxID_ANY, wxT("Texture Type"), wxDefaultPosition, wxDefaultSize, 0);
    m_Label_TextureType->Wrap(-1);
    m_Sizer_Bottom_Texture_Label->Add(m_Label_TextureType, 0, wxALL, 5);
    this->m_Sizer_Bottom_Texture->Add(m_Sizer_Bottom_Texture_Label, 1, wxEXPAND, 5);
    wxBoxSizer* m_Sizer_Bottom_Texture_Type;
    m_Sizer_Bottom_Texture_Type = new wxBoxSizer(wxHORIZONTAL);
    this->m_Radio_Image = new wxRadioButton(this->m_Panel_Bottom, wxID_ANY, wxT("Image"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    this->m_Radio_Image->SetValue(true);
    m_Sizer_Bottom_Texture_Type->Add(this->m_Radio_Image, 0, wxALL, 5);
    this->m_Radio_PrimColor = new wxRadioButton(this->m_Panel_Bottom, wxID_ANY, wxT("Primitive Color"), wxDefaultPosition, wxDefaultSize, 0);
    m_Sizer_Bottom_Texture_Type->Add(this->m_Radio_PrimColor, 0, wxALL, 5);
    this->m_Radio_Omit = new wxRadioButton(this->m_Panel_Bottom, wxID_ANY, wxT("Omit"), wxDefaultPosition, wxDefaultSize, 0);
    m_Sizer_Bottom_Texture_Type->Add(this->m_Radio_Omit, 0, wxALL, 5);
    this->m_Sizer_Bottom_Texture->Add(m_Sizer_Bottom_Texture_Type, 1, wxEXPAND, 5);

    this->m_Sizer_Bottom_Texture_Setup = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* m_Sizer_Bottom_Texture_Image;
    m_Sizer_Bottom_Texture_Image = new wxBoxSizer(wxVERTICAL);
    this->m_Image_Texture = new wxStaticBitmap(this->m_Panel_Bottom, wxID_ANY, texture_missing->GetTextureData()->wxbmp, wxDefaultPosition, wxSize(64, 64), wxBORDER_STATIC);
    m_Sizer_Bottom_Texture_Image->Add(this->m_Image_Texture, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
    this->m_Label_Texture = new wxStaticText(this->m_Panel_Bottom, wxID_ANY, wxT("32x32"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_Label_Texture->Wrap(-1);
    m_Sizer_Bottom_Texture_Image->Add(this->m_Label_Texture, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
    this->m_Sizer_Bottom_Texture_Setup->Add(m_Sizer_Bottom_Texture_Image, 0, 0, 5);
    wxGridSizer* m_Sizer_Bottom_Texture_Settings;
    m_Sizer_Bottom_Texture_Settings = new wxGridSizer(0, 3, 0, 0);
    wxString m_Choice_Texture_ImgFormatChoices[] = {"G_IM_FMT_RGBA", "G_IM_FMT_YUV", "G_IM_FMT_CI", "G_IM_FMT_IA", "G_IM_FMT_I"};
    int m_Choice_Texture_ImgFormatNChoices = sizeof(m_Choice_Texture_ImgFormatChoices) / sizeof(wxString);
    this->m_Choice_Texture_ImgFormat = new wxChoice(this->m_Panel_Bottom, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_Choice_Texture_ImgFormatNChoices, m_Choice_Texture_ImgFormatChoices, 0);
    this->m_Choice_Texture_ImgFormat->SetSelection(0);
    m_Sizer_Bottom_Texture_Settings->Add(this->m_Choice_Texture_ImgFormat, 0, wxALL, 5);
    wxString m_Choice_Texture_SFlagChoices[] = {"G_TX_MIRROR", "G_TX_NOMIRROR", "G_TX_WRAP", "G_TX_CLAMP"};
    int m_Choice_Texture_SFlagNChoices = sizeof(m_Choice_Texture_SFlagChoices) / sizeof(wxString);
    this->m_Choice_Texture_SFlag = new wxChoice(this->m_Panel_Bottom, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_Choice_Texture_SFlagNChoices, m_Choice_Texture_SFlagChoices, 0);
    this->m_Choice_Texture_SFlag->SetSelection(0);
    m_Sizer_Bottom_Texture_Settings->Add(this->m_Choice_Texture_SFlag, 0, wxALL, 5);
    this->m_Button_Texture_Advanced = new wxButton(this->m_Panel_Bottom, wxID_ANY, wxT("Advanced Render Settings"), wxDefaultPosition, wxDefaultSize, 0);
    m_Sizer_Bottom_Texture_Settings->Add(this->m_Button_Texture_Advanced, 0, wxALL, 5);
    wxString m_Choice_Texture_ImgDepthChoices[] = {"G_IM_SIZ_32b", "G_IM_SIZ_16b", "G_IM_SIZ_8b", "G_IM_SIZ_4b"};
    int m_Choice_Texture_ImgDepthNChoices = sizeof(m_Choice_Texture_ImgDepthChoices) / sizeof(wxString);
    this->m_Choice_Texture_ImgDepth = new wxChoice(this->m_Panel_Bottom, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_Choice_Texture_ImgDepthNChoices, m_Choice_Texture_ImgDepthChoices, 0);
    this->m_Choice_Texture_ImgDepth->SetSelection(1);
    m_Sizer_Bottom_Texture_Settings->Add(this->m_Choice_Texture_ImgDepth, 0, wxALL, 5);
    wxString m_Choice_Texture_TFlagChoices[] = {"G_TX_MIRROR", "G_TX_NOMIRROR", "G_TX_WRAP", "G_TX_CLAMP"};
    int m_Choice_Texture_TFlagNChoices = sizeof(m_Choice_Texture_TFlagChoices) / sizeof(wxString);
    this->m_Choice_Texture_TFlag = new wxChoice(this->m_Panel_Bottom, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_Choice_Texture_TFlagNChoices, m_Choice_Texture_TFlagChoices, 0);
    this->m_Choice_Texture_TFlag->SetSelection(0);
    m_Sizer_Bottom_Texture_Settings->Add(this->m_Choice_Texture_TFlag, 0, wxALL, 5);
    wxBoxSizer* m_Sizer_Bottom_Texture_Extra;
    m_Sizer_Bottom_Texture_Extra = new wxBoxSizer(wxVERTICAL);
    this->m_CheckBox_Texture_LoadFirst = new wxCheckBox(this->m_Panel_Bottom, wxID_ANY, wxT("Load First"), wxDefaultPosition, wxDefaultSize, 0);
    m_Sizer_Bottom_Texture_Extra->Add(this->m_CheckBox_Texture_LoadFirst, 0, wxALL, 5);
    this->m_CheckBox_Texture_DontLoad = new wxCheckBox(this->m_Panel_Bottom, wxID_ANY, wxT("Don't Load"), wxDefaultPosition, wxDefaultSize, 0);
    m_Sizer_Bottom_Texture_Extra->Add(this->m_CheckBox_Texture_DontLoad, 0, wxALL, 5);
    m_Sizer_Bottom_Texture_Settings->Add(m_Sizer_Bottom_Texture_Extra, 1, wxEXPAND, 5);
    this->m_Sizer_Bottom_Texture_Setup->Add(m_Sizer_Bottom_Texture_Settings, 1, 0, 5);
    this->m_Sizer_Bottom_Texture->Add(this->m_Sizer_Bottom_Texture_Setup, 1, wxEXPAND, 5);

    this->m_Sizer_Bottom_PrimCol_Setup = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* m_Sizer_Bottom_PrimCol_Image;
    m_Sizer_Bottom_PrimCol_Image = new wxBoxSizer(wxVERTICAL);
    this->m_Panel_PrimCol = new wxPanel(this->m_Panel_Bottom, wxID_ANY, wxDefaultPosition, wxSize(64, 64), wxBORDER_SUNKEN | wxTAB_TRAVERSAL);
    this->m_Panel_PrimCol->SetForegroundColour(wxColour(0, 255, 0));
    this->m_Panel_PrimCol->SetBackgroundColour(wxColour(0, 255, 0));
    this->m_Panel_PrimCol->SetMinSize(wxSize(64, 64));
    this->m_Panel_PrimCol->SetMaxSize(wxSize(64, 64));
    m_Sizer_Bottom_PrimCol_Image->Add(this->m_Panel_PrimCol, 1, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
    this->m_Label_PrimColRGB = new wxStaticText(this->m_Panel_Bottom, wxID_ANY, wxT("0, 255, 0"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_Label_PrimColRGB->Wrap(-1);
    this->m_Label_PrimColHex = new wxStaticText(this->m_Panel_Bottom, wxID_ANY, wxT("00FF00"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_Label_PrimColHex->Wrap(-1);
    m_Sizer_Bottom_PrimCol_Image->Add(this->m_Label_PrimColRGB, 0, wxALIGN_CENTER_HORIZONTAL, 5);
    m_Sizer_Bottom_PrimCol_Image->Add(this->m_Label_PrimColHex, 0, wxALIGN_CENTER_HORIZONTAL, 5);
    this->m_Sizer_Bottom_PrimCol_Setup->Add(m_Sizer_Bottom_PrimCol_Image, 0, 0, 5);
    wxGridSizer* m_Sizer_Bottom_PrimCol_Settings;
    m_Sizer_Bottom_PrimCol_Settings = new wxGridSizer(0, 1, 0, 0);
    this->m_Button_PrimCol_Advanced = new wxButton(this->m_Panel_Bottom, wxID_ANY, wxT("Advanced Render Settings"), wxDefaultPosition, wxDefaultSize, 0);
    m_Sizer_Bottom_PrimCol_Settings->Add(m_Button_PrimCol_Advanced, 0, wxALL, 5);
    wxBoxSizer* m_Sizer_Bottom_PrimCol_Extra;
    m_Sizer_Bottom_PrimCol_Extra = new wxBoxSizer(wxVERTICAL);
    this->m_CheckBox_PrimCol_LoadFirst = new wxCheckBox(this->m_Panel_Bottom, wxID_ANY, wxT("Load First"), wxDefaultPosition, wxDefaultSize, 0);
    m_Sizer_Bottom_PrimCol_Extra->Add(this->m_CheckBox_PrimCol_LoadFirst, 0, wxALL, 5);
    this->m_CheckBox_PrimCol_DontLoad = new wxCheckBox(this->m_Panel_Bottom, wxID_ANY, wxT("Don't Load"), wxDefaultPosition, wxDefaultSize, 0);
    m_Sizer_Bottom_PrimCol_Extra->Add(this->m_CheckBox_PrimCol_DontLoad, 0, wxALL, 5);
    m_Sizer_Bottom_PrimCol_Settings->Add(m_Sizer_Bottom_PrimCol_Extra, 1, wxEXPAND, 5);
    this->m_Sizer_Bottom_PrimCol_Setup->Add(m_Sizer_Bottom_PrimCol_Settings, 1, 0, 5);
    this->m_Sizer_Bottom_Texture->Add(this->m_Sizer_Bottom_PrimCol_Setup, 1, wxEXPAND, 5);

    // Hide both by default
    this->m_Sizer_Bottom_Texture_Setup->Show(false);
    this->m_Sizer_Bottom_PrimCol_Setup->Show(false);

    // Add the animation sizers to the bottom panel
    this->m_Sizer_Bottom_Animation = new wxGridBagSizer(0, 0);
    this->m_Sizer_Bottom_Animation->SetFlexibleDirection(wxBOTH);
    this->m_Sizer_Bottom_Animation->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);
    this->m_Slider_Animation = new wxSlider(this->m_Panel_Bottom, wxID_ANY, 0, 0, 1, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    this->m_Sizer_Bottom_Animation->Add(this->m_Slider_Animation, wxGBPosition(0, 0), wxGBSpan(1, 1), wxALL | wxEXPAND, 5);
    this->m_Button_AnimationToggle = new wxBitmapButton(this->m_Panel_Bottom, wxID_ANY, icon_play, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW | 0);
    this->m_Sizer_Bottom_Animation->Add(this->m_Button_AnimationToggle, wxGBPosition(0, 2), wxGBSpan(1, 1), wxALL, 5);
    this->m_Sizer_Bottom_Animation->AddGrowableCol(0);
    this->m_Sizer_Bottom->Add(this->m_Sizer_Bottom_Animation, 1, wxEXPAND, 5);

    // Add all the sizers to the bottom panel
    this->m_Sizer_Bottom->Add(this->m_Sizer_Bottom_Texture, 1, wxEXPAND, 5);
    this->m_Panel_Bottom->SetSizer(this->m_Sizer_Bottom);
    this->m_Panel_Bottom->Layout();
    this->m_Sizer_Bottom->Fit(this->m_Panel_Bottom);

    // Hide the bottom panels by default
    this->m_Sizer_Bottom_Mesh->ShowItems(false);
    this->m_Sizer_Bottom_Texture->ShowItems(false);
    this->m_Sizer_Bottom_Animation->ShowItems(false);

    // Actually perform the horizontal window splitting
    this->m_Splitter_Horizontal->SplitHorizontally(this->m_Panel_Top, this->m_Panel_Bottom, 300);
    m_Sizer_Main->Add(this->m_Splitter_Horizontal, 1, wxEXPAND, 5);

    // Finalize the layout
    this->SetSizer(m_Sizer_Main);
    this->Layout();
    this->Centre(wxBOTH);

    // Add a main loop
    this->m_Timer_MainLoop.SetOwner(this, wxID_ANY);
    this->m_Timer_MainLoop.Start(1000.0f/PROGRAM_FRAMERATE);
    this->Connect(wxID_ANY, wxEVT_TIMER, wxTimerEventHandler(Main::m_Timer_MainLoopOnTimer));

    // Connect any other events
    this->m_Menu_File->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Main::m_MenuItem_ImportOnMenuSelection), this, m_MenuItem_FileImport->GetId());
    this->m_Menu_File->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Main::m_MenuItem_ImportTextureOnMenuSelection), this, m_MenuItem_FileTextureImport->GetId());
    this->m_Menu_File->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Main::m_MenuItem_ExportTextureOnMenuSelection), this, m_MenuItem_FileTextureExport->GetId());
    this->m_Menu_Animation->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Main::m_MenuItem_AnimationPlayToggleOnMenuSelection), this, this->m_MenuItem_AnimationPlayToggle->GetId());
    this->m_Menu_Animation->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Main::m_MenuItem_AnimationReverseOnMenuSelection), this, this->m_MenuItem_AnimationReverse->GetId());
    this->m_SubMenu_AnimationSpeed->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Main::m_MenuItem_AnimationFasterOnMenuSelection), this, this->m_MenuItem_AnimationFaster->GetId());
    this->m_SubMenu_AnimationSpeed->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Main::m_MenuItem_AnimationNormalOnMenuSelection), this, this->m_MenuItem_AnimationNormal->GetId());
    this->m_SubMenu_AnimationSpeed->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Main::m_MenuItem_AnimationSlowerOnMenuSelection), this, this->m_MenuItem_AnimationSlower->GetId());
    this->m_Menu_View->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Main::m_MenuItem_ViewGridOnSelected), this, m_MenuItem_ViewGrid->GetId());
    this->m_Menu_View->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Main::m_MenuItem_ViewOriginOnSelected), this, m_MenuItem_ViewOrigin->GetId());
    this->m_Menu_View->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Main::m_MenuItem_ViewLightingOnSelected), this, m_MenuItem_ViewLighting->GetId());
    this->m_Menu_View->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Main::m_MenuItem_ViewMeshRootsOnSelected), this, m_MenuItem_ViewMeshRoots->GetId());
    this->m_Menu_View->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Main::m_MenuItem_ViewYUpOnSelected), this, m_MenuItem_ViewYUp->GetId());
    this->m_Menu_View->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Main::m_MenuItem_ViewHighlightOnSelected), this, m_MenuItem_ViewHighlight->GetId());
    this->m_CheckBox_Mesh_Billboard->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(Main::m_CheckBox_Mesh_BillboardOnCheck), NULL, this);
    this->m_Radio_Image->Connect(wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler(Main::m_Radio_TextureTypeOnButton1), NULL, this);
    this->m_Radio_PrimColor->Connect(wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler(Main::m_Radio_TextureTypeOnButton2), NULL, this);
    this->m_Radio_Omit->Connect(wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler(Main::m_Radio_TextureTypeOnButton3), NULL, this);
    this->m_Slider_Animation->Connect(wxEVT_SCROLL_TOP, wxScrollEventHandler(Main::m_Slider_AnimationOnScroll), NULL, this);
    this->m_Slider_Animation->Connect(wxEVT_SCROLL_BOTTOM, wxScrollEventHandler(Main::m_Slider_AnimationOnScroll), NULL, this);
    this->m_Slider_Animation->Connect(wxEVT_SCROLL_LINEUP, wxScrollEventHandler(Main::m_Slider_AnimationOnScroll), NULL, this);
    this->m_Slider_Animation->Connect(wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler(Main::m_Slider_AnimationOnScroll), NULL, this);
    this->m_Slider_Animation->Connect(wxEVT_SCROLL_PAGEUP, wxScrollEventHandler(Main::m_Slider_AnimationOnScroll), NULL, this);
    this->m_Slider_Animation->Connect(wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler(Main::m_Slider_AnimationOnScroll), NULL, this);
    this->m_Slider_Animation->Connect(wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler(Main::m_Slider_AnimationOnScroll), NULL, this);
    this->m_Slider_Animation->Connect(wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler(Main::m_Slider_AnimationOnScroll), NULL, this);
    this->m_Slider_Animation->Connect(wxEVT_SCROLL_CHANGED, wxScrollEventHandler(Main::m_Slider_AnimationOnScroll), NULL, this);
    this->m_Button_AnimationToggle->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Main::m_Button_AnimationToggleOnClick), NULL, this);
    this->m_Image_Texture->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(Main::m_Image_TextureOnLeftDown), NULL, this);
    this->m_Choice_Texture_ImgFormat->Connect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(Main::m_Choice_Texture_ImgFormatOnChoice), NULL, this);
    this->m_Choice_Texture_SFlag->Connect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(Main::m_Choice_Texture_SFlagOnChoice), NULL, this);
    this->m_Choice_Texture_ImgDepth->Connect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(Main::m_Choice_Texture_ImgDepthOnChoice), NULL, this);
    this->m_Choice_Texture_TFlag->Connect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(Main::m_Choice_Texture_TFlagOnChoice), NULL, this);
    this->m_Panel_PrimCol->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(Main::m_Panel_PrimColOnLeftDown), NULL, this);
    this->m_Button_Texture_Advanced->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Main::m_Button_AdvancedOnClick), NULL, this);
    this->m_CheckBox_Texture_LoadFirst->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(Main::m_CheckBox_LoadFirstOnCheckBox), NULL, this);
    this->m_CheckBox_Texture_DontLoad->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(Main::m_CheckBox_DontLoadOnCheckBox), NULL, this);
    this->m_Button_PrimCol_Advanced->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Main::m_Button_AdvancedOnClick), NULL, this);
    this->m_CheckBox_PrimCol_LoadFirst->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(Main::m_CheckBox_LoadFirstOnCheckBox), NULL, this);
    this->m_CheckBox_PrimCol_DontLoad->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(Main::m_CheckBox_DontLoadOnCheckBox), NULL, this);
    this->m_TreeCtrl_ModelData->Connect(wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler(Main::m_TreeCtrl_ModelDataOnTreeSelChanged), NULL, this);
    this->m_Label_PrimColRGB->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(Main::m_Label_PrimColOnLeftDown), NULL, this);
    this->m_Label_PrimColHex->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(Main::m_Label_PrimColOnLeftDown), NULL, this);
}


/*==============================
    Main (Destructor)
    Cleans up the class before deletion
==============================*/

Main::~Main()
{
    // If Main is destroyed, the app is closing so no need to worry about memory management :^)
}


/*==============================
    Main::m_Splitter_HorizontalOnIdle
    Initializes the horizontal splitter
    @param The wxWidgets idle event
==============================*/

void Main::m_Splitter_HorizontalOnIdle(wxIdleEvent& event)
{
    this->m_Splitter_Horizontal->SetSashPosition(268);
    this->m_Splitter_Horizontal->Disconnect(wxEVT_IDLE, wxIdleEventHandler(Main::m_Splitter_HorizontalOnIdle), NULL, this);
}


/*==============================
    Main::m_Splitter_VerticalOnIdle
    Initializes the vertical splitter
    @param The wxWidgets idle event
==============================*/

void Main::m_Splitter_VerticalOnIdle(wxIdleEvent& event)
{
    this->m_Splitter_Vertical->SetSashPosition(400);
    this->m_Splitter_Vertical->Disconnect(wxEVT_IDLE, wxIdleEventHandler(Main::m_Splitter_VerticalOnIdle), NULL, this);
}


/*==============================
    Main::m_MenuItem_ImportOnMenuSelection
    Handles clicking on the Sausage64 Import menu option
    @param The wxWidgets command event
==============================*/

void Main::m_MenuItem_ImportOnMenuSelection(wxCommandEvent& event)
{
    s64Model* newmodel;
    wxFileDialog file(this, _("Import S64 Model"), "", "", "Sausage64 model file (*.S64)|*.S64", wxFD_OPEN);

    // Ensure we didn't cancel the file opening dialog
    if (file.ShowModal() == wxID_CANCEL)
        return;

    // Get our path and try to parse the model
    wxFileName path = file.GetPath();
    newmodel = new s64Model();
    if (!newmodel->GenerateFromFile(path.GetFullPath().ToStdString()))
    {
        wxMessageBox("Problem parsing Sausage64 Model.", "S64 Import error", wxOK | wxICON_EXCLAMATION, this);
        delete newmodel;
        newmodel = NULL;
        return;
    }

    // Replace our model pointer with the newly parsed model
    if (this->m_Sausage64Model != NULL)
    {
        this->m_TreeCtrl_ModelData->DeleteAllItems();
        delete this->m_Sausage64Model;
    }
    this->m_Sausage64Model = newmodel;
    this->m_MenuItem_FileTextureImport->Enable(true);
    this->m_MenuItem_FileTextureExport->Enable(true);

    // Unhighlight the previous model
    highlighted_mesh = NULL;
    highlighted_texture = NULL;
    highlighted_anim = NULL;
    this->m_Sizer_Bottom_Mesh->ShowItems(false);
    this->m_Sizer_Bottom_Texture->ShowItems(false);
    this->m_Sizer_Bottom_Animation->ShowItems(false);

    // Generate the tree list
    wxTreeItemId MDLData = this->m_TreeCtrl_ModelData->AddRoot(path.GetFullName());
    if (this->m_Sausage64Model->GetMeshCount() > 0)
    {
        std::list<s64Mesh*>* meshlist = this->m_Sausage64Model->GetMeshList();
        wxTreeItemId treenode = this->m_TreeCtrl_ModelData->AppendItem(MDLData, "Meshes");
        for (std::list<s64Mesh*>::iterator it = meshlist->begin(); it != meshlist->end(); ++it)
            this->m_TreeCtrl_ModelData->AppendItem(treenode, (*it)->name);
    }
    if (this->m_Sausage64Model->GetTextureCount() > 0)
    {
        std::list<n64Texture*>* texlist = this->m_Sausage64Model->GetTextureList();
        wxTreeItemId treenode = this->m_TreeCtrl_ModelData->AppendItem(MDLData, "Textures");
        for (std::list<n64Texture*>::iterator it = texlist->begin(); it != texlist->end(); ++it)
            this->m_TreeCtrl_ModelData->AppendItem(treenode, (*it)->name);
    }
    if (this->m_Sausage64Model->GetAnimCount() > 0)
    {
        std::list<s64Anim*>* animlist = this->m_Sausage64Model->GetAnimList();
        wxTreeItemId treenode = this->m_TreeCtrl_ModelData->AppendItem(MDLData, "Animations");
        for (std::list<s64Anim*>::iterator it = animlist->begin(); it != animlist->end(); ++it)
            this->m_TreeCtrl_ModelData->AppendItem(treenode, (*it)->name);
    }
}


/*==============================
    Main::m_MenuItem_ImportTextureOnMenuSelection
    Handles clicking on the Texture Def Import menu option
    @param The wxWidgets command event
==============================*/

void Main::m_MenuItem_ImportTextureOnMenuSelection(wxCommandEvent& event)
{
    wxString line;
    wxTextFile file;
    wxFileDialog dialog(this, _("Import Texture Definition"), "", "", "Texture definition file (*.txt)|*.txt", wxFD_OPEN);

    // Ensure we didn't cancel the file opening dialog
    if (dialog.ShowModal() == wxID_CANCEL)
        return;

    // Read the file
    file.Open(dialog.GetPath());
    line = file.GetFirstLine();
    while (!file.Eof())
    {
        wxStringTokenizer tkz = wxStringTokenizer(line, wxT(" "));
        wxString token = tkz.GetNextToken();
        n64Texture* tex = this->m_Sausage64Model->GetTextureFromName(token.ToStdString());

        // If we did not find the given texture name, then skip
        if (tex == NULL)
        {
            line = file.GetNextLine();
            continue;
        }

        // Get the texture type
        token = tkz.GetNextToken();
        if (token == "OMIT")
        {
            tex->CreateDefaultUnknown();
            line = file.GetNextLine();
            continue;
        }
        else if (token == "PRIMCOL")
        {
            unsigned long r, g, b;
            tex->CreateDefaultPrimCol();
            tkz.GetNextToken().ToULong(&r);
            tkz.GetNextToken().ToULong(&g);
            tkz.GetNextToken().ToULong(&b);
            tex->GetPrimColorData()->r = (uint8_t)r;
            tex->GetPrimColorData()->g = (uint8_t)g;
            tex->GetPrimColorData()->b = (uint8_t)b;
        }
        else if (token == "TEXTURE")
        {
            wxDir dir;
            wxString imgfilename;
            bool cont;
            unsigned long w, h;
            tkz.GetNextToken().ToULong(&w);
            tkz.GetNextToken().ToULong(&h);
            tex->CreateDefaultTexture(w, h);

            // See if an image of the texture is in the directory, and if so, load it automatically
            dir.Open(dialog.GetDirectory());
            cont = dir.GetFirst(&imgfilename, wxEmptyString, wxDIR_FILES);
            while (cont)
            {
                if (imgfilename.ToStdString().find(tex->name) != std::string::npos)
                {
                    tex->SetImageFromFile((dir.GetName() + "/" + imgfilename).ToStdString());
                    break;
                }
                cont = dir.GetNext(&imgfilename);
            }
        }

        // Now go through each flag until the end
        while (tkz.HasMoreTokens())
            tex->SetFlag(tkz.GetNextToken().ToStdString());

        // Go to the next line of the file
        line = file.GetNextLine();
    }

    // Close the file, we're done with it
    file.Close();

    // Refresh the bottom panel
    if (highlighted_texture != NULL)
    {
        switch (highlighted_texture->type)
        {
            case TYPE_PRIMCOL:
                this->m_Radio_PrimColor->SetValue(true);
                this->m_Sizer_Bottom_Texture_Setup->Show(false);
                this->m_Sizer_Bottom_PrimCol_Setup->Show(true);
                this->RefreshPrimColPanel();
                break;
            case TYPE_TEXTURE:
                this->m_Radio_Image->SetValue(true);
                this->m_Sizer_Bottom_Texture_Setup->Show(true);
                this->m_Sizer_Bottom_PrimCol_Setup->Show(false);
                this->RefreshTextureImage();
                break;
            default: break;
        }
    }
}


/*==============================
    Main::m_MenuItem_ExportTextureOnMenuSelection
    Handles clicking on the Texture Def Export menu option
    @param The wxWidgets command event
==============================*/

void Main::m_MenuItem_ExportTextureOnMenuSelection(wxCommandEvent& event)
{
    // Pop up a file dialogue
    wxFileDialog fileDialogue(this, _("Export Texture Definition"), "", "", "Text image (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    // Ensure the user didn't cancel
    if (fileDialogue.ShowModal() == wxID_CANCEL)
        return;

    // Create the file
    wxFile file;
    file.Create(fileDialogue.GetPath(), true);
    file.Open(fileDialogue.GetPath(), wxFile::write);

    // Iterate through each texture
    std::list<n64Texture*>* texlist = this->m_Sausage64Model->GetTextureList();
    for (std::list<n64Texture*>::iterator it = texlist->begin(); it != texlist->end(); ++it)
    {
        n64Texture* tex = *it;

        // Write the name and texture type
        file.Write(tex->name);
        switch (tex->type)
        {
            case TYPE_TEXTURE:
                file.Write(" TEXTURE");
                file.Write(wxString::Format(wxT(" %zu %zu"), tex->GetTextureData()->w, tex->GetTextureData()->h));
                if (tex->GetTextureData()->coltype != DEFAULT_IMAGEFORMAT)
                    file.Write(" " + tex->GetTextureData()->coltype);
                if (tex->GetTextureData()->colsize != DEFAULT_IMAGESIZE)
                    file.Write(" " + tex->GetTextureData()->colsize);
                if (tex->GetTextureData()->texmodes != DEFAULT_TEXFLAGS)
                    file.Write(" " + tex->GetTextureData()->texmodes);
                if (tex->GetTextureData()->texmodet != DEFAULT_TEXFLAGT)
                    file.Write(" " + tex->GetTextureData()->texmodet);
                break;
            case TYPE_PRIMCOL:
                file.Write(" PRIMCOL");
                file.Write(wxString::Format(wxT(" %d %d %d"), tex->GetPrimColorData()->r, tex->GetPrimColorData()->g, tex->GetPrimColorData()->b));
                break;
            case TYPE_UNKNOWN:
                file.Write(" OMIT\r\n");
                continue;
        }

        // Handle loading flags
        if (tex->dontload)
            file.Write(" DONTLOAD");
        if (tex->loadfirst)
            file.Write(" LOADFIRST");

        // Handle cycle mode
        if (tex->cycle != DEFAULT_CYCLE)
            file.Write(" " + tex->cycle);

        // Handle combine modes
        switch (tex->type)
        {
            case TYPE_TEXTURE:
                if (tex->texfilter != DEFAULT_TEXFILTER)
                    file.Write(" " + tex->texfilter);
                if (tex->combinemode1 != DEFAULT_COMBINE1_TEX)
                    file.Write(" " + tex->combinemode1);
                if (tex->cycle == "G_CYC_2CYCLE" && tex->combinemode2 != DEFAULT_COMBINE2_TEX)
                    file.Write(" " + tex->combinemode2);
                else if (tex->cycle == "G_CYC_1CYCLE" && tex->combinemode1 != DEFAULT_COMBINE1_TEX)
                    file.Write(" " + tex->combinemode1);
                break;
            case TYPE_PRIMCOL:
                if (tex->combinemode1 != DEFAULT_COMBINE1_PRIM)
                    file.Write(" " + tex->combinemode1);
                if (tex->cycle == "G_CYC_2CYCLE" && tex->combinemode2 != DEFAULT_COMBINE2_PRIM)
                    file.Write(" " + tex->combinemode2);
                else if (tex->cycle == "G_CYC_1CYCLE" && tex->combinemode1 != DEFAULT_COMBINE1_PRIM)
                    file.Write(" " + tex->combinemode1);
                break;
            default: break;
        }

        // Handle render modes
        if (tex->rendermode1 != DEFAULT_RENDERMODE1)
            file.Write(" " + tex->rendermode1);
        if (tex->cycle == "G_CYC_2CYCLE" && tex->rendermode2 != DEFAULT_RENDERMODE2)
            file.Write(" " + tex->rendermode2);
        else if (tex->cycle == "G_CYC_1CYCLE" && tex->rendermode1 != DEFAULT_RENDERMODE1)
            file.Write(" " + tex->rendermode1 + "2");

        // Write the geometry flags that are new from the defaults
        std::string defaultgeo[] = DEFAULT_GEOFLAGS;
        int size = sizeof(defaultgeo) / sizeof(defaultgeo[0]);
        for (std::list<std::string>::iterator itflags = tex->geomode.begin(); itflags != tex->geomode.end(); ++itflags)
        {
            bool skip = false;
            std::string flag = *itflags;

            // If this flag exists in the list of default flags, skip it
            for (int i=0; i<size; i++)
            {
                if (defaultgeo[i] == flag)
                {
                    skip = true;
                    break;
                }
            }
            if (skip)
                continue;

            // Write the flag
            file.Write(" " + flag);
        }

        // Finally, negate default flags which aren't present
        for (int i=0; i<size; i++)
        {
            bool hasflag = false;
            for (std::list<std::string>::iterator itflags = tex->geomode.begin(); itflags != tex->geomode.end(); ++itflags)
            {
                std::string flag = *itflags;
                if (defaultgeo[i] == flag)
                {
                    hasflag = true;
                    break;
                }
            }

            // Write the negated flag
            if (!hasflag)
                file.Write(" !" + defaultgeo[i]);
        }
        file.Write("\r\n");
    }

    // Finished writing the file
    file.Close();
}


/*==============================
    Main::m_TreeCtrl_ModelDataOnTreeSelChanged
    Handles clicking on a tree item
    @param The wxWidgets tree event
==============================*/

void Main::m_TreeCtrl_ModelDataOnTreeSelChanged(wxTreeEvent& event)
{
    wxTreeItemId parent = this->m_TreeCtrl_ModelData->GetItemParent(event.GetItem());
    if (parent == NULL)
        return;

    // Get the name of the highlighted item, and the name of its parent
    wxString name = this->m_TreeCtrl_ModelData->GetItemText(event.GetItem());
    wxString parentname = this->m_TreeCtrl_ModelData->GetItemText(parent);
    if (parentname == "Meshes")
    {
        std::list<s64Mesh*>* meshlist = this->m_Sausage64Model->GetMeshList();
        for (std::list<s64Mesh*>::iterator it = meshlist->begin(); it != meshlist->end(); ++it)
        {
            s64Mesh* mesh = *it;
            if (name == mesh->name)
            {
                highlighted_mesh = mesh;
                break;
            }
        }
        highlighted_texture = NULL;
        this->m_Sizer_Bottom_Mesh->ShowItems(true);
        this->m_Sizer_Bottom_Texture->ShowItems(false);
        this->m_Sizer_Bottom_Animation->ShowItems(false);
        this->m_CheckBox_Mesh_Billboard->SetValue(highlighted_mesh->billboard);
        this->m_Sizer_Bottom->Layout();
    }
    else if (parentname == "Textures")
    {
        std::list<n64Texture*>* texturelist = this->m_Sausage64Model->GetTextureList();
        for (std::list<n64Texture*>::iterator it = texturelist->begin(); it != texturelist->end(); ++it)
        {
            n64Texture* tex = *it;
            if (name == tex->name)
            {
                highlighted_texture = tex;
                break;
            }
        }
        highlighted_mesh = NULL;
        this->m_Sizer_Bottom_Mesh->ShowItems(false);
        this->m_Sizer_Bottom_Texture->ShowItems(true);
        this->m_Sizer_Bottom_Animation->ShowItems(false);

        // Show the correct stuff based on the texture type
        switch (highlighted_texture->type)
        {
            case TYPE_TEXTURE:
                this->m_Sizer_Bottom_Texture_Setup->Show(true);
                this->m_Sizer_Bottom_PrimCol_Setup->Show(false);
                this->m_Radio_Image->SetValue(true);
                this->RefreshTextureImage();
                break;
            case TYPE_PRIMCOL:
                this->m_Sizer_Bottom_Texture_Setup->Show(false);
                this->m_Sizer_Bottom_PrimCol_Setup->Show(true);
                this->m_Radio_PrimColor->SetValue(true);
                this->RefreshPrimColPanel();
                break;
            case TYPE_UNKNOWN:
                this->m_Sizer_Bottom_Texture_Setup->Show(false);
                this->m_Sizer_Bottom_PrimCol_Setup->Show(false);
                this->m_Radio_Omit->SetValue(true);
                break;
        }
        this->m_Sizer_Bottom->Layout();
    }
    else if (parentname == "Animations")
    {
        std::list<s64Anim*>* animlist = this->m_Sausage64Model->GetAnimList();
        for (std::list<s64Anim*>::iterator it = animlist->begin(); it != animlist->end(); ++it)
        {
            s64Anim* anim = *it;
            if (name == anim->name)
            {
                highlighted_anim = anim;
                highlighted_anim_length = anim->keyframes.back()->keyframe;
                highlighted_anim_tick = 0;
                break;
            }
        }
        highlighted_mesh = NULL;
        highlighted_texture = NULL;
        this->m_Sizer_Bottom_Mesh->ShowItems(false);
        this->m_Sizer_Bottom_Texture->ShowItems(false);
        this->m_Sizer_Bottom_Animation->ShowItems(true);
        this->m_Slider_Animation->SetValue(0);
        this->m_Slider_Animation->SetMax(highlighted_anim_length);
        this->m_Sizer_Bottom->Layout();
    }
    else
    {
        highlighted_mesh = NULL;
        highlighted_texture = NULL;
        this->m_Sizer_Bottom_Mesh->ShowItems(false);
        this->m_Sizer_Bottom_Texture->ShowItems(false);
        this->m_Sizer_Bottom_Animation->ShowItems(false);
    }
}


/*==============================
    Main::m_MenuItem_AnimationPlayToggleOnMenuSelection
    Handles clicking on the Animation Play menu option
    @param The wxWidgets command event
==============================*/

void Main::m_MenuItem_AnimationPlayToggleOnMenuSelection(wxCommandEvent& event)
{
    settings_animating = event.IsChecked();
}


/*==============================
    Main::m_MenuItem_AnimationReverseOnMenuSelection
    Handles clicking on the Animation Reverse menu option
    @param The wxWidgets command event
==============================*/

void Main::m_MenuItem_AnimationReverseOnMenuSelection(wxCommandEvent& event)
{
    settings_animatingreverse = event.IsChecked();
}


/*==============================
    Main::m_MenuItem_AnimationFasterOnMenuSelection
    Handles clicking on the Animation Faster menu option
    @param The wxWidgets command event
==============================*/

void Main::m_MenuItem_AnimationFasterOnMenuSelection(wxCommandEvent& event)
{
    settings_playbackspeed += 0.1f;
}


/*==============================
    Main::m_MenuItem_AnimationNormalOnMenuSelection
    Handles clicking on the Animation Normal menu option
    @param The wxWidgets command event
==============================*/

void Main::m_MenuItem_AnimationNormalOnMenuSelection(wxCommandEvent& event)
{
    settings_playbackspeed = DEFAULT_PLAYBACK;
}


/*==============================
    Main::m_MenuItem_AnimationSlowerOnMenuSelection
    Handles clicking on the Animation Slower menu option
    @param The wxWidgets command event
==============================*/

void Main::m_MenuItem_AnimationSlowerOnMenuSelection(wxCommandEvent& event)
{
    if (settings_playbackspeed > 0.2f)
        settings_playbackspeed -= 0.1f;
}


/*==============================
    Main::m_MenuItem_ViewGridOnSelected
    Handles clicking on the View Grid menu option
    @param The wxWidgets command event
==============================*/

void Main::m_MenuItem_ViewGridOnSelected(wxCommandEvent& event)
{
    settings_showgrid = event.IsChecked();
}


/*==============================
    Main::m_MenuItem_ViewOriginOnSelected
    Handles clicking on the View Origin menu option
    @param The wxWidgets command event
==============================*/

void Main::m_MenuItem_ViewOriginOnSelected(wxCommandEvent& event)
{
    settings_showorigin = event.IsChecked();
}


/*==============================
    Main::m_MenuItem_ViewLightingOnSelected
    Handles clicking on the View Lighting menu option
    @param The wxWidgets command event
==============================*/

void Main::m_MenuItem_ViewLightingOnSelected(wxCommandEvent& event)
{
    settings_showlighting = event.IsChecked();
}


/*==============================
    Main::m_MenuItem_ViewMeshRootsOnSelected
    Handles clicking on the View Roots menu option
    @param The wxWidgets command event
==============================*/

void Main::m_MenuItem_ViewMeshRootsOnSelected(wxCommandEvent& event)
{
    settings_showmeshroots = event.IsChecked();
}


/*==============================
    Main::m_MenuItem_ViewYUpOnSelected
    Handles clicking on the View Y Axis UP menu option
    @param The wxWidgets command event
==============================*/

void Main::m_MenuItem_ViewYUpOnSelected(wxCommandEvent& event)
{
    settings_yaxisup = event.IsChecked();
    this->m_Model_Canvas->SetYUp(settings_yaxisup);
}


/*==============================
    Main::m_MenuItem_ViewHighlightOnSelected
    Handles clicking on the View Highlighted menu option
    @param The wxWidgets command event
==============================*/

void Main::m_MenuItem_ViewHighlightOnSelected(wxCommandEvent& event)
{
    settings_showhighlight = event.IsChecked();
}


/*==============================
    Main::m_CheckBox_Mesh_BillboardOnCheck
    Handles clicking on the Billboard checkbox
    @param The wxWidgets command event
==============================*/

void Main::m_CheckBox_Mesh_BillboardOnCheck(wxCommandEvent& event)
{
    highlighted_mesh->billboard = event.IsChecked();
}


/*==============================
    Main::m_Slider_AnimationOnScroll
    Handles clicking on the Slider
    @param The wxWidgets scroll event
==============================*/

void Main::m_Slider_AnimationOnScroll(wxScrollEvent& event)
{
    settings_animating = false;
    this->m_MenuItem_AnimationPlayToggle->Check(false);
    highlighted_anim_tick = this->m_Slider_Animation->GetValue();
    if (highlighted_anim_tick == highlighted_anim_length)
        highlighted_anim_tick -= 0.01f;
}


/*==============================
    Main::m_Button_AnimationToggleOnClick
    Handles clicking on the Play button
    @param The wxWidgets command event
==============================*/

void Main::m_Button_AnimationToggleOnClick(wxCommandEvent& event)
{
    settings_animating = !settings_animating;
    this->m_MenuItem_AnimationPlayToggle->Check(settings_animating);
}


/*==============================
    Main::m_Radio_TextureTypeOnButton1
    Handles clicking on the Texture radio button
    @param The wxWidgets command event
==============================*/

void Main::m_Radio_TextureTypeOnButton1(wxCommandEvent& event)
{
    if (event.IsChecked() && highlighted_texture->type != TYPE_TEXTURE)
    {
        this->m_Sizer_Bottom_Texture_Setup->Show(true);
        this->m_Sizer_Bottom_PrimCol_Setup->Show(false);
        this->m_Panel_Bottom->Layout();
        highlighted_texture->CreateDefaultTexture();
        this->RefreshTextureImage();
    }
}


/*==============================
    Main::m_Radio_TextureTypeOnButton2
    Handles clicking on the Primitive Color radio button
    @param The wxWidgets command event
==============================*/

void Main::m_Radio_TextureTypeOnButton2(wxCommandEvent& event)
{
    if (event.IsChecked() && highlighted_texture->type != TYPE_PRIMCOL)
    {
        this->m_Sizer_Bottom_Texture_Setup->Show(false);
        this->m_Sizer_Bottom_PrimCol_Setup->Show(true);
        highlighted_texture->CreateDefaultPrimCol();
        this->m_Panel_Bottom->Layout();
        this->RefreshPrimColPanel();
    }
}


/*==============================
    Main::m_Radio_TextureTypeOnButton3
    Handles clicking on the Omit radio button
    @param The wxWidgets command event
==============================*/

void Main::m_Radio_TextureTypeOnButton3(wxCommandEvent& event)
{
    if (event.IsChecked() && highlighted_texture->type != TYPE_UNKNOWN)
    {
        this->m_Sizer_Bottom_Texture_Setup->Show(false);
        this->m_Sizer_Bottom_PrimCol_Setup->Show(false);
        highlighted_texture->CreateDefaultUnknown();
    }
}


/*==============================
    Main::m_Image_TextureOnLeftDown
    Handles clicking on the texture image
    @param The wxWidgets mouse event
==============================*/

void Main::m_Image_TextureOnLeftDown(wxMouseEvent& event)
{
    wxFileDialog file(this, _("Import Texture Image"), "", "", "PNG files(*.png)|*.png|BMP files(*.bmp)|*.bmp", wxFD_OPEN);

    // Ensure we didn't cancel the file opening dialog
    if (file.ShowModal() == wxID_CANCEL)
        return;

    // Get our path and generate the texture from it
    highlighted_texture->SetImageFromFile(file.GetPath().ToStdString());
    this->RefreshTextureImage();
}


/*==============================
    Main::m_Choice_Texture_ImgFormatOnChoice
    Handles clicking on the Image Format dropdown
    @param The wxWidgets command event
==============================*/

void Main::m_Choice_Texture_ImgFormatOnChoice(wxCommandEvent& event)
{
    highlighted_texture->GetTextureData()->coltype = this->m_Choice_Texture_ImgFormat->GetString(event.GetSelection());
}


/*==============================
    Main::m_Choice_Texture_SFlagOnChoice
    Handles clicking on the S Flag dropdown
    @param The wxWidgets command event
==============================*/

void Main::m_Choice_Texture_SFlagOnChoice(wxCommandEvent& event)
{
    highlighted_texture->GetTextureData()->texmodes = this->m_Choice_Texture_SFlag->GetString(event.GetSelection());
    highlighted_texture->RegenerateTexture();
}


/*==============================
    Main::m_Choice_Texture_ImgDepthOnChoice
    Handles clicking on the Image Depth
    @param The wxWidgets command event
==============================*/

void Main::m_Choice_Texture_ImgDepthOnChoice(wxCommandEvent& event)
{
    highlighted_texture->GetTextureData()->colsize = this->m_Choice_Texture_ImgDepth->GetString(event.GetSelection());
}


/*==============================
    Main::m_Choice_Texture_TFlagOnChoice
    Handles clicking on the T Flag dropdown
    @param The wxWidgets command event
==============================*/

void Main::m_Choice_Texture_TFlagOnChoice(wxCommandEvent& event)
{
    highlighted_texture->GetTextureData()->texmodet = this->m_Choice_Texture_TFlag->GetString(event.GetSelection());
    highlighted_texture->RegenerateTexture();
}


/*==============================
    Main::m_Panel_PrimColOnLeftDown
    Handles clicking on the Primitive Color rectangle
    @param The wxWidgets mouse event
==============================*/

void Main::m_Panel_PrimColOnLeftDown(wxMouseEvent& event)
{;
    uint8_t oldcols[4];
    texCol* col = highlighted_texture->GetPrimColorData();

    // Get the old color, in case the user cancels
    oldcols[0] = col->r;
    oldcols[1] = col->g;
    oldcols[2] = col->b;
    oldcols[3] = col->a;

    // Show a color dialogue
    wxColourData coldata;
    coldata.SetColour(wxColour(col->r, col->g, col->b));
    wxColourDialog dialog(this, &coldata);
    dialog.Bind(wxEVT_COLOUR_CHANGED, &Main::OnPrimColChanged, this);
    if (dialog.ShowModal() == wxID_OK)
    {
        coldata = dialog.GetColourData();
        col->r = coldata.GetColour().Red();
        col->g = coldata.GetColour().Green();
        col->b = coldata.GetColour().Blue();
        col->a = 255;
    }
    else
    {
        col->r = oldcols[0];
        col->g = oldcols[1];
        col->b = oldcols[2];
        col->a = oldcols[3];
    }
    this->RefreshPrimColPanel();
}


/*==============================
    Main::m_Panel_PrimColOnLeftDown
    Handles clicking on the Advanced Texture Settings button
    @param The wxWidgets command event
==============================*/

void Main::m_Button_AdvancedOnClick(wxCommandEvent& event)
{
    AdvancedRenderSettings* dialog = new AdvancedRenderSettings(this);
    dialog->ShowModal();
}


/*==============================
    Main::m_CheckBox_LoadFirstOnCheckBox
    Handles clicking on the Load First checkbox
    @param The wxWidgets command event
==============================*/

void Main::m_CheckBox_LoadFirstOnCheckBox(wxCommandEvent& event)
{
    highlighted_texture->loadfirst = event.IsChecked();
}


/*==============================
    Main::m_CheckBox_DontLoadOnCheckBox
    Handles clicking on the Don't Load checkbox
    @param The wxWidgets command event
==============================*/

void Main::m_CheckBox_DontLoadOnCheckBox(wxCommandEvent& event)
{
    highlighted_texture->dontload = event.IsChecked();
}


/*==============================
    Main::m_Timer_MainLoopOnTimer
    Handles the main loop timer
    @param The wxWidgets timer event
==============================*/

void Main::m_Timer_MainLoopOnTimer(wxTimerEvent& event)
{
    static wxLongLong lasttime = wxGetLocalTimeMillis();
    wxLongLong curtime = wxGetLocalTimeMillis();
    double elapsed = ((curtime - lasttime).ToDouble())/(1000.0f/PROGRAM_FRAMERATE);

    // Read the controls
    this->m_Model_Canvas->HandleControls();

    // Animate the model
    if (settings_animating)
    {
        if (settings_animatingreverse)
            this->m_Model_Canvas->AdvanceAnim(-settings_playbackspeed*elapsed);
        else
            this->m_Model_Canvas->AdvanceAnim(settings_playbackspeed*elapsed);
        this->m_Slider_Animation->SetValue(highlighted_anim_tick);
    }

    // Refresh the OpenGL context
    this->m_Model_Canvas->Refresh();

    // Store the current time
    lasttime = curtime;
}


/*==============================
    Main::m_Label_PrimColOnLeftDown
    Handles clicking on the primcolor color labels
    @param The wxWidgets timer event
==============================*/

void Main::m_Label_PrimColOnLeftDown(wxMouseEvent& event)
{
    // Create the dialogue
    texCol* col = highlighted_texture->GetPrimColorData();
    ColorPickerHelper* dialog = new ColorPickerHelper(this);
    dialog->SetRGB(col->r, col->g, col->b);
    if (dialog->ShowModal() == wxID_OK)
    {
        wxColourDialogEvent ev;
        ev.SetColour(wxColor(dialog->GetRed(), dialog->GetGreen(), dialog->GetBlue()));
        this->OnPrimColChanged(ev);
    }
}


/*==============================
    Main::GetLoadedModel
    Gets the currently loaded Sausage64 model
    @returns A pointer to the S64 model
==============================*/

s64Model* Main::GetLoadedModel()
{
    return this->m_Sausage64Model;
}


/*==============================
    Main::OnPrimColChanged
    Handles the primitive color dialog changing
    @param The wxWidgets color dialog event
==============================*/

void Main::OnPrimColChanged(wxColourDialogEvent& event)
{
    texCol* col = highlighted_texture->GetPrimColorData();
    col->r = event.GetColour().Red();
    col->g = event.GetColour().Green();
    col->b = event.GetColour().Blue();
    col->a = 255;
    this->RefreshPrimColPanel();
}


/*==============================
    Main::RefreshPrimColPanel
    Refreshes the primitive color panel
==============================*/

void Main::RefreshPrimColPanel()
{
    texCol* col = highlighted_texture->GetPrimColorData();
    this->m_Label_PrimColRGB->SetLabel(wxString::Format(wxT("%d, %d, %d"), col->r, col->g, col->b));
    this->m_Label_PrimColHex->SetLabel(wxString::Format(wxT("%02X%02X%02X"), col->r, col->g, col->b));
    this->m_Panel_PrimCol->SetForegroundColour(wxColour(col->r, col->g, col->b));
    this->m_Panel_PrimCol->SetBackgroundColour(wxColour(col->r, col->g, col->b));
    this->m_CheckBox_PrimCol_LoadFirst->SetValue(highlighted_texture->loadfirst);
    this->m_CheckBox_PrimCol_DontLoad->SetValue(highlighted_texture->dontload);
    this->m_Panel_Bottom->Layout();
    this->m_Panel_Bottom->Refresh();
    this->m_Model_Canvas->Refresh();
}


/*==============================
    Main::RefreshTextureImage
    Refreshes the texture image panel
==============================*/

void Main::RefreshTextureImage()
{
    texImage* img = highlighted_texture->GetTextureData();
    this->m_Image_Texture->SetBitmap(img->wxbmp);
    this->m_Label_Texture->SetLabel(wxString::Format(wxT("%zux%zu"), img->w, img->h));
    this->m_Choice_Texture_ImgFormat->SetSelection(this->m_Choice_Texture_ImgFormat->FindString(highlighted_texture->GetTextureData()->coltype));
    this->m_Choice_Texture_ImgDepth->SetSelection(this->m_Choice_Texture_ImgDepth->FindString(highlighted_texture->GetTextureData()->colsize));
    this->m_Choice_Texture_SFlag->SetSelection(this->m_Choice_Texture_SFlag->FindString(highlighted_texture->GetTextureData()->texmodes));
    this->m_Choice_Texture_TFlag->SetSelection(this->m_Choice_Texture_TFlag->FindString(highlighted_texture->GetTextureData()->texmodet));
    this->m_CheckBox_Texture_LoadFirst->SetValue(highlighted_texture->loadfirst);
    this->m_CheckBox_Texture_DontLoad->SetValue(highlighted_texture->dontload);
    this->m_Panel_Bottom->Layout();
    this->m_Image_Texture->Update();
    this->m_Image_Texture->Refresh();
    this->m_Panel_Bottom->Refresh();
}


/*********************************
   AdvancedRenderSettings Class
*********************************/

/*==============================
    AdvancedRenderSettings (Constructor)
    Initializes the class
==============================*/

AdvancedRenderSettings::AdvancedRenderSettings(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxDialog(parent, id, title, pos, size, style)
{
    this->SetSizeHints(wxDefaultSize, wxDefaultSize);

    // Add the primary sizers
    wxFlexGridSizer* m_Sizer_Main;
    m_Sizer_Main = new wxFlexGridSizer(0, 1, 0, 0);
    m_Sizer_Main->AddGrowableCol(0);
    m_Sizer_Main->AddGrowableRow(2);
    m_Sizer_Main->SetFlexibleDirection(wxBOTH);
    m_Sizer_Main->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_ALL);
    wxFlexGridSizer* m_Sizer_Modes;
    m_Sizer_Modes = new wxFlexGridSizer(0, 2, 0, 0);
    m_Sizer_Modes->SetFlexibleDirection(wxBOTH);
    m_Sizer_Modes->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

    // Add the top row labels
    wxStaticText* m_Label_Cycle;
    m_Label_Cycle = new wxStaticText(this, wxID_ANY, wxT("Cycle Type"), wxDefaultPosition, wxDefaultSize, 0);
    m_Label_Cycle->Wrap(-1);
    m_Sizer_Modes->Add(m_Label_Cycle, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
    wxStaticText* m_Label_Filter;
    m_Label_Filter = new wxStaticText(this, wxID_ANY, wxT("Texture Filter"), wxDefaultPosition, wxDefaultSize, 0);
    m_Label_Filter->Wrap(-1);
    m_Label_Filter->Enable(highlighted_texture->type == TYPE_TEXTURE);
    m_Sizer_Modes->Add(m_Label_Filter, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

    // Add the second row choices
    wxString m_Choice_CycleChoices[] = { wxT("G_CYC_1CYCLE"), wxT("G_CYC_2CYCLE") };
    int m_Choice_CycleNChoices = sizeof(m_Choice_CycleChoices) / sizeof(wxString);
    this->m_Choice_Cycle = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_Choice_CycleNChoices, m_Choice_CycleChoices, 0);
    this->m_Choice_Cycle->SetSelection(this->m_Choice_Cycle->FindString(highlighted_texture->cycle));
    m_Sizer_Modes->Add(this->m_Choice_Cycle, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
    wxString m_Choice_FilterChoices[] = { wxT("G_TF_POINT"), wxT("G_TF_AVERAGE"), wxT("G_TF_BILERP") };
    int m_Choice_FilterNChoices = sizeof(m_Choice_FilterChoices) / sizeof(wxString);
    this->m_Choice_Filter = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_Choice_FilterNChoices, m_Choice_FilterChoices, 0);
    this->m_Choice_Filter->SetSelection(this->m_Choice_Filter->FindString(highlighted_texture->texfilter));
    this->m_Choice_Filter->Enable(highlighted_texture->type == TYPE_TEXTURE);
    m_Sizer_Modes->Add(this->m_Choice_Filter, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

    // Add the third row labels
    wxStaticText* m_Label_Combine1;
    m_Label_Combine1 = new wxStaticText(this, wxID_ANY, wxT("Combine Mode 1"), wxDefaultPosition, wxDefaultSize, 0);
    m_Label_Combine1->Wrap(-1);
    m_Sizer_Modes->Add(m_Label_Combine1, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
    this->m_Label_Combine2 = new wxStaticText(this, wxID_ANY, wxT("Combine Mode 2"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_Label_Combine2->Wrap(-1);
    this->m_Label_Combine2->Enable(this->m_Choice_Cycle->GetSelection() == 1);
    m_Sizer_Modes->Add(this->m_Label_Combine2, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

    // Add the fourth row choices
    wxString m_Choice_Combine1Choices[] = { wxT("G_CC_PRIMITIVE"), wxT("G_CC_SHADE"), wxT("G_CC_MODULATEI"), wxT("G_CC_MODULATEIA"), wxT("G_CC_MODULATEIDECALA"), wxT("G_CC_MODULATERGB"), wxT("G_CC_MODULATERGBA"), wxT("G_CC_MODULATERGBDECALA"), wxT("G_CC_MODULATEI_PRIM"), wxT("G_CC_MODULATEIA_PRIM"), wxT("G_CC_MODULATEIDECALA_PRIM"), wxT("G_CC_MODULATERGB_PRIM"), wxT("G_CC_MODULATERGBA_PRIM"), wxT("G_CC_MODULATERGBDECALA_PRIM"), wxT("G_CC_PRIMLITE"), wxT("G_CC_DECALRGB"), wxT("G_CC_DECALRGBA"), wxT("G_CC_BLENDI"), wxT("G_CC_BLENDIA"), wxT("G_CC_BLENDIDECALA"), wxT("G_CC_BLENDRGBA"), wxT("G_CC_BLENDRGBDECALA"), wxT("G_CC_ADDRGB"), wxT("G_CC_ADDRGBDECALA"), wxT("G_CC_REFLECTRGB"), wxT("G_CC_REFLECTRGBDECALA"), wxT("G_CC_HILITERGB"), wxT("G_CC_HILITERGBA"), wxT("G_CC_HILITERGBDECALA"), wxT("G_CC_SHADEDECALA"), wxT("G_CC_BLENDPE"), wxT("G_CC_BLENDPEDECALA"), wxT("G_CC_TRILERP"), wxT("G_CC_INTERFERENCE") };
    int m_Choice_Combine1NChoices = sizeof(m_Choice_Combine1Choices) / sizeof(wxString);
    this->m_Choice_Combine1 = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_Choice_Combine1NChoices, m_Choice_Combine1Choices, 0);
    this->m_Choice_Combine1->SetSelection(this->m_Choice_Combine1->FindString(highlighted_texture->combinemode1));
    m_Sizer_Modes->Add(this->m_Choice_Combine1, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
    wxString m_Choice_Combine2Choices[] = { wxT("G_CC_PASS2"), wxT("G_CC_MODULATEI2"), wxT("G_CC_MODULATEIA2"), wxT("G_CC_MODULATERGB2"), wxT("G_CC_MODULATERGBA2"), wxT("G_CC_MODULATEI_PRIM2"), wxT("G_CC_MODULATEIA_PRIM2"), wxT("G_CC_MODULATERGB_PRIM2"), wxT("G_CC_MODULATERGBA_PRIM2"), wxT("G_CC_DECALRGB2"), wxT("G_CC_BLENDI2"), wxT("G_CC_BLENDIA2"), wxT("G_CC_HILITERGB2"), wxT("G_CC_HILITERGBA2"), wxT("G_CC_HILITERGBDECALA2"), wxT("G_CC_HILITERGBPASSA2") };
    int m_Choice_Combine2NChoices = sizeof(m_Choice_Combine2Choices) / sizeof(wxString);
    this->m_Choice_Combine2 = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_Choice_Combine2NChoices, m_Choice_Combine2Choices, 0);
    this->m_Choice_Combine2->SetSelection(this->m_Choice_Combine2->FindString(highlighted_texture->combinemode2));
    this->m_Choice_Combine2->Enable(this->m_Choice_Cycle->GetSelection() == 1);
    m_Sizer_Modes->Add(m_Choice_Combine2, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

    // Add the fifth row labels
    wxStaticText* m_Label_Render1;
    m_Label_Render1 = new wxStaticText(this, wxID_ANY, wxT("Render Mode 1"), wxDefaultPosition, wxDefaultSize, 0);
    m_Label_Render1->Wrap(-1);
    m_Sizer_Modes->Add(m_Label_Render1, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
    this->m_Label_Render2 = new wxStaticText(this, wxID_ANY, wxT("Render Mode 2"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_Label_Render2->Wrap(-1);
    this->m_Label_Render2->Enable(this->m_Choice_Cycle->GetSelection() == 1);
    m_Sizer_Modes->Add(this->m_Label_Render2, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
    
    // Add the sixth row choices
    wxString m_Choice_Render1Choices[] = { wxT("G_RM_AA_ZB_OPA_SURF"), wxT("G_RM_AA_ZB_XLU_SURF"), wxT("G_RM_AA_ZB_OPA_DECAL"), wxT("G_RM_AA_ZB_XLU_DECAL"), wxT("G_RM_AA_ZB_OPA_INTER"), wxT("G_RM_AA_ZB_XLU_INTER"), wxT("G_RM_AA_ZB_XLU_LINE"), wxT("G_RM_AA_ZB_DEC_LINE"), wxT("G_RM_AA_ZB_TEX_EDGE"), wxT("G_RM_AA_ZB_TEX_INTER"), wxT("G_RM_AA_ZB_SUB_SURF"), wxT("G_RM_AA_ZB_PCL_SURF"), wxT("G_RM_AA_ZB_OPA_TERR"), wxT("G_RM_AA_ZB_TEX_TERR"), wxT("G_RM_AA_ZB_SUB_TERR"), wxT("G_RM_RA_ZB_OPA_SURF"), wxT("G_RM_RA_ZB_OPA_DECAL"), wxT("G_RM_RA_ZB_OPA_INTER"), wxT("G_RM_AA_OPA_SURF"), wxT("G_RM_AA_XLU_SURF"), wxT("G_RM_AA_XLU_LINE"), wxT("G_RM_AA_DEC_LINE"), wxT("G_RM_AA_TEX_EDGE"), wxT("G_RM_AA_SUB_SURF"), wxT("G_RM_AA_PCL_SURF"), wxT("G_RM_AA_OPA_TERR"), wxT("G_RM_AA_TEX_TERR"), wxT("G_RM_AA_SUB_TERR"), wxT("G_RM_RA_OPA_SURF"), wxT("G_RM_ZB_OPA_SURF"), wxT("G_RM_ZB_XLU_SURF"), wxT("G_RM_ZB_OPA_DECAL"), wxT("G_RM_ZB_XLU_DECAL"), wxT("G_RM_ZB_CLD_SURF"), wxT("G_RM_ZB_OVL_SURF"), wxT("G_RM_ZB_PCL_SURF"), wxT("G_RM_OPA_SURF"), wxT("G_RM_XLU_SURF"), wxT("G_RM_CLD_SURF"), wxT("G_RM_TEX_EDGE"), wxT("G_RM_PCL_SURF"), wxT("G_RM_G_RM_ADD"), wxT("G_RM_FOG_SHADE_A"), wxT("G_RM_FOG_PRIM_A"), wxT("G_RM_PASS") };
    int m_Choice_Render1NChoices = sizeof(m_Choice_Render1Choices) / sizeof(wxString);
    this->m_Choice_Render1 = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_Choice_Render1NChoices, m_Choice_Render1Choices, 0);
    this->m_Choice_Render1->SetSelection(this->m_Choice_Render1->FindString(highlighted_texture->rendermode1));
    m_Sizer_Modes->Add(this->m_Choice_Render1, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
    wxString m_Choice_Render2Choices[] = { wxT("G_RM_AA_ZB_OPA_SURF2"), wxT("G_RM_AA_ZB_XLU_SURF2"), wxT("G_RM_AA_ZB_OPA_DECAL2"), wxT("G_RM_AA_ZB_XLU_DECAL2"), wxT("G_RM_AA_ZB_OPA_INTER2"), wxT("G_RM_AA_ZB_XLU_INTER2"), wxT("G_RM_AA_ZB_XLU_LINE2"), wxT("G_RM_AA_ZB_DEC_LINE2"), wxT("G_RM_AA_ZB_TEX_EDGE2"), wxT("G_RM_AA_ZB_TEX_INTER2"), wxT("G_RM_AA_ZB_SUB_SURF2"), wxT("G_RM_AA_ZB_PCL_SURF2"), wxT("G_RM_AA_ZB_OPA_TERR2"), wxT("G_RM_AA_ZB_TEX_TERR2"), wxT("G_RM_AA_ZB_SUB_TERR2"), wxT("G_RM_RA_ZB_OPA_SURF2"), wxT("G_RM_RA_ZB_OPA_DECAL2"), wxT("G_RM_RA_ZB_OPA_INTER2"), wxT("G_RM_AA_OPA_SURF2"), wxT("G_RM_AA_XLU_SURF2"), wxT("G_RM_AA_XLU_LINE2"), wxT("G_RM_AA_DEC_LINE2"), wxT("G_RM_AA_TEX_EDGE2"), wxT("G_RM_AA_SUB_SURF2"), wxT("G_RM_AA_PCL_SURF2"), wxT("G_RM_AA_OPA_TERR2"), wxT("G_RM_AA_TEX_TERR2"), wxT("G_RM_AA_SUB_TERR2"), wxT("G_RM_RA_OPA_SURF2"), wxT("G_RM_ZB_OPA_SURF2"), wxT("G_RM_ZB_XLU_SURF2"), wxT("G_RM_ZB_OPA_DECAL2"), wxT("G_RM_ZB_XLU_DECAL2"), wxT("G_RM_ZB_CLD_SURF2"), wxT("G_RM_ZB_OVL_SURF2"), wxT("G_RM_ZB_PCL_SURF2"), wxT("G_RM_OPA_SURF2"), wxT("G_RM_XLU_SURF2"), wxT("G_RM_CLD_SURF2"), wxT("G_RM_TEX_EDGE2"), wxT("G_RM_PCL_SURF2"), wxT("G_RM_ADD2") };
    int m_Choice_Render2NChoices = sizeof(m_Choice_Render2Choices) / sizeof(wxString);
    this->m_Choice_Render2 = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_Choice_Render2NChoices, m_Choice_Render2Choices, 0);
    this->m_Choice_Render2->SetSelection(this->m_Choice_Render2->FindString(highlighted_texture->rendermode2));
    this->m_Choice_Render2->Enable(this->m_Choice_Cycle->GetSelection() == 1);
    m_Sizer_Modes->Add(this->m_Choice_Render2, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
    m_Sizer_Main->Add(m_Sizer_Modes, 1, wxALIGN_CENTER_HORIZONTAL, 5);

    // Add the geometry flags section
    this->m_Label_Geometry = new wxStaticText(this, wxID_ANY, wxT("Geometry Flags"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_Label_Geometry->Wrap(-1);
    m_Sizer_Main->Add(this->m_Label_Geometry, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
    wxString m_Checklist_GeoFlagsChoices[] = { wxT("G_SHADE"), wxT("G_LIGHTING"), wxT("G_SHADING_SMOOTH"), wxT("G_ZBUFFER"), wxT("G_TEXTURE_GEN"), wxT("G_TEXTURE_GEN_LINEAR"), wxT("G_CULL_FRONT"), wxT("G_CULL_BACK"), wxT("G_FOG"), wxT("G_CLIPPING") };
    int m_Checklist_GeoFlagsNChoices = sizeof(m_Checklist_GeoFlagsChoices) / sizeof(wxString);
    this->m_Checklist_GeoFlags = new wxCheckListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_Checklist_GeoFlagsNChoices, m_Checklist_GeoFlagsChoices, wxLB_NEEDED_SB);
    m_Sizer_Main->Add(this->m_Checklist_GeoFlags, 0, wxALL | wxEXPAND, 5);
    for (int i=0; i<m_Checklist_GeoFlagsNChoices; i++)
    {
        for (std::list<std::string>::iterator itgeo = highlighted_texture->geomode.begin(); itgeo != highlighted_texture->geomode.end(); ++itgeo)
        {
            if (*itgeo == this->m_Checklist_GeoFlags->GetString(i))
            {
                this->m_Checklist_GeoFlags->Check(i);
                break;
            }
        }
    }

    // Add the close button
    this->m_Button_Close = new wxButton(this, wxID_ANY, wxT("Close"), wxDefaultPosition, wxDefaultSize, 0);
    m_Sizer_Main->Add(m_Button_Close, 1, wxALIGN_CENTER | wxALL, 5);

    // Finalize the layout
    this->SetSizer(m_Sizer_Main);
    this->Layout();
    m_Sizer_Main->Fit(this);
    this->Centre(wxBOTH);

    // Connect events
    this->m_Choice_Cycle->Connect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(AdvancedRenderSettings::m_Choice_CycleOnChoice), NULL, this);
    this->m_Choice_Filter->Connect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(AdvancedRenderSettings::m_Choice_FilterOnChoice), NULL, this);
    this->m_Choice_Combine1->Connect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(AdvancedRenderSettings::m_Choice_Combine1OnChoice), NULL, this);
    this->m_Choice_Combine2->Connect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(AdvancedRenderSettings::m_Choice_Combine2OnChoice), NULL, this);
    this->m_Choice_Render1->Connect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(AdvancedRenderSettings::m_Choice_Render1OnChoice), NULL, this);
    this->m_Choice_Render2->Connect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(AdvancedRenderSettings::m_Choice_Render2OnChoice), NULL, this);
    this->m_Checklist_GeoFlags->Connect(wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, wxCommandEventHandler(AdvancedRenderSettings::m_Checklist_GeoFlagsOnToggled), NULL, this);
    this->m_Button_Close->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(AdvancedRenderSettings::m_Button_CloseOnClick), NULL, this);
}


/*==============================
    AdvancedRenderSettings (Destructor)
    Cleans up the class before deletion
==============================*/

AdvancedRenderSettings::~AdvancedRenderSettings()
{
    this->m_Choice_Cycle->Disconnect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(AdvancedRenderSettings::m_Choice_CycleOnChoice), NULL, this);
    this->m_Choice_Filter->Disconnect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(AdvancedRenderSettings::m_Choice_FilterOnChoice), NULL, this);
    this->m_Choice_Combine1->Disconnect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(AdvancedRenderSettings::m_Choice_Combine1OnChoice), NULL, this);
    this->m_Choice_Combine2->Disconnect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(AdvancedRenderSettings::m_Choice_Combine2OnChoice), NULL, this);
    this->m_Choice_Render1->Disconnect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(AdvancedRenderSettings::m_Choice_Render1OnChoice), NULL, this);
    this->m_Choice_Render2->Disconnect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(AdvancedRenderSettings::m_Choice_Render2OnChoice), NULL, this);
    this->m_Checklist_GeoFlags->Disconnect(wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, wxCommandEventHandler(AdvancedRenderSettings::m_Checklist_GeoFlagsOnToggled), NULL, this);
    this->m_Button_Close->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(AdvancedRenderSettings::m_Button_CloseOnClick), NULL, this);
}


/*==============================
    AdvancedRenderSettings::m_Choice_CycleOnChoice
    Handles clicking on the Cycle Type dropdown
    @param The wxWidgets command event
==============================*/

void AdvancedRenderSettings::m_Choice_CycleOnChoice(wxCommandEvent& event)
{
    bool is2cycle = event.GetSelection() == 1;
    highlighted_texture->cycle = this->m_Choice_Cycle->GetString(event.GetSelection());
    this->m_Label_Combine2->Enable(is2cycle);
    this->m_Choice_Combine2->Enable(is2cycle);
    this->m_Label_Render2->Enable(is2cycle);
    this->m_Choice_Render2->Enable(is2cycle);
}


/*==============================
    AdvancedRenderSettings::m_Choice_FilterOnChoice
    Handles clicking on the Texture Filter dropdown
    @param The wxWidgets command event
==============================*/

void AdvancedRenderSettings::m_Choice_FilterOnChoice(wxCommandEvent& event)
{
    highlighted_texture->texfilter = this->m_Choice_Filter->GetString(event.GetSelection());
    highlighted_texture->RegenerateTexture();
}


/*==============================
    AdvancedRenderSettings::m_Choice_Combine1OnChoice
    Handles clicking on the Color Combiner 1 Mode dropdown
    @param The wxWidgets command event
==============================*/

void AdvancedRenderSettings::m_Choice_Combine1OnChoice(wxCommandEvent& event)
{
    highlighted_texture->combinemode1 = this->m_Choice_Combine1->GetString(event.GetSelection());
}


/*==============================
    AdvancedRenderSettings::m_Choice_Combine2OnChoice
    Handles clicking on the Color Combiner 2 Mode dropdown
    @param The wxWidgets command event
==============================*/

void AdvancedRenderSettings::m_Choice_Combine2OnChoice(wxCommandEvent& event)
{
    highlighted_texture->combinemode2 = this->m_Choice_Combine2->GetString(event.GetSelection());
}


/*==============================
    AdvancedRenderSettings::m_Choice_Render1OnChoice
    Handles clicking on the Render Mode 1 dropdown
    @param The wxWidgets command event
==============================*/

void AdvancedRenderSettings::m_Choice_Render1OnChoice(wxCommandEvent& event)
{
    highlighted_texture->rendermode1 = this->m_Choice_Render1->GetString(event.GetSelection());
}


/*==============================
    AdvancedRenderSettings::m_Choice_Render2OnChoice
    Handles clicking on the Render Mode 2 dropdown
    @param The wxWidgets command event
==============================*/

void AdvancedRenderSettings::m_Choice_Render2OnChoice(wxCommandEvent& event)
{
    highlighted_texture->rendermode2 = this->m_Choice_Render2->GetString(event.GetSelection());
}


/*==============================
    AdvancedRenderSettings::m_Checklist_GeoFlagsOnToggled
    Handles clicking on the Geometry Mode checklist
    @param The wxWidgets command event
==============================*/

void AdvancedRenderSettings::m_Checklist_GeoFlagsOnToggled(wxCommandEvent& event)
{
    std::string selection = this->m_Checklist_GeoFlags->GetString(event.GetSelection()).ToStdString();
    if (!this->m_Checklist_GeoFlags->IsChecked(event.GetSelection()))
    {
        highlighted_texture->geomode.remove(selection);
    }
    else
        highlighted_texture->geomode.push_back(selection);
}


/*==============================
    AdvancedRenderSettings::m_Button_CloseOnClick
    Handles clicking on the Close button
    @param The wxWidgets command event
==============================*/

void AdvancedRenderSettings::m_Button_CloseOnClick(wxCommandEvent& event)
{
    delete this;
}


/*********************************
     ColorPickerHelper Class
*********************************/

/*==============================
    ColorPickerHelper (Constructor)
    Initializes the class
==============================*/

ColorPickerHelper::ColorPickerHelper( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxDefaultSize, wxDefaultSize );

    // Create the main sizer
    this->m_Sizer_Main = new wxFlexGridSizer(0, 1, 0, 0);
    this->m_Sizer_Main->AddGrowableCol(0);
    this->m_Sizer_Main->AddGrowableRow(0);
    this->m_Sizer_Main->SetFlexibleDirection(wxBOTH);
    this->m_Sizer_Main->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

    // Create the color sizer
    this->m_Sizer_Colors = new wxFlexGridSizer(0, 3, 0, 0);
    this->m_Sizer_Colors->AddGrowableCol(1);
    this->m_Sizer_Colors->AddGrowableCol(2);
    this->m_Sizer_Colors->AddGrowableRow(0);
    this->m_Sizer_Colors->AddGrowableRow(1);
    this->m_Sizer_Colors->AddGrowableRow(2);
    this->m_Sizer_Colors->SetFlexibleDirection(wxBOTH);
    this->m_Sizer_Colors->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_NONE);

    // Add the red row
    this->m_Label_Red = new wxStaticText(this, wxID_ANY, wxT("Red:"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_Label_Red->Wrap(-1);
    this->m_Sizer_Colors->Add(this->m_Label_Red, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    this->m_TextCtrl_RGB_Red = new wxTextCtrl(this, wxID_ANY, wxT("0"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_TextCtrl_RGB_Red->SetMinSize(wxSize(32, -1));
    this->m_Sizer_Colors->Add(this->m_TextCtrl_RGB_Red, 0, wxALL|wxEXPAND, 5);
    this->m_TextCtrl_Hex_Red = new wxTextCtrl(this, wxID_ANY, wxT("00"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_TextCtrl_Hex_Red->SetMinSize(wxSize(32, -1));
    this->m_Sizer_Colors->Add(m_TextCtrl_Hex_Red, 0, wxALL|wxEXPAND, 5);

    // Add the green row
    this->m_Label_Green = new wxStaticText(this, wxID_ANY, wxT("Green:"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_Label_Green->Wrap(-1);
    this->m_Sizer_Colors->Add(this->m_Label_Green, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    this->m_TextCtrl_RGB_Green = new wxTextCtrl( this, wxID_ANY, wxT("255"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_TextCtrl_RGB_Green->SetMinSize( wxSize( 32,-1 ) );
    this->m_Sizer_Colors->Add(this->m_TextCtrl_RGB_Green, 0, wxALL|wxEXPAND, 5);
    this->m_TextCtrl_Hex_Green = new wxTextCtrl( this, wxID_ANY, wxT("FF"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_TextCtrl_Hex_Green->SetMinSize( wxSize( 32,-1 ) );
    this->m_Sizer_Colors->Add(this->m_TextCtrl_Hex_Green, 0, wxALL|wxEXPAND, 5);

    // Add the blue row
    this->m_Label_Blue = new wxStaticText(this, wxID_ANY, wxT("Blue:"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_Label_Blue->Wrap(-1);
    this->m_Sizer_Colors->Add(this->m_Label_Blue, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    this->m_TextCtrl_RGB_Blue = new wxTextCtrl(this, wxID_ANY, wxT("0"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_TextCtrl_RGB_Blue->SetMinSize(wxSize(32, -1));
    this->m_Sizer_Colors->Add(this->m_TextCtrl_RGB_Blue, 0, wxALL|wxEXPAND, 5);
    this->m_TextCtrl_Hex_Blue = new wxTextCtrl(this, wxID_ANY, wxT("00"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_TextCtrl_Hex_Blue->SetMinSize(wxSize(32, -1));
    this->m_Sizer_Colors->Add(this->m_TextCtrl_Hex_Blue, 0, wxALL|wxEXPAND, 5);

    // Add the color sizer to the main sizer
    this->m_Sizer_Main->Add(this->m_Sizer_Colors, 1, wxEXPAND, 5);

    // Add the buttons
    this->m_Sizer_Buttons = new wxGridSizer(0, 2, 0, 0);
    this->m_Button_Apply = new wxButton(this, wxID_ANY, wxT("Apply"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_Sizer_Buttons->Add(this->m_Button_Apply, 0, wxALIGN_RIGHT|wxALL, 5);
    this->m_Button_Cancel = new wxButton(this, wxID_ANY, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_Sizer_Buttons->Add(this->m_Button_Cancel, 0, wxALIGN_LEFT|wxALL, 5);
    this->m_Sizer_Main->Add(this->m_Sizer_Buttons, 1, wxEXPAND, 5);

    // Set the layout
    this->SetSizer(this->m_Sizer_Main);
    this->Layout();
    this->Centre(wxBOTH);

    // Connect Events
    this->m_TextCtrl_RGB_Red->Connect(wxEVT_KILL_FOCUS, wxCommandEventHandler(ColorPickerHelper::m_TextCtrl_OnTextChanged), NULL, this);
    this->m_TextCtrl_Hex_Red->Connect(wxEVT_KILL_FOCUS, wxCommandEventHandler(ColorPickerHelper::m_TextCtrl_OnTextChanged), NULL, this);
    this->m_TextCtrl_RGB_Green->Connect(wxEVT_KILL_FOCUS, wxCommandEventHandler(ColorPickerHelper::m_TextCtrl_OnTextChanged), NULL, this);
    this->m_TextCtrl_Hex_Green->Connect(wxEVT_KILL_FOCUS, wxCommandEventHandler(ColorPickerHelper::m_TextCtrl_OnTextChanged), NULL, this);
    this->m_TextCtrl_RGB_Blue->Connect(wxEVT_KILL_FOCUS, wxCommandEventHandler(ColorPickerHelper::m_TextCtrl_OnTextChanged), NULL, this);
    this->m_TextCtrl_Hex_Blue->Connect(wxEVT_KILL_FOCUS, wxCommandEventHandler(ColorPickerHelper::m_TextCtrl_OnTextChanged), NULL, this);
    this->m_Button_Apply->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ColorPickerHelper::m_Button_OnApplyPressed), NULL, this);
    this->m_Button_Cancel->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ColorPickerHelper::m_Button_OnCancelPressed), NULL, this);
}


/*==============================
    ColorPickerHelper (Destructor)
    Cleans up the class before deletion
==============================*/

ColorPickerHelper::~ColorPickerHelper()
{
    // Disconnect Events
    this->m_TextCtrl_RGB_Red->Disconnect(wxEVT_KILL_FOCUS, wxCommandEventHandler(ColorPickerHelper::m_TextCtrl_OnTextChanged), NULL, this);
    this->m_TextCtrl_Hex_Red->Disconnect(wxEVT_KILL_FOCUS, wxCommandEventHandler(ColorPickerHelper::m_TextCtrl_OnTextChanged), NULL, this);
    this->m_TextCtrl_RGB_Green->Connect(wxEVT_KILL_FOCUS, wxCommandEventHandler(ColorPickerHelper::m_TextCtrl_OnTextChanged), NULL, this);
    this->m_TextCtrl_Hex_Green->Disconnect(wxEVT_KILL_FOCUS, wxCommandEventHandler(ColorPickerHelper::m_TextCtrl_OnTextChanged), NULL, this);
    this->m_TextCtrl_RGB_Blue->Disconnect(wxEVT_KILL_FOCUS, wxCommandEventHandler(ColorPickerHelper::m_TextCtrl_OnTextChanged), NULL, this);
    this->m_TextCtrl_Hex_Blue->Disconnect(wxEVT_KILL_FOCUS, wxCommandEventHandler(ColorPickerHelper::m_TextCtrl_OnTextChanged), NULL, this);
    this->m_Button_Apply->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ColorPickerHelper::m_Button_OnApplyPressed), NULL, this);
    this->m_Button_Cancel->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ColorPickerHelper::m_Button_OnCancelPressed), NULL, this);
}


/*==============================
    ColorPickerHelper::m_TextCtrl_OnTextChanged
    Handles text control changing and validation
    @param The wxWidgets command event
==============================*/

void ColorPickerHelper::m_TextCtrl_OnTextChanged(wxCommandEvent& event)
{
    long int val;
    wxTextCtrl* rgb;
    wxTextCtrl* hex;
    if (event.GetId() == this->m_TextCtrl_RGB_Red->GetId())
    {
        rgb = this->m_TextCtrl_RGB_Red;
        hex = this->m_TextCtrl_Hex_Red;
        rgb->GetValue().ToLong(&val);
    }
    else if (event.GetId() == this->m_TextCtrl_Hex_Red->GetId())
    {
        rgb = this->m_TextCtrl_RGB_Red;
        hex = this->m_TextCtrl_Hex_Red;
        hex->GetValue().ToLong(&val, 16);
    }
    else if (event.GetId() == this->m_TextCtrl_RGB_Green->GetId())
    {
        rgb = this->m_TextCtrl_RGB_Green;
        hex = this->m_TextCtrl_Hex_Green;
        rgb->GetValue().ToLong(&val);
    }
    else if (event.GetId() == this->m_TextCtrl_Hex_Green->GetId())
    {
        rgb = this->m_TextCtrl_RGB_Green;
        hex = this->m_TextCtrl_Hex_Green;
        hex->GetValue().ToLong(&val, 16);
    }
    else if (event.GetId() == this->m_TextCtrl_RGB_Blue->GetId())
    {
        rgb = this->m_TextCtrl_RGB_Blue;
        hex = this->m_TextCtrl_Hex_Blue;
        rgb->GetValue().ToLong(&val);
    }
    else
    {
        rgb = this->m_TextCtrl_RGB_Blue;
        hex = this->m_TextCtrl_Hex_Blue;
        hex->GetValue().ToLong(&val, 16);
    }

    if (val < 0)
        val = 0;
    if (val > 255)
        val = 255;
    rgb->ChangeValue(wxString::Format("%d", (int)val));
    hex->ChangeValue(wxString::Format("%02X", (int)val));
}


/*==============================
    ColorPickerHelper::m_Button_OnApplyPressed
    Handles apply button pressing
    @param The wxWidgets command event
==============================*/

void ColorPickerHelper::m_Button_OnApplyPressed(wxCommandEvent& event)
{
    this->EndModal(wxID_OK);
}


/*==============================
    ColorPickerHelper::m_Button_OnCancelPressed
    Handles cancel button pressing
    @param The wxWidgets command event
==============================*/

void ColorPickerHelper::m_Button_OnCancelPressed(wxCommandEvent& event)
{
    this->EndModal(wxID_CANCEL);
}


/*==============================
    ColorPickerHelper::SetRGB
    Sets the RGB value to initialize the helper with
    @param The red value
    @param The green value
    @param The blue value
==============================*/

void ColorPickerHelper::SetRGB(uint8_t r, uint8_t g, uint8_t b)
{
    this->m_TextCtrl_RGB_Red->ChangeValue(wxString::Format("%d", r));
    this->m_TextCtrl_RGB_Green->ChangeValue(wxString::Format("%d", g));
    this->m_TextCtrl_RGB_Blue->ChangeValue(wxString::Format("%d", b));
    this->m_TextCtrl_Hex_Red->ChangeValue(wxString::Format("%02X", r));
    this->m_TextCtrl_Hex_Green->ChangeValue(wxString::Format("%02X", g));
    this->m_TextCtrl_Hex_Blue->ChangeValue(wxString::Format("%02X", b));
}


/*==============================
    ColorPickerHelper::GetRed
    Gets the red value created in the color picker
    @returns The red value
==============================*/

uint8_t ColorPickerHelper::GetRed()
{
    return wxAtoi(this->m_TextCtrl_RGB_Red->GetValue());
}


/*==============================
    ColorPickerHelper::GetGreen
    Gets the green value created in the color picker
    @returns The green value
==============================*/

uint8_t ColorPickerHelper::GetGreen()
{
    return wxAtoi(this->m_TextCtrl_RGB_Green->GetValue());
}


/*==============================
    ColorPickerHelper::GetBlue
    Gets the blue value created in the color picker
    @returns The blue value
==============================*/

uint8_t ColorPickerHelper::GetBlue()
{
    return wxAtoi(this->m_TextCtrl_RGB_Blue->GetValue());
}