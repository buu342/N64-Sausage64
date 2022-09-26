#include <math.h>
#include "main.h"
#include "modelcanvas.h"
#include "sausage_texture.h"
#include "Include/glm/glm/gtx/compatibility.hpp"
#include "Include/glm/glm/gtc/type_ptr.hpp"
#include "Include/glm/glm/gtx/euler_angles.hpp"


/*********************************
              Macros
*********************************/

// Camera macros
#define GRIDSIZE 10000.0
#define CAMSPEED 300.0f
#define CAMSENSITIVITY 0.1f
#define UPVECTORZ glm::vec3(0.0f, 0.0f, 1.0f)
#define UPVECTORY glm::vec3(0.0f, 1.0f, 0.0f)

// Camera default macros
#define CAMSTART_POSZ   glm::vec3(-144.0f, -144.0f, 144.0f)
#define CAMSTART_POSY   glm::vec3(-144.0f, 144.0f, -144.0f)
#define CAMSTART_PITCH -12
#define CAMSTART_YAW   47


/*********************************
             Globals
*********************************/

// Missing texture
n64Texture* texture_missing;
GLuint texture_missing_id;

// Highlighted model sections
s64Mesh* highlighted_mesh = NULL;
n64Texture* highlighted_texture = NULL;
s64Anim* highlighted_anim = NULL;

// Highlighted animation info
size_t highlighted_anim_length = 0;
float  highlighted_anim_tick = 0;

// Default OpenGL lighting
static const GLfloat default_ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
static const GLfloat default_diffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat default_specular[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

// Last mouse position for first person camera rotation
static wxPoint lastmousepos;


/*==============================
    ModelCanvas (Constructor)
    Initializes the class
    @param The parent window
    @param The window ID
    @param The window position
    @param The window size
    @param The window style
    @param The window name
==============================*/

ModelCanvas::ModelCanvas(wxWindow* parent, const wxGLAttributes& attriblist, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name) : wxGLCanvas(parent, attriblist, id, pos, size, style | wxFULL_REPAINT_ON_RESIZE, name)
{
    this->m_context = new wxGLContext(this);
    this->Connect(wxEVT_PAINT, wxPaintEventHandler(ModelCanvas::m_Canvas_OnPaint), NULL, this);
    if (settings_yaxisup)
    {
        this->m_campos = CAMSTART_POSY;
        this->m_camyaw = CAMSTART_YAW + 90;
    }
    else
    {
        this->m_campos = CAMSTART_POSZ;
        this->m_camyaw = CAMSTART_YAW;
    }
    this->m_campitch = CAMSTART_PITCH;
    this->m_camdir = this->DirFromAngles(this->m_campitch, this->m_camyaw);
    this->m_deltatime = 0;
    this->m_lasttime = 0;
    this->m_mouseheld = false;
    this->m_app = NULL;
    lastmousepos = wxGetMousePosition();

    SetCurrent(*this->m_context);
    this->InitializeOpenGL();

    this->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(ModelCanvas::m_Canvas_OnMouse), NULL, this);
    this->Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(ModelCanvas::m_Canvas_OnMouse), NULL, this);
}


/*==============================
    ModelCanvas (Destructor)
    Cleans up the class before deletion
==============================*/

ModelCanvas::~ModelCanvas()
{
    this->Disconnect(wxEVT_PAINT, wxPaintEventHandler(ModelCanvas::m_Canvas_OnPaint), NULL, this);
    this->Disconnect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(ModelCanvas::m_Canvas_OnMouse), NULL, this);
    this->Disconnect(wxEVT_RIGHT_UP, wxMouseEventHandler(ModelCanvas::m_Canvas_OnMouse), NULL, this);
}


/*==============================
    ModelCanvas::SetApp
    Sets the parent app class
    @param A pointer to the parent app class
==============================*/

void ModelCanvas::SetApp(void* app)
{
    this->m_app = app;
}


/*==============================
    ModelCanvas::InitializeOpenGL
    Initializes OpenGL
==============================*/

void ModelCanvas::InitializeOpenGL()
{
    // Initialize our missing texture
    texture_missing = new n64Texture(TYPE_TEXTURE);
    glGenTextures(1, &texture_missing_id);
    glBindTexture(GL_TEXTURE_2D, texture_missing_id);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, texture_missing->GetTextureData()->wximg.GetData());
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, default_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, default_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, default_specular);
}


/*==============================
    ModelCanvas::SetupView
    Initializes the view for rendering
==============================*/

void ModelCanvas::SetupView()
{
    const wxSize ClientSize = GetClientSize()*GetContentScaleFactor();
    const float aspect = ((float)ClientSize.x)/ClientSize.y;
    glm::vec3 up;
    if (settings_yaxisup)
        up = UPVECTORY;
    else
        up = UPVECTORZ;
    const glm::vec3 camright = glm::normalize(glm::cross(up, this->m_camdir));
    const GLfloat ambient[] = {0.5f, 0.5f, 0.5f, 0.5f};

    glViewport(0, 0, ClientSize.x, ClientSize.y);
    glScissor(0, 0, ClientSize.x, ClientSize.y);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_TEXTURE_2D);

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, aspect, 10.0f, 10000.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMultMatrixf(&glm::lookAt(this->m_campos, this->m_campos + this->m_camdir, glm::cross(this->m_camdir, camright))[0][0]);
}


/*==============================
    ModelCanvas::RenderGrid
    Draws a grid on the floor
==============================*/

void ModelCanvas::RenderGrid()
{
    int i;

    if (!settings_showgrid)
        return;

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glLineWidth(1.0f);

    glBegin(GL_LINES);
    glColor4ub(88, 88, 88, 255);
    for (i=0; i<=256; i++)
    {
        double x = -GRIDSIZE + (i*(GRIDSIZE/256)*2);
        if (settings_yaxisup)
        {
            glVertex3d(x, 0, -GRIDSIZE);
            glVertex3d(x, 0, GRIDSIZE);
            glVertex3d(-GRIDSIZE, 0, x);
            glVertex3d(GRIDSIZE, 0, x);
        }
        else
        {
            glVertex3d(x, -GRIDSIZE, 0);
            glVertex3d(x, GRIDSIZE, 0);
            glVertex3d(-GRIDSIZE, x, 0);
            glVertex3d(GRIDSIZE, x, 0);
        }
    }

    glColor4ub(128, 128, 128, 255);
    for (i=0; i<=16; i++)
    {
        double x = -GRIDSIZE + (i*(GRIDSIZE/16)*2);
        if (settings_yaxisup)
        {
            glVertex3d(x, 0, -GRIDSIZE);
            glVertex3d(x, 0, GRIDSIZE);
            glVertex3d(-GRIDSIZE, 0, x);
            glVertex3d(GRIDSIZE, 0, x);
        }
        else
        {
            glVertex3d(x, -GRIDSIZE, 0);
            glVertex3d(x, GRIDSIZE, 0);
            glVertex3d(-GRIDSIZE, x, 0);
            glVertex3d(GRIDSIZE, x, 0);
        }
    }
    glEnd();

    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
}


/*==============================
    ModelCanvas::RenderOrigin
    Renders a grid on a given coordinate
    @param The x coordinate
    @param The y coordinate
    @param The z coordinate
    @param The scale to render the origin as
==============================*/

void ModelCanvas::RenderOrigin(float x, float y, float z, float scale)
{
    glDepthRange(0.0f, 0.0f);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glLineWidth(2.0f);
    glBegin(GL_LINES);

    // x
    glColor4ub(255, 0, 0, 255);
    glVertex3f(x, y, z);
    glVertex3f(x + scale, y, z);
    // y
    glColor4ub(0, 255, 0, 255);
    glVertex3f(x, y, z);
    glVertex3f(x, y + scale, z);
    // z
    glColor4ub(0, 0, 255, 255);
    glVertex3f(x, y, z);
    glVertex3f(x, y, z + scale);

    glEnd();
    glLineWidth(1.0f);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);

    glDepthRange(0.0f, 1.0f);
}

/*==============================
    ModelCanvas::RenderSausage64
    Renders a Sausage64 model, in an inefficient way
==============================*/

void ModelCanvas::RenderSausage64()
{
    float highlightcolor[4] = { 1.0f, 1.0f, 0.0f, 1.0f };
    s64Keyframe* curkeyf = NULL;
    s64Keyframe* nextkeyf = NULL;
    s64Model* mdl = ((Main*)this->m_app)->GetLoadedModel();
    n64Texture* lasttex = NULL;

    // If no model is loaded, stop
    if (mdl == NULL)
        return;

    // Get the current animation
    if (highlighted_anim != NULL)
    {
        if (!settings_animatingreverse)
        {
            for (std::list<s64Keyframe*>::iterator itkeyf = highlighted_anim->keyframes.begin(); itkeyf != highlighted_anim->keyframes.end(); ++itkeyf)
            {
                s64Keyframe* keyf = *itkeyf;
                if (keyf->keyframe > highlighted_anim_tick)
                {
                    nextkeyf = keyf;
                    break;
                }
                curkeyf = keyf;
            }
        }
        else
        {
            for (std::list<s64Keyframe*>::reverse_iterator itkeyf = highlighted_anim->keyframes.rbegin(); itkeyf != highlighted_anim->keyframes.rend(); ++itkeyf)
            {
                s64Keyframe* keyf = *itkeyf;
                if (keyf->keyframe <= highlighted_anim_tick)
                {
                    curkeyf = keyf;
                    break;
                }
                nextkeyf = keyf;
            }
        }
    }

    // Handle highlight (it's really just fog)
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogfv(GL_FOG_COLOR, highlightcolor);
    glFogf(GL_FOG_DENSITY, 0.8f);
    glHint(GL_FOG_HINT, GL_NICEST);
    glFogf(GL_FOG_START, 0.0f);
    glFogf(GL_FOG_END, 500.0f + 500.0f*sin(wxGetLocalTimeMillis().ToDouble()/100.0f));

    // Handle each mesh
    for (std::list<s64Mesh*>::iterator itmesh = mdl->GetMeshList()->begin(); itmesh != mdl->GetMeshList()->end(); ++itmesh)
    {
        s64FrameData* fdata = NULL;
        s64FrameData* nextfdata = NULL;
        s64Mesh* mesh = *itmesh;

        // Position the mesh based on the current animation frame
        glPushMatrix();
        if (highlighted_anim != NULL)
        {
            for (std::list<s64FrameData*>::iterator itfdata = curkeyf->framedata.begin(); itfdata != curkeyf->framedata.end(); ++itfdata)
            {
                fdata = *itfdata;
                if (fdata->mesh == mesh)
                    break;
            }
            for (std::list<s64FrameData*>::iterator itfdata = nextkeyf->framedata.begin(); itfdata != nextkeyf->framedata.end(); ++itfdata)
            {
                nextfdata = *itfdata;
                if (nextfdata->mesh == mesh)
                    break;
            }
            float l = ((float)(highlighted_anim_tick - curkeyf->keyframe)) / ((float)(nextkeyf->keyframe - curkeyf->keyframe));
            glm::mat4 mat = glm::mat4(1.0f);
            glm::vec3 translation = glm::lerp(fdata->translation, nextfdata->translation, l) + mesh->root;
            mat = glm::translate(mat, translation);
            if (mesh->billboard)
            {
                glm::vec3 direction = -glm::normalize(this->m_campos - translation);
                mat = mat * glm::inverse(glm::lookAt(glm::vec3(0, 0, 0), direction, glm::cross(direction, glm::normalize(glm::cross(UPVECTORZ, direction)))));
            }
            else
                mat = mat*glm::toMat4(glm::slerp(fdata->rotation, nextfdata->rotation, l));
            mat = glm::scale(mat, glm::lerp(fdata->scale, nextfdata->scale, l));
            glMultMatrixf(&mat[0][0]);
        }
        else
        {
            glm::mat4 mat = glm::mat4(1.0f);
            mat = glm::translate(mat, glm::vec3(mesh->root.x, mesh->root.y, mesh->root.z));
            if (mesh->billboard)
            {
                glm::vec3 direction = -glm::normalize(this->m_campos - glm::vec3(mesh->root.x, mesh->root.y, mesh->root.z));
                //glm::quat quaternion = glm::quat(glm::vec3(glm::radians(-90.0f), 0, 0));
                mat = mat*glm::inverse(glm::lookAt(glm::vec3(0,0,0), direction, glm::cross(direction, glm::normalize(glm::cross(UPVECTORZ, direction)))));
                //mat = mat*glm::mat4_cast(quaternion);
            }
            glMultMatrixf(&mat[0][0]);
            if (mesh->billboard)
                this->RenderOrigin(0.0f, 0.0f, 0.0f, 50.0f);
        }

        // Render the mesh roots
        if (settings_showmeshroots)
            this->RenderOrigin(0.0f, 0.0f, 0.0f, 10.0f);

        // Generate the triangles for each mesh
        for (std::list<s64Face*>::iterator itface = mesh->faces.begin(); itface != mesh->faces.end(); ++itface)
        {
            texCol* col = NULL;
            bool hashighlight = false;
            s64Face* face = *itface;
            std::list<s64Vert*> verts = face->verts;

            // Handle render settings
            if (face->texture != lasttex)
            {
                if (settings_showlighting && face->texture->HasGeoFlag("G_LIGHTING"))
                    glEnable(GL_LIGHTING);
                else
                    glDisable(GL_LIGHTING);
                if (face->texture->HasGeoFlag("G_ZBUFFER"))
                    glEnable(GL_DEPTH_TEST);
                else
                    glDisable(GL_DEPTH_TEST);
                glEnable(GL_CULL_FACE);
                if (face->texture->HasGeoFlag("G_CULL_FRONT") && face->texture->HasGeoFlag("G_CULL_BACK"))
                    glCullFace(GL_FRONT_AND_BACK);
                else if (face->texture->HasGeoFlag("G_CULL_FRONT"))
                    glCullFace(GL_FRONT);
                else if (face->texture->HasGeoFlag("G_CULL_BACK"))
                    glCullFace(GL_BACK);
                else
                    glDisable(GL_CULL_FACE);
                if (face->texture->HasGeoFlag("G_SHADING_SMOOTH"))
                    glShadeModel(GL_SMOOTH);
                else
                    glShadeModel(GL_FLAT);
                switch (face->texture->type)
                {
                    case TYPE_PRIMCOL:
                        glDisable(GL_TEXTURE);
                        glDisable(GL_TEXTURE_2D);
                        glEnable(GL_COLOR_MATERIAL);
                        glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
                        break;
                    case TYPE_TEXTURE:
                        glEnable(GL_TEXTURE);
                        glEnable(GL_TEXTURE_2D);
                        glDisable(GL_COLOR_MATERIAL);
                        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, default_diffuse);
                        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, default_ambient);
                        glBindTexture(GL_TEXTURE_2D, face->texture->GetTextureData()->glid);
                        break;
                    case TYPE_UNKNOWN:
                        glEnable(GL_TEXTURE);
                        glEnable(GL_TEXTURE_2D);
                        glDisable(GL_COLOR_MATERIAL);
                        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, default_diffuse);
                        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, default_ambient);
                        glBindTexture(GL_TEXTURE_2D, texture_missing_id);
                        break;
                }
                lasttex = face->texture;
            }
            if (face->texture->type == TYPE_PRIMCOL)
                col = face->texture->GetPrimColorData();

            // Enable fog if highlighted
            if (settings_showhighlight && (highlighted_mesh == mesh || highlighted_texture == face->texture))
                glEnable(GL_FOG);
            else
                glDisable(GL_FOG);

            // Render
            glBegin(GL_TRIANGLES);
            for (std::list<s64Vert*>::iterator itvert = verts.begin(); itvert != verts.end(); ++itvert)
            {
                s64Vert* vert = *itvert;
                if (face->texture->type == TYPE_PRIMCOL)
                {
                    if (col == NULL)
                        continue;
                    glColor3f(((float)col->r)/255.0f, ((float)col->g)/255.0f, ((float)col->b)/255.0f);
                }
                else
                    glColor3f(1.0f, 1.0f, 1.0f);
                glTexCoord2f(vert->UV.x, vert->UV.y);
                glNormal3f(vert->normal.x, vert->normal.y, vert->normal.z);
                glVertex3f(vert->pos.x, vert->pos.y, vert->pos.z);
            }
            glEnd();
        }

        // Pop the matrix for this mesh
        glPopMatrix();
    }
    glDisable(GL_FOG);
}


/*==============================
    ModelCanvas::DirFromAngles
    Calculates a direction vector given a pitch and yaw value
    @param The pitch
    @param The yaw
    @returns The direction vector
==============================*/

glm::vec3 ModelCanvas::DirFromAngles(float pitch, float yaw)
{
    glm::vec3 ang;
    if (settings_yaxisup)
    {
        ang.x = -cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        ang.y = sin(glm::radians(pitch));
        ang.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    }
    else
    {
        ang.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        ang.y = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        ang.z = sin(glm::radians(pitch));
    }
    return ang;
}


/*==============================
    ModelCanvas::HandleControls
    Handles keyboard and mouse controls
==============================*/

void ModelCanvas::HandleControls()
{
    if (!this->HasFocus())
        return;

    float speed = CAMSPEED*((float)this->m_deltatime.ToDouble())/1000000.0f;

    if (this->m_mouseheld)
    {
        wxSize center = this->GetClientSize()/2;
        if (settings_yaxisup)
        {
            this->m_camyaw -= (wxGetMousePosition().x - lastmousepos.x)*CAMSENSITIVITY;
            this->m_campitch -= (wxGetMousePosition().y - lastmousepos.y)*CAMSENSITIVITY;
            this->m_campitch = fmax(-89.9f, fmin(this->m_campitch, 89.9f));
        }
        else
        {
            this->m_camyaw -= (wxGetMousePosition().x - lastmousepos.x)*CAMSENSITIVITY;
            this->m_campitch -= (wxGetMousePosition().y - lastmousepos.y)*CAMSENSITIVITY;
            this->m_campitch = fmax(-89.9f, fmin(this->m_campitch, 89.9f));
        }
        this->m_camdir = this->DirFromAngles(this->m_campitch, this->m_camyaw);

        this->WarpPointer(center.x, center.y);
        lastmousepos = wxGetMousePosition();
    }

    if (wxGetKeyState(WXK_SHIFT))
        speed *= 5;

    if (wxGetKeyState((wxKeyCode)'W') || wxGetKeyState((wxKeyCode)'w'))
    {
        this->m_campos += this->m_camdir*speed;
    }

    if (wxGetKeyState((wxKeyCode)'S') || wxGetKeyState((wxKeyCode)'s'))
    {
        this->m_campos -= this->m_camdir*speed;
    }

    if (wxGetKeyState((wxKeyCode)'A') || wxGetKeyState((wxKeyCode)'a'))
    {
        if (settings_yaxisup)
            this->m_campos -= glm::normalize(glm::cross(this->m_camdir, UPVECTORY)) * speed;
        else
            this->m_campos -= glm::normalize(glm::cross(this->m_camdir, UPVECTORZ)) * speed;
    }

    if (wxGetKeyState((wxKeyCode)'D') || wxGetKeyState((wxKeyCode)'d'))
    {
        if (settings_yaxisup)
            this->m_campos += glm::normalize(glm::cross(this->m_camdir, UPVECTORY)) * speed;
        else
            this->m_campos += glm::normalize(glm::cross(this->m_camdir, UPVECTORZ)) * speed;
    }
}


/*==============================
    ModelCanvas::AdvanceAnim
    Advances the current animation by a given tick amount
    @param The tick amount to advance the animation by
==============================*/

void ModelCanvas::AdvanceAnim(float tickamount)
{
    if (highlighted_anim == NULL)
        return;
    highlighted_anim_tick += tickamount;

    // If the animation ended, call the callback function and roll the tick value over
    if (highlighted_anim_tick >= highlighted_anim_length)
    {       
        float division = highlighted_anim_tick/((float)highlighted_anim_length);
        highlighted_anim_tick = (division - ((int)division))*((float)highlighted_anim_length);
    }
    else if (highlighted_anim_tick <= 0)
    { 
        float division = highlighted_anim_tick/((float)highlighted_anim_length);
        highlighted_anim_tick = (1 + (division - ((int)division)))*((float)highlighted_anim_length);
    }
}


/*==============================
    ModelCanvas::SetYUp
    Converts the scene between Z up and Y up
    @param Whether the scene should be Y up or not
==============================*/

void ModelCanvas::SetYUp(bool val)
{
    this->m_campitch = CAMSTART_PITCH;
    this->m_camyaw = CAMSTART_YAW;
    if (val)
    {
        this->m_campos = CAMSTART_POSY;
        this->m_camyaw += 90;
    }
    else
        this->m_campos = CAMSTART_POSZ;
    this->m_camdir = this->DirFromAngles(this->m_campitch, this->m_camyaw);
}


/*==============================
    ModelCanvas::m_Canvas_OnMouse
    Handles mouse clicking on the canvas
    @param The wxWidgets mouse event
==============================*/

void ModelCanvas::m_Canvas_OnMouse(wxMouseEvent& event)
{
    if (event.RightDown() || event.RightIsDown())
    {
        wxSize center = this->GetClientSize() / 2;
        this->SetFocus();
        this->m_mouseheld = true;
        this->SetCursor(cursor_blank);
        this->WarpPointer(center.x, center.y);
        lastmousepos = wxGetMousePosition();
        if (this->GetCapture() == NULL)
            this->CaptureMouse();
    }
    else if (event.RightUp())
    {
        this->m_mouseheld = false;
        this->SetCursor(wxNullCursor);
        if (this->GetCapture() != NULL)
            this->ReleaseMouse();
    }
}


/*==============================
    ModelCanvas::m_Canvas_OnPaint
    Handles rendering the canvas
    @param The wxWidgets paint event
==============================*/

void ModelCanvas::m_Canvas_OnPaint(wxPaintEvent& event)
{
    wxLongLong curtime = wxGetUTCTimeUSec();
    this->m_deltatime = curtime - this->m_lasttime;
    this->m_lasttime = curtime;

    // Set the paint context
    wxPaintDC dc(this);
    SetCurrent(*this->m_context);

    // Clear the frame buffer with a color, and initialize the depth buffer
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);

    // Setup the render view
    this->SetupView();

    // Draw a grid on the floor
    this->RenderGrid();
    if (settings_showorigin)
        this->RenderOrigin(0.0f, 0.0f, 0.0f, 100.0f);

    // Render our model
    this->RenderSausage64();

    // Finish rendering
    glFlush();
    SwapBuffers();
}