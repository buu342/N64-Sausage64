#pragma once

typedef struct IUnknown IUnknown;

#include <string>
#include <list>
#include <stack>
#include "Include/glm/glm/glm.hpp"
#include "sausage_material.h"
#include "sausage_mesh.h"
#include "sausage_animation.h"


/*********************************
           Custom Types
*********************************/

typedef enum {
    STATE_NONE,
    STATE_COMMENTBLOCK,
    STATE_MESH,
    STATE_VERTICES,
    STATE_FACES,
    STATE_ANIMATION,
    STATE_KEYFRAME
} lexState;


/*********************************
             Classes
*********************************/

class s64Model
{
    private:
        std::list<s64Mesh*> m_meshes;
        std::list<n64Material*> m_materials;
        std::list<s64Anim*> m_anims;
        std::stack<lexState> m_lexer_statestack;
        
    protected:
    
    public:
        s64Model();
        ~s64Model();
        n64Material* GetMaterialFromName(std::string name);
        bool GenerateFromFile(std::string path);
        int GetMeshCount();
        int GetMaterialCount();
        int GetAnimCount();
        std::list<s64Mesh*>* GetMeshList();
        std::list<n64Material*>* GetMaterialList();
        std::list<s64Anim*>* GetAnimList();
};