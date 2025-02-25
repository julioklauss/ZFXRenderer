#pragma once

#include <WinSock2.h>
#include "ZFXQueue.h"
#include <ZFXNetworkDevice.h>
#include <cstdio>

class ZFXSocketObject
{
	public:
		ZFXSocketObject(FILE* pLog);
		~ZFXSocketObject(void);

		bool	IsRunning(void) { return m_bRunning; }

		// socket MISC function
		HRESULT	CreateSocket(void);
		HRESULT	Bind(int nPort);
		HRESULT Listen(void);
		HRESULT Accept(SOCKET* skToNewClient);
		HRESULT Connect(char* chServer, int nPort);
		void	Disconnect(void);

		// send and receive data
		int		Send(const char*, UINT);
		int		Send(const char*, UINT, SOCKET);
		HRESULT	Receive(SOCKET sk);

		// get information
		SOCKET	GetSocket(void) { return m_skSocket; }

		// information about the inbox
		bool	IsPkgWaiting(void)		{ return (m_pInbox->GetCount() > 0); }
		UINT	GetNextPkgSize(void)	{ return (m_pInbox->GetFrontSize()); }
		HRESULT	GetNextPkg(ZFXPACKAGE*);

	private:
		ZFXQueue*	m_pInbox;
		SOCKET		m_skSocket;
		char*		m_Buffer;
		bool		m_bRunning;
};	//	class