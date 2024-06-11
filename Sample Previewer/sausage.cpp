#include "sausage.h"


/*********************************
              Macros
*********************************/

#define STRBUFF_SIZE 512


/*==============================
    s64Model (Constructor)
    Initializes the class
==============================*/

s64Model::s64Model()
{
    this->m_lexer_statestack.push(STATE_NONE);
}


/*==============================
    s64Model (Destructor)
    Cleans up the class before deletion
==============================*/

s64Model::~s64Model()
{
    for (std::list<s64Mesh*>::iterator it = this->m_meshes.begin(); it != this->m_meshes.end(); ++it)
        delete *it;
    for (std::list<n64Material*>::iterator it = this->m_materials.begin(); it != this->m_materials.end(); ++it)
        delete *it;
    for (std::list<s64Anim*>::iterator it = this->m_anims.begin(); it != this->m_anims.end(); ++it)
        delete *it;
}


/*==============================
    s64Model::GetMaterialFromName
    Gets an material struct pointer given a name
    @param The name of the material to find
    @returns A pointer to the requested material, or NULL
==============================*/

n64Material* s64Model::GetMaterialFromName(std::string name)
{
    for (std::list<n64Material*>::iterator itmat = this->m_materials.begin(); itmat != this->m_materials.end(); ++itmat)
        if (name == (*itmat)->name)
            return *itmat;
    return NULL;
}


/*==============================
    s64Model::GenerateFromFile
    Generates a Sausage64 model from a .S64 file
    @param The filepath to the .S64 model
    @returns Whether the model generated successfully
==============================*/

bool s64Model::GenerateFromFile(std::string path)
{
    s64Mesh* curmesh = NULL;
    s64Vert* curvert = NULL;
    s64Face* prevface = NULL;
    s64Face* curface = NULL;
    s64Anim* curanim = NULL;
    s64Keyframe* curkeyframe = NULL;
    s64FrameData* curframedata = NULL;
    n64Material* curmat = NULL;
    std::list<s64Vert*>::iterator vertit;
    FILE* fp = fopen(path.c_str(), "r+");
    if (fp == NULL)
        return false;

    // Read the file until we reached the end
    while (!feof(fp))
    {
        char strbuf[STRBUFF_SIZE];
        char* strdata;

        // Read a string from the text file
        if (fgets(strbuf, STRBUFF_SIZE, fp) == NULL && !feof(fp))
        {
            fclose(fp);
            return false;
        }

        // Split the string by spaces
        strdata = strtok(strbuf, " ");

        // Parse each substring
        do
        {
            // Handle C comment lines
            if (strstr(strdata, "//") != NULL)
                break;

            // Handle C comment block starting
            if (strstr(strdata, "/*") != NULL)
            {
                this->m_lexer_statestack.push(STATE_COMMENTBLOCK);
                break;
            }

            // Handle C comment blocks
            if (this->m_lexer_statestack.top() == STATE_COMMENTBLOCK)
            {
                if (strstr(strdata, "*/") != NULL)
                    this->m_lexer_statestack.pop();
                continue;
            }

            // Handle Begin
            if (!strcmp(strdata, "BEGIN"))
            {
                // Handle the next substring
                strdata = strtok(NULL, " ");
                switch (this->m_lexer_statestack.top())
                {
                    case STATE_MESH:
                        strdata[strcspn(strdata, "\r\n")] = 0;
                        if (!strcmp(strdata, "VERTICES"))
                            this->m_lexer_statestack.push(STATE_VERTICES);
                        else if (!strcmp(strdata, "FACES"))
                            this->m_lexer_statestack.push(STATE_FACES);
                        break;
                    case STATE_ANIMATION:
                        if (!strcmp(strdata, "KEYFRAME"))
                        {
                            this->m_lexer_statestack.push(STATE_KEYFRAME);
                            curkeyframe = new s64Keyframe();
                            curkeyframe->keyframe = atoi(strtok(NULL, " "));
                            curanim->keyframes.push_back(curkeyframe);
                        }
                        break;
                    case STATE_NONE:
                        if (!strcmp(strdata, "MESH"))
                        {
                            this->m_lexer_statestack.push(STATE_MESH);

                            // Get the mesh name and fix the trailing newline
                            strdata = strtok(NULL, " ");
                            strdata[strcspn(strdata, "\r\n")] = 0;

                            // Create and initialize the mesh
                            curmesh = new s64Mesh();
                            curmesh->name = strdata;
                            this->m_meshes.push_back(curmesh);
                        }
                        else if (!strcmp(strdata, "ANIMATION"))
                        {
                            this->m_lexer_statestack.push(STATE_ANIMATION);

                            // Get the animation name and fix the trailing newline
                            strdata = strtok(NULL, " ");
                            strdata[strcspn(strdata, "\r\n")] = 0;

                            // Create the animation
                            curanim = new s64Anim();
                            curanim->name = strdata;
                            this->m_anims.push_back(curanim);
                        }
                        break;
                    default: break;
                }
            }
            else if (!strcmp(strdata, "END")) // Handle End
            {
                this->m_lexer_statestack.pop();
            }
            else
            {
                int vertcount;
                switch (this->m_lexer_statestack.top())
                {
                    case STATE_MESH:
                        if (!strcmp(strdata, "ROOT"))
                        {
                            curmesh->root.x = (float)atof(strtok(NULL, " "));
                            curmesh->root.y = (float)atof(strtok(NULL, " "));
                            curmesh->root.z = (float)atof(strtok(NULL, " "));
                        }
                        if (!strcmp(strdata, "PROPERTIES"))
                        {
                            while ((strdata = strtok(NULL, " ")) != NULL)
                            {
                                strdata[strcspn(strdata, "\r\n")] = 0;
                                curmesh->props.push_back(strdata);
                            }
                            curmesh->ParseProperties();
                        }
                        break;
                    case STATE_VERTICES:
                        curvert = new s64Vert();
                        curmesh->verts.push_back(curvert);

                        // Set the vertex data
                        curvert->pos.x = (float)atof(strdata) - curmesh->root.x;
                        curvert->pos.y = (float)atof(strtok(NULL, " ")) - curmesh->root.y;
                        curvert->pos.z = (float)atof(strtok(NULL, " ")) - curmesh->root.z;
                        curvert->normal.x = (float)atof(strtok(NULL, " "));
                        curvert->normal.y = (float)atof(strtok(NULL, " "));
                        curvert->normal.z = (float)atof(strtok(NULL, " "));
                        curvert->color.x = (float)atof(strtok(NULL, " "));
                        curvert->color.y = (float)atof(strtok(NULL, " "));
                        curvert->color.z = (float)atof(strtok(NULL, " "));
                        curvert->UV.x = (float)atof(strtok(NULL, " "));
                        curvert->UV.y = (float)atof(strtok(NULL, " "));
                        break;
                    case STATE_FACES:

                        curface = new s64Face();
                        curmesh->faces.push_back(curface);

                        // Set the face data
                        vertcount = atoi(strdata);
                        if (vertcount > 4)
                        {
                            fclose(fp);
                            return false;
                        }
                        curface->verts.push_back(curmesh->GetVertFromIndex(atoi(strtok(NULL, " "))));
                        curface->verts.push_back(curmesh->GetVertFromIndex(atoi(strtok(NULL, " "))));
                        curface->verts.push_back(curmesh->GetVertFromIndex(atoi(strtok(NULL, " "))));

                        // Handle quads
                        prevface = NULL;
                        if (vertcount == 4)
                        {
                            prevface = curface;
                            curface = new s64Face();
                            curmesh->faces.push_back(curface);
                            curface->verts.push_back(prevface->GetVertFromIndex(0));
                            curface->verts.push_back(prevface->GetVertFromIndex(2));
                            curface->verts.push_back(curmesh->GetVertFromIndex(atoi(strtok(NULL, " "))));
                        }

                        // Get the material name and check if it exists already
                        strdata = strtok(NULL, " ");
                        strdata[strcspn(strdata, "\r\n")] = 0;
                        curmat = this->GetMaterialFromName(strdata);
                        if (curmat == NULL)
                        {
                            curmat = new n64Material(TYPE_UNKNOWN);
                            curmat->name = strdata;
                            this->m_materials.push_back(curmat);
                        }
                        curface->material = curmat;

                        // Assign the face to the previous face as well, if we have a quad
                        if (prevface != NULL)
                        {
                            prevface->material = curmat;
                            prevface = NULL;
                        }

                        // If this material hasn't been added to this mesh yet, do so
                        if (curmesh->GetMaterialFromName(strdata) == NULL)
                            curmesh->materials.push_back(curmat);
                        break;
                    case STATE_KEYFRAME:
                        curframedata = new s64FrameData();
                        curkeyframe->framedata.push_back(curframedata);
                        for (std::list<s64Mesh*>::iterator itfdata = this->m_meshes.begin(); itfdata != this->m_meshes.end(); ++itfdata)
                        {
                            if (!strcmp(strdata, (*itfdata)->name.c_str()))
                            {
                                curframedata->mesh = (*itfdata);
                                break;
                            }
                        }
                        curframedata->translation.x = (float)atof(strtok(NULL, " "));
                        curframedata->translation.y = (float)atof(strtok(NULL, " "));
                        curframedata->translation.z = (float)atof(strtok(NULL, " "));
                        curframedata->rotation.w = (float)atof(strtok(NULL, " "));
                        curframedata->rotation.x = (float)atof(strtok(NULL, " "));
                        curframedata->rotation.y = (float)atof(strtok(NULL, " "));
                        curframedata->rotation.z = (float)atof(strtok(NULL, " "));
                        curframedata->scale.x = (float)atof(strtok(NULL, " "));
                        curframedata->scale.y = (float)atof(strtok(NULL, " "));
                        curframedata->scale.z = (float)atof(strtok(NULL, " "));
                        break;
                    default: break;
                }
            }

        }
        while ((strdata = strtok(NULL, " ")) != NULL);
    }

    // Close the file, as we're done with it.
    fclose(fp);

    // Correct animation keyframes that don't start on zero
    for (std::list<s64Anim*>::iterator itanim = this->m_anims.begin(); itanim != this->m_anims.end(); ++itanim)
    {
        int firstframe = -1;
        s64Anim* anim = *itanim;
        for (std::list<s64Keyframe*>::iterator itkeyf = anim->keyframes.begin(); itkeyf != anim->keyframes.end(); ++itkeyf)
        {
            s64Keyframe* keyf = *itkeyf;
            if (firstframe == -1 && keyf->keyframe == 0)
                break;
            if (firstframe == -1 && keyf->keyframe != 0)
                firstframe = keyf->keyframe;
            keyf->keyframe -= firstframe;
        }
    }

    // Return success
    return true;
}


/*==============================
    s64Model::GetMeshCount
    Gets the number of meshes in this model
    @param The number of meshes
==============================*/

int s64Model::GetMeshCount()
{
    return this->m_meshes.size();
}


/*==============================
    s64Model::GetMaterialCount
    Gets the number of materials in this model
    @param The number of materials
==============================*/

int s64Model::GetMaterialCount()
{
    return this->m_materials.size();
}


/*==============================
    s64Model::GetAnimCount
    Gets the number of animations in this model
    @param The number of animations
==============================*/

int s64Model::GetAnimCount()
{
    return this->m_anims.size();
}


/*==============================
    s64Model::GetMeshList
    Gets the list of meshes in this model
    @returns A pointer to the list of meshes
==============================*/

std::list<s64Mesh*>* s64Model::GetMeshList()
{
    return &this->m_meshes;
}


/*==============================
    s64Model::GetMaterialList
    Gets the list of materials in this model
    @returns A pointer to the list of materials
==============================*/

std::list<n64Material*>* s64Model::GetMaterialList()
{
    return &this->m_materials;
}


/*==============================
    s64Model::GetAnimList
    Gets the list of animations in this model
    @returns A pointer to the list of animations
==============================*/

std::list<s64Anim*>* s64Model::GetAnimList()
{
    return &this->m_anims;
}