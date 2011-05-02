#ifndef _UISLIDER_H
#define _UISLIDER_H

#include "UIElement.h"

class UISlider : public UIElement
{
public:
	UISlider(IDirect3DDevice9* Device, LONG top, LONG left, LONG right, LONG bottom, 
		D3DXVECTOR3* center, D3DXVECTOR3* position, float minX, float maxX, float minRange, float maxRange);
	~UISlider();

	bool IsHovered(float mouseX, float mouseY);
	bool IsClicked(float mouseX, float mouseY, bool isButtonClicked);

	/*bool GetClicked() { return mClicked; }
	void SetClicked(bool Clicked) { mClicked = Clicked; }
	bool GetHovered() { return mHovered; }
	void SetHovered(bool Hovered) { mHovered = Hovered; }*/
	
	virtual void Initialise();
	virtual void Update(float MouseX);
	virtual void Draw();
	virtual void Release();

private:

	float CalculatePercentageOnScreen();
	float CalculatePercentageOnSlider();

	RECT* mFontRect;
	int mWidth;
	
	bool mIsClicked;

	float mMinX, mMaxX;
	float mMinRange, mMaxRange;

	float PercentageOnScreen, PercentageOnSlider;

};

#endif