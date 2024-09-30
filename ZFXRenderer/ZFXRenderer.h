// This file contain the declaration of class ZFXRenderer. It decide which DLL to load
#pragma once
#ifndef ZFXRENDERDEVICE_H
#define ZFXRENDERDEVICE_H

#include "ZFXRenderDevice.h"

class ZFXRenderer
{
public:
	ZFXRenderer(HINSTANCE hInst);
	~ZFXRenderer(void);

	HRESULT				CreateDevice(const char* chAPI);
	void				Release(void);
	LPZFXRENDERDEVICE	GetDevice(void) { return m_pDevice; }
	HINSTANCE			GetModule(void) { return m_hDLL; }

private:
	ZFXRenderDevice*	m_pDevice;
	HINSTANCE			m_hInst;
	HMODULE				m_hDLL;
}; // class
typedef struct ZFXRenderer* LPZFXRENDERER;

#endif