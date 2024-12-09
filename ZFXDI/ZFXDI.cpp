#include "ZFXDI.h"

void ZFXDIDevice::Create(LPDIRECTINPUT8 pDI, HWND hWnd, FILE* pLog)
{
	m_pLog		= pLog;
	m_hWnd		= hWnd;
	m_pDI		= pDI;
	m_pDevice	= NULL;
}

void ZFXDIDevice::Release(void)
{
	if (m_pDevice) {
		m_pDevice->Unacquire();
		m_pDevice->Release();
		m_pDevice = NULL;
	}
}	//	Release

HRESULT ZFXDIDevice::CrankUp(REFGUID rguid, LPCDIDATAFORMAT pdf)
{
	DWORD dwFlags = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE;

	// if already existing destroy it
	if (m_pDevice) {
		m_pDevice->Unacquire();
		m_pDevice->Release();
		m_pDevice = NULL;
	}

	// 1. Step: create the device
	if (FAILED(m_pDI->CreateDevice(rguid, &m_pDevice, NULL)))
		return ZFX_FAIL;

	// 2. Step: define the right data format
	if (FAILED(m_pDevice->SetDataFormat(pdf)))
		return ZFX_FAIL;

	// 3. Step: set the cooperative level
	if (FAILED(m_pDevice->SetCooperativeLevel(m_hWnd, dwFlags)))
		return ZFX_FAIL;

	return ZFX_OK;
}	//	CrankUp

HRESULT ZFXDIDevice::GetData(ZFXINPUTDEV Type, void* pData, DWORD* pdwNum)
{
	HRESULT	hr = ZFX_FAIL;
	size_t	size = 0;

	// is this a mouse or a keyboard/joystick?
	if (Type == IDV_MOUSE) {
		size = sizeof(DIDEVICEOBJECTDATA);
		hr = m_pDevice->GetDeviceData(size, (DIDEVICEOBJECTDATA*)pData, pdwNum, 0);
	}
	else {
		if (Type == IDV_KEYBOARD)
			size = sizeof(char) * 256;
		else size = sizeof(DIJOYSTATE);
		hr = m_pDevice->GetDeviceState(size, pData);
	}

	// query failed?
	if (FAILED(hr)) {
		// device acquired at all?
		if ((hr == DIERR_NOTACQUIRED) || (hr == DIERR_INPUTLOST)) {
			hr = m_pDevice->Acquire();
			while (hr == DIERR_INPUTLOST)
				hr = m_pDevice->Acquire();

			// another application has priority!
			if (hr == DIERR_OTHERAPPHASPRIO)
				return ZFX_OK;

			// we got it back
			if (SUCCEEDED(hr)) {
				if (Type == IDV_MOUSE)
					hr = m_pDevice->GetDeviceData(size, (DIDEVICEOBJECTDATA*)pData, pdwNum, 0);
				else
					hr = m_pDevice->GetDeviceState(size, pData);
			}
			// another error
			if (FAILED(hr))
				return ZFX_FAIL;
		}
		// another error
		else
			return ZFX_FAIL;
	}
	return ZFX_OK;
}	//	GetData