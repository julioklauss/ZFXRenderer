#pragma once

#include <ZFX.h>
#include <cstdio>
#include <dinput.h>
#include <ZFXInputDevice.h>

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

#define MOUSE_BUTTON_LIMIT 7

class ZFXMouse : public ZFXDIDevice
{
	public:
		ZFXMouse(LPDIRECTINPUT8, HWND, FILE*);
		~ZFXMouse(void);

		HRESULT	Init(void);
		HRESULT	Update(void);

		void	SetCage(RECT rcCage)	{ m_rcCage = rcCage; }

		bool	IsPressed(UINT nBtn)	{ if (nBtn < MOUSE_BUTTON_LIMIT) return m_bPressed[nBtn];
											return false; }
		bool	IsReleased(UINT nBtn)	{ if (nBtn < MOUSE_BUTTON_LIMIT) return m_bReleased[nBtn];
											return false; }
	private:
		HANDLE	m_hEvent;
		RECT	m_rcCage;
		LONG	m_lMaxScroll;
		LONG	m_lMinScroll;
		bool	m_bPressed[MOUSE_BUTTON_LIMIT];
		bool	m_bReleased[MOUSE_BUTTON_LIMIT];
		POINT	m_Delta;
};	//	class

class ZFXJoystick : public ZFXDIDevice
{
	public:
		ZFXJoystick(LPDIRECTINPUT8, HWND, FILE*);
		~ZFXJoystick(void);

		HRESULT	Init(void);
		HRESULT	Update(void);

		bool JoystickFound(void)							{ return m_bJoyFound; }
		BOOL EnumJoyCallback(const DIDEVICEINSTANCE* pI);
		void GetName(char* pJoyName)						{ memcpy(pJoyName, m_Name, sizeof(char) * 256); }
		bool IsPressed(UINT nBtn)							{ if (nBtn < m_dwNumBtns) return m_bPressed[nBtn];
																return false; }
		bool IsReleased(UINT nBtn)							{ if (nBtn < m_dwNumBtns) return m_bReleased[nBtn];
																return false; }
	private:
		GUID	m_guid;
		char	m_Name[256];
		bool	m_bJoyFound;
		bool	m_bPressed[12];
		bool	m_bReleased[12];
		DWORD	m_dwNumBtns;
};	//	class

class ZFXDI : public ZFXInputDevice
{
	public:
		ZFXDI(HINSTANCE hDLL);
		~ZFXDI(void);

		HRESULT Init(HWND, const RECT*, bool);

		void Release(void);
		bool IsRunning(void) { return m_bRunning; }
		bool HasJoystick(char* pJoyName);

		HRESULT	Update(void);

		bool	IsPressed(ZFXINPUTDEV idType, UINT nID);
		bool	IsReleased(ZFXINPUTDEV idType, UINT nID);
		HRESULT GetPosition(ZFXINPUTDEV idType, POINT* pPt);

	private:
		LPDIRECTINPUT8	m_pDI;
		ZFXKeyboard*	m_pKB;
		ZFXMouse*		m_pMouse;
		ZFXJoystick*	m_pJoy;
};	//	class