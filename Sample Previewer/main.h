#pragma once

typedef struct IUnknown IUnknown;

#include <list>
#include <wx/wx.h>
#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/panel.h>
#include <wx/treectrl.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/checkbox.h>
#include <wx/slider.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/gbsizer.h>
#include <wx/stattext.h>
#include <wx/radiobut.h>
#include <wx/statbmp.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/colourdata.h>
#include <wx/colordlg.h>
#include <wx/timer.h>
#include <wx/frame.h>
#include "sausage.h"
#include "modelcanvas.h"


/*********************************
              Macros
*********************************/

#define PROGRAM_NAME "Chorizo"


/*********************************
             Globals
*********************************/

// Images
extern wxCursor cursor_blank;
extern wxBitmap icon_play;

// Program settings
extern bool  settings_showgrid;
extern bool  settings_showorigin;
extern bool  settings_showlighting;
extern bool  settings_showmeshroots;
extern bool  settings_yaxisup;
extern bool  settings_showhighlight;
extern bool  settings_animating;
extern float settings_playbackspeed;
extern bool  settings_animatingreverse;


/*********************************
             Classes
*********************************/

class Main : public wxFrame
{
    private:
    
    protected:
        wxMenuBar* m_MenuBar;
        wxMenu* m_Menu_File;
        wxMenuItem* m_MenuItem_FileTextureImport;
        wxMenuItem* m_MenuItem_FileTextureExport;
        wxMenu* m_Menu_Animation;
        wxMenuItem* m_MenuItem_AnimationPlayToggle;
        wxMenuItem* m_MenuItem_AnimationReverse;
        wxMenu* m_SubMenu_AnimationSpeed;
        wxMenuItem* m_MenuItem_AnimationFaster;
        wxMenuItem* m_MenuItem_AnimationNormal;
        wxMenuItem* m_MenuItem_AnimationSlower;
        wxMenu* m_Menu_View;
        wxSplitterWindow* m_Splitter_Horizontal;
        wxPanel* m_Panel_Top;
        wxSplitterWindow* m_Splitter_Vertical;
        ModelCanvas* m_Model_Canvas;
        wxPanel* m_Panel_TreeCtrl;
        wxTreeCtrl* m_TreeCtrl_ModelData;
        wxTreeItemId* m_TreeItem_Texture;
        wxPanel* m_Panel_Bottom;
        wxBoxSizer* m_Sizer_Bottom;
        wxBoxSizer* m_Sizer_Bottom_Mesh;
        wxFlexGridSizer* m_Sizer_Bottom_Texture;
        wxGridBagSizer* m_Sizer_Bottom_Animation;
        wxCheckBox* m_CheckBox_Mesh_Billboard;
        wxSlider* m_Slider_Animation;
        wxBitmapButton* m_Button_AnimationToggle;
        wxRadioButton* m_Radio_Image;
        wxRadioButton* m_Radio_PrimColor;
        wxRadioButton* m_Radio_Omit;
        wxBoxSizer* m_Sizer_Bottom_Texture_Setup;
        wxStaticBitmap* m_Image_Texture;
        wxStaticText* m_Label_Texture;
        wxChoice* m_Choice_Texture_ImgFormat;
        wxChoice* m_Choice_Texture_SFlag;
        wxChoice* m_Choice_Texture_ImgDepth;
        wxChoice* m_Choice_Texture_TFlag;
        wxBoxSizer* m_Sizer_Bottom_PrimCol_Setup;
        wxPanel* m_Panel_PrimCol;
        wxStaticText* m_Label_PrimColRGB;
        wxStaticText* m_Label_PrimColHex;
        wxButton* m_Button_Texture_Advanced;
        wxCheckBox* m_CheckBox_Texture_LoadFirst;
        wxCheckBox* m_CheckBox_Texture_DontLoad;
        wxButton* m_Button_PrimCol_Advanced;
        wxCheckBox* m_CheckBox_PrimCol_LoadFirst;
        wxCheckBox* m_CheckBox_PrimCol_DontLoad;
        wxTimer m_Timer_MainLoop;
        s64Model* m_Sausage64Model;
        
    public:
        Main();
        ~Main();
        void m_Splitter_HorizontalOnIdle(wxIdleEvent& event);
        void m_Splitter_VerticalOnIdle(wxIdleEvent& event);
        void m_Timer_MainLoopOnTimer(wxTimerEvent& event);
        void m_MenuItem_ImportOnMenuSelection(wxCommandEvent& event);
        void m_MenuItem_ImportTextureOnMenuSelection(wxCommandEvent& event);
        void m_MenuItem_ExportTextureOnMenuSelection(wxCommandEvent& event);
        void m_MenuItem_AnimationPlayToggleOnMenuSelection(wxCommandEvent& event);
        void m_MenuItem_AnimationReverseOnMenuSelection(wxCommandEvent& event);
        void m_MenuItem_AnimationFasterOnMenuSelection(wxCommandEvent& event);
        void m_MenuItem_AnimationNormalOnMenuSelection(wxCommandEvent& event);
        void m_MenuItem_AnimationSlowerOnMenuSelection(wxCommandEvent& event);
        void m_MenuItem_ViewGridOnSelected(wxCommandEvent& event);
        void m_MenuItem_ViewOriginOnSelected(wxCommandEvent& event);
        void m_MenuItem_ViewLightingOnSelected(wxCommandEvent& event);
        void m_MenuItem_ViewMeshRootsOnSelected(wxCommandEvent& event);
        void m_MenuItem_ViewYUpOnSelected(wxCommandEvent& event);
        void m_MenuItem_ViewHighlightOnSelected(wxCommandEvent& event);
        void m_CheckBox_Mesh_BillboardOnCheck(wxCommandEvent& event);
        void m_Slider_AnimationOnScroll(wxScrollEvent& event);
        void m_Button_AnimationToggleOnClick(wxCommandEvent& event);
        void m_Radio_TextureTypeOnButton1(wxCommandEvent& event);
        void m_Radio_TextureTypeOnButton2(wxCommandEvent& event);
        void m_Radio_TextureTypeOnButton3(wxCommandEvent& event);
        void m_Image_TextureOnLeftDown(wxMouseEvent& event);
        void m_Choice_Texture_ImgFormatOnChoice(wxCommandEvent& event);
        void m_Choice_Texture_SFlagOnChoice(wxCommandEvent& event);
        void m_Panel_PrimColOnLeftDown(wxMouseEvent& event);
        void m_Button_AdvancedOnClick(wxCommandEvent& event);
        void m_Choice_Texture_ImgDepthOnChoice(wxCommandEvent& event);
        void m_Choice_Texture_TFlagOnChoice(wxCommandEvent& event);
        void m_CheckBox_LoadFirstOnCheckBox(wxCommandEvent& event);
        void m_CheckBox_DontLoadOnCheckBox(wxCommandEvent& event);
        void m_TreeCtrl_ModelDataOnTreeSelChanged(wxTreeEvent& event);
        void m_Label_PrimColOnLeftDown(wxMouseEvent& event);
        void OnPrimColChanged(wxColourDialogEvent& event);
        s64Model* GetLoadedModel();
        void RefreshPrimColPanel();
        void RefreshTextureImage();
};

class AdvancedRenderSettings : public wxDialog
{
	private:
    
	protected:
		wxChoice* m_Choice_Cycle;
		wxChoice* m_Choice_Filter;
		wxChoice* m_Choice_Combine1;
		wxChoice* m_Choice_Combine2;
        wxStaticText* m_Label_Combine2;
		wxStaticText* m_Label_Render2;
		wxChoice* m_Choice_Render1;
		wxChoice* m_Choice_Render2;
		wxStaticText* m_Label_Geometry;
		wxCheckListBox* m_Checklist_GeoFlags;
		wxButton* m_Button_Close;
        
	public:
		AdvancedRenderSettings( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Advanced Render Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );
		~AdvancedRenderSettings();
        void m_Choice_CycleOnChoice(wxCommandEvent& event);
        void m_Choice_FilterOnChoice(wxCommandEvent& event);
        void m_Choice_Combine1OnChoice(wxCommandEvent& event);
        void m_Choice_Combine2OnChoice(wxCommandEvent& event);
        void m_Choice_Render1OnChoice(wxCommandEvent& event);
        void m_Choice_Render2OnChoice(wxCommandEvent& event);
        void m_Checklist_GeoFlagsOnToggled(wxCommandEvent& event);
        void m_Button_CloseOnClick(wxCommandEvent& event);
};

class ColorPickerHelper : public wxDialog
{
    private:

    protected:
        wxFlexGridSizer* m_Sizer_Main;
        wxFlexGridSizer* m_Sizer_Colors;
        wxGridSizer* m_Sizer_Buttons;
        wxStaticText* m_Label_Red;
        wxTextCtrl* m_TextCtrl_RGB_Red;
        wxTextCtrl* m_TextCtrl_Hex_Red;
        wxStaticText* m_Label_Green;
        wxTextCtrl* m_TextCtrl_RGB_Green;
        wxTextCtrl* m_TextCtrl_Hex_Green;
        wxStaticText* m_Label_Blue;
        wxTextCtrl* m_TextCtrl_RGB_Blue;
        wxTextCtrl* m_TextCtrl_Hex_Blue;
        wxButton* m_Button_Apply;
        wxButton* m_Button_Cancel;

    public:
        ColorPickerHelper(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Manual Color Input"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(200, 210), long style = wxDEFAULT_DIALOG_STYLE );
        ~ColorPickerHelper();
        void m_TextCtrl_OnTextChanged(wxCommandEvent& event);
        void m_Button_OnApplyPressed(wxCommandEvent& event);
        void m_Button_OnCancelPressed(wxCommandEvent& event);
        void SetRGB(uint8_t r, uint8_t g, uint8_t b);
        uint8_t GetRed();
        uint8_t GetGreen();
        uint8_t GetBlue();
};