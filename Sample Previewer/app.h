#pragma once

typedef struct IUnknown IUnknown;

#include <wx/wx.h>
#include "main.h"


/*********************************
             Classes
*********************************/

class App : public wxApp
{
	private:
		Main* m_frame1 = nullptr;
        
	protected:
    
	public:
		App();
		~App();
		virtual bool OnInit();
};