#include "ZFXDA.h"

bool g_bLF = false;
size_t g_szoff3 = sizeof(float) * 3; //shortcut size of three floats for copying vectors

ZFXDA::ZFXDA(HINSTANCE hDLL)
{
	m_hDLL			= hDLL;
	m_pLoader		= NULL;
	m_pListener		= NULL;
	m_pPerformance	= NULL;
	m_pSounds		= NULL;
	m_pLog			= NULL;
	m_bRunning		= false;

	// initializing structures
	m_dsListener.dwSize = sizeof(DS3DLISTENER);
	m_dsBuffer.dwSize = sizeof(DS3DBUFFER);
}	//	constructor

ZFXDA::~ZFXDA()
{
	Release();
}	//	destructor

HRESULT ZFXDA::Init(HWND hWnd, const char* chPath, bool bSaveLog)
{
	HRESULT	hr;
	WCHAR	wPath[MAX_PATH];

	m_hWndMain	= hWnd;
	g_bLF		= bSaveLog;

	// COM initialization
	CoInitialize(NULL);

	// create an instance of the loader object
	hr = CoCreateInstance(CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC, IID_IDirectMusicLoader8, (void**)&m_pLoader);
	if (FAILED(hr))
		return ZFX_FAIL;

	// create an instance of the performance object
	hr = CoCreateInstance(
		CLSID_DirectMusicPerformance,	// Class-ID
		NULL,							// aggregating the object
		CLSCTX_INPROC,					// context
		IID_IDirectMusicPerformance8,	// reference ID
		(void**)&m_pPerformance);		// address
	if (FAILED(hr))
		return ZFX_FAIL;

	// default path for sound files
	if (MultiByteToWideChar(CP_ACP, 0, chPath, -1, wPath, MAX_PATH) == 0)
		return ZFX_FAIL;

	if (FAILED(hr == m_pLoader->SetSearchDirectory(GUID_DirectMusicAllTypes, wPath, false)))
		return ZFX_FAIL;

	// initializing the performance object
	if (FAILED(hr = m_pPerformance->InitAudio(NULL, NULL, hWnd, DMUS_APATH_SHARED_STEREOPLUSREVERB, 64, DMUS_AUDIOF_ALL, NULL)))
		return ZFX_FAIL;

	// pointer to default audio path
	if (FAILED(m_pPerformance->GetDefaultAudioPath(&m_pDAPath)))
		return ZFX_FAIL;

	// get pointer to listener in path
	if (FAILED(m_pDAPath->GetObjectInPath(0, DMUS_PATH_PRIMARY_BUFFER, 0, GUID_NULL, 0, IID_IDirectSound3DListener8, (void**)&m_pListener)))
		return ZFX_FAIL;

	m_bRunning = true;
	return ZFX_OK;
}	//	Init

void ZFXDA::Release()
{
	if (m_pSounds) {
		for (UINT i = 0; i < m_NumSounds; ++i)
		{
			if (m_pSounds[i].pSegment) {
				m_pSounds[i].pSegment->Unload(m_pPerformance);
				m_pSounds[i].pSegment->Release();
				m_pSounds[i].pSegment = NULL;
				delete[] m_pSounds[i].chName;
				m_pSounds[i].chName = NULL;
			}
		}
		free(m_pSounds);
	}

	if (m_pLoader) {
		m_pLoader->Release();
		m_pLoader = NULL;
	}

	if (m_pListener) {
		m_pListener->Release();
		m_pListener = NULL;
	}

	if (m_pPerformance) {
		m_pPerformance->Stop(NULL, NULL, 0, 0);
		m_pPerformance->CloseDown();
		m_pPerformance->Release();
		m_pPerformance = NULL;
	}

	// shutting down COM
	if (m_bRunning) CoUninitialize();
	m_bRunning = false;
}	//	release

HRESULT ZFXDA::LoadSound(const char* chName, UINT* nID)
{
	WCHAR	wName[MAX_PATH];
	HRESULT	hr;

	if (MultiByteToWideChar(CP_ACP, 0, chName, -1, wName, MAX_PATH) == 0)
		return ZFX_FAIL;

	// is this sound file already loaded?
	for (UINT i = 0; i < m_NumSounds; ++i)
	{
		if (strcmp(chName, m_pSounds[i].chName) == 0) {
			*nID = i;
			return ZFX_OK;
		}
	}	// for

	// 50 new slots for the sounds
	if ((m_NumSounds % 50) == 0) {
		int n = (m_NumSounds + 50) * sizeof(ZFXSOUND);
		m_pSounds = (ZFXSOUND*)realloc(m_pSounds, n);
		if (!m_pSounds)
			return ZFX_OUTOFMEMORY;
	}

	m_pSounds[m_NumSounds].chName = new char[strlen(chName)+1];
	memcpy(m_pSounds[m_NumSounds].chName, chName, strlen(chName) + 1);

	m_pSounds[m_NumSounds].bChanged = false;

	// load the file
	if (FAILED(hr = m_pLoader->LoadObjectFromFile(
										CLSID_DirectMusicSegment,					// class
										IID_IDirectMusicSegment8,					// interface type
										wName,										// name
										(void**)&m_pSounds[m_NumSounds].pSegment)))	// address
	{
		if ((hr == DMUS_E_LOADER_FAILEDOPEN) || (hr == DMUS_E_LOADER_FAILEDCREATE))
			return ZFX_FILENOTFOUND;
		else if (hr == DMUS_E_LOADER_FORMATNOTSUPPORTED)
			return ZFX_INVALIDPARAM;
		else if (hr == E_OUTOFMEMORY)
			return ZFX_OUTOFMEMORY;
		return ZFX_FAIL;
	}

	IDirectMusicSegment8* pSeg = m_pSounds[m_NumSounds].pSegment;
	// download instruments
	if (FAILED(pSeg->Download(m_pPerformance))) {
		pSeg->Release();
		pSeg = NULL;
		return ZFX_FAIL;
	}

	// create an audio path
	m_pPerformance->CreateStandardAudioPath(DMUS_APATH_DYNAMIC_3D, 64, TRUE, &m_pSounds[m_NumSounds].p3DPath);

	m_pSounds[m_NumSounds].p3DPath->GetObjectInPath(
		DMUS_PCHANNEL_ALL,								// performance channel
		DMUS_PATH_BUFFER,								// stage in path
		0,												// index in path
		GUID_NULL,										// class
		0,												// index
		IID_IDirectSound3DBuffer,						// type
		(void**)&m_pSounds[m_NumSounds].p3DBuffer);		// address

	m_NumSounds++;
	return ZFX_OK;
}	//	LoadSound

void ZFXDA::PlaySound(UINT nID, bool bLoop)
{
	if (nID >= m_NumSounds)
		return;

	// any changes?
	if (m_pSounds[nID].bChanged) {
		m_pListener->CommitDeferredSettings();
		m_pSounds[nID].bChanged = false;
	}

	if (bLoop)
		m_pSounds[nID].pSegment->SetRepeats(DMUS_SEG_REPEAT_INFINITE);

	// play as secondary buffer
	m_pPerformance->PlaySegment(m_pSounds[nID].pSegment, DMUS_SEGF_DEFAULT | DMUS_SEGF_SECONDARY, 0, 0);
}	//	PlaySound

void ZFXDA::StopSound(UINT nID)
{
	if (nID >= m_NumSounds)
		return;
	m_pPerformance->Stop(m_pSounds[nID].pSegment, 0, 0, 0);
}	//	StopSound

void ZFXDA::SetListener(ZFXVector vPos, ZFXVector vDir, ZFXVector vUp, ZFXVector vSpeed)
{
	m_pListener->GetAllParameters(&m_dsListener);

	memcpy(&m_dsListener.vPosition, &vPos, g_szoff3);
	memcpy(&m_dsListener.vOrientFront, &vDir, g_szoff3);
	memcpy(&m_dsListener.vOrientTop, &vUp, g_szoff3);
	memcpy(&m_dsListener.vVelocity, &vSpeed, g_szoff3);

	if (m_pListener)
		m_pListener->SetAllParameters(&m_dsListener, DS3D_IMMEDIATE);
}	//	SetListener

void ZFXDA::SetSoundPosition(ZFXVector vPos, UINT nID)
{
	IDirectSound3DBuffer8* p3DBuffer;
	if (nID >= m_NumSounds)
		return;

	p3DBuffer = m_pSounds[m_NumSounds].p3DBuffer;
	m_pSounds[m_NumSounds].bChanged = true;

	p3DBuffer->GetAllParameters(&m_dsBuffer);

	m_dsBuffer.dwMode = DS3DMODE_NORMAL;
	memcpy(&m_dsBuffer.vPosition, &vPos, g_szoff3);

	p3DBuffer->SetAllParameters(&m_dsBuffer, DS3D_DEFERRED);
}	//	SetSoundPosition

void ZFXDA::SetSoundDirection(ZFXVector vDir, ZFXVector vV, UINT nID)
{
	IDirectSound3DBuffer8* p3DBuffer;
	if (nID >= m_NumSounds)
		return;

	p3DBuffer = m_pSounds[m_NumSounds].p3DBuffer;
	m_pSounds[m_NumSounds].bChanged = true;

	p3DBuffer->GetAllParameters(&m_dsBuffer);
	m_dsBuffer.dwMode = DS3DMODE_NORMAL;
	memcpy(&m_dsBuffer.vVelocity, &vV, g_szoff3);
	memcpy(&m_dsBuffer.vConeOrientation, &vDir, g_szoff3);

	p3DBuffer->SetAllParameters(&m_dsBuffer, DS3D_DEFERRED);
}	//	SetSoundDistance

void ZFXDA::SetSoundMaxDist(float fDis, UINT nID)
{
	if (nID >= m_NumSounds)
		return;
	IDirectSound3DBuffer8* p3DBuffer;
	if (nID >= m_NumSounds)
		return;

	p3DBuffer = m_pSounds[m_NumSounds].p3DBuffer;
	m_pSounds[m_NumSounds].bChanged = true;

	p3DBuffer->GetAllParameters(&m_dsBuffer);

	m_dsBuffer.dwMode = DS3DMODE_NORMAL;
	m_dsBuffer.flMaxDistance = fDis;

	p3DBuffer->SetAllParameters(&m_dsBuffer, DS3D_DEFERRED);
}	//	SetSoundMaxDist