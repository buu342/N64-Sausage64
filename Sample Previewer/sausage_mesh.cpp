#include "sausage_mesh.h"


/*********************************
          s64Vert Class
*********************************/

/*==============================
    s64Vert (Constructor)
    Initializes the class
==============================*/

s64Vert::s64Vert()
{
	this->pos = glm::vec3(0.0f, 0.0f, 0.0f);
	this->normal = glm::vec3(0.0f, 0.0f, 0.0f);
	this->color = glm::vec3(0.0f, 0.0f, 0.0f);
	this->UV = glm::vec2(0.0f, 0.0f);
}


/*==============================
    s64Vert (Destructor)
    Cleans up the class before deletion
==============================*/

s64Vert::~s64Vert()
{
    
}


/*********************************
          s64Face Class
*********************************/

/*==============================
    s64Face (Constructor)
    Initializes the class
==============================*/

s64Face::s64Face()
{
	this->material = NULL;
}


/*==============================
    s64Face (Destructor)
    Cleans up the class before deletion
==============================*/

s64Face::~s64Face()
{

}


/*==============================
    s64Face::GetVertFromIndex
    Gets an s64Vert struct pointer given a vertex index
    @param The vertex index to get
    @returns A pointer to the requested vertex, or NULL
==============================*/

s64Vert* s64Face::GetVertFromIndex(unsigned int index)
{
	if (index >= this->verts.size())
		return NULL;
	std::list<s64Vert*>::iterator vertit = this->verts.begin();
	std::advance(vertit, index);
	return *vertit;
}


/*********************************
          s64Mesh Class
*********************************/

/*==============================
    s64Mesh (Constructor)
    Initializes the class
==============================*/

s64Mesh::s64Mesh()
{
	this->name = "";
	this->root = glm::vec3(0.0f, 0.0f, 0.0f);
	this->billboard = false;
}


/*==============================
    s64Mesh (Destructor)
    Cleans up the class before deletion
==============================*/

s64Mesh::~s64Mesh()
{
	for (std::list<s64Vert*>::iterator itvert = this->verts.begin(); itvert != this->verts.end(); ++itvert)
		delete *itvert;
	for (std::list<s64Face*>::iterator itface = this->faces.begin(); itface != this->faces.end(); ++itface)
		delete *itface;
}


/*==============================
    s64Mesh::ParseProperties
    Parses the mesh properties from the list of properties 
==============================*/

void s64Mesh::ParseProperties()
{
	for (std::list<std::string>::iterator itprops = this->props.begin(); itprops != this->props.end(); ++itprops)
	{
		std::string str = *itprops;
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){return ::tolower(c);});
		if (str == "billboard")
			this->billboard = true;
	}
}


/*==============================
    s64Mesh::GetVertFromIndex
    Gets an s64Vert struct pointer given a vertex index
    @param The vertex index to get
    @returns A pointer to the requested vertex, or NULL
==============================*/

s64Vert* s64Mesh::GetVertFromIndex(unsigned int index)
{
	if (index >= this->verts.size())
		return NULL;
	std::list<s64Vert*>::iterator vertit = this->verts.begin();
	std::advance(vertit, index);
	return *vertit;
}


/*==============================
    s64Mesh::GetMaterialFromName
    Gets an material struct pointer given a name
    @param The name of the material to find
    @returns A pointer to the requested material, or NULL
==============================*/

n64Material* s64Mesh::GetMaterialFromName(std::string name)
{
	for (std::list<n64Material*>::iterator itmat = this->materials.begin(); itmat != this->materials.end(); ++itmat)
		if (name == (*itmat)->name.c_str())
			return *itmat;
	return NULL;
}