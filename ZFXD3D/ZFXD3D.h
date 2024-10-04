#pragma once

#include <D3D9.h>
#include <d3dx9.h>
#include <ZFXRenderDevice.h>

#define MAX_3DHWND 8
#define MAX_SHADER 20

#define IDENTITY(m) { memset(&m, 0, sizeof(D3DMATRIX)); m._11 = m._22 = m._33 = m._44 = 1.0f; }

// vertex type definitions
#define FVF_PVERTEX ( D3DFVF_XYZ )
#define FVF_VERTEX ( D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 )
#define FVF_LVERTEX ( D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 )
#define FVF_CVERTEX ( D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 )
#define FVF_T3VERTEX ( D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX3 )
#define FVF_TVERTEX ( D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE3(0) )

BOOL WINAPI DllEntryPoint(HINSTANCE hDll, DWORD fdwReason, LPVOID lpvRserved);

extern "C"	__declspec(dllexport) HRESULT CreateRenderDevice(HINSTANCE hDll, ZFXRenderDevice **pInterface);
extern "C"	__declspec(dllexport) HRESULT ReleaseRenderDevice(ZFXRenderDevice **pInterface);

// one for each AdapterFormat-BackbufferFormat-WindowMode
// (windowed or fullscreen) combination that is valid
struct ZFXCOMBOINFO 
{
	UINT					nAdapter;			// belongs to
	D3DDEVTYPE				d3dDevType;			// HAL, SW, REF
	bool					bWindowed;			// windowed mode

	D3DFORMAT				fmtAdapter;			// pixelbuffer
	D3DFORMAT				fmtBackBuffer;		// backbuffer
	D3DFORMAT				fmtDepthStencil;	// z/stencil format

	DWORD					dwBehavior;			// vertex processing
	D3DMULTISAMPLE_TYPE		msType;				// multisample type
};
/*---------------------------------------------------------*/

// up to three for each adapter
struct ZFXDEVICEINFO
{
	UINT					nAdapter;			// belongs to
	D3DDEVTYPE				d3dDevType;			// HAL, SW, REF
	D3DCAPS9				d3dCaps;			// capabilities
	ZFXCOMBOINFO			d3dCombo[80];		// combo
	UINT					nNumCombo;			// number of combos
};
/*---------------------------------------------------------*/

struct ZFXADAPTERINFO
{
	D3DADAPTER_IDENTIFIER9	d3dAdapterIdentifier;
	UINT					nAdapter;			// which one
	D3DDISPLAYMODE			d3ddspmd[150];		// display modes
	UINT					nNumModes;			// number of modes
	ZFXDEVICEINFO			d3dDevs[3];			// list of devices
	UINT					nNumDevs;			// number of devices
};
/*---------------------------------------------------------*/

// enumeration
class ZFXD3DEnum
{
	public:
		ZFXD3DEnum(int nMinDepth, int nMinStencil)
		{
			m_nMinDepth = nMinDepth;
			m_nMinStencil = nMinStencil;
		}
		~ZFXD3DEnum(void) {}

		// enumerate all stuff
		HRESULT Enum(HWND, HWND, HWND, HWND, HWND, HWND, HWND, FILE*);

		// combobox selection changed
		void ChangedAdapter(void);
		void ChangedDevice(void);
		void ChangedAdapterFmt(void);
		void ChangedWindowMode(void);

		// get final settings to crank up
		void GetSelections(ZFXDEVICEINFO* pD, D3DDISPLAYMODE* dspmd, D3DFORMAT* fmtA, D3DFORMAT* fmtB);

		LPDIRECT3D9		m_pD3D;
		ZFXADAPTERINFO	m_xAdapterInfo[10];
		DWORD			m_dwNumAdapters;

	private:
		D3DDISPLAYMODE	m_dspmd;			// current desktop display mode
		D3DFORMAT		m_fmtAdapter[5];	// list of possible adapter formats
		UINT			m_nNumFmt;			// adapter formats possible
		UINT			m_nMinWidth;		// minimum screen width
		UINT			m_nMinHeigth;		// minimum screen height
		UINT			m_nMinBits;			// minimum backbuffer bits
		UINT			m_nMinDepth;		// minimum depth bits
		UINT			m_nMinStencil;		// minimum sctencil bits
		FILE*			m_pLog;				// log file opened by zfxd3d class

		// handle to GUI items given from zfxd3d dialog
		HWND			m_hADAPTER;			// adapter combobox
		HWND			m_hMODE;			// mode combobox
		HWND			m_hDEVICE;			// device combobox
		HWND			m_hADAPTERFMT;		// adapter format combobox
		HWND			m_hBACKFMT;			// backbuffer format combobox
		HWND			m_hWND;				// radiobtn windowed
		HWND			m_hFULL;			// radiobtn fullscreen

		HRESULT			EnumAdapters(void);
		HRESULT			EnumDevices(ZFXADAPTERINFO&);
		HRESULT			EnumCombos(ZFXDEVICEINFO&);

		UINT			GetBits(D3DFORMAT);
		HRESULT			ConfirmDevice(D3DCAPS9*, DWORD, D3DFORMAT);
		bool			ConfirmDepthFmt(ZFXCOMBOINFO*);

		// helper functions
		void			AddItem(HWND hWnd, const char* ch, void* pData);
		void*			GetSelectedItem(HWND hWnd);
		bool			ContainsString(HWND hWnd, const char* ch);
		const char*		D3DDevTypeToString(D3DDEVTYPE devType);
		const char*		D3DFormatToString(D3DFORMAT format);
		const char*		BehaviorTypeToString(DWORD vpt);
};

class ZFXD3D : public ZFXRenderDevice
{
	public:
		ZFXD3D(HINSTANCE hDLL);
		~ZFXD3D(void);

		// Initialization
		HRESULT Init(HWND, const HWND*, int, int, int, bool);
		BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
		HRESULT InitWindowed(HWND, const HWND*, int, bool);
		HRESULT OneTimeInit(void);

		// Interface functions
		UINT	GetActiveSkinID(void) { return m_nActiveSkin; }
		void	SetActiveSkinID(UINT nID) { m_nActiveSkin = nID; }
		void	Release(void);
		bool	IsRunning(void) { return m_bRunning; }
		void	GetResolution(POINT* pPt) { pPt->x = m_dwWidth; pPt->y = m_dwHeight; }
		HRESULT BeginRendering(bool, bool, bool);
		HRESULT Clear(bool, bool, bool);
		void	EndRendering(void);
		void	SetClearColor(float, float, float);
		HRESULT UseWindow(UINT nHwnd);

		ZFXSkinManager* GetSkinManager() { return m_pSkinMan; }
		ZFXVertexCacheManager* GetVertexCacheManager() { return m_pVertexMan; }

		// View functions
		HRESULT	SetView3D(const ZFXVector&, const ZFXVector&, const ZFXVector&, const ZFXVector&);
		HRESULT SetViewLookAt(const ZFXVector&, const ZFXVector&, const ZFXVector&);
		void	SetClippingPlanes(float, float);
		HRESULT	SetMode(ZFXENGINEMODE, int);
		HRESULT InitStage(float, ZFXVIEWPORT*, int n);
		HRESULT GetFrustum(ZFXPlane*);
		void	Transform2DTo3D(const POINT&, ZFXVector*, ZFXVector*);
		POINT	Transform3DTo2D(const ZFXVector&);
		void	SetWorldTransform(const ZFXMatrix*);

		// Shader functions
		HRESULT CreateVShader(const void*, UINT, bool, bool, UINT*);
		HRESULT	CreatePShader(const void*, UINT, bool, bool, UINT*);
		HRESULT ActivateVShader(UINT, ZFXVERTEXID);
		HRESULT ActivatePShader(UINT);
		bool	UsesShaders(void) { return m_bUseShaders; }
		void	UsesShaders(bool bUsesShaders) { m_bUseShaders = bUsesShaders; }
		bool	UsesAdditiveBlending(void) { return m_bAdditive; }
		void	UsesAdditiveBlending(bool bAdditive);
		bool	CanDoShaders(void) { return m_bCanDoShaders; }
		void	CanDoShaders(bool bCanDoShaders) { m_bCanDoShaders = bCanDoShaders; }

		// Rendering functions
		void			SetBackfaceCulling(ZFXRENDERSTATE);
		void			SetDepthBufferMode(ZFXRENDERSTATE);
		ZFXCOLOR		GetWireColor(void) { return m_clrWire; }
		void			SetShadeMode(ZFXRENDERSTATE, float, const ZFXCOLOR*);
		ZFXRENDERSTATE	GetShadeMode(void) { return m_ShadeMode; }
		void			SetAmbientLight(float fRed, float fGreen, float fBlue);

		// fonts and text
		HRESULT	CreateFont(const char*, int, bool, bool, bool, DWORD, UINT*);
		HRESULT DrawText(UINT, int, int, UCHAR, UCHAR, UCHAR, const char*, ...);

	private:
		ZFXD3DEnum*				m_pEnum;
		LPDIRECT3D9				m_pD3D;
		LPDIRECT3DDEVICE9		m_pDevice;
		LPDIRECT3DSWAPCHAIN9	m_pChain[MAX_3DHWND];
		D3DPRESENT_PARAMETERS	m_d3dpp;
		D3DCOLOR				m_ClearColor;
		UINT					m_nActiveSkin;
		bool					m_bIsScreenRunning;
		bool					m_bStencil;

		// start the API
		HRESULT Go(void);

		void Log(const char*, ...);

		// view stuff
		D3DMATRIX	m_mView2D,			// viewmatrix 2D
					m_mView3D,			// viewmatrix 3D
					m_mProj2D,			// projection orthog.
					m_mProjP[4],		// projection persp.
					m_mProj0[4],		// projection orthog.
					m_mWorld,			// world transformation
					m_mViewProj,		// combo-matrix for 3D
					m_mWorldViewProj;	// combo-matrix for 3D
		void	Prepare2D(void);
		void	CalcViewProjMatrix(void);
		void	CalcWorldViewProjMatrix(void);
		HRESULT	CalcPerspProjMatrix(float, float, D3DMATRIX*);

		// shader stuff
		LPDIRECT3DVERTEXDECLARATION9	m_pDeclVertex;
		LPDIRECT3DVERTEXDECLARATION9	m_pDeclLVertex;
		LPDIRECT3DVERTEXDECLARATION9	m_pDeclPVertex;
		LPDIRECT3DVERTEXDECLARATION9	m_pDeclCVertex;
		LPDIRECT3DVERTEXDECLARATION9	m_pDecl3TVertex;
		LPDIRECT3DVERTEXDECLARATION9	m_pDeclTVertex;

		LPDIRECT3DVERTEXSHADER9		m_pVShader[MAX_SHADER];
		LPDIRECT3DPIXELSHADER9		m_pPShader[MAX_SHADER];
		UINT								m_nNumVShaders;
		UINT								m_nNumPShaders;

		D3DMATERIAL9					m_StdMtrl;

		void PrepareShaderStuff(void);

		// font
		LPD3DXFONT* m_pFont;		// font objects
		UINT		m_nNumFonts;	// number of fonts
}; // class