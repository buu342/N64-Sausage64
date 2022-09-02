#include "sausage_animation.h"


/*********************************
        s64FrameData Class
*********************************/

/*==============================
    s64FrameData (Constructor)
    Initializes the class
==============================*/

s64FrameData::s64FrameData()
{
	this->mesh = NULL;
	this->translation = glm::vec3(0.0f, 0.0f, 0.0f);
	this->rotation = glm::quat(0.0f, 0.0f, 0.0f, 0.0f);
	this->scale = glm::vec3(0.0f, 0.0f, 0.0f);
}


/*==============================
    s64FrameData (Destructor)
    Cleans up the class before deletion
==============================*/

s64FrameData::~s64FrameData()
{

}


/*********************************
        s64Keyframe Class
*********************************/

/*==============================
    s64Keyframe (Constructor)
    Initializes the class
==============================*/

s64Keyframe::s64Keyframe()
{
	this->keyframe = 0;
}


/*==============================
    s64Keyframe (Destructor)
    Cleans up the class before deletion
==============================*/

s64Keyframe::~s64Keyframe()
{
	for (std::list<s64FrameData*>::iterator it = this->framedata.begin(); it != this->framedata.end(); ++it)
		delete* it;
}


/*********************************
          s64Anim Class
*********************************/

/*==============================
    s64Anim (Constructor)
    Initializes the class
==============================*/

s64Anim::s64Anim()
{
	this->name = "";
}


/*==============================
    s64Anim (Destructor)
    Cleans up the class before deletion
==============================*/

s64Anim::~s64Anim()
{
	for (std::list<s64Keyframe*>::iterator it = this->keyframes.begin(); it != this->keyframes.end(); ++it)
		delete *it;
}