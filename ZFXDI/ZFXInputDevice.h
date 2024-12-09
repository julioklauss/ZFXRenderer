#pragma once

#include <ZFX.h>

class ZFXInputDevice
{
	protected:
		HWND		m_hWndMain;
		HINSTANCE	m_hDLL;
		bool		m_bRunning;

	public:
		ZFXInputDevice(void) {};
		virtual ~ZFXInputDevice(void) {};

		virtual HRESULT	Init(HWND, const RECT*, bool) = 0;
		virtual void	Release(void) = 0;
		virtual bool	IsRunning(void) = 0;
		virtual bool	HasJoystick(char*) = 0;
		virtual HRESULT Update(void) = 0;

		// works with mouse and joystick only
		virtual HRESULT	GetPosition(ZFXINPUTDEV, POINT*) = 0;

		// works with keyboard, mouse and joystick
		virtual bool	IsPressed(ZFXINPUTDEV, UINT) = 0;

		// works with keyboard, mouse and joystick
		virtual bool	IsReleased(ZFXINPUTDEV, UINT) = 0;
};	//	class
