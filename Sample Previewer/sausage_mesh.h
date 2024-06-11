#pragma once

typedef struct IUnknown IUnknown;

#include <string>
#include <list>
#include "Include/glm/glm/glm.hpp"
#include "sausage_material.h"


/*********************************
             Classes
*********************************/

class s64Vert
{
    private:
    
    protected:
    
    public:
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec2 UV;
        s64Vert();
        ~s64Vert();
};

class s64Face
{
    private:
    
    protected:
    
    public:
        std::list<s64Vert*> verts;
        n64Material* material;
        s64Face();
        ~s64Face();
        s64Vert* GetVertFromIndex(unsigned int index);
};

class s64Mesh
{
    private:
    
    protected:
    
    public:
        std::string name;
        glm::vec3 root;
        std::list<s64Vert*> verts;
        std::list<s64Face*> faces;
        std::list<n64Material*> materials;
        std::list<std::string> props;
        bool billboard;
        s64Mesh();
        ~s64Mesh();
        void ParseProperties();
        s64Vert* GetVertFromIndex(unsigned int index);
        n64Material* GetMaterialFromName(std::string name);
};