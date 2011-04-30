#ifndef _USERINTERFACE_H
#define _USERINTERFACE_H

#include <list>
//#include "UIElement.h"
#include "UIButton.h"

using namespace std;

class UserInterface
{
public:
	UserInterface(IDirect3DDevice9* pDevice,  int ScreenWidth, int ScreenHeight);
	~UserInterface();

	void Initialise();
	void Update();
	void Draw();
	void Release();

private:

	UIButton* test;

	list<UIElement*> mUIElements;

	IDirect3DDevice9* pDevice;

	int mScreenWidth, mScreenHeight;
};


#endif