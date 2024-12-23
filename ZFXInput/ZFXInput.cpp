// ZFXInput.cpp : Defines the functions for the static library.
//

#include "stdafx.h"
#include "ZFXInput.h"

/**
* Constructor: Nothing spectacular
*/
ZFXInput::ZFXInput(HINSTANCE hInst)
{
	m_hInst		= hInst;
	m_pDevice	= NULL;
	m_hDLL		= NULL;
}
/*----------------------------------------------------------------*/

/**
* Destructor: Just call the Release method
*/
ZFXInput::~ZFXInput()
{
	Release();
}
/*----------------------------------------------------------------*/

/**
* Create the dll objects. This function loads the appropriate dll
*/
HRESULT ZFXInput::CreateDevice()
{
	// load the DLL containing interface implementation
	m_hDLL = LoadLibraryExA("ZFXDI.dll", NULL, 0);
	if (!m_hDLL) {
		MessageBoxA(NULL, "Loading ZFXDI.dll from lib failed.", "ZFXEngine - error", MB_OK | MB_ICONERROR);
		return E_FAIL;
	}

	CREATEINPUTDEVICE _CreateInputDevice = 0;
	HRESULT hr;

	// function pointer to dll's "CreateInputDevice" function
	_CreateInputDevice = (CREATEINPUTDEVICE)GetProcAddress(m_hDLL, "CreateInputDevice");
	// call dll's create function
	hr = _CreateInputDevice(m_hDLL, &m_pDevice);
	if (FAILED(hr)) {
		MessageBoxA(NULL, "CreateInputDevice() from lib failed.", "ZFXEngine - error", MB_OK | MB_ICONERROR);
		m_pDevice = NULL;
		return E_FAIL;
	}

	return S_OK;
}	//	CreateDevice
/*----------------------------------------------------------------*/

/**
* Cleanup the dll objects
*/
void ZFXInput::Release()
{
	RELEASEINPUTDEVICE _ReleaseInputDevice = 0;
	HRESULT hr;

	if (m_hDLL) {
		// function pointer to dll "ReleaseInputDevice" function
		_ReleaseInputDevice = (RELEASEINPUTDEVICE)GetProcAddress(m_hDLL, "ReleaseInputDevice");
	}
	// call dll's release function
	if (m_pDevice) {
		hr = _ReleaseInputDevice(&m_pDevice);
		if (FAILED(hr)) {
			m_pDevice = NULL;
		}
	}
}	//	Release
/*----------------------------------------------------------------*/