// This file take the definition of the interface, a.k.a the abstract class the implementation in DLL derived from
#pragma once
#ifndef ZFXRENDERINTERFACE_H
#define ZFXRENDERINTERFACE_H

#include <Windows.h>
#include <cstdio>
#include "ZFX_skinman.h"
#include "ZFX_vcache.h"
#include <ZFX3D.h>
#include "ZFX.h"

#define MAX_3DHWND 8

class ZFXRenderDevice
{
	protected:
		// INIT
		HWND		m_hWndMain;				// main window
		HWND		m_hWnd[MAX_3DHWND];		// render windows
		UINT		m_nNumhWnd;				// number of render windows
		UINT		m_nActiveWnd;			// active window
		HINSTANCE	m_hDLL;					// DLL module
		DWORD		m_dwWidth;				// screen width
		DWORD		m_dwHeight;				// screen height
		bool		m_bWindowed;			// windowed mode?
		char		m_chAdapter[256];		// graphics adapter name
		FILE		*m_pLog;				// logfile
		bool		m_bRunning;

		// MANAGER
		ZFXSkinManager* m_pSkinMan;				// material and textures
		ZFXVertexCacheManager* m_pVertexMan;	// vertex cache manager

		// VIEW 
		float			m_fNear,			// near plane
						m_fFar;				// far plane
		ZFXENGINEMODE	m_Mode;				// 2D, 3D, ...
		int				m_nStage;			// stage (0 - 3)
		ZFXVIEWPORT		m_VP[4];			// viewports

		// RENDERING
		bool			m_bUseShaders;		// shaders or fixed function pipeline
		bool			m_bCanDoShaders;	// can we use shaders at all?
		bool			m_bAdditive;		// use additive rendering
		ZFXCOLOR		m_clrWire;			// color for wireframe rendering
		ZFXRENDERSTATE	m_ShadeMode;		// wireframe rendering?
	
	public:
		ZFXRenderDevice(void) {};
		virtual ~ZFXRenderDevice(void) {};

		// INIT/RELEASE STUFF:
		// -------------------
		virtual HRESULT Init(HWND, const HWND*, int, int, int, bool) = 0;
		virtual void	Release(void) = 0;
		virtual bool	IsRunning(void) = 0;
		virtual void	GetResolution(POINT*) = 0;

		// RENDERER STUFF:
		// ----------------
		virtual HRESULT UseWindow(UINT nHwnd) = 0;
		virtual HRESULT BeginRendering(bool bClearPixel, bool bClearDepth, bool bClearStencil) = 0;
		virtual void	EndRendering(void) = 0;
		virtual HRESULT Clear(bool bClearPixel, bool bClearDepth, bool bClearStencil) = 0;
		virtual void	SetClearColor(float fRed, float fGreen, float fBlue) = 0;

		// MANAGER GETTER:
		// ----------------
		virtual ZFXSkinManager* GetSkinManager() = 0;
		virtual ZFXVertexCacheManager* GetVertexCacheManager() = 0;

		// VIEW STUFF:
		// ----------------
		// viewmatrix from vRight, vUp, vDir, vPos
		virtual HRESULT	SetView3D(const ZFXVector&, const ZFXVector&, const ZFXVector&, const ZFXVector&) = 0;

		// viewmatrix from position, fix point, WorldUp
		virtual HRESULT	SetViewLookAt(const ZFXVector&, const ZFXVector&, const ZFXVector&) = 0;

		// near and far clipping plane
		virtual void	SetClippingPlanes(float, float) = 0;

		// stage modus, 0 := perspection, 1 := ortho
		virtual HRESULT	SetMode(ZFXENGINEMODE, int n) = 0;

		// field of view and viewport for stage n
		virtual HRESULT	InitStage(float, ZFXVIEWPORT*, int n) = 0;

		// plane of the viewing frustum
		virtual HRESULT	GetFrustum(ZFXPlane*) = 0;

		// screen coordinates to world ray
		virtual void	Transform2DTo3D(const POINT& pt, ZFXVector* vc0, ZFXVector* vcD) = 0;

		// world coordinates to screen coordinates
		virtual POINT	Transform3DTo2D(const ZFXVector& vcP) = 0;

		// set world transformation matrix or NULL
		virtual void	SetWorldTransform(const ZFXMatrix*) = 0;

		// SHADER STUFF:
		// ----------------
		virtual HRESULT CreateVShader(const void*, UINT, bool, bool, UINT*) = 0;
		virtual HRESULT	CreatePShader(const void*, UINT, bool, bool, UINT*) = 0;
		virtual HRESULT ActivateVShader(UINT, ZFXVERTEXID) = 0;
		virtual HRESULT ActivatePShader(UINT) = 0;
		virtual bool	UsesShaders(void) = 0;
		virtual void	UsesShaders(bool) = 0;
		virtual bool	CanDoShaders(void) = 0;
		virtual void	CanDoShaders(bool) = 0;

		// RENDERING STUFF:
		// ----------------
		virtual void	SetBackfaceCulling(ZFXRENDERSTATE) = 0;
		virtual void	SetDepthBufferMode(ZFXRENDERSTATE) = 0;

		virtual void	SetShadeMode(ZFXRENDERSTATE, float, const ZFXCOLOR*) = 0;
		virtual ZFXRENDERSTATE	GetShadeMode(void) = 0;

		virtual HRESULT CreateFont(const char*, int, bool, bool, bool, DWORD, UINT*) = 0;
		virtual HRESULT DrawText(UINT, int, int, UCHAR, UCHAR, UCHAR, char*, ...) = 0;
		virtual void	SetAmbientLight(float fRed, float fGreen, float fBlue) = 0;

		virtual void	UsesAdditiveBlending(bool) = 0;
		virtual bool	UsesAdditiveBlending(void) = 0;
};	// class
typedef struct ZFXRenderDevice* LPZFXRENDERDEVICE;

extern "C"
{
	typedef HRESULT(*CREATERENDERDEVICE) (HINSTANCE hDLL, ZFXRenderDevice** pInterface);
	typedef HRESULT(*RELEASERENDERDEVICE) (ZFXRenderDevice** pInterface);
}

#endif