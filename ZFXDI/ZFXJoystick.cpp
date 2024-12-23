#include "ZFXDI.h"

ZFXJoystick* g_pThis = NULL;

ZFXJoystick::ZFXJoystick(LPDIRECTINPUT8 pDI, HWND hWnd, FILE* pLog)
{
	Create(pDI, hWnd, pLog);
}

ZFXJoystick::~ZFXJoystick()
{
	Release();
}

BOOL CALLBACK gEnumJoyCallback(const DIDEVICEINSTANCE* pInst, void* pUserData)
{
	return g_pThis->EnumJoyCallback(pInst);
}	//	gEnumJoyCallback

HRESULT ZFXJoystick::Init(void)
{
	DIPROPRANGE	diprg;
	DIDEVCAPS	diCaps;

	// some initializations
	memset(m_bPressed, 0, sizeof(m_bPressed));
	memset(m_bReleased, 0, sizeof(m_bReleased));
	m_bJoyFound			= false;
	m_lX = m_lY = m_lZ	= 0;
	g_pThis				= this;

	// enumerate attached joysticks
	m_pDI->EnumDevices(DI8DEVCLASS_GAMECTRL, (LPDIENUMDEVICESCALLBACK)gEnumJoyCallback, &m_guid, DIEDFL_ATTACHEDONLY);

	// none found?
	if (!m_bJoyFound)
		return ZFX_FAIL;

	// final settings
	diprg.diph.dwSize		= sizeof(DIPROPRANGE);
	diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	diprg.diph.dwHow		= DIPH_BYOFFSET;
	diprg.lMin				= -1000;
	diprg.lMax				= +1000;

	diprg.diph.dwObj		= DIJOFS_X;
	m_pDevice->SetProperty(DIPROP_RANGE, &diprg.diph);
	
	diprg.diph.dwObj		= DIJOFS_Y;
	m_pDevice->SetProperty(DIPROP_RANGE, &diprg.diph);

	diprg.diph.dwObj		= DIJOFS_Z;
	m_pDevice->SetProperty(DIPROP_RANGE, &diprg.diph);
	
	// number of buttons
	if (SUCCEEDED(m_pDevice->GetCapabilities(&diCaps)))
		m_dwNumBtns = diCaps.dwButtons;
	else
		m_dwNumBtns = 4;
	
	m_pDevice->Acquire();
	return ZFX_OK;
}	//	Init

BOOL ZFXJoystick::EnumJoyCallback(const DIDEVICEINSTANCE* pInst)
{
	// try to crank up this one
	if (SUCCEEDED(CrankUp(pInst->guidInstance, &c_dfDIJoystick))) {
		m_bJoyFound = true;
		strcpy(m_Name, (char*)pInst->tszProductName);
		return DIENUM_STOP;
	}
	return DIENUM_CONTINUE;
}	//	EnumJoyCallback

HRESULT ZFXJoystick::Update(void)
{
	DIJOYSTATE js;

	// poll the joystick
	m_pDevice->Poll();

	// get the data from the joystick
	if (FAILED(GetData(IDV_JOYSTICK, &js, NULL)))
		return ZFX_FAIL;

	// joystick buttons
	for (DWORD i = 0; i < m_dwNumBtns; ++i)
	{
		m_bReleased[i] = false;

		if (js.rgbButtons[i] & 0x80)
			m_bPressed[i] = true;
		else {
			if (m_bPressed[i])
				m_bReleased[i] = true;
			m_bPressed[i] = false;
		}
	}

	// position of the stick
	m_lX	= js.lX;
	m_lY	= js.lY;
	m_lZ	= js.lZ;
	m_lRx	= js.lRx;
	m_lRy	= js.lRy;
	m_lRz	= js.lRz;
	return ZFX_OK;
}	//	Update