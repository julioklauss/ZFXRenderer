#include "ZFXDI.h"

#define BUFFER_SIZE 16

ZFXMouse::ZFXMouse(LPDIRECTINPUT8 pDI, HWND hWnd, FILE* pLog)
{
	Create(pDI, hWnd, pLog);
}

ZFXMouse::~ZFXMouse(void)
{
	Release();
}

HRESULT ZFXMouse::Init(void)
{
	// clear memory
	memset(m_bPressed, 0, sizeof(bool) * 3);
	memset(m_bReleased, 0, sizeof(bool) * 3);
	m_lX = m_lY = 0;
	
	if (FAILED(CrankUp(GUID_SysMouse, &c_dfDIMouse)))
		return ZFX_FAIL;

	// event notification
	if (!(m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL)))
		return ZFX_FAIL;

	// build mouse buffer
	DIPROPDWORD	dipdw;
	dipdw.diph.dwSize		= sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dipdw.diph.dwObj		= 0;
	dipdw.diph.dwHow		= DIPH_DEVICE;
	dipdw.dwData			= BUFFER_SIZE;

	if (FAILED(m_pDevice->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph)))
		return ZFX_FAIL;

	m_pDevice->Acquire();
	return ZFX_OK;
}	//	Init

HRESULT ZFXMouse::Update(void)
{
	DIDEVICEOBJECTDATA od[BUFFER_SIZE];
	DWORD dwNumElem = BUFFER_SIZE;

	// read data from the mouse buffer
	if (FAILED(GetData(IDV_MOUSE, &od[0], &dwNumElem)))
		return ZFX_FAIL;

	m_bReleased[0] = m_bReleased[1] = m_bReleased[2] = false;

	// now we have dwNum mouse events to process
	for (DWORD i = 0; i < dwNumElem; ++i)
	{
		switch (od[i].dwOfs)
		{
			// MOVEMENT
			case DIMOFS_X: {
				m_lX += od[i].dwData;
				if (m_lX < m_rcCage.left)
					m_lX = m_rcCage.left;
				else if (m_lX > m_rcCage.right)
					m_lX = m_rcCage.right;
			}	break;

			case DIMOFS_Y: {
				m_lY += od[i].dwData;
				if (m_lY < m_rcCage.top)
					m_lY = m_rcCage.top;
				else if (m_lY > m_rcCage.bottom)
					m_lY = m_rcCage.bottom;
			}	break;

			// MOUSE_KEYS
			case DIMOFS_BUTTON0: {
				if (od[i].dwData & 0x80) {
					m_bPressed[0] = true;
				}
				else {
					if (m_bPressed[0])
						m_bReleased[0] = true;
					m_bPressed[0] = false;
				}
			}	break;

			case DIMOFS_BUTTON1: {
				if (od[i].dwData & 0x80)
					m_bPressed[1] = true;
				else {
					if (m_bPressed[1])
						m_bReleased[1] = true;
					m_bPressed[1] = false;
				}
			}	break;

			case DIMOFS_BUTTON2: {
				if (od[i].dwData & 0x80)
					m_bPressed[2] = true;
				else {
					if (m_bPressed[2])
						m_bReleased[2] = true;
					m_bPressed[2] = false;
				}
			}	break;
		};	//	swicth
	}	//	for
	return ZFX_OK;
}	// Update