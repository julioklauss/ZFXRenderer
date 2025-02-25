#include <cstdio>
#include "ZFXWS.h"

bool g_bLF = false;
int g_PkgSize = sizeof(ZFXPACKAGE);

ZFXWS::ZFXWS(HINSTANCE hDLL)
{
	m_hDLL		= hDLL;
	m_pSockObj	= NULL;
	m_Event		= NULL;
	m_Buffer	= NULL;
	m_nPort		= 0;
	m_ClID		= 1;		// 0 reserved for server
	m_ClCount	= 0;
	m_bRunning	= false;
}	//	constructor
/*----------------------------------------------------------------*/

ZFXWS::~ZFXWS()
{
	Release();
}	//	destructor
/*----------------------------------------------------------------*/

void ZFXWS::Release()
{
	if (m_Mode == NMD_SERVER) {
		for (int i = 0; i < m_ClCount; ++i)
		{
			shutdown(m_Clients[i].skToClient, 0x02);
			closesocket(m_Clients[i].skToClient);
			m_Clients[i].skToClient = INVALID_SOCKET;
		}
	}
	if (m_pSockObj) {
		delete m_pSockObj;
		m_pSockObj = NULL;
	}
	if (m_Event) {
		WSACloseEvent(m_Event);
		delete m_Event;
		m_Event = NULL;
	}
	if (m_Buffer) {
		delete[] m_Buffer;
		m_Buffer = NULL;
	}
	WSACleanup();
	m_bRunning = false;
}

HRESULT ZFXWS::Init(HWND hWnd, ZFXNETMODE nmd, int nPort, char* pIP, UINT nMaxPkgSize, bool bSaveLog)
{
	WSADATA	wsaData;
	UINT	nEvents = 0;
	WORD	wVersion;
	int		nRes;

	m_nMaxSize	= nMaxPkgSize;
	m_Event		= WSACreateEvent();
	m_Buffer	= new char[m_nMaxSize];
	m_hWndMain	= hWnd;
	m_nPort		= nPort;
	m_Mode		= nmd;
	g_bLF		= bSaveLog;

	if (pIP)
		sprintf_s(m_pIP, "%s", pIP);

	wVersion = MAKEWORD(2, 0);

	if ((nRes = WSAStartup(wVersion, &wsaData)) != 0)
		return ZFX_FAIL;

	// create master socket object listening
	if (m_Mode == NMD_SERVER) {
		if (FAILED(CreateServer(&m_pSockObj)))
			return ZFX_FAIL;
	}
	// create socket object as client
	else if (m_Mode == NMD_CLIENT) {
		if (strcmp(m_pIP, "") == 0)
			sprintf(m_pIP, "LOCALHOST");

		if (FAILED(CreateClient(&m_pSockObj)))
			return ZFX_FAIL;
	}
	else return ZFX_INVALIDPARAM;

	m_bRunning = true;
	return ZFX_OK;
}	//	Init

HRESULT ZFXWS::CreateServer(ZFXSocketObject** ppSkObject)
{
	UINT nEvents = 0;

	(*ppSkObject) = new ZFXSocketObject(m_pLog);

	if (!(*ppSkObject))
		return ZFX_FAIL;

	// 1. Step: create a socket object
	if (FAILED((*ppSkObject)->CreateSocket()))
		return ZFX_FAIL;

	// 2. Step: bind to port
	if (FAILED((*ppSkObject)->Bind(m_nPort)))
		return ZFX_FAIL;

	// 3. Step: set to listening guard
	if (FAILED((*ppSkObject)->Listen()))
		return ZFX_FAIL;

	nEvents |= FD_READ | FD_WRITE | FD_CONNECT | FD_ACCEPT | FD_CLOSE;

	// 4. Step: set Windows notification
	/*if (WSAAsyncSelect((*ppSkObject)->GetSocket(), m_hWndMain, WM_ZFXSERVER, nEvents) == SOCKET_ERROR) {
		m_pSockObj->Disconnect();
		return ZFX_FAIL;
	}*/
	if (WSAEventSelect((*ppSkObject)->GetSocket(), m_Event, nEvents) == SOCKET_ERROR) {
		m_pSockObj->Disconnect();
		return ZFX_FAIL;
	}

	// set all clients as invalid
	for (int i = 0; i < 256; ++i)
	{
		m_Clients[i].skToClient = INVALID_SOCKET;
		m_Clients[i].nID = 0;
	}
	return ZFX_OK;
}	//	CreateServer

HRESULT ZFXWS::CreateClient(ZFXSocketObject** ppSkObject)
{
	UINT nEvents = 0;

	(*ppSkObject) = new ZFXSocketObject(m_pLog);

	if (!(*ppSkObject))
		return ZFX_FAIL;

	// 1. Step: create socket object
	if (FAILED((*ppSkObject)->CreateSocket()))
		return ZFX_FAIL;

	if (m_pIP == NULL)
		gethostname(m_pIP, 10);

	// 2. Step: try to connect
	if (FAILED((*ppSkObject)->Connect(m_pIP, m_nPort)))
		return ZFX_FAIL;

	nEvents |= FD_READ | FD_CLOSE;

	// 3. Step: set Windows notification
	/*if (WSAAsyncSelect((*ppSkObject)->GetSocket(), m_hWndMain, WM_SOCKET, nEvents) == SOCKET_ERROR) {
		m_pSockObj->Disconnect();
		return ZFX_FAIL;
	}*/
	if (WSAEventSelect((*ppSkObject)->GetSocket(), m_Event, nEvents) == SOCKET_ERROR) {
		m_pSockObj->Disconnect();
		return ZFX_FAIL;
	}
	return ZFX_OK;
}	//	Create Client

HRESULT ZFXWS::SendToServer(const ZFXPACKAGE* pPkg)
{
	int nBytes = 0;
	int nSize = g_PkgSize + pPkg->nLength;

	if (m_Mode != NMD_CLIENT)
		return ZFX_FAIL;
	if (nSize > m_nMaxSize)
		return ZFX_OUTOFMEMORY;

	// data serialization
	memcpy(m_Buffer, pPkg, g_PkgSize);
	memcpy(m_Buffer + g_PkgSize, pPkg->pData, pPkg->nLength);

	nBytes = m_pSockObj->Send(m_Buffer, nSize);
	if ((nBytes == SOCKET_ERROR) || (nBytes < nSize))
		return ZFX_FAIL;
	return ZFX_OK;
}	//	SendToServer

HRESULT ZFXWS::SendToClients(const ZFXPACKAGE* pPkg)
{
	HRESULT	hr = ZFX_OK;
	int		nBytes = 0;
	int		nSize = g_PkgSize + pPkg->nLength;

	if (m_Mode != NMD_SERVER)
		return ZFX_FAIL;
	if (nSize > m_nMaxSize)
		return ZFX_OUTOFMEMORY;

	// data serialization
	memcpy(m_Buffer, pPkg, g_PkgSize);
	memcpy(m_Buffer + g_PkgSize, pPkg->pData, pPkg->nLength);

	for (UINT i = 0; i < m_ClCount; ++i)
	{
		if (m_Clients[i].skToClient != INVALID_SOCKET) {
			nBytes = m_pSockObj->Send(m_Buffer, nSize, m_Clients[i].skToClient);
			if ((nBytes == SOCKET_ERROR) || (nBytes < nSize))
				hr = ZFX_FAIL;
		}
	}
	return hr;
}	//	SendToClients

HRESULT ZFXWS::MsgProc(WPARAM wp, LPARAM lp)
{
	WORD wEvent, wError;

	wError = HIWORD(lp);
	wEvent = LOWORD(lp);

	// evaluate which event occurred
	switch (wEvent) {
		// new client is accepted
		case FD_CONNECT:	break;

		// new client is knocking
		case FD_ACCEPT:		{ return OnAccept(); } break;
	
		// there is data to be received
		case FD_READ:		{ return OnReceive(wp); } break;

		// a socket is closing
		case FD_CLOSE:		{ return OnDisconnect(wp); } break;
	
		// after sending data
		case FD_WRITE:		break;
	}
	return ZFX_OK;
}	//	MsgProc

HRESULT ZFXWS::OnAccept(void)
{
	int nSize = 0, nBytes = 0, i = m_ClCount;

	if (m_ClCount >= 255)
		return ZFX_OUTOFMEMORY;

	if (FAILED(m_pSockObj->Accept(&(m_Clients[i].skToClient))))
		return ZFX_FAIL;

	// SEND THE ID TO THE NEW CLIENT:
	ZFXPACKAGE* pPkg = (ZFXPACKAGE*)m_Buffer;
	pPkg->pData		= &m_Buffer[g_PkgSize];
	pPkg->nLength	= sizeof(UINT);
	pPkg->nType		= 0;		// ID Msg
	pPkg->nSender	= 0;		// Server
	memcpy(pPkg->pData, &m_ClID, sizeof(UINT));

	// increase counter
	m_Clients[i].nID = m_ClID;
	++m_ClCount;
	++m_ClID;

	nSize = g_PkgSize + pPkg->nLength;
	nBytes = m_pSockObj->Send(m_Buffer, nSize, m_Clients[i].skToClient);
	if ((nBytes == SOCKET_ERROR) || (nBytes < nSize))
		return ZFX_FAIL;

	// INFORM ALL OTHER CLIENTS ABOUT THE NEW GUY IN TOWN
	pPkg->nType = 1;
	SendToClients(pPkg);
	return ZFX_OK;
}	//	OnAccept

HRESULT ZFXWS::OnDisconnect(SOCKET skDisconnecting)
{
	ZFXPACKAGE	Pkg;
	UCHAR		i = 0;

	if (skDisconnecting == INVALID_SOCKET)
		return ZFX_FAIL;

	if (m_Mode == NMD_SERVER) {
		// delete from list
		for (i = 0; i < m_ClCount; ++i)
		{
			if (m_Clients[i].skToClient == skDisconnecting)
				break;
		}
		if (i >= m_ClCount)
			return ZFX_FAIL;

		// close the socket
		shutdown(m_Clients[i].skToClient, 0x02);
		closesocket(m_Clients[i].skToClient);
		m_Clients[i].skToClient = INVALID_SOCKET;

		// INFORM THE OTHERS
		Pkg.pData	= &m_Buffer[g_PkgSize];
		Pkg.nLength = sizeof(UINT);
		Pkg.nType	= 2;		// ID disconnecting Message
		Pkg.nSender = 0;		// Server
		memcpy(Pkg.pData, &m_Clients[i].nID, sizeof(UINT));

		SendToClients(&Pkg);

		// copy last entry to free position
		memcpy(&m_Clients[i], &m_Clients[m_ClCount - 1], sizeof(ZFXCLIENT));
		--m_ClCount;
	}
	else {
		shutdown(m_pSockObj->GetSocket(), 0x02);
		closesocket(m_pSockObj->GetSocket());
	}
	return ZFX_OK;
}	//	OnDisconnect

HRESULT ZFXWS::OnReceive(SOCKET skReceiving)
{
	if (m_bRunning)
		return m_pSockObj->Receive(skReceiving);
	else
		return ZFX_FAIL;
}	//	OnReceive