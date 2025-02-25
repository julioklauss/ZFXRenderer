#pragma once

#include "ZFXSocketObject.h"
#include <ZFXNetworkDevice.h>

BOOL WINAPI DllEntryPoint(HINSTANCE hDll, DWORD fdwReason, LPVOID lpvRserved);

typedef struct ZFXCLIENT_TYPE
{
	SOCKET	skToClient;
	UINT	nID;
} ZFXCLIENT;

class ZFXWS : public ZFXNetworkDevice
{
	public:
		ZFXWS(HINSTANCE hDLL);
		~ZFXWS(void);

		// interface functions
		HRESULT Init(HWND, ZFXNETMODE, int, char*, UINT, bool);
		void	Release(void);
		bool	IsRunning(void) { return m_bRunning; }
		HRESULT	MsgProc(WPARAM wp, LPARAM lp);
		HRESULT	SendToServer(const ZFXPACKAGE*);
		HRESULT	SendToClients(const ZFXPACKAGE*);
		HRESULT	ServerUpdate(void);
		bool	IsPkgWaiting(void)				{ return m_pSockObj->IsPkgWaiting(); }
		UINT	GetNextPkgSize(void)			{ return m_pSockObj->GetNextPkgSize(); }
		HRESULT	GetNextPkg(ZFXPACKAGE* pPkg)	{ return m_pSockObj->GetNextPkg(pPkg); }
	
	private:
		ZFXSocketObject*	m_pSockObj;
		WSAEVENT			m_Event;
		ZFXNETMODE			m_Mode;
		ZFXCLIENT			m_Clients[256];
		char				m_ClCount;
		UINT				m_ClID;
		char*				m_Buffer;
		FILE*				m_pLog;

		// initializing ZFXSocketObject
		HRESULT	CreateServer(ZFXSocketObject** ppSkObject);
		HRESULT	CreateClient(ZFXSocketObject** ppSkObject);

		// message processing
		HRESULT	OnAccept(void);
		HRESULT	OnReceive(SOCKET skReceiving);
		HRESULT	OnDisconnect(SOCKET skDisconnecting);
};	//	class