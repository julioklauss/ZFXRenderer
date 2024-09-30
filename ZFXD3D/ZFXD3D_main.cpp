#include "ZFXD3D.h"

bool g_bLF;

HRESULT ZFXD3D::UseWindow(UINT nHwnd)
{
	LPDIRECT3DSURFACE9 pBack = NULL;

	if (!m_d3dpp.Windowed)
		return ZFX_OK;
	else if (nHwnd >= m_nNumhWnd)
		return ZFX_FAIL;

	// try to get the appropriate back buffer
	if (FAILED(m_pChain[nHwnd]->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pBack)))
		return ZFX_FAIL;

	// and activate it for the device
	m_pDevice->SetRenderTarget(0, pBack);
	pBack->Release();
	m_nActiveWnd = nHwnd;
	return ZFX_OK;
}

HRESULT ZFXD3D::BeginRendering(bool bClearPixel, bool bClearDepth, bool bClearStencil)
{
	DWORD dw = 0;

	// anything to be cleared?
	if (bClearPixel || bClearDepth || bClearStencil)
	{
		if (bClearPixel)	
			dw |= D3DCLEAR_TARGET;
		
		if (bClearDepth)	
			dw |= D3DCLEAR_ZBUFFER;

		if (bClearStencil && m_bStencil)
			dw |= D3DCLEAR_STENCIL;

		if (FAILED(m_pDevice->Clear(0, NULL, dw, m_ClearColor, 1.0f, 0)))
			return ZFX_FAIL;
	}

	if (FAILED(m_pDevice->BeginScene()))
		return ZFX_FAIL;

	m_bIsScreenRunning = true;
	return ZFX_OK;
}	// BeginRendering
/*--------------------*/

HRESULT ZFXD3D::Clear(bool bClearPixel, bool bClearDepth, bool bClearStencil)
{
	DWORD dw = 0;

	if (bClearPixel)	
		dw |= D3DCLEAR_TARGET;
	
	if (bClearDepth)	
		dw != D3DCLEAR_ZBUFFER;

	if (bClearStencil && m_bStencil)
		dw |= D3DCLEAR_STENCIL;

	if (m_bIsScreenRunning)
		m_pDevice->EndScene();

	if (FAILED(m_pDevice->Clear(0, NULL, dw, m_ClearColor, 1.0f, 0)))
		return ZFX_FAIL;

	if (m_bIsScreenRunning)
		m_pDevice->BeginScene();
}	// Clear
/*--------------------*/

void ZFXD3D::EndRendering(void)
{
	m_pDevice->EndScene();
	m_pDevice->Present(NULL, NULL, NULL, NULL);
	m_bIsScreenRunning = false;
}	// EndRendering
/*--------------------*/

void ZFXD3D::SetClearColor(float fRed, float fGreen, float fBlue)
{
	m_ClearColor = D3DCOLOR_COLORVALUE(fRed, fGreen, fBlue, 1.0f);
}	// SetClearColor
/*--------------------*/