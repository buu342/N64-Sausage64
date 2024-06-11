#include <wx/gdicmn.h>
#include "Resources/MISSING.h"
#include "sausage_material.h"
#include <wx/glcanvas.h>
#ifdef MACOS
    #include <OpenGL/glu.h>
#else
    #include <GL/glu.h>
#endif


/*==============================
    n64Material (Constructor)
    Initializes the class
==============================*/

n64Material::n64Material(matType type)
{
	std::string defaultgeo[] = DEFAULT_GEOFLAGS;
	int size = sizeof(defaultgeo)/sizeof(defaultgeo[0]);

    // Initialize each attribute to the default value
	this->name = "";
	this->type = TYPE_UNKNOWN;
	this->cycle = DEFAULT_CYCLE;
	this->rendermode1 = DEFAULT_RENDERMODE1;
	this->rendermode2 = DEFAULT_RENDERMODE2;
	this->texfilter = DEFAULT_TEXFILTER;
	for (int i=0; i<size; i++)
		this->geomode.push_back(defaultgeo[i]);
	this->dontload = false;
	this->loadfirst = false;

	// Generate the material data
	switch (type)
	{
		case TYPE_PRIMCOL:
			this->CreateDefaultPrimCol();
			break;
		case TYPE_TEXTURE:
			this->CreateDefaultTexture();
			break;
		default:
			this->CreateDefaultUnknown();
			break;
	}
}


/*==============================
    n64Material (Destructor)
    Cleans up the class before deletion
==============================*/

n64Material::~n64Material()
{
	switch (this->type)
	{
		case TYPE_PRIMCOL:
			delete ((matCol*)this->data);
			break;
		case TYPE_TEXTURE:
			glDeleteTextures(1, &this->GetTextureData()->glid);
			delete ((matImage*)this->data);
			break;
		default:
			break;
	}
}


/*==============================
    n64Material::GetTextureData
    Gets the material data
    @returns A pointer to the texture data struct
==============================*/

matImage* n64Material::GetTextureData()
{
	if (this->type != TYPE_TEXTURE)
		return NULL;
	return (matImage*)this->data;
}


/*==============================
    n64Material::GetPrimColorData
    Gets the primitive color data
    @returns A pointer to the primitive color data struct
==============================*/

matCol* n64Material::GetPrimColorData()
{
	if (this->type != TYPE_PRIMCOL)
		return NULL;
	return (matCol*)this->data;
}


/*==============================
    n64Material::CreateDefaultTexture
    Creates a default texture
    @param The width to initialize the texture with
    @param The height to initialize the texture with
==============================*/

void n64Material::CreateDefaultTexture(uint32_t w, uint32_t h)
{
    // Delete any previous data if it exists
	switch (this->type)
	{
		case TYPE_TEXTURE:
			glDeleteTextures(1, &this->GetTextureData()->glid);
			break;
		case TYPE_PRIMCOL:
			delete ((matCol*)this->data);
		default: // Intentional fallthrough
			this->combinemode1 = DEFAULT_COMBINE1_TEX;
			this->combinemode2 = DEFAULT_COMBINE2_TEX;
			this->data = new matImage();
			break;
	}
    
    // Initialize the texture attributes
	this->type = TYPE_TEXTURE;
	this->SetImageFromData(MISSING_png, MISSING_png_length, w, h);
	this->GetTextureData()->coltype = DEFAULT_IMAGEFORMAT;
	this->GetTextureData()->colsize = DEFAULT_IMAGESIZE;
	this->GetTextureData()->texmodes = DEFAULT_TEXFLAGS;
	this->GetTextureData()->texmodet = DEFAULT_TEXFLAGT;
}


/*==============================
    n64Material::CreateDefaultPrimCol
    Creates a default primitive color
==============================*/

void n64Material::CreateDefaultPrimCol()
{
    // Delete any previous data if it exists
	switch (this->type)
    {
        case TYPE_PRIMCOL:
            break;
        case TYPE_TEXTURE:
            glDeleteTextures(1, &this->GetTextureData()->glid);
            delete ((matCol*)this->data);
        default: // Intentional fallthrough
            this->combinemode1 = DEFAULT_COMBINE1_PRIM;
            this->combinemode2 = DEFAULT_COMBINE2_PRIM;
            this->data = new matCol();
            break;
	}
    
    // Initialize the primcolor attributes
	this->type = TYPE_PRIMCOL;
	this->GetPrimColorData()->r = 0;
	this->GetPrimColorData()->g = 255;
	this->GetPrimColorData()->b = 0;
	this->GetPrimColorData()->a = 255;
}


/*==============================
    n64Material::CreateDefaultUnknown
    Creates a default unknown texture
==============================*/

void n64Material::CreateDefaultUnknown()
{
    // Delete any previous data if it exists
	switch (this->type)
	{
		case TYPE_PRIMCOL:
			delete ((matCol*)this->data);
			break;
		case TYPE_TEXTURE:
			glDeleteTextures(1, &this->GetTextureData()->glid);
			delete ((matImage*)this->data);
			break;
		default: 
			break;
	}
    
    // Initialize the material attributes
	this->type = TYPE_UNKNOWN;
	this->combinemode1 = DEFAULT_COMBINE1_TEX;
	this->combinemode2 = DEFAULT_COMBINE2_TEX;
	this->data = NULL;
}


/*==============================
    n64Material::SetImageFromFile
    Sets the texture image from a file path
    @param The path to the image to load
==============================*/

void n64Material::SetImageFromFile(std::string path)
{
	wxImage img;
	wxImage old = this->GetTextureData()->wximg;

	// Generate the wxImage
	if (!this->GetTextureData()->wximg.LoadFile(path))
	{
		img = old;
		return;
	}
	this->GetTextureData()->w = this->GetTextureData()->wximg.GetWidth();
	this->GetTextureData()->h = this->GetTextureData()->wximg.GetHeight();

	// Create a wxBitmap, scaled to match the preview box
	img = this->GetTextureData()->wximg;
	img.Rescale(64, 64, wxIMAGE_QUALITY_NEAREST);
	this->GetTextureData()->wxbmp = wxBitmap(img);

	// Generate the OpenGL data
	glGenTextures(1, &this->GetTextureData()->glid);
	glBindTexture(GL_TEXTURE_2D, this->GetTextureData()->glid);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Get the image data
	GLubyte* imgdata = this->GetTextureData()->wximg.GetData();
	GLubyte* alpadata = this->GetTextureData()->wximg.GetAlpha();
	int depth = this->GetTextureData()->wximg.HasAlpha() ? 4 : 3;
	GLubyte* finalimage = new GLubyte[this->GetTextureData()->w*this->GetTextureData()->h*depth];

	// Generate image data in the format OpenGL expects (with alpha included)
	for (size_t y=0; y<this->GetTextureData()->h; y++)
	{
		for (size_t x=0; x<this->GetTextureData()->w; x++)
		{
			finalimage[(x + y*this->GetTextureData()->w)*depth + 0] = imgdata[(x + y*this->GetTextureData()->w)*3];
			finalimage[(x + y*this->GetTextureData()->w)*depth + 1] = imgdata[(x + y*this->GetTextureData()->w)*3 + 1];
			finalimage[(x + y*this->GetTextureData()->w)*depth + 2] = imgdata[(x + y*this->GetTextureData()->w)*3 + 2];
			if (depth == 4)
				finalimage[(x + y*this->GetTextureData()->w)*depth + 3] = alpadata[x + y*this->GetTextureData()->w];
		}
	}

	// Make the texture and free the memory
	glTexImage2D(GL_TEXTURE_2D, 0, depth, this->GetTextureData()->w, this->GetTextureData()->h, 0, this->GetTextureData()->wximg.HasAlpha() ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, finalimage);
	delete[] finalimage;
}


/*==============================
    n64Material::SetImageFromData
    Sets the texture image from a binary file
    @param The binary data to read
    @param The size of the data
    @param The width to force the texture to, or zero
    @param The height to force the texture to, or zero
==============================*/

void n64Material::SetImageFromData(const unsigned char* data, size_t size, uint32_t w, uint32_t h)
{
	wxImage img;

	// Generate the wxImage and wxBitmap
	this->GetTextureData()->wxbmp = wxBitmap::NewFromPNGData(data, size);
	this->GetTextureData()->wximg = this->GetTextureData()->wxbmp.ConvertToImage();
	this->GetTextureData()->w = (w == 0) ? this->GetTextureData()->wximg.GetWidth() : w;
	this->GetTextureData()->h = (h == 0) ? this->GetTextureData()->wximg.GetHeight() : h;

	// Create a wxBitmap, scaled to match the preview box
	img = this->GetTextureData()->wxbmp.ConvertToImage();
	img.Rescale(64, 64, wxIMAGE_QUALITY_NEAREST);
	this->GetTextureData()->wxbmp = wxBitmap(img);

	// Generate the OpenGL data
	glGenTextures(1, &this->GetTextureData()->glid);
	glBindTexture(GL_TEXTURE_2D, this->GetTextureData()->glid);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->GetTextureData()->wximg.GetWidth(), this->GetTextureData()->wximg.GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, this->GetTextureData()->wximg.GetData());
}


/*==============================
    n64Material::RegenerateTexture
    Regenerates the OpenGL texture
==============================*/

void n64Material::RegenerateTexture()
{
	glDeleteTextures(1, &this->GetTextureData()->glid);

	// Generate the OpenGL data
	glGenTextures(1, &this->GetTextureData()->glid);
	glBindTexture(GL_TEXTURE_2D, this->GetTextureData()->glid);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	if (this->texfilter == "G_TF_POINT")
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	else
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	if (this->GetTextureData()->texmodes == "G_TX_MIRROR")
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	else if (this->GetTextureData()->texmodes == "G_TX_WRAP")
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	else
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	if (this->GetTextureData()->texmodet == "G_TX_MIRROR")
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	else if (this->GetTextureData()->texmodet == "G_TX_WRAP")
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	else
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Get the image data
	GLubyte* imgdata = this->GetTextureData()->wximg.GetData();
	GLubyte* alpadata = this->GetTextureData()->wximg.GetAlpha();
	int depth = this->GetTextureData()->wximg.HasAlpha() ? 4 : 3;
	GLubyte* finalimage = new GLubyte[this->GetTextureData()->w * this->GetTextureData()->h * depth];

	// Generate image data in the format OpenGL expects (with alpha included)
	for (size_t y = 0; y < this->GetTextureData()->h; y++)
	{
		for (size_t x = 0; x < this->GetTextureData()->w; x++)
		{
			finalimage[(x + y * this->GetTextureData()->w) * depth + 0] = imgdata[(x + y * this->GetTextureData()->w) * 3];
			finalimage[(x + y * this->GetTextureData()->w) * depth + 1] = imgdata[(x + y * this->GetTextureData()->w) * 3 + 1];
			finalimage[(x + y * this->GetTextureData()->w) * depth + 2] = imgdata[(x + y * this->GetTextureData()->w) * 3 + 2];
			if (depth == 4)
				finalimage[(x + y * this->GetTextureData()->w) * depth + 3] = alpadata[x + y * this->GetTextureData()->w];
		}
	}

	// make the texture and free the memory
	glTexImage2D(GL_TEXTURE_2D, 0, depth, this->GetTextureData()->w, this->GetTextureData()->h, 0, this->GetTextureData()->wximg.HasAlpha() ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, finalimage);
	delete[] finalimage;
}


/*==============================
    n64Material::HasGeoFlag
    Checks if the material has a given geometry flag
    @param A string with the flag to check
    @returns Whether the material has the given flag
==============================*/

bool n64Material::HasGeoFlag(std::string flag)
{
	for (std::list<std::string>::iterator itflag = this->geomode.begin(); itflag != this->geomode.end(); ++itflag)
		if (flag == *itflag)
			return true;
	return false;
}


/*==============================
    n64Material::SetFlag
    Sets a geometry flag on the material.
    This code was ported from Arabiki
    @param A string with the flag to set
==============================*/

void n64Material::SetFlag(std::string flag)
{
	// Static variables to keep track of stuff between successive calls to this function
	static bool render2 = false;
	static bool combine2 = false;
	static bool texmode2 = false;

	// Set the texture flags
	if (flag.find(DONTLOAD) != std::string::npos)
	{
		this->dontload = TRUE;
	}
	else if (flag.find(LOADFIRST) != std::string::npos)
	{
		this->loadfirst = TRUE;
	}
	else if (flag.find(G_CYC_) != std::string::npos)
	{
		this->cycle = flag;
	}
	else if (flag.find(G_TF_) != std::string::npos)
	{
		this->texfilter = flag;
	}
	else if (flag.find(G_CC_) != std::string::npos)
	{
		if (!combine2)
		{
			this->combinemode1 = flag;
			combine2 = true;
		}
		else
			this->combinemode2 = flag;
	}
	else if (flag.find(G_RM_) != std::string::npos)
	{
		if (!render2)
		{
			this->rendermode1 = flag;
			render2 = true;
		}
		else
			this->rendermode2 = flag;
	}
	else if (flag.find(G_IM_FMT_) != std::string::npos)
	{
		this->GetTextureData()->coltype = flag;
	}
	else if (flag.find(G_IM_SIZ_) != std::string::npos)
	{
		this->GetTextureData()->colsize = flag;
	}
	else if (flag.find(G_TX_) != std::string::npos)
	{
		if (!texmode2)
		{
			this->GetTextureData()->texmodes = flag;
			texmode2 = TRUE;
		}
		else
			this->GetTextureData()->texmodet = flag;
	}
	else
	{
		if (flag[0] != '!') // Set texture flag
		{
			this->geomode.push_back(flag);
		}
		else // Remove texture flag
		{
			flag.erase(0, 1);
			this->geomode.remove(flag);
		}
	}
}