#pragma once

typedef struct IUnknown IUnknown;

#include <string>
#include <list>
#include "Include/glm/glm/glm.hpp"
#include "Include/glm/glm/gtx/quaternion.hpp"
#include "sausage_mesh.h"


/*********************************
             Classes
*********************************/

class s64FrameData
{
    private:
    
    protected:
    
    public:
        s64Mesh* mesh;
        glm::vec3 translation;
        glm::quat rotation;
        glm::vec3 scale;
        s64FrameData();
        ~s64FrameData();
};

class s64Keyframe
{
    private:
    
    protected:
    
    public:
        unsigned int keyframe;
        std::list<s64FrameData*> framedata;
        s64Keyframe();
        ~s64Keyframe();
};

class s64Anim
{
    private:
    
    protected:
    
    public:
        std::string name;
        std::list<s64Keyframe*> keyframes;
        s64Anim();
        ~s64Anim();
};