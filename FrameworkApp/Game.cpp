#include <d3d9.h>
#include <d3dx9.h>
#include <time.h>
#include "Game.h"
#include "XModel.h"
#include "D3DFont.h"
#include "BasicLightingInterface.h"
#include "Dwarf.h"
#include "DirectInput.h"
#include "FPCamera.h"
#include "App Framework\Shader Interface\PhongLightingInterface.h"
#include "App Framework\Shader Interface\Animated\AnimatedInterface.h"
#include "App Framework\Animation\Vertex.h"
#include "App Framework\Animation\SkinnedMesh.h"
#include "App\Render Targets\DrawableRenderTarget.h"
#include "App\Render Targets\DrawableTex2D.h"
#include "App\Objects\Render Objects\Citadel.h"

//LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL; // Buffer to hold vertices

D3DXMATRIXA16 matWorld, matView, matProj, matWorldInverseTranspose;
D3DXVECTOR4 vViewVector;

D3DFont* m_Font;

PhongLightingInterface* m_PhongInterface;
PhongLighting m_PhongContainer;

AnimatedInterface* m_AnimatedInterface;
AnimatedContainer m_AnimatedContainer;

Citadel* m_Citadel;

DirectInput* m_DInput;

FPCamera* m_Camera;

IDirect3DVertexBuffer9* mRadarVB;

SkinnedMesh* m_SkinnedMesh;

Dwarf* m_Dwarf;

DirLight mLight;
Mtrl     mWhiteMtrl;

DrawableRenderTarget* m_RenderTarget;
DrawableRenderTarget* m_ShadowTarget;
DrawableTex2D* mShadowMap;

IDirect3DTexture9* m_WhiteTexture;

ID3DXEffect* mFX;
	D3DXHANDLE   mhBuildShadowMapTech;
	D3DXHANDLE   mhLightWVP;

	D3DXHANDLE   mhTech;
	D3DXHANDLE   mhWVP;
	D3DXHANDLE   mhWorldInvTrans;
	D3DXHANDLE   mhEyePosW;
	D3DXHANDLE   mhWorld;
	D3DXHANDLE   mhTex;
	D3DXHANDLE   mhShadowMap;
	D3DXHANDLE   mhMtrl;
	D3DXHANDLE   mhLight;
 
	SpotLight mSpotLight;
	D3DXMATRIXA16 m_LightViewProj;

Game::Game(LPDIRECT3DDEVICE9 g_pd3dDevice)
{
	pDevice = g_pd3dDevice;
}

Game::Game(LPDIRECT3DDEVICE9 g_pd3dDevice, HANDLE mutex, queue<ModelPacket>* Receive, queue<ModelPacket>* Send)
{
	pDevice = g_pd3dDevice;
	ReceiveQ = Receive;
	SendQ = Send;
	mutexHandle = mutex;

	m_PacketTicker = 0.0f;

	//Set the buffer to identify this as the server sending the packet
	sprintf_s(m_ModelPacket.Buffer, sizeof(m_ModelPacket.Buffer), "Server position packet");
	//Using 0 to identify the server
	m_ModelPacket.ID = 0;

	srand ( (unsigned int)time(NULL) );
}

Game::~Game()
{
}

bool Game::Initialise()
{
	//CalculateMatrices();
	InitAllVertexDeclarations(pDevice);

	// Turn off culling, so we see the front and back of the triangle
    pDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );

	// Turn off culling, so we see the front and back of the triangle
    pDevice->SetRenderState(D3DRS_ZENABLE , D3DZB_TRUE );

    // Turn on D3D lighting, since we are providing our own vertex colors
    pDevice->SetRenderState( D3DRS_LIGHTING, TRUE );

	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE); 
	return true;
}

bool Game::LoadContent()
{
	m_Font = new D3DFont(pDevice);

	m_PhongInterface = new PhongLightingInterface(pDevice);

	m_DInput = new DirectInput();

	ConnectionStatus = false;
	
	m_Citadel = new Citadel(pDevice);

	D3DXVECTOR3 vEyePt( 0.0f, 5.0f,-20.0f );
    D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );

	m_Camera = new FPCamera(vEyePt,	vLookatPt, vUpVec, (int)m_WindowWidth, (int)m_WindowHeight);	

	D3DXMatrixPerspectiveFovLH(&matProj,D3DX_PI / 4.0f, m_WindowWidth/m_WindowHeight , 1.0f, 1.0f);

	m_SkinnedMesh = new SkinnedMesh(pDevice, "Models/Tiny", "tiny.x", "Tiny_skin.bmp");

	m_Dwarf = new Dwarf(pDevice);

	mLight.dirW    = D3DXVECTOR3(0.0f, -1.0f, 1.0f);
	D3DXVec3Normalize(&mLight.dirW, &mLight.dirW);
	mLight.ambient = D3DXCOLOR(0.5f, 0.5f, 0.5f, 1.0f);
	mLight.diffuse = D3DXCOLOR(0.8f, 0.8f, 0.8f, 1.0f);
	mLight.spec    = D3DXCOLOR(0.8f, 0.8f, 0.8f, 1.0f);

	mWhiteMtrl.ambient = WHITE*0.9f;
	mWhiteMtrl.diffuse = WHITE*0.6f;
	mWhiteMtrl.spec    = WHITE*0.6f;
	mWhiteMtrl.specPower = 48.0f;

	m_RenderTarget = new DrawableRenderTarget(pDevice, (UINT)m_WindowWidth, (UINT)m_WindowHeight);
	m_ShadowTarget = new DrawableRenderTarget(pDevice, 512, 512);

	// Create shadow map.
	D3DVIEWPORT9 vp = {0, 0, 512, 512, 0.0f, 1.0f};
	mShadowMap = new DrawableTex2D(pDevice, 512, 512, 1, D3DFMT_R32F, true, D3DFMT_D24X8, vp, false);

	m_AnimatedInterface = new AnimatedInterface(pDevice);

	D3DXCreateTextureFromFile(pDevice, "whitetex.dds", &m_WhiteTexture);

	 // Create the FX from a .fx file.
	ID3DXBuffer* errors = 0;
	HR(D3DXCreateEffectFromFile(pDevice, "Shaders/LightShadow.fx", 
		0, 0, D3DXSHADER_DEBUG, 0, &mFX, &errors));
	if( errors )
		MessageBox(0, (char*)errors->GetBufferPointer(), 0, 0);

	// Obtain handles.
	mhTech               = mFX->GetTechniqueByName("LightShadowTech");
	mhBuildShadowMapTech = mFX->GetTechniqueByName("BuildShadowMapTech");
	mhLightWVP           = mFX->GetParameterByName(0, "gLightWVP");
	mhWVP                = mFX->GetParameterByName(0, "gWVP");
	mhWorld              = mFX->GetParameterByName(0, "gWorld");
	mhMtrl               = mFX->GetParameterByName(0, "gMtrl");
	mhLight              = mFX->GetParameterByName(0, "gLight");
	mhEyePosW            = mFX->GetParameterByName(0, "gEyePosW");
	mhTex                = mFX->GetParameterByName(0, "gTex");
	mhShadowMap          = mFX->GetParameterByName(0, "gShadowMap");

	// Set some light properties; other properties are set in update function,
	// where they are animated.
	mSpotLight.ambient   = D3DXCOLOR(0.5f, 0.5f, 0.5f, 1.0f);
	mSpotLight.diffuse   = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	mSpotLight.spec      = D3DXCOLOR(0.5f, 0.5f, 0.5f, 1.0f);
	mSpotLight.spotPower = 32.0f;

	return true;
}

D3DXVECTOR3 KeyboardDwarfVelocity(0.0f, 0.0f, 0.0f);
const float camSpeed = 0.1f;
void Game::HandleInput()
{
	//Update direct input
	m_DInput->Update();

	if(m_DInput->GetMouseState(0))		
		m_Camera->mouseMove();		
	else
		m_Camera->First(true);

	//Zero the velocity every frame
	KeyboardDwarfVelocity = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

	//Check the key presses
	//W
	if(m_DInput->GetKeyState(1))
		m_Camera->Move(camSpeed*m_DeltaTime, camSpeed*m_DeltaTime, camSpeed*m_DeltaTime);
	
	//S
	if(m_DInput->GetKeyState(2))
		m_Camera->Move(-camSpeed*m_DeltaTime, -camSpeed*m_DeltaTime, -camSpeed*m_DeltaTime);

	//A
	if(m_DInput->GetKeyState(3))
		m_Camera->Strafe(camSpeed*m_DeltaTime, camSpeed*m_DeltaTime, camSpeed*m_DeltaTime);

	//D
	if(m_DInput->GetKeyState(4))
		m_Camera->Strafe(-camSpeed*m_DeltaTime, -camSpeed*m_DeltaTime, -camSpeed*m_DeltaTime);			
}

void Game::Update()
{
	m_Camera->Update(m_DeltaTime);
	
	m_Citadel->Update();

	m_Dwarf->Update();

	CalculateMatrices();

	m_Font->Update(m_DeltaTime, m_WindowWidth, m_WindowHeight);	

	// Animate the skinned mesh.
	m_SkinnedMesh->Update(m_DeltaTime);

	// Animate spot light by rotating it on y-axis with respect to time.
	D3DXMATRIX lightView;
	D3DXVECTOR3 lightPosW(00.0f, 50.0f, -115.0f);
	//D3DXVECTOR3 lightPosW(125.0f, 50.0f, 0.0f);
	D3DXVECTOR3 lightTargetW(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 lightUpW(0.0f, 1.0f, 0.0f);

	static float t = 0.0f;
	t += m_DeltaTime;
	if( t >= 2.0f*D3DX_PI )
		t = 0.0f;
	D3DXMATRIX Ry;
	D3DXMatrixRotationY(&Ry, t);
	D3DXVec3TransformCoord(&lightPosW, &lightPosW, &Ry);

	D3DXMatrixLookAtLH(&lightView, &lightPosW, &lightTargetW, &lightUpW);
	
	D3DXMATRIX lightLens;
	float lightFOV = D3DX_PI*0.25f;
	D3DXMatrixPerspectiveFovLH(&lightLens, lightFOV, 1.0f, 1.0f, 200.0f);

	m_LightViewProj = lightView * lightLens;

	// Setup a spotlight corresponding to the projector.
	D3DXVECTOR3 lightDirW = lightTargetW - lightPosW;
	D3DXVec3Normalize(&lightDirW, &lightDirW);
	mSpotLight.posW      = lightPosW;
	mSpotLight.dirW      = lightDirW;

	/*printf("%f\n", lightPosW.x);
	printf("%f\n", lightPosW.y);
	printf("%f\n", lightPosW.z);*/
}

void Game::Draw()
{
	//pDevice->GetTransform(D3DTS_PROJECTION, m_ShadowTarget->getOldProjectionPointer());
	//pDevice->GetRenderTarget(0, m_ShadowTarget->getBackBufferPointer());

	//pDevice->SetRenderTarget(0, m_ShadowTarget->getRenderSurface());

	//pDevice->BeginScene();

	mShadowMap->beginScene();

	// Clear the backbuffer to a blue color
    pDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFFFFFFFF, 1.0f, 0 );

	mFX->SetTechnique(mhBuildShadowMapTech);
	UINT numberOfShadowPasses = 1;
	mFX->Begin(&numberOfShadowPasses, 0);
	mFX->BeginPass(0);

		mFX->SetMatrix(mhLightWVP, &(m_Dwarf->GetWorld() * m_LightViewProj));
		HR(mFX->CommitChanges());

		//m_Dwarf->DrawToShadowMap();

		mFX->SetMatrix(mhLightWVP, &(m_Citadel->GetWorld() * m_LightViewProj));
		HR(mFX->CommitChanges());

		m_Citadel->DrawToShadowMap();

	//End the pass
	mFX->EndPass();
	mFX->End();	

	UINT numOfPasses = 0;
		
		m_AnimatedInterface->GetEffect()->SetTechnique(m_AnimatedInterface->GetShadowTechnique());

		m_AnimatedInterface->GetEffect()->Begin(&numOfPasses, 0);
		m_AnimatedInterface->GetEffect()->BeginPass(0);		

		m_AnimatedInterface->UpdateShadowVariables(&(*m_SkinnedMesh->GetWorld() * m_LightViewProj),
			m_SkinnedMesh->getFinalXFormArray(), m_SkinnedMesh->numBones());

		m_SkinnedMesh->Draw();

		m_AnimatedInterface->GetEffect()->EndPass();
		m_AnimatedInterface->GetEffect()->End();

	mShadowMap->endScene();
	//pDevice->EndScene();

	//render scene with texture
	//set back buffer
	pDevice->SetRenderTarget(0, m_ShadowTarget->getBackBuffer());
	pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,255), 1.0f, 0);

	pDevice->GetTransform(D3DTS_PROJECTION, m_RenderTarget->getOldProjectionPointer());
	pDevice->GetRenderTarget(0, m_RenderTarget->getBackBufferPointer());

	pDevice->SetRenderTarget(0, m_RenderTarget->getRenderSurface());

	pDevice->BeginScene();

	// Clear the backbuffer to a blue color
    pDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(100, 149, 237), 1.0f, 0 );

	//Draw the scene
		mFX->SetTechnique(mhTech);
		//mFX->SetTexture(mhShadowMap, m_ShadowTarget->getRenderTexture());
		mFX->SetTexture(mhShadowMap, mShadowMap->d3dTex());
		UINT numPasses = 1;
		mFX->Begin(&numPasses, 0);
		mFX->BeginPass(0);

		mFX->SetMatrix(mhWVP, &(m_Dwarf->GetWorld() * matView * *m_RenderTarget->getProjectionPointer()));
		D3DXMATRIX DwarfWorldInverseTranspose;
		D3DXMatrixInverse(&DwarfWorldInverseTranspose, NULL, m_Dwarf->GetWorldPointer());
		D3DXMatrixTranspose(&DwarfWorldInverseTranspose, &DwarfWorldInverseTranspose);
		mFX->SetMatrix(mhWorldInvTrans, &DwarfWorldInverseTranspose);
		mFX->SetValue(mhEyePosW, m_Camera->getPosition(), sizeof(D3DXVECTOR3));
		mFX->SetMatrix(mhWorld, m_Dwarf->GetWorldPointer());
		mFX->SetMatrix(mhLightWVP, &(m_Dwarf->GetWorld() * m_LightViewProj));
		
		mFX->SetValue(mhMtrl, m_Dwarf->GetMaterial(), sizeof(Mtrl));
		mFX->SetValue(mhLight, &mSpotLight, sizeof(SpotLight));
		HR(mFX->CommitChanges());

		//m_Dwarf->Draw(mFX, mhTex);

		mFX->SetMatrix(mhWVP, &(m_Citadel->GetWorld() * matView * *m_RenderTarget->getProjectionPointer()));
		D3DXMATRIX CitadelWorldInverseTranspose;
		D3DXMatrixInverse(&CitadelWorldInverseTranspose, NULL, m_Citadel->GetWorldPointer());
		D3DXMatrixTranspose(&CitadelWorldInverseTranspose, &CitadelWorldInverseTranspose);
		mFX->SetMatrix(mhWorldInvTrans, &DwarfWorldInverseTranspose);
		mFX->SetValue(mhEyePosW, m_Camera->getPosition(), sizeof(D3DXVECTOR3));
		mFX->SetMatrix(mhWorld, m_Citadel->GetWorldPointer());
		mFX->SetMatrix(mhLightWVP, &(m_Citadel->GetWorld() * m_LightViewProj));
		//mFX->SetTexture(mhShadowMap, m_ShadowTarget->getRenderTexture());
		mFX->SetValue(mhMtrl, m_Citadel->GetMaterial(), sizeof(Mtrl));
		mFX->SetValue(mhLight, &mSpotLight, sizeof(SpotLight));
		HR(mFX->CommitChanges());

		m_Citadel->Draw(mFX, mhTex);

		mFX->EndPass();
		mFX->End();

		/*//Draw the scene
		m_PhongInterface->GetEffect()->SetTechnique(m_PhongInterface->GetTechnique());
		UINT numberOfPasses = 1;
		m_PhongInterface->GetEffect()->Begin(&numberOfPasses, 0);
		m_PhongInterface->GetEffect()->BeginPass(0);

		//////Update the world matrix for the object
		//m_Citadel->UpdateShaderVariables(&m_PhongContainer);
		//////Set the variables - This is essentially my version of CommitChanges()
		//SetPhongShaderVariables(m_Citadel->GetWorld());
		////Draw the model
		//m_Citadel->Draw(m_PhongInterface->GetEffect(), m_PhongInterface->GetTextureHandle());	

		////Update the world matrix for the object
		//m_Dwarf->UpdateShaderVariables(&m_PhongContainer);
		////Set the variables - This is essentially my version of CommitChanges()
		//SetPhongShaderVariables(m_Dwarf->GetWorld());
		//Draw the model
		//m_Dwarf->Draw(m_PhongInterface->GetEffect(), m_PhongInterface->GetTextureHandle());

		//End the pass
		m_PhongInterface->GetEffect()->EndPass();
		m_PhongInterface->GetEffect()->End();	*/

		numOfPasses = 0;
		
		m_AnimatedInterface->GetEffect()->SetTechnique(m_AnimatedInterface->GetTechnique());

		m_AnimatedInterface->GetEffect()->Begin(&numOfPasses, 0);
		m_AnimatedInterface->GetEffect()->BeginPass(0);		

		m_SkinnedMesh->UpdateShaderVariables(&m_AnimatedContainer);

		SetAnimatedInterfaceVariables();

		m_SkinnedMesh->Draw();

		m_AnimatedInterface->GetEffect()->EndPass();
		m_AnimatedInterface->GetEffect()->End();

	pDevice->EndScene();

	//render scene with texture
	//set back buffer
	pDevice->SetRenderTarget(0, m_RenderTarget->getBackBuffer());
	pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,255), 1.0f, 0);

	if( SUCCEEDED( pDevice->BeginScene() ) )
    {
		// Clear the backbuffer to a blue color
		pDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0 );

		m_RenderTarget->Draw();
		

		m_Font->Draw();	

		// End the scene
		pDevice->EndScene();
	}

    // Present the backbuffer contents to the display
    pDevice->Present( NULL, NULL, NULL, NULL );
}

void Game::Unload()
{
	m_Font->Release();

	m_PhongInterface->Release();
	m_AnimatedInterface->Release();
	m_Dwarf->Release();

	m_Citadel->Release();

	m_SkinnedMesh->Release();

	mShadowMap->onLostDevice();

	mFX->Release();
}

void Game::CalculateMatrices()
{
    // Set up our view matrix. A view matrix can be defined given an eye point,
    // a point to lookat, and a direction for which way is up. Here, we set the
    // eye five units back along the z-axis and up three units, look at the
    // origin, and define "up" to be in the y-direction.
    D3DXVECTOR3 vEyePt( -5.0f, 10.0f,-12.5f );
    D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );
	D3DXMATRIX mViewTransform; 
	D3DXMatrixRotationY(&mViewTransform, 0);
	D3DXVec3Transform(&vViewVector, &(vLookatPt - vEyePt), &mViewTransform );
	D3DXMatrixLookAtLH( &matView, m_Camera->getPosition(), m_Camera->getLookAt(), m_Camera->getUp() );
    //pDevice->SetTransform( D3DTS_VIEW, &matView );

    // For the projection matrix, we set up a perspective transform (which
    // transforms geometry from 3D view space to 2D viewport space, with
    // a perspective divide making objects smaller in the distance). To build
    // a perpsective transform, we need the field of view (1/4 pi is common),
    // the aspect ratio, and the near and far clipping planes (which define at
    // what distances geometry should be no longer be rendered).	

    //pDevice->SetTransform( D3DTS_PROJECTION, &matProj );
}

void Game::SetShaderVariables()
{
	////Update the view and projection matrices in the container
	//m_LightingContainer.matProj = matProj;
	//m_LightingContainer.matView = matView;	
	//
	////Update the view vector
	//m_LightingContainer.vViewVector = vViewVector;
	////Pass it in the lighting interface
	//m_LightingInterface->UpdateHandles(&m_LightingContainer);
}

void Game::SetPhongShaderVariables(D3DXMATRIX World)
{
	//Update the view and projection matrices in the container
	m_PhongContainer.m_WVP = World * matView * *m_RenderTarget->getProjectionPointer();
	m_PhongContainer.m_EyePosW = *m_Camera->getPosition();
	m_PhongContainer.m_Light = mLight;
	//m_PhongContainer.m_LightWVP = World * m_LightViewProj;
	m_PhongContainer.m_ShadowMap = m_ShadowTarget->getRenderTexture();	
	//m_PhongContainer.m_ShadowMap = m_WhiteTexture;	
	
	//Pass it in the lighting interface
	m_PhongInterface->UpdateHandles(&m_PhongContainer);
}

void Game::SetAnimatedInterfaceVariables()
{
	m_AnimatedContainer.m_EyePos = *m_Camera->getPosition();
	m_AnimatedContainer.m_WVP = *m_SkinnedMesh->GetWorld() * matView * *m_RenderTarget->getProjectionPointer();
	m_AnimatedContainer.m_ShadowMap = mShadowMap->d3dTex();

	m_AnimatedInterface->UpdateHandles(&m_AnimatedContainer, m_SkinnedMesh->getFinalXFormArray(), m_SkinnedMesh->numBones());
}

void Game::SetPacketVariables()
{
	
}

void Game::SendPacket()
{
	
}