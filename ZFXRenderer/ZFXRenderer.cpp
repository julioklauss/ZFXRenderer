// ZFXRenderer.cpp : Defines the functions for the static library.
//
// This file contains the implemetation of class ZFXRenderer

#include "ZFXRenderer.h"

ZFXRenderer::ZFXRenderer(HINSTANCE hInst)
{
	m_hInst		= hInst;
	m_hDLL		= NULL;
	m_pDevice	= NULL;
}

ZFXRenderer::~ZFXRenderer(void)
{
	Release();
}

HRESULT ZFXRenderer::CreateDevice(const char* chAPI)
{
	char buffer[300];

	if (strcmp(chAPI, "Direct3D") == 0)
	{
		m_hDLL = LoadLibrary("ZFXD3D.dll");
		if (!m_hDLL)
		{
			MessageBox(NULL,
				"Loading ZFXD3D.dll failed.",
				"ZFXEngine - error", MB_OK | MB_ICONERROR);
			return E_FAIL;
		}
	}
	else
	{
		_snprintf_s(buffer, 300, "API '%s' not supported.", chAPI);
		MessageBox(NULL, buffer, "ZFXEngine - error", MB_OK | MB_ICONERROR);
		return E_FAIL;
	}

	CREATERENDERDEVICE _CreateRenderDevice = 0;
	HRESULT hr;

	// pointer to DLL function "CreateRenderDevice"
	_CreateRenderDevice = (CREATERENDERDEVICE)GetProcAddress(m_hDLL, "CreateRenderDevice");
	if (!_CreateRenderDevice) return E_FAIL;

	// call DLL function to create the device
	hr = _CreateRenderDevice(m_hDLL, &m_pDevice);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			"CreateRenderDevice() from lib failed.",
			"ZFXEngine - error", MB_OK | MB_ICONERROR);
		m_pDevice = NULL;
		return E_FAIL;
	}

	return S_OK;
} // CreateDevice

void ZFXRenderer::Release(void)
{
	RELEASERENDERDEVICE _ReleaseRenderDevice = 0;
	HRESULT hr;

	if (m_hDLL)
	{
		// pointer to dll function "ReleaseRenderDevice"
		_ReleaseRenderDevice = (RELEASERENDERDEVICE)GetProcAddress(m_hDLL, "ReleaseRenderDevice");
	}

	// call dll's release function
	if (m_pDevice)
	{
		hr = _ReleaseRenderDevice(&m_pDevice);
		if (FAILED(hr))
		{
			m_pDevice = NULL;
		}
	}
} // Release