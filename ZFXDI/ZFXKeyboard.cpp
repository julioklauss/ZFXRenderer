#include "ZFXDI.h"

ZFXKeyboard::ZFXKeyboard(LPDIRECTINPUT8 pDI, HWND hWnd, FILE* pLog)
{
	Create(pDI, hWnd, pLog);
}

ZFXKeyboard::~ZFXKeyboard(void)
{
	Release();
}

HRESULT ZFXKeyboard::Init(void)
{
	if (FAILED(CrankUp(GUID_SysKeyboard, &c_dfDIKeyboard)))
		return ZFX_FAIL;

	// clear memory
	memset(m_Keys, 0, sizeof(m_Keys));
	memset(m_KeysOld, 0, sizeof(m_KeysOld));

	// acquire the device
	m_pDevice->Acquire();
	return ZFX_OK;
}	//	Init

HRESULT ZFXKeyboard::Update(void)
{
	memcpy(m_KeysOld, m_Keys, sizeof(m_Keys));

	// query status
	if (FAILED(GetData(IDV_KEYBOARD, &m_Keys[0], NULL)))
		return ZFX_FAIL;
	return ZFX_OK;
}	//	Update

bool ZFXKeyboard::IsPressed(UINT nID)
{
	if (m_Keys[nID] & 0x80)
		return true;
	return false;
}	//	IsPressed

bool ZFXKeyboard::IsReleased(UINT nID)
{
	if ((m_KeysOld[nID] & 0x80) && !(m_Keys[nID] & 0x80))
		return true;
	return false;
}	//	IsReleased