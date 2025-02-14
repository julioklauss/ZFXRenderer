#pragma once

#include <d3d9.h>
#include <dsound.h>
#include <ZFXAudioDevice.h>

struct ZFXSOUND_TYPE
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
};