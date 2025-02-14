#pragma once

#include <dmusici.h>
#include <dsound.h>
#include <ZFXAudioDevice.h>

BOOL WINAPI DllEntryPoint(HINSTANCE hDll, DWORD fdwReason, LPVOID lpvRserved);

typedef struct ZFXSOUND_TYPE
{
	char*					chName;
	bool					bChanged;
	IDirectMusicSegment8*	pSegment;
	IDirectMusicAudioPath8* p3DPath;
	IDirectSound3DBuffer8*	p3DBuffer;
} ZFXSOUND;

class ZFXDA : public ZFXAudioDevice
{
	public:
		ZFXDA(HINSTANCE hDLL);
		~ZFXDA(void);

		HRESULT	Init(HWND, const char*, bool);

		// interface functions
		void	Release(void);
		bool	IsRunning(void) { return m_bRunning; }

		void	SetListener(ZFXVector, ZFXVector, ZFXVector, ZFXVector);

		HRESULT	LoadSound(const char*, UINT*);
		void	PlaySound(UINT nID, bool);
		void	StopSound(UINT nID);
		void	StopAll(void)	{ if (m_pPerformance)
									m_pPerformance->Stop(NULL, NULL, 0, 0); }

		void	SetSoundPosition(ZFXVector, UINT);
		void	SetSoundMaxDist(float, UINT);
		void	SetSoundDirection(ZFXVector, ZFXVector, UINT);

	private:
		IDirectMusicLoader8*		m_pLoader;
		IDirectMusicPerformance8*	m_pPerformance;
		IDirectSound3DListener8*	m_pListener;
		IDirectMusicAudioPath8*		m_pDAPath;
		DS3DLISTENER				m_dsListener;
		DS3DBUFFER					m_dsBuffer;
		ZFXSOUND*					m_pSounds;
		UINT						m_NumSounds;
		FILE*						m_pLog;
};	//	class