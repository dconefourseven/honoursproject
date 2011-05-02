#include "UISlider.h"

UISlider::UISlider(IDirect3DDevice9* Device, LONG top, LONG left, LONG right, LONG bottom, 
		D3DXVECTOR3* center, D3DXVECTOR3* position)
		: UIElement(Device, top, left, right, bottom, 
		center, position)
{
	mFontRect = new RECT();

	mFontRect->bottom = bottom + (LONG)position->y;
	mFontRect->right = right + (LONG)position->x;
	mFontRect->top = (LONG)position->y;
	mFontRect->left = (LONG)position->x;

	mWidth = right;

	mIsClicked = false;
}

UISlider::~UISlider()
{
}

bool UISlider::IsHovered(float mouseX, float mouseY)
{
	if(mouseX < mFontRect->left - 10)
		return false;
	if(mouseX > mFontRect->right + 10)
		return false;
	if(mouseY < mFontRect->top)
		return false;
	if(mouseY > mFontRect->bottom)
		return false;
    
    return true;
}

bool UISlider::IsClicked(float mouseX, float mouseY, bool isButtonClicked)
{
	if(!isButtonClicked)
	{
		mIsClicked = false;
		return false;
	}

	if(!IsHovered(mouseX, mouseY)) 
	{
		mIsClicked = false;
		return false;
	}

	mIsClicked = true;
	return true;
}

void UISlider::Initialise()
{
	UIElement::Initialise();
}

void UISlider::Update(float MouseX)
{
	UIElement::Update();
	if(mIsClicked)
	{
		UIElement::mPosition->x = MouseX - mWidth/2;
		mFontRect->right = (LONG)(mWidth + UIElement::mPosition->x);
		mFontRect->left = (LONG)UIElement::mPosition->x;		
	}
}

void UISlider::Draw()
{
	UIElement::Draw(mIsClicked);
}

void UISlider::Release()
{
	UIElement::Release();
}
