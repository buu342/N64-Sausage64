#pragma once

typedef struct IUnknown IUnknown;

#include <algorithm>
#include <memory>
#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <wx/wxprec.h>
#include <GL/glu.h>
#include "Include/glm/glm/glm.hpp"
#include "Include/glm/glm/ext/matrix_transform.hpp"


/*********************************
             Globals
*********************************/

// Placeholder missing texture
extern n64Texture* texture_missing;
extern GLuint texture_missing_id;

// Highlighted objects
extern s64Mesh* highlighted_mesh;
extern n64Texture* highlighted_texture;
extern s64Anim* highlighted_anim;

// Highlighted animation information
extern size_t highlighted_anim_length;
extern float  highlighted_anim_tick;


/*********************************
             Classes
*********************************/

class ModelCanvas : public wxGLCanvas 
{
    private:
        glm::vec3 m_campos;
        glm::vec3 m_camdir;
        float m_camyaw;
        float m_campitch;
        wxGLContext* m_context;
        wxLongLong m_deltatime;
        wxLongLong m_lasttime;
        bool m_mouseheld;
        void* m_app;
        
    protected:
    
    public:
        ModelCanvas(wxWindow *parent, const wxGLAttributes &attribs, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name);
        ~ModelCanvas();
        void      SetApp(void* app);
        glm::vec3 DirFromAngles(float pitch, float yaw);
        void      HandleControls();
        void      AdvanceAnim(float tickamount);
        void      InitializeOpenGL();
        void      SetupView();
        void      RenderGrid();
        void      RenderOrigin(float x, float y, float z, float scale);
        void      RenderSausage64();
        void      m_Canvas_OnMouse(wxMouseEvent& event);
        void      m_Canvas_OnPaint(wxPaintEvent& event);
        void      SetYUp(bool val);
};