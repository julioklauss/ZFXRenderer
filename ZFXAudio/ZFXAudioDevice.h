#pragma once

#include <windows.h>
#include <stdio.h>
#include <ZFX.h>
#include <ZFX3D.h>

class ZFXAudioDevice
{
	protected:
		HWND		m_hWndMain;		// main window
		HINSTANCE	m_hDLL;			// DLL handle
		bool		m_bRunning;		// init done
	
	public:
		ZFXAudioDevice(void) {};
		virtual ~ZFXAudioDevice(void) {};

		virtual HRESULT	Init(HWND, const char*, bool) = 0;
		virtual void	Release(void) = 0;
		virtual bool	IsRunning(void) = 0;

		// stop all audio input
		virtual void	StopAll(void) = 0;

		// load a sound file from disc
		virtual HRESULT	LoadSound(const char*, UINT*) = 0;
		
		// play a certain sound
		virtual void	PlaySound(UINT, bool bLoop) = 0;

		// stop a certain sound
		virtual void	StopSound(UINT) = 0;

		// listener parameters
		virtual void	SetListener(ZFXVector vPos, ZFXVector vDir, ZFXVector vUp, ZFXVector vV) = 0;

		// parameters of the sound source
		virtual void	SetSoundPosition(ZFXVector, UINT) = 0;
		virtual void	SetSoundDirection(ZFXVector, ZFXVector vV, UINT) = 0;
		virtual void	SetSoundMaxDist(float, UINT) = 0;
};	//	class
typedef class ZFXAudioDevice* LPZFXAUDIODEVICE;

/*----------------------------------------------------------------*/

extern "C"
{
	HRESULT CreateAudioDevice(HINSTANCE hDLL, ZFXAudioDevice** pInterface);
	typedef HRESULT(*CREATEAUDIODEVICE)(HINSTANCE hDll, ZFXAudioDevice** pInterface);

	HRESULT ReleaseAudioDevice(ZFXAudioDevice** pInterface);
	typedef HRESULT(*RELEASEAUDIODEVICE)(ZFXAudioDevice** pInterface);
}