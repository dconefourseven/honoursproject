#include "D3DFont.h"
#include <windows.h>
#include <stdio.h>

D3DFont::D3DFont(LPDIRECT3DDEVICE9 device)
{
	pDevice = device;
	CreateFont();
}

D3DFont::~D3DFont()
{
}

void D3DFont::CreateFont()
{
	//Create the font used to render the frame counter
	D3DXCreateFont( pDevice, 18, 0, 
		FW_BOLD, 0, FALSE, DEFAULT_CHARSET, 
		OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, 
		DEFAULT_PITCH | FF_DONTCARE, TEXT("Courier New"), &m_Font );
}

char infoString[80];
void D3DFont::Update(float deltaTime, float width, float height)
{
	rect.bottom = (LONG)height;
	rect.right = (LONG)width;
	rect.top = 0;
	rect.left = 0;
	float fps = 1/deltaTime;
	sprintf_s(m_DeltaTimeString, sizeof(m_DeltaTimeString) + 50, "%f ms per frame \n%f frames per second", deltaTime, fps);
}

void D3DFont::Update(float deltaTime, float width, float height, float samples)
{
	rect.bottom = (LONG)height;
	rect.right = (LONG)width;
	rect.top = 0;
	rect.left = 0;
	float fps = 1/deltaTime;
	sprintf_s(m_DeltaTimeString, sizeof(m_DeltaTimeString) + 50, 
		"%f ms per frame \n%f frames per second \n Current Samples: %f", 
		deltaTime, fps, samples);
}

void D3DFont::Draw()
{
	//Draw the font last so it is above everything
	m_Font->DrawText(0, m_DeltaTimeString, -1, &rect, DT_TOP | DT_LEFT /*draw in the top left corner*/, D3DCOLOR_ARGB(255,255,255,0));// yellow text
}

void D3DFont::Release()
{
	//pDevice->Release();
	m_Font->Release();
}