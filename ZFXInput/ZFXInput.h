#include "ZFXInputDevice.h"

#pragma once

class ZFXInput
{
	public:
		ZFXInput(HINSTANCE hInst);
		~ZFXInput();

		HRESULT				CreateDevice();
		LPZFXINPUTEDEVICE	GetDevice() { return m_pDevice; }
		HINSTANCE			GetModule() { return m_hDLL; }
		void				Release();

	private:
		ZFXInputDevice*		m_pDevice;
		HINSTANCE			m_hInst;
		HMODULE				m_hDLL;
};	//	class

typedef class ZFXInput* LPZFXINPUT;