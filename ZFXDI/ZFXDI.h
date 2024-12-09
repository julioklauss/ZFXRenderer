#pragma once

#include <ZFX.h>
#include <cstdio>
#include <dinput.h>

class ZFXDIDevice
{
	public:
		ZFXDIDevice(void) { /*nothing*/; }
		virtual ~ZFXDIDevice(void) { /*nothing*/; }

		// base functions
		virtual void	Create(LPDIRECTINPUT8, HWND, FILE*);
		virtual void	Release(void);
		virtual HRESULT	CrankUp(REFGUID rguid, LPCDIDATAFORMAT lpdf);

		// accessor functions
		virtual void	GetPosition(POINT* pPoint) { (*pPoint).x = m_lX; (*pPoint).y = m_lY; }

		// pure virtual functions
		virtual HRESULT	Init(void) = 0;
		virtual HRESULT Update(void) = 0;

	protected:
		virtual HRESULT	GetData(ZFXINPUTDEV Type, void* pData, DWORD* dwNum);

		LPDIRECTINPUTDEVICE8	m_pDevice;
		LPDIRECTINPUT8			m_pDI;
		HWND					m_hWnd;
		long					m_lX;
		long					m_lY;
		FILE*					m_pLog;
};	//	class

class ZFXKeyboard : public ZFXDIDevice
{
	public:
		ZFXKeyboard(LPDIRECTINPUT8, HWND, FILE*);
		~ZFXKeyboard(void);

		HRESULT	Init(void);
		HRESULT	Update(void);

		bool	IsPressed(UINT nID);
		bool	IsReleased(UINT nID);

	private:
		char	m_Keys[256];
		char	m_KeysOld[256];
};	//	class