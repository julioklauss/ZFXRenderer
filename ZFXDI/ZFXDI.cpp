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

ZFXDI::ZFXDI(HINSTANCE hDLL)
{
	m_hDLL		= hDLL;
	m_pDI		= NULL;
	m_pLog		= NULL;
	m_bRunning	= false;
	m_pKB		= NULL;
	m_pMouse	= NULL;
	m_pJoy		= NULL;
}

ZFXDI::~ZFXDI()
{
	Release();
}

void ZFXDI::Release()
{
	if (m_pKB) {
		delete m_pKB;
		m_pKB = NULL;
	}
	if (m_pMouse) {
		delete m_pMouse;
		m_pMouse = NULL;
	}
	if (m_pJoy) {
		delete m_pJoy;
		m_pJoy = NULL;
	}
	if (m_pDI) {
		m_pDI->Release();
		m_pDI = NULL;
	}
}

HRESULT ZFXDI::Init(HWND hWnd, const RECT* rcMouseCage, bool bSaveLog)
{
	HRESULT hr;
	m_hWndMain = hWnd;

	// create DirectInput main object
	if (FAILED(hr = DirectInput8Create(m_hDLL, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_pDI, NULL)))
		return ZFX_FAIL;

	// create all input device objects
	m_pKB		= new ZFXKeyboard(m_pDI, hWnd, m_pLog);
	m_pMouse	= new ZFXMouse(m_pDI, hWnd, m_pLog);
	m_pJoy		= new ZFXJoystick(m_pDI, hWnd, m_pLog);

	// initializing all input device objects
	if (FAILED(m_pKB->Init())) {
		if (m_pKB)
			delete m_pKB;
		m_pKB = NULL;
		return ZFX_FAIL;
	}

	if (FAILED(m_pMouse->Init())) {
		if (m_pMouse)
			delete m_pMouse;
		m_pMouse = NULL;
		return ZFX_FAIL;
	}
	if (rcMouseCage)
		m_pMouse->SetCage(*rcMouseCage);

	if (FAILED(m_pJoy->Init())) {
		if (m_pJoy)
			delete m_pJoy;
		m_pJoy = NULL;
	}

	m_bRunning = true;
	return ZFX_OK;
}	//	Init

HRESULT ZFXDI::Update(void)
{
	HRESULT	hr;

	if (!IsRunning())
		return ZFX_FAIL;

	if (m_pKB) {
		if (FAILED(hr = m_pKB->Update()))
			return hr;
	}

	if (m_pMouse) {
		if (FAILED(hr = m_pMouse->Update()))
			return hr;
	}

	if (m_pJoy) {
		if (FAILED(hr = m_pJoy->Update()))
			return hr;
	}
	return ZFX_OK;
}	//	Update

bool ZFXDI::HasJoystick(char* pJoyName)
{
	if (m_pJoy) {
		if (pJoyName)
			m_pJoy->GetName(pJoyName);
		return true;
	}
	return false;
}	//	HasJoystick

HRESULT ZFXDI::GetPosition(ZFXINPUTDEV idType, POINT* pPt)
{
	if (idType == IDV_MOUSE) {
		m_pMouse->GetPosition(pPt);
		return ZFX_OK;
	}
	else if (idType == IDV_JOYSTICK) {
		if (m_pJoy)
			m_pJoy->GetPosition(pPt);
		else {
			(*pPt).x = 0;
			(*pPt).y = 0;
		}
		return ZFX_OK;
	}
	else return ZFX_INVALIDPARAM;
}	//	GetPosition

bool ZFXDI::IsPressed(ZFXINPUTDEV idType, UINT nBtn)
{
	if (idType == IDV_MOUSE)
		return m_pMouse->IsPressed(nBtn);
	else if (idType == IDV_KEYBOARD)
		return m_pKB->IsPressed(nBtn);
	else if ((idType == IDV_JOYSTICK) && (m_pJoy))
		return m_pJoy->IsPressed(nBtn);
	else
		return false;
}	//	Pressed

bool ZFXDI::IsReleased(ZFXINPUTDEV idType, UINT nBtn)
{
	if (idType == IDV_MOUSE)
		return m_pMouse->IsReleased(nBtn);
	else if (idType == IDV_KEYBOARD)
		return m_pKB->IsReleased(nBtn);
	else if ((idType == IDV_JOYSTICK) && (m_pJoy))
		return m_pJoy->IsReleased(nBtn);
	else
		return false;
}	//	Released