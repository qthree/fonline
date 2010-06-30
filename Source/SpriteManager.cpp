#include "StdAfx.h"
#include "SpriteManager.h"
#include "Common.h"
#include "F2Palette.h"

#pragma MESSAGE("Add supporting of effects for sprites.")

#ifndef ZOOM
#define ZOOM 1
#endif

#define TEX_FRMT                  D3DFMT_A8R8G8B8
#define D3D_HR(expr)              {HRESULT hr__=expr; if(hr__!=D3D_OK){WriteLog(__FUNCTION__" - "#expr", error<%s>.\n",(char*)DXGetErrorString(hr__)); return 0;}}
#define SPR_BUFFER_COUNT          (10000)
#define SPRITES_POOL_GROW_SIZE    (10000)
#define SPRITES_RESIZE_COUNT      (100)

/************************************************************************/
/* Sprites                                                              */
/************************************************************************/

SpriteVec Sprites::spritesPool;

void Sprite::Unvalidate()
{
	if(Valid)
	{
		if(ValidCallback)
		{
			*ValidCallback=false;
			ValidCallback=NULL;
		}
		Valid=false;
	}
}

void Sprites::GrowPool(size_t size)
{
	spritesPool.reserve(spritesPool.size()+size);
	for(size_t i=0;i<size;i++) spritesPool.push_back(new Sprite());
}

void Sprites::ClearPool()
{
	for(SpriteVecIt it=spritesPool.begin(),end=spritesPool.end();it!=end;++it)
	{
		Sprite* spr=*it;
		spr->Unvalidate();
		delete spr;
	}
	spritesPool.clear();
}

Sprite& Sprites::PutSprite(size_t index, DWORD map_pos, int x, int y, DWORD id, DWORD* id_ptr, short* ox, short* oy, BYTE* alpha, BYTE* light, bool* callback)
{
	if(index>=spritesTreeSize)
	{
		spritesTreeSize=index+1;
		if(spritesTreeSize>=spritesTree.size()) Resize(spritesTreeSize+SPRITES_RESIZE_COUNT);
	}
	Sprite* spr=spritesTree[index];
	spr->MapPos=map_pos;
	spr->MapPosInd=index;
	spr->ScrX=x;
	spr->ScrY=y;
	spr->SprId=id;
	spr->PSprId=id_ptr;
	spr->OffsX=ox;
	spr->OffsY=oy;
	spr->Alpha=alpha;
	spr->Light=light;
	spr->Valid=true;
	spr->ValidCallback=callback;
	if(callback) *callback=true;
	spr->Egg=Sprite::EggNone;
	spr->Contour=Sprite::ContourNone;
	spr->ContourColor=0;
	spr->Color=0;
	spr->FlashMask=0;
	return *spr;
}

Sprite& Sprites::AddSprite(DWORD map_pos, int x, int y, DWORD id, DWORD* id_ptr, short* ox, short* oy, BYTE* alpha, BYTE* light, bool* callback)
{
	return PutSprite(spritesTreeSize,map_pos,x,y,id,id_ptr,ox,oy,alpha,light,callback);
}

Sprite& Sprites::InsertSprite(DWORD map_pos, int x, int y, DWORD id, DWORD* id_ptr, short* ox, short* oy, BYTE* alpha, BYTE* light, bool* callback)
{
	size_t index=0;
	for(SpriteVecIt it=spritesTree.begin(),end=spritesTree.begin()+spritesTreeSize;it!=end;++it)
	{
		Sprite* spr=*it;
		if(spr->MapPos>map_pos) break;
		index++;
	}
	spritesTreeSize++;
	if(spritesTreeSize>=spritesTree.size()) Resize(spritesTreeSize+SPRITES_RESIZE_COUNT);
	if(index<spritesTreeSize-1)
	{
		spritesTree.insert(spritesTree.begin()+index,spritesTree.back());
		spritesTree.pop_back();
	}
	return PutSprite(index,map_pos,x,y,id,id_ptr,ox,oy,alpha,light,callback);
}

void Sprites::Resize(size_t size)
{
	size_t tree_size=spritesTree.size();
	size_t pool_size=spritesPool.size();
	if(size>tree_size) // Get from pool
	{
		size_t diff=size-tree_size;
		if(diff>pool_size) GrowPool(diff>SPRITES_POOL_GROW_SIZE?diff:SPRITES_POOL_GROW_SIZE);
		spritesTree.reserve(tree_size+diff);
		//spritesTree.insert(spritesTree.end(),spritesPool.rbegin(),spritesPool.rbegin()+diff);
		//spritesPool.erase(spritesPool.begin()+tree_size-diff,spritesPool.end());
		for(size_t i=0;i<diff;i++)
		{
			spritesTree.push_back(spritesPool.back());
			spritesPool.pop_back();
		}
	}
	else if(size<tree_size) // Put in pool
	{
		size_t diff=tree_size-size;
		if(diff>tree_size-spritesTreeSize) spritesTreeSize-=diff-(tree_size-spritesTreeSize);

		// Unvalidate putted sprites
		for(SpriteVec::reverse_iterator it=spritesTree.rbegin(),end=spritesTree.rbegin()+diff;it!=end;++it)
			(*it)->Unvalidate();

		// Put
		spritesPool.reserve(pool_size+diff);
		//spritesPool.insert(spritesPool.end(),spritesTree.rbegin(),spritesTree.rbegin()+diff);
		//spritesTree.erase(spritesTree.begin()+tree_size-diff,spritesTree.end());
		for(size_t i=0;i<diff;i++)
		{
			spritesPool.push_back(spritesTree.back());
			spritesTree.pop_back();
		}
	}
}

void Sprites::Unvalidate()
{
	for(SpriteVecIt it=spritesTree.begin(),end=spritesTree.begin()+spritesTreeSize;it!=end;++it)
		(*it)->Unvalidate();
	spritesTreeSize=0;
}

SprInfoVec* SortSpritesSurfSprData=NULL;
bool SortSpritesSurf(Sprite* spr1, Sprite* spr2)
{
	SpriteInfo* si1=(*SortSpritesSurfSprData)[spr1->PSprId?*spr1->PSprId:spr1->SprId];
	SpriteInfo* si2=(*SortSpritesSurfSprData)[spr2->PSprId?*spr2->PSprId:spr2->SprId];
	return si1->Surf && si2->Surf && si1->Surf->Texture<si2->Surf->Texture;
}
void Sprites::SortBySurfaces()
{
	std::sort(spritesTree.begin(),spritesTree.begin()+spritesTreeSize,SortSpritesSurf);
}

bool SortSpritesMapPos(Sprite* spr1, Sprite* spr2)
{
	if(spr1->MapPos==spr2->MapPos) return spr1->MapPosInd<spr2->MapPosInd;
	return spr1->MapPos<spr2->MapPos;
}
void Sprites::SortByMapPos()
{
	std::sort(spritesTree.begin(),spritesTree.begin()+spritesTreeSize,SortSpritesMapPos);
}

/************************************************************************/
/* Sprite manager                                                       */
/************************************************************************/

SpriteManager::SpriteManager(): isInit(0),flushSprCnt(0),curSprCnt(0),hWnd(NULL),direct3D(NULL),SurfType(0),
dxDevice(NULL),pVB(NULL),pIB(NULL),waitBuf(NULL),curTexture(NULL),PreRestore(NULL),PostRestore(NULL),
drawOffsetX(NULL),drawOffsetY(NULL),baseTexture(0),
eggSurfWidth(1.0f),eggSurfHeight(1.0f),eggSprWidth(1),eggSprHeight(1),
contoursTexture(NULL),contoursTextureSurf(NULL),contours3dSurf(NULL),contoursMidTexture(NULL),contoursMidTextureSurf(NULL),
contoursPS(NULL),contoursCT(NULL),contoursAdded(false),
modeWidth(0),modeHeight(0)
{
	//ZeroMemory(&displayMode,sizeof(displayMode));
	ZeroMemory(&presentParams,sizeof(presentParams));
	ZeroMemory(&mngrParams,sizeof(mngrParams));
	ZeroMemory(&deviceCaps,sizeof(deviceCaps));
	baseColor=D3DCOLOR_ARGB(255,128,128,128);
	surfList.reserve(100);
	SortSpritesSurfSprData=&sprData; // For sprites sorting
}

bool SpriteManager::Init(SpriteMngrParams& params)
{
	if(isInit) return false;
	WriteLog("Sprite manager initialization...\n");

	mngrParams=params;
	flushSprCnt=params.SprFlushVal;
	baseTexture=params.BaseTexture;
	curSprCnt=0;
	PreRestore=params.PreRestoreFunc;
	PostRestore=params.PostRestoreFunc;
	drawOffsetX=params.DrawOffsetX;
	drawOffsetY=params.DrawOffsetY;
	modeWidth=params.ScreenWidth;
	modeHeight=params.ScreenHeight;

	direct3D=Direct3DCreate(D3D_SDK_VERSION);
	if(!direct3D)
	{
		WriteLog("Create Direct3D fail.\n");
		return false;
	}

	//ZeroMemory(&displayMode,sizeof(displayMode));
	//D3D_HR(direct3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT,&displayMode));
	ZeroMemory(&deviceCaps,sizeof(deviceCaps));
	D3D_HR(direct3D->GetDeviceCaps(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,&deviceCaps));

	ZeroMemory(&presentParams,sizeof(presentParams));
	presentParams.BackBufferCount=1;
	presentParams.Windowed=(params.FullScreen?FALSE:TRUE);
	presentParams.SwapEffect=D3DSWAPEFFECT_DISCARD;
	presentParams.EnableAutoDepthStencil=TRUE;
	presentParams.AutoDepthStencilFormat=D3DFMT_D24S8;
	presentParams.hDeviceWindow=params.WndHeader;
	presentParams.BackBufferWidth=params.ScreenWidth;
	presentParams.BackBufferHeight=params.ScreenHeight;
	presentParams.BackBufferFormat=D3DFMT_X8R8G8B8;

#ifdef DX8RENDER
	if(!params.VSync) presentParams.FullScreen_PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
#elif DX9RENDER
	if(!params.VSync) presentParams.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
#endif

	presentParams.MultiSampleType=(D3DMULTISAMPLE_TYPE)params.MultiSampling;
	if(params.MultiSampling<0)
	{
		presentParams.MultiSampleType=D3DMULTISAMPLE_NONE;
		for(int i=4;i>=1;i--)
		{
			if(SUCCEEDED(direct3D->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,
				presentParams.BackBufferFormat,!params.FullScreen,(D3DMULTISAMPLE_TYPE)i,NULL)))
			{
				presentParams.MultiSampleType=(D3DMULTISAMPLE_TYPE)i;
				break;
			}
		}
	}
	if(presentParams.MultiSampleType!=D3DMULTISAMPLE_NONE)
	{
		HRESULT hr=direct3D->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,presentParams.BackBufferFormat,!params.FullScreen,
			presentParams.MultiSampleType,&presentParams.MultiSampleQuality);
		if(FAILED(hr))
		{
			WriteLog("Multisampling %dx not supported. Disabled.\n",(int)presentParams.MultiSampleType);
			presentParams.MultiSampleType=D3DMULTISAMPLE_NONE;
			presentParams.MultiSampleQuality=0;
		}
		if(presentParams.MultiSampleQuality) presentParams.MultiSampleQuality--;
	}

	int vproc=(!params.SoftwareSkinning && deviceCaps.DevCaps&D3DDEVCAPS_HWTRANSFORMANDLIGHT && deviceCaps.VertexShaderVersion>=D3DPS_VERSION(1,1) &&
		deviceCaps.MaxVertexBlendMatrices>=2?D3DCREATE_HARDWARE_VERTEXPROCESSING:D3DCREATE_SOFTWARE_VERTEXPROCESSING);

	D3D_HR(direct3D->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,params.WndHeader,vproc,&presentParams,&dxDevice));

	// Contours
	if(deviceCaps.PixelShaderVersion>=D3DPS_VERSION(2,0))
	{
		// Contours shader
		ID3DXBuffer* shader=NULL,*errors=NULL;
		HRESULT hr=D3DXCompileShaderFromResource(NULL,MAKEINTRESOURCE(IDR_PS_CONTOUR),NULL,NULL,"Main","ps_2_0",0,&shader,&errors,&contoursCT);
		if(SUCCEEDED(hr))
		{
			hr=dxDevice->CreatePixelShader((DWORD*)shader->GetBufferPointer(),&contoursPS);
			shader->Release();
			if(FAILED(hr))
			{
				WriteLog(__FUNCTION__" - Can't create contours pixel shader, error<%s>. Used old style contours.\n",DXGetErrorString(hr));
				contoursPS=NULL;
			}
		}
		else
		{
			if(errors) WriteLog(__FUNCTION__" - Shader 2d contours compilation messages:\n<\n%s>\n",errors->GetBufferPointer());
			WriteLog(__FUNCTION__" - Shader 2d contours compilation fail, error<%s>. Used old style contours.\n",DXGetErrorString(hr));
		}
		SAFEREL(errors);
	}

	if(!Animation3d::StartUp(dxDevice,params.SoftwareSkinning)) return false;
	if(!InitRenderStates()) return false;
	if(!InitBuffers()) return false;

	// Sprites buffer
	sprData.resize(SPR_BUFFER_COUNT);
	for(SprInfoVecIt it=sprData.begin(),end=sprData.end();it!=end;++it) (*it)=NULL;

	// Transparent egg
	isInit=true;
	eggValid=false;
	eggX=0;
	eggY=0;

	if(!LoadSprite("egg.png",PT_ART_MISC,&sprEgg))
	{
		WriteLog("Load sprite egg fail.\n");
		isInit=false;
		return false;
	}

	eggSurfWidth=(float)surfList[0]->Width;
	eggSurfHeight=(float)surfList[0]->Height;
	eggSprWidth=sprEgg->Width;
	eggSprHeight=sprEgg->Height;

	D3D_HR(dxDevice->SetTexture(1,sprEgg->Surf->Texture));
	D3D_HR(dxDevice->Clear(0,NULL,D3DCLEAR_TARGET,D3DCOLOR_XRGB(0,0,0),1.0f,0));
	WriteLog("Sprite manager initialization complete.\n");
	return true;
}

bool SpriteManager::InitBuffers()
{
	SAFEDELA(waitBuf);
	SAFEREL(pVB);
	SAFEREL(pIB);
	SAFEREL(contoursTexture);
	SAFEREL(contoursTextureSurf);
	SAFEREL(contoursMidTexture);
	SAFEREL(contoursMidTextureSurf);
	SAFEREL(contours3dSurf);

	// Vertex buffer
#ifdef DX8RENDER
	D3D_HR(dxDevice->CreateVertexBuffer(flushSprCnt*4*sizeof(MYVERTEX),D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC,D3DFVF_MYVERTEX,D3DPOOL_DEFAULT,&pVB));
#elif DX9RENDER
	D3D_HR(dxDevice->CreateVertexBuffer(flushSprCnt*4*sizeof(MYVERTEX),D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC,D3DFVF_MYVERTEX,D3DPOOL_DEFAULT,&pVB,NULL));
#endif

	// Index buffer
#ifdef DX8RENDER
	D3D_HR(dxDevice->CreateIndexBuffer(flushSprCnt*6*sizeof(WORD),D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&pIB));
#elif DX9RENDER
	D3D_HR(dxDevice->CreateIndexBuffer(flushSprCnt*6*sizeof(WORD),D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&pIB,NULL));
#endif

	WORD* ind=new WORD[6*flushSprCnt];
	if(!ind) return false;
	for(int i=0;i<flushSprCnt;i++)
	{
		ind[6*i+0]=4*i+0;
		ind[6*i+1]=4*i+1;
		ind[6*i+2]=4*i+3;
		ind[6*i+3]=4*i+1;
		ind[6*i+4]=4*i+2;
		ind[6*i+5]=4*i+3;
	}

	void* buf;
#ifdef DX8RENDER
	D3D_HR(pIB->Lock(0,0,(BYTE**)&buf,0));
#elif DX9RENDER
	D3D_HR(pIB->Lock(0,0,(void**)&buf,0));
#endif
	memcpy(buf,ind,flushSprCnt*6*sizeof(WORD));
	D3D_HR(pIB->Unlock());
	delete[] ind;

#ifdef DX8RENDER
	D3D_HR(dxDevice->SetIndices(pIB,0));
	D3D_HR(dxDevice->SetStreamSource(0,pVB,sizeof(MYVERTEX)));
	D3D_HR(dxDevice->SetVertexShader(D3DFVF_MYVERTEX));
#elif DX9RENDER
	D3D_HR(dxDevice->SetIndices(pIB));
	D3D_HR(dxDevice->SetStreamSource(0,pVB,0,sizeof(MYVERTEX)));
	D3D_HR(dxDevice->SetVertexShader(NULL));
	D3D_HR(dxDevice->SetFVF(D3DFVF_MYVERTEX));
#endif

	waitBuf=new MYVERTEX[flushSprCnt*4];
	if(!waitBuf) return false;

	if(contoursPS)
	{
		// Contours render target
		D3D_HR(direct3D->CheckDepthStencilMatch(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,D3DFMT_X8R8G8B8,D3DFMT_A8R8G8B8,D3DFMT_D24S8));
		D3D_HR(dxDevice->CreateRenderTarget(modeWidth,modeHeight,D3DFMT_A8R8G8B8,presentParams.MultiSampleType,presentParams.MultiSampleQuality,FALSE,&contours3dSurf,NULL));
		D3D_HR(D3DXCreateTexture(dxDevice,modeWidth,modeHeight,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&contoursTexture));
		D3D_HR(contoursTexture->GetSurfaceLevel(0,&contoursTextureSurf));
		D3D_HR(D3DXCreateTexture(dxDevice,modeWidth,modeHeight,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&contoursMidTexture));
		D3D_HR(contoursMidTexture->GetSurfaceLevel(0,&contoursMidTextureSurf));
	}
	return true;
}

bool SpriteManager::InitRenderStates()
{
	D3D_HR(dxDevice->SetRenderState(D3DRS_LIGHTING,FALSE));
	D3D_HR(dxDevice->SetRenderState(D3DRS_ZENABLE,FALSE));
	D3D_HR(dxDevice->SetRenderState(D3DRS_STENCILENABLE,FALSE));
	D3D_HR(dxDevice->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE));
	D3D_HR(dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE));
	D3D_HR(dxDevice->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA));
	D3D_HR(dxDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA));

//	D3D_HR(dxDevice->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE));
//	D3D_HR(dxDevice->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATEREQUAL));
//	D3D_HR(dxDevice->SetRenderState(D3DRS_ALPHAREF,100));

#ifdef DX8RENDER
	D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_MIPFILTER,D3DTEXF_NONE));
	D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_MINFILTER,D3DTEXF_LINEAR)); // Zoom Out
	D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_MAGFILTER,D3DTEXF_LINEAR)); // Zoom In
#elif DX9RENDER
	D3D_HR(dxDevice->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_NONE));
	D3D_HR(dxDevice->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR)); // Zoom Out
	D3D_HR(dxDevice->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR)); // Zoom In
#endif

	D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_COLOROP,  D3DTOP_MODULATE2X));
	D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE));
	D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE));
	D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_ALPHAOP , D3DTOP_MODULATE));
	D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE));
	D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE));
	D3D_HR(dxDevice->SetTextureStageState(1,D3DTSS_COLOROP , D3DTOP_SELECTARG2));
	D3D_HR(dxDevice->SetTextureStageState(1,D3DTSS_COLORARG1,D3DTA_TEXTURE));
	D3D_HR(dxDevice->SetTextureStageState(1,D3DTSS_COLORARG2,D3DTA_CURRENT));
	D3D_HR(dxDevice->SetTextureStageState(1,D3DTSS_ALPHAOP , D3DTOP_MODULATE));
	D3D_HR(dxDevice->SetTextureStageState(1,D3DTSS_ALPHAARG1,D3DTA_TEXTURE));
	D3D_HR(dxDevice->SetTextureStageState(1,D3DTSS_ALPHAARG2,D3DTA_CURRENT));

	D3D_HR(dxDevice->SetRenderState(D3DRS_LIGHTING,TRUE));
	D3D_HR(dxDevice->SetRenderState(D3DRS_DITHERENABLE,TRUE));
	D3D_HR(dxDevice->SetRenderState(D3DRS_SPECULARENABLE,FALSE));
	//D3D_HR(dxDevice->SetRenderState(D3DRS_CULLMODE,D3DCULL_CCW));
	D3D_HR(dxDevice->SetRenderState(D3DRS_AMBIENT,D3DCOLOR_XRGB(80,80,80)));
	D3D_HR(dxDevice->SetRenderState(D3DRS_NORMALIZENORMALS,TRUE));
	/*D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_COLOROP,  D3DTOP_MODULATE));
	D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE));
	D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_CURRENT));
	D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_ALPHAOP,  D3DTOP_MODULATE));
	D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE));
	D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE));
	D3D_HR(dxDevice->SetTextureStageState(1,D3DTSS_COLOROP , D3DTOP_DISABLE));*/

	// Stencil
	/*	D3D_HR(dxDevice->SetRenderState(D3DRS_STENCILFUNC,D3DCMP_ALWAYS));
	D3D_HR(dxDevice->SetRenderState(D3DRS_STENCILMASK,0xFFFFFFFF));
	D3D_HR(dxDevice->SetRenderState(D3DRS_STENCILWRITEMASK,0xFFFFFFFF));
	D3D_HR(dxDevice->SetRenderState(D3DRS_STENCILZFAIL,D3DSTENCILOP_KEEP));
	D3D_HR(dxDevice->SetRenderState(D3DRS_STENCILFAIL,D3DSTENCILOP_KEEP));
	D3D_HR(dxDevice->SetRenderState(D3DRS_STENCILPASS,D3DSTENCILOP_REPLACE));*/
	return true;
}

void SpriteManager::Clear()
{
	WriteLog("Sprite manager finish...\n");

	for(SurfVecIt it=surfList.begin(),end=surfList.end();it!=end;++it) SAFEDEL(*it);
	surfList.clear();
	for(SprInfoVecIt it=sprData.begin(),end=sprData.end();it!=end;++it) SAFEDEL(*it);
	sprData.clear();
	for(OneSurfVecIt it=callVec.begin(),end=callVec.end();it!=end;++it) SAFEDEL(*it);
	callVec.clear();

	Animation3d::Finish();
	SAFEREL(pVB);
	SAFEREL(pIB);
	SAFEDELA(waitBuf);
	SAFEREL(dxDevice);
	SAFEREL(contours3dSurf);
	SAFEREL(contoursTextureSurf);
	SAFEREL(contoursTexture);
	SAFEREL(contoursMidTextureSurf);
	SAFEREL(contoursMidTexture);
	SAFEREL(contoursCT);
	SAFEREL(contoursPS);
	SAFEREL(direct3D);

	isInit=false;
	WriteLog("Sprite manager finish complete.\n");
}

bool SpriteManager::Restore()
{
	if(!isInit) return false;

	// Release resources
	SAFEREL(pVB);
	SAFEREL(pIB);
	SAFEREL(contoursTexture);
	SAFEREL(contoursTextureSurf);
	SAFEREL(contoursMidTexture);
	SAFEREL(contoursMidTextureSurf);
	SAFEREL(contours3dSurf);
	Animation3d::PreRestore();
	if(PreRestore) (*PreRestore)();

	// Reset device
	D3D_HR(dxDevice->Reset(&presentParams));
	D3D_HR(dxDevice->Clear(0,NULL,D3DCLEAR_TARGET,D3DCOLOR_XRGB(0,0,0),1.0f,0));

	// Create resources
	if(!InitRenderStates()) return false;
	if(!InitBuffers()) return false;
	D3D_HR(dxDevice->SetTexture(1,sprEgg->Surf->Texture)); // Transparent egg
	if(PostRestore) (*PostRestore)();
	if(!Animation3d::StartUp(dxDevice,mngrParams.SoftwareSkinning)) return false;

	return true;
}

bool SpriteManager::BeginScene(DWORD clear_color)
{
	HRESULT hr=dxDevice->TestCooperativeLevel();
	if(hr!=D3D_OK && (hr!=D3DERR_DEVICENOTRESET || !Restore())) return false;

	if(clear_color) D3D_HR(dxDevice->Clear(0,NULL,D3DCLEAR_TARGET,clear_color,1.0f,0));
	D3D_HR(dxDevice->BeginScene());
	Animation3d::BeginScene();
	return true;
}

void SpriteManager::EndScene()
{
	Flush();
	dxDevice->EndScene();
	dxDevice->Present(NULL,NULL,NULL,NULL);
}

#define SURF_SPRITES_OFFS        (1)
Surface* SpriteManager::CreateNewSurf(WORD w, WORD h)
{
	if(!isInit) return NULL;

	// Check power of two
	int ww=w+SURF_SPRITES_OFFS*2;
	w=baseTexture;
	int hh=h+SURF_SPRITES_OFFS*2;
	h=baseTexture;
	while(w<ww) w*=2;
	while(h<hh) h*=2;

	LPDIRECT3DTEXTURE tex=NULL;
#ifdef DX8RENDER
	D3D_HR(dxDevice->CreateTexture(w,h,1,0,TEX_FRMT,D3DPOOL_MANAGED,&tex));
#elif DX9RENDER
	D3D_HR(dxDevice->CreateTexture(w,h,1,0,TEX_FRMT,D3DPOOL_MANAGED,&tex,NULL));
#endif

	Surface* surf=new Surface();
	surf->Type=SurfType;
	surf->Texture=tex;
	surf->Width=w;
	surf->Height=h;
	surf->BusyH=(surfList.empty()?0:SURF_SPRITES_OFFS);
	surf->FreeX=(surfList.empty()?0:SURF_SPRITES_OFFS);
	surf->FreeY=(surfList.empty()?0:SURF_SPRITES_OFFS);
	surfList.push_back(surf);
	return surf;
}

Surface* SpriteManager::FindSurfacePlace(SpriteInfo* si, int& x, int& y)
{
	WORD w=si->Width;
	WORD h=si->Height;

	// Find already created surface, from end
	for(SurfVecIt it=surfList.begin(),end=surfList.end();it!=end;++it)
	{
		Surface* surf=*it;
		if(surf->Type==SurfType)
		{
			if(surf->Width-surf->FreeX>=w+SURF_SPRITES_OFFS && surf->Height-surf->FreeY>=h+SURF_SPRITES_OFFS)
			{
				x=surf->FreeX+SURF_SPRITES_OFFS;
				y=surf->FreeY;
			}
			else if(surf->Width>=w+SURF_SPRITES_OFFS && surf->Height-surf->BusyH>=h+SURF_SPRITES_OFFS)
			{
				x=SURF_SPRITES_OFFS;
				y=surf->BusyH+SURF_SPRITES_OFFS;
			}
			else continue;
			return surf;
		}
	}

	// Create new
	Surface* surf=CreateNewSurf(w,h);
	if(!surf) return NULL;
	x=surf->FreeX;
	y=surf->FreeY;
	return surf;
}

void SpriteManager::FreeSurfaces(int surf_type)
{
	for(SurfVecIt it=surfList.begin();it!=surfList.end();)
	{
		Surface* surf=*it;
		if(surf->Type==surf_type)
		{
			for(SprInfoVecIt it_=sprData.begin(),end_=sprData.end();it_!=end_;++it_)
			{
				SpriteInfo* si=*it_;
				if(si && si->Surf==surf)
				{
					delete si;
					(*it_)=NULL;
				}
			}

			delete surf;
			it=surfList.erase(it);
		}
		else ++it;
	}
}

void SpriteManager::SaveSufaces()
{
	static int folder_cnt=0;
	static int rnd_num=0;
	if(!rnd_num) rnd_num=Random(1000,9999);

	int surf_size=0;
	for(SurfVecIt it=surfList.begin(),end=surfList.end();it!=end;++it)
	{
		Surface* surf=*it;
		surf_size+=surf->Width*surf->Height*4;
	}

	char path[256];
	sprintf(path,".\\%d_%03d_%d.%03dmb\\",rnd_num,folder_cnt,surf_size/1000000,surf_size%1000000/1000);
	CreateDirectory(path,NULL);

	int cnt=0;
	char name[256];
	for(SurfVecIt it=surfList.begin(),end=surfList.end();it!=end;++it)
	{
		Surface* surf=*it;
		LPDIRECT3DSURFACE s;
		surf->Texture->GetSurfaceLevel(0,&s);
		sprintf(name,"%s%d_%d_%ux%u.",path,surf->Type,cnt,surf->Width,surf->Height);
#ifdef DX8RENDER
		StringAppend(name,"bmp");
		D3DXSaveSurfaceToFile(name,D3DXIFF_BMP,s,NULL,NULL);
#elif DX9RENDER
		StringAppend(name,"png");
		D3DXSaveSurfaceToFile(name,D3DXIFF_PNG,s,NULL,NULL);
#endif
		s->Release();
		cnt++;
	}

	folder_cnt++;
}

DWORD SpriteManager::FillSurfaceFromMemory(SpriteInfo* si, void* data, DWORD size)
{
	// Parameters
	DWORD w,h;
	bool fast=(*(DWORD*)data==MAKEFOURCC('F','0','F','A'));
	if(!si) si=new SpriteInfo();

	// Get width, height
	// FOnline fast format
	if(fast)
	{
		w=*((DWORD*)data+1);
		h=*((DWORD*)data+2);
	}
	// From file in memory
	else
	{
		D3DXIMAGE_INFO img;
		D3D_HR(D3DXGetImageInfoFromFileInMemory(data,size,&img));
		w=img.Width;
		h=img.Height;
	}

	// Find place on surface
	si->Width=w;
	si->Height=h;
	int x,y;
	Surface* surf=FindSurfacePlace(si,x,y);
	if(!surf) return 0;

	LPDIRECT3DSURFACE dst_surf;
	D3D_HR(surf->Texture->GetSurfaceLevel(0,&dst_surf));

	// Copy
	// FOnline fast format
	if(fast)
	{
		D3DLOCKED_RECT rdst;
		RECT r={x,y,x+w,y+h};
		D3D_HR(dst_surf->LockRect(&rdst,&r,0)); // D3DLOCK_DISCARD
		BYTE* ptr=(BYTE*)((DWORD*)data+3);
		for(int i=0;i<h;i++) memcpy((BYTE*)rdst.pBits+rdst.Pitch*i,ptr+w*4*i,w*4);
		dst_surf->UnlockRect();
	}
	// From file in memory
	else
	{
		// Try load image
		LPDIRECT3DSURFACE src_surf;
#ifdef DX8RENDER
		D3D_HR(dxDevice->CreateImageSurface(w,h,TEX_FRMT,&src_surf));
#elif DX9RENDER
		D3D_HR(dxDevice->CreateOffscreenPlainSurface(w,h,TEX_FRMT,D3DPOOL_SCRATCH,&src_surf,NULL));
#endif

		D3D_HR(D3DXLoadSurfaceFromFileInMemory(src_surf,NULL,NULL,data,size,NULL,D3DX_FILTER_NONE,D3DCOLOR_XRGB(0,0,0xFF),NULL)); //D3DX_DEFAULT need???
		RECT src_r={0,0,w,h};
		D3DLOCKED_RECT rsrc,rdst;
		RECT dest_r={x,y,x+w,y+h};
		D3D_HR(src_surf->LockRect(&rsrc,&src_r,D3DLOCK_READONLY));
		D3D_HR(dst_surf->LockRect(&rdst,&dest_r,0)); // D3DLOCK_DISCARD
		for(int i=0;i<h;i++) memcpy((BYTE*)rdst.pBits+rdst.Pitch*i,(BYTE*)rsrc.pBits+rsrc.Pitch*i,w*4);
		D3D_HR(src_surf->UnlockRect());
		D3D_HR(dst_surf->UnlockRect());
		src_surf->Release();
	}
	dst_surf->Release();

	// Set parameters
	si->Surf=surf;
	surf->FreeX=x+w;
	surf->FreeY=y;
	if(y+h>surf->BusyH) surf->BusyH=y+h;
	si->SprRect.L=float(x)/float(surf->Width);
	si->SprRect.T=float(y)/float(surf->Height);
	si->SprRect.R=float(x+w)/float(surf->Width);
	si->SprRect.B=float(y+h)/float(surf->Height);

	// Store sprite
	size_t index=1;
	for(size_t j=sprData.size();index<j;index++) if(!sprData[index]) break;
	if(index<sprData.size()) sprData[index]=si;
	else sprData.push_back(si);
	return index;
}

DWORD SpriteManager::ReloadSprite(DWORD spr_id, const char* fname, int path_type)
{
	if(!isInit) return spr_id;
	if(!fname || !fname[0]) return spr_id;

	SpriteInfo* si=(spr_id?GetSpriteInfo(spr_id):NULL);
	if(!si)
	{
		spr_id=LoadSprite(fname,path_type,NULL);
	}
	else
	{
		for(SurfVecIt it=surfList.begin(),end=surfList.end();it!=end;++it)
		{
			Surface* surf=*it;
			if(si->Surf==surf)
			{
				delete surf;
				surfList.erase(it);
				SAFEDEL(sprData[spr_id]);
				spr_id=LoadSprite(fname,path_type,NULL);
				break;
			}
		}
	}

	return spr_id;
}

DWORD SpriteManager::LoadSprite(const char* fname, int path_type, SpriteInfo** ppInfo)
{
	if(!isInit) return 0;
	if(!fname || !fname[0]) return 0;

	if(!fileMngr.LoadFile(fname,path_type)) return 0;

	const char* ext=FileManager::GetExtension(fname);
	if(!ext)
	{
		fileMngr.UnloadFile();
		WriteLog(__FUNCTION__" - Extension not found, file<%s>.\n",fname);
		return 0;
	}

	if(_stricmp(ext,"frm") && _stricmp(ext,"fr0") && _stricmp(ext,"fr1") && _stricmp(ext,"fr2") &&
		_stricmp(ext,"fr3") && _stricmp(ext,"fr4") && _stricmp(ext,"fr5")) return LoadSpriteAlt(fname,path_type,ppInfo);

	SpriteInfo* lpinf=new SpriteInfo;

	short offs_x, offs_y;
	fileMngr.SetCurPos(0xA);
	offs_x=fileMngr.GetBEWord();
	fileMngr.SetCurPos(0x16);
	offs_y=fileMngr.GetBEWord();

	lpinf->OffsX=offs_x;
	lpinf->OffsY=offs_y;

	fileMngr.SetCurPos(0x3E);
	WORD w=fileMngr.GetBEWord();
	WORD h=fileMngr.GetBEWord();

	// Create FOnline fast format
	DWORD size=12+h*w*4;
	BYTE* data=new BYTE[size];
	*((DWORD*)data)=MAKEFOURCC('F','0','F','A'); //FOnline FAst
	*((DWORD*)data+1)=w;
	*((DWORD*)data+2)=h;
	DWORD* ptr=(DWORD*)data+3;
	DWORD* palette=(DWORD*)FoPalette;
	fileMngr.SetCurPos(0x4A);
	for(int i=0,j=w*h;i<j;i++) *(ptr+i)=palette[fileMngr.GetByte()];

	// Fill
	DWORD result=FillSurfaceFromMemory(lpinf,data,size);
	delete[] data;
	fileMngr.UnloadFile();
	if(ppInfo && result) (*ppInfo)=lpinf;
	return result;
}

DWORD SpriteManager::LoadSpriteAlt(const char* fname, int path_type, SpriteInfo** ppInfo)
{
	const char* ext=FileManager::GetExtension(fname);
	if(!ext)
	{
		fileMngr.UnloadFile();
		WriteLog(__FUNCTION__" - Unknown extension of file <%s>.\n",fname);
		return 0;
	}

	if(!_stricmp(ext,"fofrm"))
	{
		char fname2[128];
		IniParser fofrm;
		fofrm.LoadFile(fileMngr.GetBuf(),fileMngr.GetFsize());
		bool is_frm=(fofrm.GetStr("frm","",fname2) || fofrm.GetStr("frm_0","",fname2) ||	fofrm.GetStr("dir_0","frm_0","",fname2));
		if(!is_frm) return 0;
		short ox=fofrm.GetInt("offs_x",0);
		short oy=fofrm.GetInt("offs_y",0);
		DWORD spr_id=LoadSprite(fname2,path_type,ppInfo);
		if(!spr_id) return 0;
		SpriteInfo* si=GetSpriteInfo(spr_id);
		si->OffsX+=ox;
		si->OffsY+=oy;
		return spr_id;
	}
	else if(!_stricmp(ext,"rix"))
	{
		return LoadRix(fname,path_type);
	}

	// Dx8, Dx9: .bmp, .dds, .dib, .hdr, .jpg, .pfm, .png, .ppm, .tga
	SpriteInfo* lpinf=new SpriteInfo;
	lpinf->OffsX=0;
	lpinf->OffsY=0;

	DWORD result=FillSurfaceFromMemory(lpinf,fileMngr.GetBuf(),fileMngr.GetFsize());
	fileMngr.UnloadFile();
	if(ppInfo && result) (*ppInfo)=lpinf;
	return result;
}

DWORD SpriteManager::LoadRix(const char *fname, int path_type)
{
	if(!isInit) return 0;
	if(!fname || !fname[0]) return 0;
	if(!fileMngr.LoadFile(fname,path_type)) return 0;

	SpriteInfo* lpinf=new SpriteInfo;
	fileMngr.SetCurPos(0x4);
	WORD w;fileMngr.CopyMem(&w,2);
	WORD h;fileMngr.CopyMem(&h,2);

	// Create FOnline fast format
	DWORD size=12+h*w*4;
	BYTE* data=new BYTE[size];
	*((DWORD*)data)=MAKEFOURCC('F','0','F','A'); //FOnline FAst
	*((DWORD*)data+1)=w;
	*((DWORD*)data+2)=h;
	DWORD* ptr=(DWORD*)data+3;
	fileMngr.SetCurPos(0xA);
	BYTE* palette=fileMngr.GetCurBuf();
	fileMngr.SetCurPos(0xA+256*3);
	for(int i=0,j=w*h;i<j;i++)
	{
		DWORD index=fileMngr.GetByte();
		DWORD r=*(palette+index*3+0)*4;
		DWORD g=*(palette+index*3+1)*4;
		DWORD b=*(palette+index*3+2)*4;
		*(ptr+i)=D3DCOLOR_XRGB(r,g,b);
	}

	DWORD result=FillSurfaceFromMemory(lpinf,data,size);
	delete[] data;
	fileMngr.UnloadFile();
	return result;
}

AnyFrames* SpriteManager::LoadAnyAnimation(const char* fname, int path_type, bool anim_pix, int dir)
{
	if(!isInit || !fname || !fname[0]) return NULL;

	const char* ext=FileManager::GetExtension(fname);
	if(!ext)
	{
		WriteLog(__FUNCTION__" - Extension not found, file<%s>.\n",fname);
		return NULL;
	}

	if(!_stricmp(ext,"fofrm")) return LoadAnyAnimationFofrm(fname,path_type,dir);
	else if(_stricmp(ext,"frm") && _stricmp(ext,"fr0") && _stricmp(ext,"fr1") && _stricmp(ext,"fr2") &&
		_stricmp(ext,"fr3") && _stricmp(ext,"fr4") && _stricmp(ext,"fr5")) return LoadAnyAnimationOneSpr(fname,path_type,dir);

	if(!fileMngr.LoadFile(fname,path_type)) return NULL;

	AnyFrames* anim=new AnyFrames();
	if(!anim)
	{
		WriteLog(__FUNCTION__" - Memory allocation fail.\n");
		return NULL;
	}

	fileMngr.SetCurPos(0x4);
	WORD frm_fps=fileMngr.GetBEWord();
	if(!frm_fps) frm_fps=10;

	fileMngr.SetCurPos(0x8);
	WORD frm_num=fileMngr.GetBEWord();

	fileMngr.SetCurPos(0xA+dir*2);
	short offs_x=fileMngr.GetBEWord();
	anim->OffsX=offs_x;
	fileMngr.SetCurPos(0x16+dir*2);
	short offs_y=fileMngr.GetBEWord();
	anim->OffsY=offs_y;

	anim->CntFrm=frm_num; 
	anim->Ticks=1000/frm_fps*frm_num;

	anim->Ind=new DWORD[frm_num];
	if(!anim->Ind) return NULL;
	anim->NextX=new short[frm_num];
	if(!anim->NextX) return NULL;
	anim->NextY=new short[frm_num];
	if(!anim->NextY) return NULL;

	fileMngr.SetCurPos(0x22+dir*4);
	DWORD cur_ptr=0x3E+fileMngr.GetBEDWord();

	int animPixType=0;
	// 0x00 - None
	// 0x01 - Slime, 229 - 232, 4
	// 0x02 - Monitors, 233 - 237, 5
	// 0x04 - FireSlow, 238 - 242, 5
	// 0x08 - FireFast, 243 - 247, 5
	// 0x10 - Shoreline, 248 - 253, 6
	// 0x20 - BlinkingRed, 254, parse on 15 frames
	const BYTE BlinkingRedVals[10]={254,210,165,120,75,45,90,135,180,225};

	for(int frm=0;frm<frm_num;frm++)
	{
		SpriteInfo* si=new SpriteInfo(); // TODO: Memory leak
		if(!si) return NULL;
		fileMngr.SetCurPos(cur_ptr);
		WORD w=fileMngr.GetBEWord();
		WORD h=fileMngr.GetBEWord();

		fileMngr.GoForward(4); // Frame size

		si->OffsX=offs_x;
		si->OffsY=offs_y;

		anim->NextX[frm]=fileMngr.GetBEWord();
		anim->NextY[frm]=fileMngr.GetBEWord();

		// Create FOnline fast format
		DWORD size=12+h*w*4;
		BYTE* data=new BYTE[size];
		if(!data)
		{
			WriteLog(__FUNCTION__" - Not enough memory, size<%u>.\n",size);
			delete anim;
			return NULL;
		}
		*((DWORD*)data)=MAKEFOURCC('F','0','F','A'); // FOnline FAst
		*((DWORD*)data+1)=w;
		*((DWORD*)data+2)=h;
		DWORD* ptr=(DWORD*)data+3;
		DWORD* palette=(DWORD*)FoPalette;
		fileMngr.SetCurPos(cur_ptr+12);

		if(!animPixType)
		{
			for(int i=0,j=w*h;i<j;i++) *(ptr+i)=palette[fileMngr.GetByte()];
		}
		else
		{
			for(int i=0,j=w*h;i<j;i++)
			{
				BYTE index=fileMngr.GetByte();
				if(index>=229 && index<255)
				{
					if(index>=229 && index<=232) {index-=frm%4; if(index<229) index+=4;}
					else if(index>=233 && index<=237) {index-=frm%5; if(index<233) index+=5;}
					else if(index>=238 && index<=242) {index-=frm%5; if(index<238) index+=5;}
					else if(index>=243 && index<=247) {index-=frm%5; if(index<243) index+=5;}
					else if(index>=248 && index<=253) {index-=frm%6; if(index<248) index+=6;}
					else
					{
						*(ptr+i)=D3DCOLOR_XRGB(BlinkingRedVals[frm%10],0,0);
						continue;
					}
				}
				*(ptr+i)=palette[index];
			}
		}

		// Check for animate pixels
		if(!frm && anim_pix)
		{
			fileMngr.SetCurPos(cur_ptr+12);
			for(int i=0,j=w*h;i<j;i++)
			{
				BYTE index=fileMngr.GetByte();
				if(index<229 || index==255) continue;
				if(index>=229 && index<=232) animPixType|=0x01;
				else if(index>=233 && index<=237) animPixType|=0x02;
				else if(index>=238 && index<=242) animPixType|=0x04;
				else if(index>=243 && index<=247) animPixType|=0x08;
				else if(index>=248 && index<=253) animPixType|=0x10;
				else animPixType|=0x20;
			}

			if(animPixType&0x01) anim->Ticks=200;
			if(animPixType&0x04) anim->Ticks=200;
			if(animPixType&0x10) anim->Ticks=200;
			if(animPixType&0x08) anim->Ticks=142;
			if(animPixType&0x02) anim->Ticks=100;
			if(animPixType&0x20) anim->Ticks=100;

			if(animPixType)
			{
				int divs[4]; divs[0]=1; divs[1]=1; divs[2]=1; divs[3]=1;
				if(animPixType&0x01) divs[0]=4;
				if(animPixType&0x02) divs[1]=5;
				if(animPixType&0x04) divs[1]=5;
				if(animPixType&0x08) divs[1]=5;
				if(animPixType&0x10) divs[2]=6;
				if(animPixType&0x20) divs[3]=10;

				frm_num=4;
				for(int i=0;i<4;i++)
				{
					if(!(frm_num%divs[i])) continue;
					frm_num++;
					i=-1;
				}

				anim->Ticks*=frm_num;
				anim->CntFrm=frm_num; 
				short nx=anim->NextX[0];
				short ny=anim->NextY[0];
				SAFEDELA(anim->Ind);
				SAFEDELA(anim->NextX);
				SAFEDELA(anim->NextY);
				anim->Ind=new DWORD[frm_num];
				if(!anim->Ind) return NULL;
				anim->NextX=new short[frm_num];
				if(!anim->NextX) return NULL;
				anim->NextY=new short[frm_num];
				if(!anim->NextY) return NULL;
				anim->NextX[0]=nx;
				anim->NextY[0]=ny;
			}
		}

		if(!animPixType) cur_ptr+=w*h+12;

		DWORD result=FillSurfaceFromMemory(si,data,size);
		delete[] data;
		if(!result)
		{
			delete anim;
			return NULL;
		}
		anim->Ind[frm]=result;
	}

	fileMngr.UnloadFile();
	return anim;
}

AnyFrames* SpriteManager::LoadAnyAnimationFofrm(const char* fname, int path_type, int dir)
{
	if(!fileMngr.LoadFile(fname,path_type)) return NULL;
	if(!iniFile.LoadFile(fileMngr.GetBuf(),fileMngr.GetFsize())) return NULL;
	fileMngr.UnloadFile();

	WORD frm_fps=iniFile.GetInt("fps",0);
	if(!frm_fps) frm_fps=10;

	WORD frm_num=iniFile.GetInt("count",0);
	if(!frm_num) frm_num=1;

	AutoPtr<AnyFrames> anim(new AnyFrames());
	if(!anim.IsValid())
	{
		WriteLog(__FUNCTION__" - Memory allocation fail.\n");
		return NULL;
	}

	anim->OffsX=iniFile.GetInt("offs_x",0);
	anim->OffsY=iniFile.GetInt("offs_y",0);
	anim->CntFrm=frm_num;
	anim->Ticks=1000/frm_fps*frm_num;
	anim->Ind=new DWORD[frm_num];
	if(!anim->Ind) return 0;
	anim->NextX=new short[frm_num];
	if(!anim->NextX) return 0;
	anim->NextY=new short[frm_num];
	if(!anim->NextY) return 0;

	char dir_str[16];
	sprintf(dir_str,"dir_%d",dir);
	bool no_app=(dir==0 && !iniFile.IsApp("dir_0"));

	if(!no_app)
	{
		anim->OffsX=iniFile.GetInt(dir_str,"offs_x",anim->OffsX);
		anim->OffsY=iniFile.GetInt(dir_str,"offs_y",anim->OffsY);
	}

	char frm_fname[MAX_FOPATH];
	FileManager::ExtractPath(fname,frm_fname);
	char* frm_name=frm_fname+strlen(frm_fname);

	for(int frm=0;frm<frm_num;frm++)
	{
		anim->NextX[frm]=iniFile.GetInt(no_app?NULL:dir_str,Str::Format("next_x_%d",frm),0);
		anim->NextY[frm]=iniFile.GetInt(no_app?NULL:dir_str,Str::Format("next_y_%d",frm),0);

		if(!iniFile.GetStr(no_app?NULL:dir_str,Str::Format("frm_%d",frm),"",frm_name) &&
			(frm!=0 || !iniFile.GetStr(no_app?NULL:dir_str,Str::Format("frm",frm),"",frm_name))) return NULL;

		SpriteInfo* spr_inf;
		DWORD spr_id=LoadSprite(frm_fname,path_type,&spr_inf);
		if(!spr_id) return NULL;

		spr_inf->OffsX+=anim->OffsX;
		spr_inf->OffsY+=anim->OffsY;
		anim->Ind[frm]=spr_id;
	}

	iniFile.UnloadFile();
	return anim.Release();
}

AnyFrames* SpriteManager::LoadAnyAnimationOneSpr(const char* fname, int path_type, int dir)
{
	DWORD spr_id=LoadSprite(fname,path_type,NULL);
	if(!spr_id) return NULL;

	AutoPtr<AnyFrames> anim(new AnyFrames());
	if(!anim.IsValid())
	{
		WriteLog(__FUNCTION__" - Memory allocation fail.\n");
		return NULL;
	}

	anim->OffsX=0;
	anim->OffsY=0;
	anim->CntFrm=1;
	anim->Ticks=1000;
	anim->Ind=new DWORD[1];
	anim->Ind[0]=spr_id;
	anim->NextX=new short[1];
	anim->NextX[0]=0;
	anim->NextY=new short[1];
	anim->NextY[0]=0;
	return anim.Release();
}

Animation3d* SpriteManager::Load3dAnimation(const char* fname, int path_type)
{
	if(!fileMngr.LoadFile(fname,path_type)) return false;
	fileMngr.UnloadFile();

	// Fill data
	Animation3d* anim3d=Animation3d::GetAnimation(fname,path_type,false);
	if(!anim3d) return NULL;

	SpriteInfo* si=new SpriteInfo();
	size_t index=1;
	for(size_t j=sprData.size();index<j;index++) if(!sprData[index]) break;
	if(index<sprData.size()) sprData[index]=si;
	else sprData.push_back(si);

	// Cross links
	anim3d->SetSprId(index);
	si->Anim3d=anim3d;
	return anim3d;
}

bool SpriteManager::Flush()
{
	if(!isInit) return false;
	if(!curSprCnt) return true;

	void* pBuffer;
	int mulpos=4*curSprCnt;
#ifdef DX8RENDER
	D3D_HR(pVB->Lock(0,sizeof(MYVERTEX)*mulpos,(BYTE**)&pBuffer,D3DLOCK_DISCARD));
#elif DX9RENDER
	D3D_HR(pVB->Lock(0,sizeof(MYVERTEX)*mulpos,(void**)&pBuffer,D3DLOCK_DISCARD));
#endif
	memcpy(pBuffer,waitBuf,sizeof(MYVERTEX)*mulpos);
	D3D_HR(pVB->Unlock());

	//������ �������
	if(!callVec.empty())
	{
		WORD rpos=0;
		for(OneSurfVecIt iv=callVec.begin(),end=callVec.end();iv!=end;++iv)
		{
			D3D_HR(dxDevice->SetTexture(0,(*iv)->Surface));
#ifdef DX8RENDER
			D3D_HR(dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,mulpos,rpos,2*(*iv)->SprCount));
#elif DX9RENDER
			D3D_HR(dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,mulpos,rpos,2*(*iv)->SprCount));
#endif
			rpos+=6*(*iv)->SprCount;
			delete (*iv);
		}

		callVec.clear();
		lastCall=NULL;
		curTexture=NULL;
	}
	else
	{
#ifdef DX8RENDER
		D3D_HR(dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,mulpos,0,2*curSprCnt));
#elif DX9RENDER
		D3D_HR(dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,mulpos,0,2*curSprCnt));
#endif
	}

	curSprCnt=0;
	return true;
}

bool SpriteManager::DrawSprite(DWORD id, int x, int y, DWORD color /* = 0 */)
{
	if(!id) return false;

	SpriteInfo* si=sprData[id];
	if(!si) return false;

	if(curTexture!=si->Surf->Texture)
	{
		lastCall=new OneSurface(si->Surf->Texture);
		callVec.push_back(lastCall);
		curTexture=si->Surf->Texture;
	}
	else if(lastCall) lastCall->SprCount++;

	int mulpos=curSprCnt*4;

	if(!color) color=COLOR_IFACE;

	waitBuf[mulpos].x=x-0.5f;
	waitBuf[mulpos].y=y+si->Height-0.5f;
	waitBuf[mulpos].tu=si->SprRect.L;
	waitBuf[mulpos].tv=si->SprRect.B;
	waitBuf[mulpos++].Diffuse=color;

	waitBuf[mulpos].x=x-0.5f;
	waitBuf[mulpos].y=y-0.5f;
	waitBuf[mulpos].tu=si->SprRect.L;
	waitBuf[mulpos].tv=si->SprRect.T;
	waitBuf[mulpos++].Diffuse=color;

	waitBuf[mulpos].x=x+si->Width-0.5f;
	waitBuf[mulpos].y=y-0.5f;
	waitBuf[mulpos].tu=si->SprRect.R;
	waitBuf[mulpos].tv=si->SprRect.T;
	waitBuf[mulpos++].Diffuse=color;

	waitBuf[mulpos].x=x+si->Width-0.5f;
	waitBuf[mulpos].y=y+si->Height-0.5f;
	waitBuf[mulpos].tu=si->SprRect.R;
	waitBuf[mulpos].tv=si->SprRect.B;
	waitBuf[mulpos].Diffuse=color;

	curSprCnt++;
	if(curSprCnt==flushSprCnt) Flush();
	return true;
}

bool SpriteManager::DrawSpriteSize(DWORD id, int x, int y, float w, float h, bool stretch_up, bool center, DWORD color /* = 0 */)
{
	if(!id) return false;

	SpriteInfo* si=sprData[id];
	if(!si) return false;

	float w_real=(float)si->Width; //Cast
	float h_real=(float)si->Height; //Cast
	float wf=w_real;
	float hf=h_real;
	float k=min(w/w_real,h/h_real);

	if(k<1.0f || (k>1.0f && stretch_up))
	{
		wf*=k;
		hf*=k;
	}

	if(center)
	{
		x+=(w-wf)/2.0f;
		y+=(h-hf)/2.0f;
	}

	if(curTexture!=si->Surf->Texture)
	{
		lastCall=new OneSurface(si->Surf->Texture);
		callVec.push_back(lastCall);
		curTexture=si->Surf->Texture;
	}
	else if(lastCall) lastCall->SprCount++;

	int mulpos=curSprCnt*4;

	if(!color) color=COLOR_IFACE;

	waitBuf[mulpos].x=x-0.5f;
	waitBuf[mulpos].y=y+hf-0.5f;
	waitBuf[mulpos].tu=si->SprRect.L;
	waitBuf[mulpos].tv=si->SprRect.B;
	waitBuf[mulpos++].Diffuse=color;

	waitBuf[mulpos].x=x-0.5f;
	waitBuf[mulpos].y=y-0.5f;
	waitBuf[mulpos].tu=si->SprRect.L;
	waitBuf[mulpos].tv=si->SprRect.T;
	waitBuf[mulpos++].Diffuse=color;

	waitBuf[mulpos].x=x+wf-0.5f;
	waitBuf[mulpos].y=y-0.5f;
	waitBuf[mulpos].tu=si->SprRect.R;
	waitBuf[mulpos].tv=si->SprRect.T;
	waitBuf[mulpos++].Diffuse=color;

	waitBuf[mulpos].x=x+wf-0.5f;
	waitBuf[mulpos].y=y+hf-0.5f;
	waitBuf[mulpos].tu=si->SprRect.R;
	waitBuf[mulpos].tv=si->SprRect.B;
	waitBuf[mulpos].Diffuse=color;

	curSprCnt++;
	if(curSprCnt==flushSprCnt) Flush();
	return true;
}

void SpriteManager::PrepareSquare(PointVec& points, FLTRECT& r, DWORD color)
{
	points.push_back(PrepPoint(r.L,r.B,color,NULL,NULL));
	points.push_back(PrepPoint(r.L,r.T,color,NULL,NULL));
	points.push_back(PrepPoint(r.R,r.B,color,NULL,NULL));
	points.push_back(PrepPoint(r.L,r.T,color,NULL,NULL));
	points.push_back(PrepPoint(r.R,r.T,color,NULL,NULL));
	points.push_back(PrepPoint(r.R,r.B,color,NULL,NULL));
}

#pragma MESSAGE("Optimize: prerender to texture.")
bool SpriteManager::PrepareBuffer(Sprites& dtree, LPDIRECT3DVERTEXBUFFER& vbuf, OneSurfVec& surfaces, bool sort_surf, BYTE alpha)
{
	SAFEREL(vbuf);
	for(OneSurfVecIt it=surfaces.begin(),end=surfaces.end();it!=end;++it)
		delete *it;
	surfaces.clear();

	DWORD cnt=dtree.Size();
	if(!cnt) return true;

	// Create vertex buffer
#ifdef DX8RENDER
	D3D_HR(dxDevice->CreateVertexBuffer(cnt*4*sizeof(MYVERTEX),D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC,D3DFVF_MYVERTEX,D3DPOOL_DEFAULT,&vbuf));
#elif DX9RENDER
	D3D_HR(dxDevice->CreateVertexBuffer(cnt*4*sizeof(MYVERTEX),D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC,D3DFVF_MYVERTEX,D3DPOOL_DEFAULT,&vbuf,NULL));
#endif

	DWORD need_size=cnt*6*sizeof(WORD);
	D3DINDEXBUFFER_DESC ibdesc;
	if(pIB) D3D_HR(pIB->GetDesc(&ibdesc));
	if(!pIB || ibdesc.Size<need_size)
	{
		SAFEREL(pIB);
		// Create index buffer
#ifdef DX8RENDER
		D3D_HR(dxDevice->CreateIndexBuffer(need_size,D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&pIB));
#elif DX9RENDER
		D3D_HR(dxDevice->CreateIndexBuffer(need_size,D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&pIB,NULL));
#endif
		WORD* indices=new WORD[6*cnt];
		if(!indices) return false;
		for(DWORD i=0;i<cnt;i++)
		{
			indices[6*i+0]=4*i+0;
			indices[6*i+1]=4*i+1;
			indices[6*i+2]=4*i+3;
			indices[6*i+3]=4*i+1;
			indices[6*i+4]=4*i+2;
			indices[6*i+5]=4*i+3;
		}

		void* ptr;
#ifdef DX8RENDER
		D3D_HR(pIB->Lock(0,0,(BYTE**)&ptr,0));
#elif DX9RENDER
		D3D_HR(pIB->Lock(0,0,(void**)&ptr,0));
#endif
		memcpy(ptr,indices,need_size);
		D3D_HR(pIB->Unlock());
		delete[] indices;

#ifdef DX8RENDER
		D3D_HR(dxDevice->SetIndices(pIB,0));
#elif DX9RENDER
		D3D_HR(dxDevice->SetIndices(pIB));
#endif
	}

	WORD mulpos=0;
	OneSurface* lc=NULL;
	MYVERTEX* local_vbuf=new MYVERTEX[cnt*4];
	if(!local_vbuf) return false;

	DWORD _color0=baseColor;
	DWORD _color1=baseColor;
	DWORD _color2=baseColor;
	DWORD _color3=baseColor;

	if(alpha)
	{
		((BYTE*)&_color0)[3]=alpha;
		((BYTE*)&_color1)[3]=alpha;
		((BYTE*)&_color2)[3]=alpha;
		((BYTE*)&_color3)[3]=alpha;
	}

	// Draw
	for(SpriteVecIt it=dtree.Begin(),end=dtree.End();it!=end;++it)
	{
		Sprite* spr=*it;
		if(!spr->Valid) continue;
		SpriteInfo* si=sprData[spr->SprId];
		if(!si) continue;

		int x=spr->ScrX-si->Width/2+si->OffsX;
		int y=spr->ScrY-si->Height+si->OffsY;
		if(drawOffsetX) x+=*drawOffsetX;
		if(drawOffsetY) y+=*drawOffsetY;
		if(spr->OffsX) x+=*spr->OffsX;
		if(spr->OffsY) y+=*spr->OffsY;

		if(!lc || lc->Surface!=si->Surf->Texture)
		{
			lc=new OneSurface(si->Surf->Texture);
			surfaces.push_back(lc);
		}
		else lc->SprCount++;

		DWORD __color0=_color0;
		DWORD __color1=_color1;
		DWORD __color2=_color2;
		DWORD __color3=_color3;

		if(spr->Alpha)
		{
			((BYTE*)&__color0)[3]=*spr->Alpha;
			((BYTE*)&__color1)[3]=*spr->Alpha;
			((BYTE*)&__color2)[3]=*spr->Alpha;
			((BYTE*)&__color3)[3]=*spr->Alpha;
		}

		/*if(spr->Light)
		{
			((BYTE*)&__color0)[3]=*(spr->Light+0);
			((BYTE*)&__color1)[3]=*(spr->Light+1);
			((BYTE*)&__color2)[3]=*(spr->Light+2);
			((BYTE*)&__color3)[3]=*(spr->Light+3);
		}*/

		// Casts
		float xf=float(x)/ZOOM-0.5f;
		float yf=float(y)/ZOOM-0.5f;
		float wf=float(si->Width)/ZOOM;
		float hf=float(si->Height)/ZOOM;

		// Fill buffer
		local_vbuf[mulpos].x=xf;
		local_vbuf[mulpos].y=yf+hf;
		local_vbuf[mulpos].tu=si->SprRect.L;
		local_vbuf[mulpos].tv=si->SprRect.B;
		local_vbuf[mulpos++].Diffuse=__color0;

		local_vbuf[mulpos].x=xf;
		local_vbuf[mulpos].y=yf;
		local_vbuf[mulpos].tu=si->SprRect.L;
		local_vbuf[mulpos].tv=si->SprRect.T;
		local_vbuf[mulpos++].Diffuse=__color1;

		local_vbuf[mulpos].x=xf+wf;
		local_vbuf[mulpos].y=yf;
		local_vbuf[mulpos].tu=si->SprRect.R;
		local_vbuf[mulpos].tv=si->SprRect.T;
		local_vbuf[mulpos++].Diffuse=__color2;

		local_vbuf[mulpos].x=xf+wf;
		local_vbuf[mulpos].y=yf+hf;
		local_vbuf[mulpos].tu=si->SprRect.R;
		local_vbuf[mulpos].tv=si->SprRect.B;
		local_vbuf[mulpos++].Diffuse=__color3;
	}

	void* ptr;
#ifdef DX8RENDER
	D3D_HR(vbuf->Lock(0,sizeof(MYVERTEX)*mulpos,(BYTE**)&ptr,D3DLOCK_DISCARD));
#elif DX9RENDER
	D3D_HR(vbuf->Lock(0,sizeof(MYVERTEX)*mulpos,(void**)&ptr,D3DLOCK_DISCARD));
#endif
	memcpy(ptr,local_vbuf,sizeof(MYVERTEX)*mulpos);
	D3D_HR(vbuf->Unlock());
	SAFEDELA(local_vbuf);

	return true;
}

bool SpriteManager::DrawPrepared(LPDIRECT3DVERTEXBUFFER& vbuf, OneSurfVec& surfaces, WORD cnt)
{
	if(!cnt) return true;
	Flush();

#ifdef DX8RENDER
	D3D_HR(dxDevice->SetStreamSource(0,vbuf,sizeof(MYVERTEX)));
#elif DX9RENDER
	D3D_HR(dxDevice->SetStreamSource(0,vbuf,0,sizeof(MYVERTEX)));
#endif

	WORD rpos=0;
	for(OneSurfVecIt it=surfaces.begin(),end=surfaces.end();it!=end;++it)
	{
		OneSurface* surf=*it;
		D3D_HR(dxDevice->SetTexture(0,surf->Surface));
#ifdef DX8RENDER
		D3D_HR(dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,cnt*4,rpos,2*surf->SprCount));
#elif DX9RENDER
		D3D_HR(dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,cnt*4,rpos,2*surf->SprCount));
#endif
		rpos+=6*surf->SprCount;
	}

#ifdef DX8RENDER
	D3D_HR(dxDevice->SetStreamSource(0,pVB,sizeof(MYVERTEX)));
#elif DX9RENDER
	D3D_HR(dxDevice->SetStreamSource(0,pVB,0,sizeof(MYVERTEX)));
#endif
	return true;
}

DWORD SpriteManager::GetColor(int r, int g, int b)
{
	r=CLAMP(r,0,255);
	g=CLAMP(g,0,255);
	b=CLAMP(b,0,255);
	return D3DCOLOR_XRGB(r,g,b);
}

void SpriteManager::GetDrawCntrRect(Sprite* prep, INTRECT* prect)
{
	DWORD id=(prep->PSprId?*prep->PSprId:prep->SprId);
	if(id>=sprData.size()) return;
	SpriteInfo* si=sprData[id];
	if(!si) return;

	if(!si->Anim3d)
	{
		int x=prep->ScrX-si->Width/2+si->OffsX;
		int y=prep->ScrY-si->Height+si->OffsY;
		if(prep->OffsX) x+=*prep->OffsX;
		if(prep->OffsY) y+=*prep->OffsY;
		prect->L=x;
		prect->T=y;
		prect->R=x+si->Width;
		prect->B=y+si->Height;
	}
	else
	{
		*prect=si->Anim3d->GetBaseBorders();
	}
}

bool SpriteManager::CompareHexEgg(WORD hx, WORD hy, Sprite::EggType egg)
{
	if(egg==Sprite::EggAlways) return true;
	if(eggHy==hy && hx%2 && !(eggHx%2)) hy--;
	switch(egg)
	{
	case Sprite::EggX: if(hx>=eggHx) return true; break;
	case Sprite::EggY: if(hy>=eggHy) return true; break;
	case Sprite::EggXandY: if(hx>=eggHx || hy>=eggHy) return true; break;
	case Sprite::EggXorY: if(hx>=eggHx && hy>=eggHy) return true; break;
	default: break;
	}
	return false;
}

void SpriteManager::SetEgg(WORD hx, WORD hy, Sprite* spr)
{
	DWORD id=(spr->PSprId?*spr->PSprId:spr->SprId);
	SpriteInfo* si=sprData[id];
	if(!si) return;

	if(!si->Anim3d)
	{
		eggX=spr->ScrX-(si->Width>>1)+si->OffsX+si->Width/2-sprEgg->Width/2+*spr->OffsX;
		eggY=spr->ScrY-si->Height+si->OffsY+si->Height/2-sprEgg->Height/2+*spr->OffsY;
	}
	else
	{
		INTRECT b=si->Anim3d->GetBaseBorders();
		eggX=b.CX()-sprEgg->Width/2-*drawOffsetX;
		eggY=b.CY()-sprEgg->Height/2-*drawOffsetY;
	}

	eggHx=hx;
	eggHy=hy;
	eggValid=true;
}

bool SpriteManager::DrawTreeCntr(Sprites& dtree, bool collect_contours, bool use_egg, DWORD pos_min, DWORD pos_max)
{
	//PointVec borders;

	if(!eggValid) use_egg=false;
	bool egg_trans=false;
	int ex=eggX+(drawOffsetX?*drawOffsetX:0);
	int ey=eggY+(drawOffsetY?*drawOffsetY:0);
	DWORD cur_tick=Timer::FastTick();

	for(SpriteVecIt it=dtree.Begin(),end=dtree.End();it!=end;++it)
	{
		// Data
		Sprite* spr=*it;
		if(!spr->Valid || spr->MapPos<pos_min) continue;

		DWORD pos=spr->MapPos;
		if(pos>pos_max) break;

		DWORD id=(spr->PSprId?*spr->PSprId:spr->SprId);
		SpriteInfo* si=sprData[id];
		if(!si) continue;

		int x=spr->ScrX-si->Width/2+si->OffsX;
		int y=spr->ScrY-si->Height+si->OffsY;
		if(drawOffsetX) x+=*drawOffsetX;
		if(drawOffsetY) y+=*drawOffsetY;
		if(spr->OffsX) x+=*spr->OffsX;
		if(spr->OffsY) y+=*spr->OffsY;

		// Check borders
		if(x/ZOOM>modeWidth || (x+si->Width)/ZOOM<0 || y/ZOOM>modeHeight || (y+si->Height)/ZOOM<0) continue;

		// Base color
		DWORD cur_color;
		if(spr->Color)
			cur_color=(spr->Color|0xFF000000);
		else
			cur_color=baseColor;

		// Light
		if(spr->Light)
		{
			int lr=*spr->Light;
			int lg=*(spr->Light+1);
			int lb=*(spr->Light+2);
			BYTE& r=((BYTE*)&cur_color)[2];
			BYTE& g=((BYTE*)&cur_color)[1];
			BYTE& b=((BYTE*)&cur_color)[0];
			int ir=(int)r+lr;
			int ig=(int)g+lg;
			int ib=(int)b+lb;
			if(ir>0xFF) ir=0xFF;
			if(ig>0xFF) ig=0xFF;
			if(ib>0xFF) ib=0xFF;
			r=ir;
			g=ig;
			b=ib;
		}

		// Alpha
		if(spr->Alpha)
		{
			((BYTE*)&cur_color)[3]=*spr->Alpha;
		}

		// Process flashing
		if(spr->FlashMask)
		{
			static int cnt=0;
			static DWORD tick=cur_tick+100;
			static bool add=true;
			if(cur_tick>=tick)
			{
				cnt+=(add?10:-10);
				if(cnt>40)
				{
					cnt=40;
					add=false;
				}
				else if(cnt<-40)
				{
					cnt=-40;
					add=true;
				}
				tick=cur_tick+100;
			}
			int r=((cur_color>>16)&0xFF)+cnt;
			int g=((cur_color>>8)&0xFF)+cnt;
			int b=(cur_color&0xFF)+cnt;
			r=CLAMP(r,0,0xFF);
			g=CLAMP(g,0,0xFF);
			b=CLAMP(b,0,0xFF);
			((BYTE*)&cur_color)[2]=r;
			((BYTE*)&cur_color)[1]=g;
			((BYTE*)&cur_color)[0]=b;
			cur_color&=spr->FlashMask;
		}

		// 3d model
		if(si->Anim3d)
		{
			// Draw collected sprites and disable egg
			Flush();
			if(egg_trans)
			{
				D3D_HR(dxDevice->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE));
				egg_trans=false;
			}

			// Draw 3d animation
			Draw3d(x,y,si->Anim3d,NULL,cur_color);

			// Process contour effect
			if(collect_contours && spr->Contour!=Sprite::ContourNone) CollectContour(x,y,si,spr);

			// Debug borders
			//INTRECT r=si->Anim3d->GetBaseBorders();
			//PrepareSquare(borders,FLTRECT(r.L,r.T,r.R,r.B),0x7f757575);
			continue;
		}

		// 2d sprite
		// Egg process
		bool egg_added=false;
		if(use_egg && spr->Egg!=Sprite::EggNone && CompareHexEgg(HEX_X_POS(pos),HEX_Y_POS(pos),spr->Egg))
		{
			int x1=x-ex;
			int y1=y-ey;
			int x2=x1+si->Width;
			int y2=y1+si->Height;

			if(!(x1>=eggSprWidth || y1>=eggSprHeight || x2<0 || y2<0))
			{
				if(!egg_trans)
				{
					Flush();
					D3D_HR(dxDevice->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_SELECTARG2));
					egg_trans=true;
				}

				x1=max(x1,0);
				y1=max(y1,0);
				x2=min(x2,eggSprWidth);
				y2=min(y2,eggSprHeight);

				float x1f=(float)x1;
				float x2f=(float)x2;
				float y1f=(float)y1;
				float y2f=(float)y2;

				int mulpos=curSprCnt*4;

				waitBuf[mulpos].tu2=x1f/eggSurfWidth;
				waitBuf[mulpos].tv2=y2f/eggSurfHeight;
				waitBuf[mulpos+1].tu2=x1f/eggSurfWidth;
				waitBuf[mulpos+1].tv2=y1f/eggSurfHeight;
				waitBuf[mulpos+2].tu2=x2f/eggSurfWidth;
				waitBuf[mulpos+2].tv2=y1f/eggSurfHeight;
				waitBuf[mulpos+3].tu2=x2f/eggSurfWidth;
				waitBuf[mulpos+3].tv2=y2f/eggSurfHeight;
				egg_added=true;
			}
		}

		if(!egg_added && egg_trans)
		{
			Flush();
			D3D_HR(dxDevice->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE));
			egg_trans=false;
		}

		// Choose surface
		if(curTexture!=si->Surf->Texture)
		{
			lastCall=new OneSurface(si->Surf->Texture);
			callVec.push_back(lastCall);
			curTexture=si->Surf->Texture;
		}
		else if(lastCall) lastCall->SprCount++;

		// Process contour effect
		if(collect_contours && spr->Contour!=Sprite::ContourNone) CollectContour(x,y,si,spr);

		// Casts
		float xf=float(x)/ZOOM-0.5f;
		float yf=float(y)/ZOOM-0.5f;
		float wf=float(si->Width)/ZOOM;
		float hf=float(si->Height)/ZOOM;

		// Fill buffer
		int mulpos=curSprCnt*4;

		waitBuf[mulpos].x=xf;
		waitBuf[mulpos].y=yf+hf;
		waitBuf[mulpos].tu=si->SprRect.L;
		waitBuf[mulpos].tv=si->SprRect.B;
		waitBuf[mulpos++].Diffuse=cur_color;

		waitBuf[mulpos].x=xf;
		waitBuf[mulpos].y=yf;
		waitBuf[mulpos].tu=si->SprRect.L;
		waitBuf[mulpos].tv=si->SprRect.T;
		waitBuf[mulpos++].Diffuse=cur_color;

		waitBuf[mulpos].x=xf+wf;
		waitBuf[mulpos].y=yf;
		waitBuf[mulpos].tu=si->SprRect.R;
		waitBuf[mulpos].tv=si->SprRect.T;
		waitBuf[mulpos++].Diffuse=cur_color;

		waitBuf[mulpos].x=xf+wf;
		waitBuf[mulpos].y=yf+hf;
		waitBuf[mulpos].tu=si->SprRect.R;
		waitBuf[mulpos].tv=si->SprRect.B;
		waitBuf[mulpos++].Diffuse=cur_color;

		curSprCnt++;

		// Draw
		if(curSprCnt==flushSprCnt) Flush();

		// Enable egg
		/*if(spr->Egg==Sprite::EggMain && !eggEnable)
		{
		eggX=x+si->Width/2-sprEgg->Width/2;
		eggY=y+si->Height/2-sprEgg->Height/2;
		eggEnable=true;
		eggValid=true;
		eggPos=pos;
		}*/
	}

	Flush();
	if(egg_trans) D3D_HR(dxDevice->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE));

	//DrawPoints(borders,D3DPT_TRIANGLELIST);
	return true;
}

bool SpriteManager::IsPixNoTransp(DWORD spr_id, int offs_x, int offs_y, bool with_zoom)
{
	if(offs_x<0 || offs_y<0) return false;
	SpriteInfo* si=GetSpriteInfo(spr_id);
	if(!si) return false;

	// 3d animation
	if(si->Anim3d)
	{
		/*if(!si->Anim3d->GetDrawIndex()) return false;

		IDirect3DSurface9* zstencil;
		D3D_HR(dxDevice->GetDepthStencilSurface(&zstencil));

		D3DSURFACE_DESC sDesc;
		D3D_HR(zstencil->GetDesc(&sDesc));
		int width=sDesc.Width;
		int height=sDesc.Height;

		D3DLOCKED_RECT desc;
		D3D_HR(zstencil->LockRect(&desc,NULL,D3DLOCK_READONLY));
		BYTE* ptr=(BYTE*)desc.pBits;
		int pitch=desc.Pitch;

		int stencil_offset=offs_y*pitch+offs_x*4+3;
		WriteLog("===========%d %d====%u\n",offs_x,offs_y,ptr[stencil_offset]);
		if(stencil_offset<pitch*height && ptr[stencil_offset]==si->Anim3d->GetDrawIndex())
		{
		D3D_HR(zstencil->UnlockRect());
		D3D_HR(zstencil->Release());
		return true;
		}

		D3D_HR(zstencil->UnlockRect());
		D3D_HR(zstencil->Release());*/
		return false;
	}

	// 2d animation
	if(with_zoom && (offs_x>si->Width/ZOOM || offs_y>si->Height/ZOOM)) return false;
	if(!with_zoom && (offs_x>si->Width || offs_y>si->Height)) return false;

	if(with_zoom)
	{
		offs_x*=ZOOM;
		offs_y*=ZOOM;
	}

	D3DSURFACE_DESC sDesc;
	D3D_HR(si->Surf->Texture->GetLevelDesc(0,&sDesc));
	int width=sDesc.Width;
	int height=sDesc.Height;

	D3DLOCKED_RECT desc;
	D3D_HR(si->Surf->Texture->LockRect(0,&desc,NULL,D3DLOCK_READONLY));
	BYTE* ptr=(BYTE*)desc.pBits;
	int pitch=desc.Pitch;

	offs_x+=si->SprRect.L*(float)width;
	offs_y+=si->SprRect.T*(float)height;
	int alpha_offset=offs_y*pitch+offs_x*4+3;
	if(alpha_offset<pitch*height && ptr[alpha_offset]>0)
	{
		D3D_HR(si->Surf->Texture->UnlockRect(0));
		return true;
	}

	D3D_HR(si->Surf->Texture->UnlockRect(0));
	return false;

	//	pDst[i] - ���� ���� 
	//	b       - pDst[ y*iHeight*4 + x*4];
	//	g       - pDst[ y*iHeight*4 + x*4+1];
	//	r       - pDst[ y*iHeight*4 + x*4+2];
	//	alpha	- pDst[ y*iHeight*4 + x*4+3];
	//	x, y - ���������� �����
}

bool SpriteManager::IsEggTransp(int pix_x, int pix_y)
{
	if(!eggValid) return false;

	int ex=eggX+(drawOffsetX?*drawOffsetX:0);
	int ey=eggY+(drawOffsetY?*drawOffsetY:0);
	int ox=pix_x-ex/ZOOM;
	int oy=pix_y-ey/ZOOM;

	if(ox<0 || oy<0 || ox>=int(eggSurfWidth/ZOOM) || oy>=int(eggSurfHeight/ZOOM)) return false;

	ox*=ZOOM;
	oy*=ZOOM;

	D3DSURFACE_DESC sDesc;
	D3D_HR(sprEgg->Surf->Texture->GetLevelDesc(0,&sDesc));

	int sWidth=sDesc.Width;
	int sHeight=sDesc.Height;

	D3DLOCKED_RECT lrDst;
	D3D_HR(sprEgg->Surf->Texture->LockRect(0,&lrDst,NULL,D3DLOCK_READONLY));

	BYTE* pDst=(BYTE*)lrDst.pBits;

	if(pDst[oy*sHeight*4+ox*4+3]<170)
	{
		D3D_HR(sprEgg->Surf->Texture->UnlockRect(0));
		return true;
	}

	D3D_HR(sprEgg->Surf->Texture->UnlockRect(0));
	return false;
}

bool SpriteManager::DrawPoints(PointVec& points, D3DPRIMITIVETYPE prim, bool with_zoom)
{
	if(points.empty()) return true;
	Flush();

	int count=points.size();
	LPDIRECT3DVERTEXBUFFER vb;
#pragma MESSAGE("Create vertex buffer once, not per draw.")
#ifdef DX8RENDER
	D3D_HR(dxDevice->CreateVertexBuffer(count*sizeof(MYVERTEX_PRIMITIVE),D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY,D3DFVF_MYVERTEX_PRIMITIVE,D3DPOOL_DEFAULT,&vb));
#elif DX9RENDER
	D3D_HR(dxDevice->CreateVertexBuffer(count*sizeof(MYVERTEX_PRIMITIVE),D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY,D3DFVF_MYVERTEX_PRIMITIVE,D3DPOOL_DEFAULT,&vb,NULL));
#endif

	void* vertices;
#ifdef DX8RENDER
	D3D_HR(vb->Lock(0,count*sizeof(MYVERTEX_PRIMITIVE),(BYTE**)&vertices,D3DLOCK_DISCARD));
#elif DX9RENDER
	D3D_HR(vb->Lock(0,count*sizeof(MYVERTEX_PRIMITIVE),(void**)&vertices,D3DLOCK_DISCARD));
#endif

	DWORD cur=0;
	for(PointVecIt it=points.begin(),end=points.end();it!=end;++it)
	{
		PrepPoint& point=(*it);
		MYVERTEX_PRIMITIVE* vertex=(MYVERTEX_PRIMITIVE*)vertices+cur;
		vertex->x=(float)point.PointX;
		vertex->y=(float)point.PointY;
		if(point.PointOffsX) vertex->x+=*point.PointOffsX;
		if(point.PointOffsY) vertex->y+=*point.PointOffsY;
		if(with_zoom)
		{
			vertex->x/=ZOOM;
			vertex->y/=ZOOM;
		}
		vertex->Diffuse=point.PointColor;
		vertex->z=0;
		vertex->rhw=1;
		cur++;
	}

	D3D_HR(vb->Unlock());

#ifdef DX8RENDER
	D3D_HR(dxDevice->SetStreamSource(0,vb,sizeof(MYVERTEX_PRIMITIVE)));
	D3D_HR(dxDevice->SetVertexShader(D3DFVF_MYVERTEX_PRIMITIVE));
#elif DX9RENDER
	D3D_HR(dxDevice->SetStreamSource(0,vb,0,sizeof(MYVERTEX_PRIMITIVE)));
	D3D_HR(dxDevice->SetVertexShader(NULL));
	D3D_HR(dxDevice->SetFVF(D3DFVF_MYVERTEX_PRIMITIVE));
#endif

	switch(prim)
	{
	case D3DPT_POINTLIST: break;
	case D3DPT_LINELIST: count/=2; break;
	case D3DPT_LINESTRIP: count-=1; break;
	case D3DPT_TRIANGLELIST: count/=3; break;
	case D3DPT_TRIANGLESTRIP: count-=2; break;
	case D3DPT_TRIANGLEFAN: count-=2; break;
	default: break;
	}
	if(count<=0)
	{
		SAFEREL(vb);
		return false;
	}

	D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_DISABLE));
	D3D_HR(dxDevice->DrawPrimitive(prim,0,count));
	D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE2X));

	SAFEREL(vb);
#ifdef DX8RENDER
	D3D_HR(dxDevice->SetStreamSource(0,pVB,sizeof(MYVERTEX)));
	D3D_HR(dxDevice->SetVertexShader(D3DFVF_MYVERTEX));
#elif DX9RENDER
	D3D_HR(dxDevice->SetStreamSource(0,pVB,0,sizeof(MYVERTEX)));
	D3D_HR(dxDevice->SetVertexShader(NULL));
	D3D_HR(dxDevice->SetFVF(D3DFVF_MYVERTEX));
#endif
	return true;
}

bool SpriteManager::Draw3d(int x, int y, Animation3d* anim3d, FLTRECT* stencil, DWORD color)
{
	// Draw previous sprites
	Flush();

	// Draw 3d
	anim3d->Draw(x,y,1.0f,stencil,color);

	// Restore 2d stream
	D3D_HR(dxDevice->SetIndices(pIB));
	D3D_HR(dxDevice->SetStreamSource(0,pVB,0,sizeof(MYVERTEX)));
	D3D_HR(dxDevice->SetVertexShader(NULL));
	D3D_HR(dxDevice->SetPixelShader(NULL));
	D3D_HR(dxDevice->SetFVF(D3DFVF_MYVERTEX));
	return true;
}

bool SpriteManager::Draw3dSize(FLTRECT rect, bool stretch_up, bool center, Animation3d* anim3d, FLTRECT* stencil, DWORD color)
{
	Flush();

	INTPOINT xy=anim3d->GetBordersPivot();
	INTRECT borders=anim3d->GetBaseBorders();
	float w_real=(float)borders.W();
	float h_real=(float)borders.H();
	float scale=min(rect.W()/w_real,rect.H()/h_real);
	if(scale>1.0f && !stretch_up) scale=1.0f;
	if(center)
	{
		xy.X+=(rect.W()-w_real*scale)/2.0f;
		xy.Y+=(rect.H()-h_real*scale)/2.0f;
	}

	anim3d->Draw(rect.L+(float)xy.X*scale,rect.T+(float)xy.Y*scale,scale,stencil,color);

	// Restore 2d stream
	D3D_HR(dxDevice->SetIndices(pIB));
	D3D_HR(dxDevice->SetStreamSource(0,pVB,0,sizeof(MYVERTEX)));
	D3D_HR(dxDevice->SetVertexShader(NULL));
	D3D_HR(dxDevice->SetPixelShader(NULL));
	D3D_HR(dxDevice->SetFVF(D3DFVF_MYVERTEX));
	return true;
}

bool SpriteManager::DrawContours()
{
	if(contoursPS && contoursAdded)
	{
		D3D_HR(dxDevice->SetTexture(0,contoursTexture));
		D3D_HR(dxDevice->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1));
		D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1));

		struct Vertex
		{
			FLOAT x,y,z,rhw;
			float tu,tv;
		} vb[6]=
		{
			{-0.5f                 ,(float)modeHeight-0.5f ,0.0f,1.0f,0.0f,1.0f},
			{-0.5f                 ,-0.5f                  ,0.0f,1.0f,0.0f,0.0f},
			{(float)modeWidth-0.5f ,(float)modeHeight-0.5f ,0.0f,1.0f,1.0f,1.0f},
			{-0.5f                 ,-0.5f                  ,0.0f,1.0f,0.0f,0.0f},
			{(float)modeWidth-0.5f ,-0.5f                  ,0.0f,1.0f,1.0f,0.0f},
			{(float)modeWidth-0.5f ,(float)modeHeight-0.5f ,0.0f,1.0f,1.0f,1.0f},
		};

		D3D_HR(dxDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST,2,(void*)vb,sizeof(Vertex)));
		D3D_HR(dxDevice->SetStreamSource(0,pVB,0,sizeof(MYVERTEX)));
		D3D_HR(dxDevice->SetFVF(D3DFVF_MYVERTEX));
		D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE2X));
		contoursAdded=false;
	}
	else if(spriteContours.Size())
	{
		DrawTreeCntr(spriteContours,false,false,0,-1);
		spriteContours.Unvalidate();
	}
	return true;
}

bool SpriteManager::CollectContour(int x, int y, SpriteInfo* si, Sprite* spr)
{
	if(!contoursPS)
	{
		if(!si->Anim3d)
		{
			DWORD contour_id=GetSpriteContour(si,spr);
			if(contour_id)
			{
				Sprite& contour_spr=spriteContours.AddSprite(0,spr->ScrX,spr->ScrY,contour_id,NULL,spr->OffsX,spr->OffsY,NULL,NULL,NULL);
				contour_spr.Color=0xFFAFAFAF;
				if(spr->Contour==Sprite::ContourRed) contour_spr.FlashMask=0xFFFF0000;
				else if(spr->Contour==Sprite::ContourYellow) contour_spr.FlashMask=0xFFFFFF00;
				else
				{
					contour_spr.FlashMask=0xFFFFFFFF;
					contour_spr.Color=spr->ContourColor;
				}
				return true;
			}
		}

		return false;
	}

	// Check borders
	Animation3d* anim3d=si->Anim3d;
	INTRECT borders=(anim3d?anim3d->GetFullBorders():INTRECT(x-1,y-1,x+si->Width+1,y+si->Height+1));
	LPDIRECT3DTEXTURE texture=(anim3d?contoursMidTexture:si->Surf->Texture);
	float ws,hs;
	FLTRECT tuv,tuvh;

	if(!anim3d)
	{
		if(borders.L>=modeWidth || borders.R<0 || borders.T>=modeHeight || borders.B<0) return true;

		D3DSURFACE_DESC desc;
		D3D_HR(texture->GetLevelDesc(0,&desc));
		ws=1.0f/desc.Width;
		hs=1.0f/desc.Height;
		tuv=FLTRECT(si->SprRect.L-ws,si->SprRect.T-hs,si->SprRect.R+ws,si->SprRect.B+hs);
		tuvh=tuv;
	}
	else
	{
		if(borders.L>=modeWidth || borders.R<0 || borders.T>=modeHeight || borders.B<0) return true;

		INTRECT init_borders=borders;
		if(borders.L<=0) borders.L=1;
		if(borders.T<=0) borders.T=1;
		if(borders.R>=modeWidth) borders.R=modeWidth-1;
		if(borders.B>=modeHeight) borders.B=modeHeight-1;

		float w=modeWidth,h=modeHeight;
		tuv.L=(float)borders.L/w;
		tuv.T=(float)borders.T/h;
		tuv.R=(float)borders.R/w;
		tuv.B=(float)borders.B/h;
		tuvh.T=(float)init_borders.T/h;
		tuvh.B=(float)init_borders.B/h;

		ws=0.1f/modeWidth;
		hs=0.1f/modeHeight;

		// Render to contours texture
		LPDIRECT3DSURFACE old_rt;
		D3D_HR(dxDevice->GetRenderTarget(0,&old_rt));
		D3D_HR(dxDevice->SetRenderTarget(0,contours3dSurf));
		D3D_HR(dxDevice->SetRenderState(D3DRS_ZENABLE,TRUE));
		D3D_HR(dxDevice->SetRenderState(D3DRS_ZFUNC,D3DCMP_NOTEQUAL));
		D3D_HR(dxDevice->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE));
		D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG2));

		struct Vertex
		{
			FLOAT x,y,z,rhw;
			DWORD diffuse;
		} vb[6]=
		{
			{(float)borders.L-0.5f,(float)borders.B-0.5f,1.0f,1.0f,0xFFFF00FF},
			{(float)borders.L-0.5f,(float)borders.T-0.5f,1.0f,1.0f,0xFFFF00FF},
			{(float)borders.R-0.5f,(float)borders.B-0.5f,1.0f,1.0f,0xFFFF00FF},
			{(float)borders.L-0.5f,(float)borders.T-0.5f,1.0f,1.0f,0xFFFF00FF},
			{(float)borders.R-0.5f,(float)borders.T-0.5f,1.0f,1.0f,0xFFFF00FF},
			{(float)borders.R-0.5f,(float)borders.B-0.5f,1.0f,1.0f,0xFFFF00FF},
		};

		D3D_HR(dxDevice->Clear(0,NULL,D3DCLEAR_TARGET,0,1.0f,0));
		D3D_HR(dxDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST,2,(void*)vb,sizeof(Vertex)));

		D3D_HR(dxDevice->SetRenderState(D3DRS_ZENABLE,FALSE));
		D3D_HR(dxDevice->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL));

		// Copy to mid surface
		RECT r={borders.L-1,borders.T-1,borders.R+1,borders.B+1};
		D3D_HR(dxDevice->StretchRect(contours3dSurf,&r,contoursMidTextureSurf,&r,D3DTEXF_NONE));

		D3D_HR(dxDevice->SetRenderTarget(0,old_rt));
		old_rt->Release();
	}

	// Calculate contour color
	DWORD contour_color=0;
	if(spr->Contour==Sprite::ContourRed) contour_color=0xFFAF0000;
	else if(spr->Contour==Sprite::ContourYellow)
	{
		contour_color=0xFFAFAF00;
		tuvh.T=-1.0f; // Disable flashing
		tuvh.B=-1.0f;
	}
	else if(spr->Contour==Sprite::ContourCustom) contour_color=spr->ContourColor;
	else contour_color=0xFFAFAFAF;

	static float color_offs=0.0f;
	static DWORD last_tick=0;
	DWORD tick=Timer::FastTick();
	if(tick-last_tick>=50)
	{
		color_offs-=0.05f;
		if(color_offs<-1.0f) color_offs=1.0f;
		last_tick=tick;
	}

	// Draw contour
	LPDIRECT3DSURFACE ds;
	D3D_HR(dxDevice->GetDepthStencilSurface(&ds));
	D3D_HR(dxDevice->SetDepthStencilSurface(NULL));
	LPDIRECT3DSURFACE old_rt;
	D3D_HR(dxDevice->GetRenderTarget(0,&old_rt));
	D3D_HR(dxDevice->SetRenderTarget(0,contoursTextureSurf));
	D3D_HR(dxDevice->SetTexture(0,texture));
	D3D_HR(dxDevice->SetPixelShader(contoursPS));
	D3D_HR(dxDevice->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1));
	D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1));

	D3D_HR(contoursCT->SetFloat(dxDevice,contoursCT->GetConstantByName(NULL,"WidthStep"),ws));
	D3D_HR(contoursCT->SetFloat(dxDevice,contoursCT->GetConstantByName(NULL,"HeightStep"),hs));
	float sb[4]={tuv.L,tuv.T,tuv.R,tuv.B};
	D3D_HR(contoursCT->SetFloatArray(dxDevice,contoursCT->GetConstantByName(NULL,"SpriteBorders"),sb,4));
	float sbh[3]={tuvh.T,tuvh.B,tuvh.B-tuvh.T};
	D3D_HR(contoursCT->SetFloatArray(dxDevice,contoursCT->GetConstantByName(NULL,"SpriteBordersHeight"),sbh,3));
	float cc[4]={float((contour_color>>16)&0xFF)/255.0f,float((contour_color>>8)&0xFF)/255.0f,float((contour_color)&0xFF)/255.0f,float((contour_color>>24)&0xFF)/255.0f};
	D3D_HR(contoursCT->SetFloatArray(dxDevice,contoursCT->GetConstantByName(NULL,"ContourColor"),cc,4));
	D3D_HR(contoursCT->SetFloat(dxDevice,contoursCT->GetConstantByName(NULL,"ContourColorOffs"),color_offs));

	struct Vertex
	{
		FLOAT x,y,z,rhw;
		float tu,tv;
	} vb[6]=
	{
		{(float)borders.L-0.5f,(float)borders.B-0.5f,0.0f,1.0f,tuv.L,tuv.B},
		{(float)borders.L-0.5f,(float)borders.T-0.5f,0.0f,1.0f,tuv.L,tuv.T},
		{(float)borders.R-0.5f,(float)borders.B-0.5f,0.0f,1.0f,tuv.R,tuv.B},
		{(float)borders.L-0.5f,(float)borders.T-0.5f,0.0f,1.0f,tuv.L,tuv.T},
		{(float)borders.R-0.5f,(float)borders.T-0.5f,0.0f,1.0f,tuv.R,tuv.T},
		{(float)borders.R-0.5f,(float)borders.B-0.5f,0.0f,1.0f,tuv.R,tuv.B},
	};

	if(!contoursAdded) D3D_HR(dxDevice->Clear(0,NULL,D3DCLEAR_TARGET,0,0.9f,0));
	D3D_HR(dxDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST,2,(void*)vb,sizeof(Vertex)));

	// Restore 2d stream
	D3D_HR(dxDevice->SetDepthStencilSurface(ds));
	ds->Release();
	D3D_HR(dxDevice->SetRenderTarget(0,old_rt));
	old_rt->Release();
	D3D_HR(dxDevice->SetVertexShader(NULL));
	D3D_HR(dxDevice->SetPixelShader(NULL));
	D3D_HR(dxDevice->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE2X));
	D3D_HR(dxDevice->SetFVF(D3DFVF_MYVERTEX));
	D3D_HR(dxDevice->SetIndices(pIB));
	D3D_HR(dxDevice->SetStreamSource(0,pVB,0,sizeof(MYVERTEX)));
	contoursAdded=true;

//	if(anim3d)
//	{
//		LPDIRECT3DSURFACE surf=NULL;
//		if(FAILED(dxDevice->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&surf))) return false;
//		D3DXSaveSurfaceToFile(".\\someBack.png",D3DXIFF_PNG,surf,NULL,NULL);
//		D3DXSaveSurfaceToFile(".\\someZ.png",D3DXIFF_PNG,contours3dSurf,NULL,NULL);
//		D3DXSaveSurfaceToFile(".\\someMid.png",D3DXIFF_PNG,contoursMidTextureSurf,NULL,NULL);
//		D3DXSaveSurfaceToFile(".\\someCont.png",D3DXIFF_PNG,contoursTextureSurf,NULL,NULL);
//		exit(0);
//	}
	return true;
}

DWORD SpriteManager::GetSpriteContour(SpriteInfo* si, Sprite* spr)
{
	// Find created
	DWORD spr_id=(spr->PSprId?*spr->PSprId:spr->SprId);
	DwordMapIt it=createdSpriteContours.find(spr_id);
	if(it!=createdSpriteContours.end()) return (*it).second;

	// Create new
	LPDIRECT3DSURFACE surf;
	D3D_HR(si->Surf->Texture->GetSurfaceLevel(0,&surf));
	D3DSURFACE_DESC desc;
	D3D_HR(surf->GetDesc(&desc));
	RECT r={desc.Width*si->SprRect.L,desc.Height*si->SprRect.T,desc.Width*si->SprRect.R,desc.Height*si->SprRect.B};
	D3DLOCKED_RECT lr;
	D3D_HR(surf->LockRect(&lr,&r,D3DLOCK_READONLY));

	DWORD sw=si->Width;
	DWORD sh=si->Height;
	DWORD iw=sw+2;
	DWORD ih=sh+2;

	// Create FOnline fast format
	DWORD size=12+ih*iw*4;
	BYTE* data=new BYTE[size];
	memset(data,0,size);
	*((DWORD*)data)=MAKEFOURCC('F','0','F','A'); //FOnline FAst
	*((DWORD*)data+1)=iw;
	*((DWORD*)data+2)=ih;
	DWORD* ptr=(DWORD*)data+3+iw+1;

	// Write contour
	WriteContour4(ptr,iw,lr,sw,sh,D3DCOLOR_XRGB(0x7F,0x7F,0x7F));
	D3D_HR(surf->UnlockRect());
	surf->Release();

	// End
	SpriteInfo* contour_si=new SpriteInfo();
	contour_si->OffsX=si->OffsX;
	contour_si->OffsY=si->OffsY+1;
	int st=SurfType;
	SurfType=si->Surf->Type;
	DWORD result=FillSurfaceFromMemory(contour_si,data,size);
	SurfType=st;
	delete[] data;
	createdSpriteContours.insert(DwordMapVal(spr_id,result));
	return result;
}

#define GET_SURF_POINT(x,y) *((DWORD*)((BYTE*)r.pBits+r.Pitch*(y)+(x)*4))
#define SET_IMAGE_POINT(x,y) *(buf+(y)*buf_w+(x))=color
void SpriteManager::WriteContour4(DWORD* buf, DWORD buf_w, D3DLOCKED_RECT& r, DWORD w, DWORD h, DWORD color)
{
	for(DWORD y=0;y<h;y++)
	{
		for(DWORD x=0;x<w;x++)
		{
			DWORD p=GET_SURF_POINT(x,y);
			if(!p) continue;
			if(!x || !GET_SURF_POINT(x-1,y)) SET_IMAGE_POINT(x-1,y);
			if(x==w-1 || !GET_SURF_POINT(x+1,y)) SET_IMAGE_POINT(x+1,y);
			if(!y || !GET_SURF_POINT(x,y-1)) SET_IMAGE_POINT(x,y-1);
			if(y==h-1 || !GET_SURF_POINT(x,y+1)) SET_IMAGE_POINT(x,y+1);
		}
	}
}

void SpriteManager::WriteContour8(DWORD* buf, DWORD buf_w, D3DLOCKED_RECT& r, DWORD w, DWORD h, DWORD color)
{
	for(DWORD y=0;y<h;y++)
	{
		for(DWORD x=0;x<w;x++)
		{
			DWORD p=GET_SURF_POINT(x,y);
			if(!p) continue;
			if(!x || !GET_SURF_POINT(x-1,y)) SET_IMAGE_POINT(x-1,y);
			if(x==w-1 || !GET_SURF_POINT(x+1,y)) SET_IMAGE_POINT(x+1,y);
			if(!y || !GET_SURF_POINT(x,y-1)) SET_IMAGE_POINT(x,y-1);
			if(y==h-1 || !GET_SURF_POINT(x,y+1)) SET_IMAGE_POINT(x,y+1);
			if((!x && !y) || !GET_SURF_POINT(x-1,y-1)) SET_IMAGE_POINT(x-1,y-1);
			if((x==w-1 && !y) || !GET_SURF_POINT(x+1,y-1)) SET_IMAGE_POINT(x+1,y-1);
			if((x==w-1 && y==h-1) || !GET_SURF_POINT(x+1,y+1)) SET_IMAGE_POINT(x+1,y+1);
			if((y==h-1 && !x) || !GET_SURF_POINT(x-1,y+1)) SET_IMAGE_POINT(x-1,y+1);
		}
	}
}

/************************************************************************/
/* Fonts                                                                */
/************************************************************************/
#define FONT_BUF_LEN	    (0x5000)
#define FONT_MAX_LINES	    (1000)
#define MAX_FONT            (9)
#define FORMAT_TYPE_DRAW    (0)
#define FORMAT_TYPE_SPLIT   (1)
#define FORMAT_TYPE_LCOUNT  (2)

struct Letter
{
	WORD Dx;
	WORD Dy;
	BYTE W;
	BYTE H;
	short OffsH;

	Letter():Dx(0),Dy(0),W(0),H(0),OffsH(0){};
};

struct Font
{
	LPDIRECT3DTEXTURE FontSurf,FontSurfBordered;

	Letter Let[256];
	int SpaceWidth;
	int MaxLettHeight;
	int EmptyVer;
	int EmptyHor;
	FLOAT ArrXY[256][4];

	Font()
	{
		MaxLettHeight=0;
		SpaceWidth=0;
		EmptyHor=0;
		EmptyVer=0;
		FontSurf=NULL;
		FontSurfBordered=NULL;
	}
};

Font Fonts[MAX_FONT];
int DefFontIndex=-1;
DWORD DefFontColor=0;

Font* GetFont(int num)
{
	if(num<0) num=DefFontIndex;
	if(num<0 || num>=MAX_FONT) return NULL;
	return &Fonts[num];
}

struct FontFormatInfo
{
	Font* CurFont;
	DWORD Flags;
	INTRECT Rect;
	char Str[FONT_BUF_LEN];
	char* PStr;
	DWORD LinesAll;
	DWORD LinesInRect;
	int CurX;
	int CurY;
	DWORD ColorDots[FONT_BUF_LEN];
	short LineWidth[FONT_MAX_LINES];
	WORD LineSpaceWidth[FONT_MAX_LINES];
	DWORD OffsColDots;
	DWORD DefColor;
	StrVec* StrLines;
	bool IsError;

	void Init(Font* font, DWORD flags, INTRECT& rect, const char* str_in)
	{
		CurFont=font;
		Flags=flags;
		LinesAll=1;
		LinesInRect=0;
		IsError=false;
		CurX=0;
		CurY=0;
		Rect=rect;
		ZeroMemory(ColorDots,sizeof(ColorDots));
		ZeroMemory(LineWidth,sizeof(LineWidth));
		ZeroMemory(LineSpaceWidth,sizeof(LineSpaceWidth));
		OffsColDots=0;
		StringCopy(Str,str_in);
		PStr=Str;
		DefColor=COLOR_TEXT;
		StrLines=NULL;
	}
	FontFormatInfo& operator=(const FontFormatInfo& _fi)
	{
		CurFont=_fi.CurFont;
		Flags=_fi.Flags;
		LinesAll=_fi.LinesAll;
		LinesInRect=_fi.LinesInRect;
		IsError=_fi.IsError;
		CurX=_fi.CurX;
		CurY=_fi.CurY;
		Rect=_fi.Rect;
		memcpy(Str,_fi.Str,sizeof(Str));
		memcpy(ColorDots,_fi.ColorDots,sizeof(ColorDots));
		memcpy(LineWidth,_fi.LineWidth,sizeof(LineWidth));
		memcpy(LineSpaceWidth,_fi.LineSpaceWidth,sizeof(LineSpaceWidth));
		OffsColDots=_fi.OffsColDots;
		DefColor=_fi.DefColor;
		PStr=Str;
		StrLines=_fi.StrLines;
		return *this;
	}
};

void SpriteManager::SetDefaultFont(int index, DWORD color)
{
	DefFontIndex=index;
	DefFontColor=color;
}

bool SpriteManager::LoadFont(int index, const char* font_name, int size_mod)
{
	int tex_w=256*size_mod;
	int tex_h=256*size_mod;

	if(index>=MAX_FONT)
	{
		WriteLog(__FUNCTION__" - Invalid index.\n");
		return false;
	}
	Font& font=Fonts[index];

	BYTE* data=new BYTE[tex_w*tex_h*4]; // TODO: Leak
	if(!data)
	{
		WriteLog(__FUNCTION__" - Data allocation fail.\n");
		return false;
	}
	ZeroMemory(data,tex_w*tex_h*4);

	if(!fileMngr.LoadFile(Str::Format("%s.bmp",font_name),PT_ART_MISC))
	{
		WriteLog(__FUNCTION__" - File <%s> not found.\n",Str::Format("%s.bmp",font_name));
		delete[] data;
		return false;
	}

	LPDIRECT3DTEXTURE image=NULL;
	D3D_HR(D3DXCreateTextureFromFileInMemoryEx(dxDevice,fileMngr.GetBuf(),fileMngr.GetFsize(),D3DX_DEFAULT,D3DX_DEFAULT,1,0,
		D3DFMT_UNKNOWN,D3DPOOL_MANAGED,D3DX_DEFAULT,D3DX_DEFAULT,D3DCOLOR_ARGB(255,0,0,0),NULL,NULL,&image));

	D3DLOCKED_RECT lr;
	D3D_HR(image->LockRect(0,&lr,NULL,D3DLOCK_READONLY));

	if(!fileMngr.LoadFile(Str::Format("%s.fnt",font_name),PT_ART_MISC))
	{
		WriteLog(__FUNCTION__" - File <%s> not found.\n",Str::Format("%s.fnt",font_name));
		delete[] data;
		SAFEREL(image);
		return false;
	}

	font.EmptyHor=fileMngr.GetBEDWord();
	font.EmptyVer=fileMngr.GetBEDWord();
	font.MaxLettHeight=fileMngr.GetBEDWord();
	font.SpaceWidth=fileMngr.GetBEDWord();
	if(!fileMngr.CopyMem(font.Let,sizeof(Letter)*256))
	{
		WriteLog(__FUNCTION__" - Incorrect size in <%s> file.\n",Str::Format("%s.fnt",font_name));
		delete[] data;
		SAFEREL(image);
		return false;
	}

	D3DSURFACE_DESC sd;
	D3D_HR(image->GetLevelDesc(0,&sd));
	DWORD wd=sd.Width;

	int cur_x=0;
	int cur_y=0;
	for(int i=0;i<256;i++)
	{
		int w=font.Let[i].W;
		int h=font.Let[i].H;
		if(!w || !h) continue;

		if(cur_x+w+2>=tex_w)
		{
			cur_x=0;
			cur_y+=font.MaxLettHeight+2;
			if(cur_y+font.MaxLettHeight+2>=tex_h)
			{
				delete[] data;
				SAFEREL(image);
				//WriteLog("<%s> growed to %d\n",font_name,size_mod*2);
				return LoadFont(index,font_name,size_mod*2);
			}
		}

		for(int j=0;j<h;j++) memcpy(data+(cur_y+j+1)*tex_w*4+(cur_x+1)*4,(BYTE*)lr.pBits+(font.Let[i].Dy+j)*sd.Width*4+font.Let[i].Dx*4,w*4);

		font.ArrXY[i][0]=(FLOAT)cur_x/tex_w;
		font.ArrXY[i][1]=(FLOAT)cur_y/tex_h;
		font.ArrXY[i][2]=(FLOAT)(cur_x+w+2)/tex_w;
		font.ArrXY[i][3]=(FLOAT)(cur_y+h+2)/tex_h;
		cur_x+=w+2;
	}

	D3D_HR(image->UnlockRect(0));
	SAFEREL(image);

	// Create texture
	D3D_HR(D3DXCreateTexture(dxDevice,tex_w,tex_h,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&font.FontSurf));
	D3D_HR(font.FontSurf->LockRect(0,&lr,NULL,0)); // D3DLOCK_DISCARD
	memcpy(lr.pBits,data,tex_w*tex_h*4);
	WriteContour8((DWORD*)data,tex_w,lr,tex_w,tex_h,D3DCOLOR_ARGB(0xFF,0,0,0)); // Create border
	D3D_HR(font.FontSurf->UnlockRect(0));

	// Create bordered texture
	D3D_HR(D3DXCreateTexture(dxDevice,tex_w,tex_h,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&font.FontSurfBordered));
	D3D_HR(font.FontSurfBordered->LockRect(0,&lr,NULL,0)); // D3DLOCK_DISCARD
	memcpy(lr.pBits,data,tex_w*tex_h*4);
	D3D_HR(font.FontSurfBordered->UnlockRect(0));
	delete[] data;
	fileMngr.UnloadFile();
	return true;
}

bool SpriteManager::LoadFontAAF(int index, const char* font_name, int size_mod)
{
	int tex_w=256*size_mod;
	int tex_h=256*size_mod;

	// Read file in buffer
	if(index>=MAX_FONT)
	{
		WriteLog(__FUNCTION__" - Invalid index.\n");
		return false;
	}
	Font& font=Fonts[index];

	if(!fileMngr.LoadFile(font_name,PT_ART_MISC))
	{
		WriteLog(__FUNCTION__" - File <%s> not found.\n",font_name);
		return false;
	}

	// Check signature
	DWORD sign=fileMngr.GetBEDWord();
	if(sign!=MAKEFOURCC('F','F','A','A'))
	{
		WriteLog(__FUNCTION__" - Signature AAFF not found.\n");
		return false;
	}

	// Read params
	// ������������ ������ ����������� �������, ������� ����������� � ����������� ��������.
	font.MaxLettHeight=fileMngr.GetBEWord();
	// �������������� �����.
	// ����� (� ��������) ����� ��������� ������������� ��������.
	font.EmptyHor=fileMngr.GetBEWord();
	// ������ �������.
	// ������ ������� '������'.
	font.SpaceWidth=fileMngr.GetBEWord();
	// ������������ �����.
	// ����� (� ��������) ����� ����� �������� ��������.
	font.EmptyVer=fileMngr.GetBEWord();

	// Write font image
	const DWORD pix_light[9]={0x22808080,0x44808080,0x66808080,0x88808080,0xAA808080,0xDD808080,0xFF808080,0xFF808080,0xFF808080};
	BYTE* data=new BYTE[tex_w*tex_h*4];
	if(!data)
	{
		WriteLog(__FUNCTION__" - Data allocation fail.\n");
		return false;
	}
	ZeroMemory(data,tex_w*tex_h*4);
	BYTE* begin_buf=fileMngr.GetBuf();
	int cur_x=0;
	int cur_y=0;

	for(int i=0;i<256;i++)
	{
		Letter& l=font.Let[i];

		l.W=fileMngr.GetBEWord();
		l.H=fileMngr.GetBEWord();
		DWORD offs=fileMngr.GetBEDWord();
		l.OffsH=-(font.MaxLettHeight-l.H);

		if(cur_x+l.W+2>=tex_w)
		{
			cur_x=0;
			cur_y+=font.MaxLettHeight+2;
			if(cur_y+font.MaxLettHeight+2>=tex_h)
			{
				delete[] data;
				//WriteLog("<%s> growed to %d\n",font_name,size_mod*2);
				return LoadFontAAF(index,font_name,size_mod*2);
			}
		}

		BYTE* pix=&begin_buf[offs+0x080C];

		for(int h=0;h<l.H;h++)
		{
			DWORD* cur_data=(DWORD*)(data+(cur_y+h+1)*tex_w*4+(cur_x+1)*4);

			for(int w=0;w<l.W;w++,pix++,cur_data++)
			{
				int val=*pix;
				if(val>9) val=0;
				if(!val) continue;
				*cur_data=pix_light[val-1];
			}
		}

		font.ArrXY[i][0]=(FLOAT)cur_x/tex_w;
		font.ArrXY[i][1]=(FLOAT)cur_y/tex_h;
		font.ArrXY[i][2]=(FLOAT)(cur_x+int(l.W)+2)/tex_w;
		font.ArrXY[i][3]=(FLOAT)(cur_y+int(l.H)+2)/tex_h;
		cur_x+=l.W+2;
	}

	// Create texture
	D3D_HR(D3DXCreateTexture(dxDevice,tex_w,tex_h,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&font.FontSurf));
	D3DLOCKED_RECT lr;
	D3D_HR(font.FontSurf->LockRect(0,&lr,NULL,0)); // D3DLOCK_DISCARD
	memcpy(lr.pBits,data,tex_w*tex_h*4);
	WriteContour8((DWORD*)data,tex_w,lr,tex_w,tex_h,D3DCOLOR_ARGB(0xFF,0,0,0)); // Create border
	D3D_HR(font.FontSurf->UnlockRect(0));

	// Create bordered texture
	D3D_HR(D3DXCreateTexture(dxDevice,tex_w,tex_h,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&font.FontSurfBordered));
	D3D_HR(font.FontSurfBordered->LockRect(0,&lr,NULL,0)); // D3DLOCK_DISCARD
	memcpy(lr.pBits,data,tex_w*tex_h*4);
	D3D_HR(font.FontSurfBordered->UnlockRect(0));
	delete[] data;
	fileMngr.UnloadFile();
	return true;
}

void FormatText(FontFormatInfo& fi, int fmt_type)
{
	char* str=fi.PStr;
	DWORD flags=fi.Flags;
	Font* font=fi.CurFont;
	INTRECT& r=fi.Rect;
	int& curx=fi.CurX;
	int& cury=fi.CurY;
	DWORD& offs_col=fi.OffsColDots;

	if(fmt_type!=FORMAT_TYPE_DRAW && fmt_type!=FORMAT_TYPE_LCOUNT && fmt_type!=FORMAT_TYPE_SPLIT)
	{
		fi.IsError=true;
		return;
	}

	if(fmt_type==FORMAT_TYPE_SPLIT && !fi.StrLines)
	{
		fi.IsError=true;
		return;
	}

	// Colorize
	DWORD* dots=NULL;
	DWORD d_offs=0;
	char* str_=str;
	char* big_buf=Str::GetBigBuf();
	big_buf[0]=0;
	if(fmt_type==FORMAT_TYPE_DRAW && FLAG(flags,FT_COLORIZE)) dots=fi.ColorDots;

	while(*str_)
	{
		char* s0=str_;
		Str::GoTo(str_,'|');
		char* s1=str_;
		Str::GoTo(str_,' ');
		char* s2=str_;

		// TODO: optimize
		// if(!_str[0] && !*s1) break;
		if(dots)
		{
			DWORD d_len=(DWORD)s2-(DWORD)s1+1;
			DWORD d=strtoul(s1+1,NULL,0);

			dots[(DWORD)s1-(DWORD)str-d_offs]=d;
			d_offs+=d_len;
		}

		*s1=0;
		StringAppend(big_buf,0x10000,s0);

		if(!*str_) break;
		str_++;
	}

	StringCopy(str,FONT_BUF_LEN,big_buf);

	// Skip lines
	DWORD skip_line=(FLAG(flags,FT_SKIPLINES(0))?flags>>16:0);

	// Format
	curx=r.L;
	cury=r.T;

	for(int i=0;str[i];i++)
	{
		int lett_len;
		switch(str[i])
		{
		case '\r': continue;
		case ' ': lett_len=font->SpaceWidth; break;
		case '\t': lett_len=font->SpaceWidth*4; break;
		default: lett_len=font->Let[(BYTE)str[i]].W+font->EmptyHor; break;
		}

		if(curx+lett_len>r.R)
		{
			if(fmt_type==FORMAT_TYPE_DRAW && FLAG(flags,FT_NOBREAK))
			{
				str[i]='\0';
				break;
			}
			else if(FLAG(flags,FT_NOBREAK_LINE))
			{
				int j=i;
				for(;str[j];j++)
				{
					if(str[j]=='\n') break;
				}

				Str::EraseInterval(&str[i],j-i);
				if(fmt_type==FORMAT_TYPE_DRAW) for(int k=i,l=MAX_FOTEXT-(j-i);k<l;k++) fi.ColorDots[k]=fi.ColorDots[k+(j-i)];
			}
			else if(str[i]!='\n')
			{
				int j=i;
				for(;j>=0;j--)
				{
					if(str[j]==' ' || str[j]=='\t')
					{
						str[j]='\n';
						i=j;
						break;
					}
					else if(str[j]=='\n')
					{
						j=-1;
						break;
					}
				}

				if(j<0)
				{
					Str::Insert(&str[i],"\n");
					if(fmt_type==FORMAT_TYPE_DRAW) for(int k=MAX_FOTEXT-1;k>i;k--) fi.ColorDots[k]=fi.ColorDots[k-1];
				}

				if(FLAG(flags,FT_ALIGN) && !skip_line)
				{
					fi.LineSpaceWidth[fi.LinesAll-1]=1;
					// Erase next first spaces
					int ii=i+1;
					for(j=ii;;j++) if(str[j]!=' ') break;
					if(j>ii)
					{
						Str::EraseInterval(&str[ii],j-ii);
						if(fmt_type==FORMAT_TYPE_DRAW) for(int k=ii,l=MAX_FOTEXT-(j-ii);k<l;k++) fi.ColorDots[k]=fi.ColorDots[k+(j-ii)];
					}
				}
			}
		}

		switch(str[i])
		{
		case '\n':
			if(!skip_line)
			{
				cury+=font->MaxLettHeight+font->EmptyVer;
				if(cury+font->MaxLettHeight>r.B && !fi.LinesInRect) fi.LinesInRect=fi.LinesAll;

				if(fmt_type==FORMAT_TYPE_DRAW) 
				{
					if(fi.LinesInRect && !FLAG(flags,FT_UPPER))
					{
						//fi.LinesAll++;
						str[i]='\0';
						break;
					}
				}
				else if(fmt_type==FORMAT_TYPE_SPLIT)
				{
					if(fi.LinesInRect && !(fi.LinesAll%fi.LinesInRect))
					{
						str[i]='\0';
						(*fi.StrLines).push_back(str);
						str=&str[i+1];
						i=-1;
					}
				}

				if(str[i+1]) fi.LinesAll++;
			}
			else
			{
				skip_line--;
				Str::EraseInterval(str,i+1);
				offs_col+=i+1;
				//	if(fmt_type==FORMAT_TYPE_DRAW)
				//		for(int k=0,l=MAX_FOTEXT-(i+1);k<l;k++) fi.ColorDots[k]=fi.ColorDots[k+i+1];
				i=-1;
			}

			curx=r.L;
			continue;
		case '\0':
			break;
		default:
			curx+=lett_len;
			continue;
		}

		if(!str[i]) break;
	}

	if(skip_line)
	{
		fi.IsError=true;
		return;
	}

	if(!fi.LinesInRect) fi.LinesInRect=fi.LinesAll;

	if(fi.LinesAll>FONT_MAX_LINES)
	{
		fi.IsError=true;
		WriteLog("===%u\n",fi.LinesAll);
		return;
	}

	if(fmt_type==FORMAT_TYPE_SPLIT)
	{
		(*fi.StrLines).push_back(string(str));
		return;
	}
	else if(fmt_type==FORMAT_TYPE_LCOUNT)
	{
		return;
	}

	// Up text
	if(FLAG(flags,FT_UPPER) && fi.LinesAll>fi.LinesInRect)
	{
		int j=0;
		int line_cur=0;
		DWORD last_col=0;
		for(;str[j];++j)
		{
			if(str[j]=='\n')
			{
				line_cur++;
				if(line_cur>=(fi.LinesAll-fi.LinesInRect)) break;
			}

			if(fi.ColorDots[j]) last_col=fi.ColorDots[j];
		}

		if(FLAG(flags,FT_COLORIZE))
		{
			offs_col+=j+1;
			if(last_col && !fi.ColorDots[j+1]) fi.ColorDots[j+1]=last_col;
		}

		str=&str[j+1];
		fi.PStr=str;

		fi.LinesAll=fi.LinesInRect;
	}

	// Align
	curx=r.L;
	cury=r.T;

	for(int i=0;i<fi.LinesAll;i++) fi.LineWidth[i]=curx;

	bool can_count=false;
	int spaces=0;
	int curstr=0;
	for(int i=0;;i++)
	{
		switch(str[i]) 
		{
		case ' ':
			curx+=font->SpaceWidth;
			if(can_count) spaces++;
			break;
		case '\t':
			curx+=font->SpaceWidth*4;
			break;
		case '\0':
		case '\n':
			fi.LineWidth[curstr]=curx;
			cury+=font->MaxLettHeight+font->EmptyVer;
			curx=r.L;

			// Erase last spaces
			/*for(int j=i-1;spaces>0 && j>=0;j--)
			{
			if(str[j]==' ')
			{
			spaces--;
			str[j]='\r';
			}
			else if(str[j]!='\r') break;
			}*/

			// Align
			if(fi.LineSpaceWidth[curstr]==1 && spaces>0)
			{
				fi.LineSpaceWidth[curstr]=font->SpaceWidth+(r.R-fi.LineWidth[curstr])/spaces;
				//WriteLog("%d) %d + ( %d - %d ) / %d = %u\n",curstr,font->SpaceWidth,r.R,fi.LineWidth[curstr],spaces,fi.LineSpaceWidth[curstr]);
			}
			else fi.LineSpaceWidth[curstr]=font->SpaceWidth;

			curstr++;
			can_count=false;
			spaces=0;
			break;
		case '\r':
			break;
		default:
			curx+=font->Let[(BYTE)str[i]].W+font->EmptyHor;
			//if(curx>fi.LineWidth[curstr]) fi.LineWidth[curstr]=curx;
			can_count=true;
			break;
		}

		if(!str[i]) break;
	}

	curx=r.L;
	cury=r.T;

	// Align X
	if(FLAG(flags,FT_CENTERX)) curx+=(r.R-fi.LineWidth[0])/2;
	else if(FLAG(flags,FT_CENTERR)) curx+=r.R-fi.LineWidth[0];
	// Align Y
	if(FLAG(flags,FT_CENTERY)) cury=r.T+(r.H()-fi.LinesAll*font->MaxLettHeight-(fi.LinesAll-1)*font->EmptyVer)/2+1;
	else if(FLAG(flags,FT_BOTTOM)) cury=r.B-(fi.LinesAll*font->MaxLettHeight+(fi.LinesAll-1)*font->EmptyVer);
}

bool SpriteManager::DrawStr(INTRECT& r, const char* str, DWORD flags, DWORD col /* = 0 */, int num_font /* = -1 */)
{
	// Check
	if(!str || !str[0]) return false;

	// Get font
	Font* font=GetFont(num_font);
	if(!font) return false;

	// Format
	if(!col && DefFontColor) col=DefFontColor;

	static FontFormatInfo fi;
	fi.Init(font,flags,r,str);
	fi.DefColor=col;
	FormatText(fi,FORMAT_TYPE_DRAW);
	if(fi.IsError) return false;

	char* str_=fi.PStr;
	DWORD offs_col=fi.OffsColDots;
	int curx=fi.CurX;
	int cury=fi.CurY;
	int curstr=0;

	if(curSprCnt) Flush();
	callVec.clear();
	lastCall=NULL;
	curTexture=NULL;

	D3D_HR(dxDevice->SetTexture(0,FLAG(flags,FT_BORDERED)?font->FontSurfBordered:font->FontSurf));

	if(FLAG(flags,FT_COLORIZE))
	{
		for(int i=offs_col;i;i--)
		{
			if(fi.ColorDots[i])
			{
				col=fi.ColorDots[i];
				break;
			}
		}
	}

	bool variable_space=false;
	for(int i=0;str_[i];i++)
	{
		if(FLAG(flags,FT_COLORIZE) && fi.ColorDots[i+offs_col]) col=fi.ColorDots[i+offs_col];

		switch(str_[i]) 
		{
		case ' ':
			curx+=(variable_space?fi.LineSpaceWidth[curstr]:font->SpaceWidth);
			continue;
		case '\t':
			curx+=font->SpaceWidth*4;
			continue;
		case '\n':
			cury+=font->MaxLettHeight+font->EmptyVer;
			curx=r.L;
			curstr++;
			variable_space=false;
			if(FLAG(flags,FT_CENTERX)) curx+=(r.R-fi.LineWidth[curstr])/2;
			else if(FLAG(flags,FT_CENTERR)) curx+=r.R-fi.LineWidth[curstr];
			continue;
		case '\r':
			continue;
		default:
			int mulpos=curSprCnt*4;
			int x=curx-1;
			int y=cury-font->Let[(BYTE)str_[i]].OffsH-1;
			int w=font->Let[(BYTE)str_[i]].W+2;
			int h=font->Let[(BYTE)str_[i]].H+2;

			FLOAT x1=font->ArrXY[(BYTE)str_[i]][0];
			FLOAT y1=font->ArrXY[(BYTE)str_[i]][1];
			FLOAT x2=font->ArrXY[(BYTE)str_[i]][2];
			FLOAT y2=font->ArrXY[(BYTE)str_[i]][3];

			waitBuf[mulpos].x=x-0.5f;
			waitBuf[mulpos].y=y+h-0.5f;
			waitBuf[mulpos].tu=x1;
			waitBuf[mulpos].tv=y2;
			waitBuf[mulpos++].Diffuse=col;

			waitBuf[mulpos].x=x-0.5f;
			waitBuf[mulpos].y=y-0.5f;
			waitBuf[mulpos].tu=x1;
			waitBuf[mulpos].tv=y1;
			waitBuf[mulpos++].Diffuse=col;

			waitBuf[mulpos].x=x+w-0.5f;
			waitBuf[mulpos].y=y-0.5f;
			waitBuf[mulpos].tu=x2;
			waitBuf[mulpos].tv=y1;
			waitBuf[mulpos++].Diffuse=col;

			waitBuf[mulpos].x=x+w-0.5f;
			waitBuf[mulpos].y=y+h-0.5f;
			waitBuf[mulpos].tu=x2;
			waitBuf[mulpos].tv=y2;
			waitBuf[mulpos++].Diffuse=col;

			curSprCnt++;
			if(curSprCnt==flushSprCnt) Flush();
			curx+=font->Let[(BYTE)str_[i]].W+font->EmptyHor;
			variable_space=true;
		}
	}
	Flush();
	return true;
}

int SpriteManager::GetLinesCount(int width, int height, const char* str, int num_font)
{
	Font* font=GetFont(num_font);
	if(!font) return 0;

	if(!str) return height/(font->MaxLettHeight+font->EmptyVer);
	if(!str[0]) return 0;

	static FontFormatInfo fi;
	fi.Init(font,0,INTRECT(0,0,width?width:modeWidth,height?height:modeHeight),str);
	FormatText(fi,FORMAT_TYPE_LCOUNT);
	if(fi.IsError) return 0;
	return fi.LinesInRect;
}

int SpriteManager::GetLinesHeight(int width, int height, const char* str, int num_font)
{
	Font* font=GetFont(num_font);
	if(!font) return 0;
	int cnt=GetLinesCount(width,height,str,num_font);
	if(cnt<=0) return 0;
	return cnt*font->MaxLettHeight+(cnt-1)*font->EmptyVer;
}

int SpriteManager::SplitLines(INTRECT& r, const char* cstr, int num_font, StrVec& str_vec)
{
	// Check & Prepare
	str_vec.clear();
	if(!cstr || !cstr[0]) return 0;

	// Get font
	Font* font=GetFont(num_font);
	if(!font) return 0;
	static FontFormatInfo fi;
	fi.Init(font,0,r,cstr);
	fi.StrLines=&str_vec;
	FormatText(fi,FORMAT_TYPE_SPLIT);
	if(fi.IsError) return 0;
	return str_vec.size();
}

/************************************************************************/
/*                                                                      */
/************************************************************************/