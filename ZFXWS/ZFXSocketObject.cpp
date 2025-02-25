#include <WS2tcpip.h>
#include "ZFXSocketObject.h"

extern int g_PkgSize;

ZFXSocketObject::ZFXSocketObject(FILE* pLog)
{
	m_skSocket	= INVALID_SOCKET;
	m_bRunning	= false;
	m_pInbox	= NULL;
	m_Buffer	= NULL;
}	//	constructor
/*---------------------------------------------------------*/

ZFXSocketObject::~ZFXSocketObject(void)
{
	if (IsRunning()) {
		Disconnect();
		m_bRunning = false;
	}
	if (m_pInbox) {
		delete m_pInbox;
		m_pInbox = NULL;
	}
	if (m_Buffer) {
		delete[] m_Buffer;
		m_Buffer = NULL;
	}
	m_skSocket = INVALID_SOCKET;
}	//	destructor

HRESULT ZFXSocketObject::CreateSocket(void)
{
	if (m_skSocket != INVALID_SOCKET)
		Disconnect();

	m_skSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_skSocket == INVALID_SOCKET) 
		return ZFX_FAIL;

	m_pInbox = new ZFXQueue();
	m_Buffer = new char[65536];
	memset(m_Buffer, 0, 65536);
	return ZFX_OK;
}

HRESULT ZFXSocketObject::Bind(int nPort)
{
	sockaddr_in saServerAddress;

	memset(&saServerAddress, 0, sizeof(sockaddr_in));
	saServerAddress.sin_family		= AF_INET;
	saServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	saServerAddress.sin_port		= htons(nPort);

	if (bind(m_skSocket, (sockaddr*)&saServerAddress, sizeof(sockaddr)) == SOCKET_ERROR) {
		Disconnect();
		return ZFX_FAIL;
	}
	return ZFX_OK;
}	//	Bind

HRESULT ZFXSocketObject::Listen(void)
{
	if (listen(m_skSocket, 32) != 0)
		return ZFX_FAIL;

	m_bRunning = true;
	return ZFX_OK;
}	//	Listen

HRESULT ZFXSocketObject::Accept(SOCKET* skToNewClient)
{
	sockaddr_in	saClientAddress;
	int			nClientSize = sizeof(sockaddr_in);

	(*skToNewClient) = accept(m_skSocket, (sockaddr*)&saClientAddress, &nClientSize);

	if ((*skToNewClient) == INVALID_SOCKET)
		return ZFX_FAIL;

	return ZFX_OK;
}	//	accept

HRESULT ZFXSocketObject::Connect(char* chServer, int nPort)
{
	sockaddr_in	saServerAddress;
	// LPHOSTENT	pHost = NULL;
	ADDRINFO*	pAddrInfo = NULL;

	// try to find the server
	memset(&saServerAddress, 0, sizeof(sockaddr_in));
	saServerAddress.sin_port		= htons(nPort);
	saServerAddress.sin_family		= AF_INET;
	// saServerAddress.sin_addr.s_addr	= inet_addr(chServer);
	inet_pton(AF_INET, chServer, &(saServerAddress.sin_addr.s_addr));

	if (saServerAddress.sin_addr.s_addr == INADDR_NONE) {
		// pHost = gethostbyname(chServer);
		getaddrinfo(chServer, NULL, NULL, &pAddrInfo);

		/* if (pHost != NULL) {
			saServerAddress.sin_addr.s_addr = ((LPIN_ADDR)pHost->h_addr)->s_addr;
		} */
		if (pAddrInfo != NULL) {
			while (pAddrInfo->ai_family != AF_INET)
			{
				if (pAddrInfo->ai_next == NULL)
					return ZFX_FAIL;
				pAddrInfo = pAddrInfo->ai_next;
			}
			saServerAddress.sin_addr = ((sockaddr_in*)pAddrInfo->ai_addr)->sin_addr;
		}
		else
			return ZFX_FAIL;
	}

	// connect to server address
	if (connect(m_skSocket, (sockaddr*)&saServerAddress, sizeof(sockaddr)) == SOCKET_ERROR) {
		Disconnect();
		return ZFX_FAIL;
	}

	m_bRunning = true;
	return ZFX_OK;
}	//	Connect

void ZFXSocketObject::Disconnect(void)
{
	if (m_skSocket != INVALID_SOCKET) {
		shutdown(m_skSocket, SD_BOTH);
		closesocket(m_skSocket);
		m_skSocket = INVALID_SOCKET;
	}
}	//	Disconnect

int ZFXSocketObject::Send(const char* pPkg, UINT nSize)
{
	UINT nSent = 0;
	UINT n = 0;

	while (nSent < nSize)
	{
		n = send(m_skSocket, pPkg + nSent, nSize - nSent, 0);
		if (n == SOCKET_ERROR)
			return n;
		else nSent += n;
	}
	return nSent;
}	//	Send
/*---------------------------------------------------------*/

int ZFXSocketObject::Send(const char* pPkg, UINT nSize, SOCKET skReceiver)
{
	UINT nSent = 0;
	UINT n = 0;

	while (nSent < nSize)
	{
		n = send(skReceiver, pPkg + nSent, nSize - nSent, 0);
		if (n == SOCKET_ERROR)
			return n;
		else nSent += n;
	}
	return nSent;
}	//	Send

HRESULT ZFXSocketObject::Receive(SOCKET sk)
{
	HRESULT	hr			= ZFX_OK;
	UINT	nSize		= 65536;	// max bytes for each read call
	UINT	nBytesRead	= 0;		// read bytes in one call
	UINT	nReadHead	= 0;		// Position in m_Buffer
	UINT	n			= 0;		// size of leftover data from the last call
	bool	bDone		= false;	// finished?

	ZFXPACKAGE*	pPkg			= NULL;
	UINT		nPkgSizeTotal	= 0;

	// read up to 65536 Bytes in each call
	// loop until no more data is waiting
	while (!bDone)
	{
		nBytesRead = recv(sk, &m_Buffer[n], nSize - n, 0);

		if (nBytesRead == SOCKET_ERROR) {
			int WSAError = WSAGetLastError();

			// ignore harmless messages
			if ((WSAError != WSAEMSGSIZE) && (WSAError != WSAEWOULDBLOCK)) {
				hr = ZFX_FAIL;
				bDone = true;
				break;
			}
		}

		// now we have nBytesRead bytes in m_Buffer
		if (nBytesRead <= 0)
			bDone = true;
		else {
			// take care of old data in the buffer
			nBytesRead += n;

			// loop until complete header is found
			while ((nBytesRead - nReadHead) > g_PkgSize)
			{
				// next bunch of data
				pPkg = (ZFXPACKAGE*)&m_Buffer[nReadHead];
				pPkg->pData = &m_Buffer[nReadHead] + g_PkgSize;

				nPkgSizeTotal = g_PkgSize + pPkg->nLength;

				// did we get the whole package?
				if ((nBytesRead - nReadHead) >= (nPkgSizeTotal)) {
					m_pInbox->Enqueue(pPkg, nPkgSizeTotal);
					nReadHead += nPkgSizeTotal;
				}
				// no back to recv()
				else {
					// copy half package to the start of the buffer
					memcpy(m_Buffer, &m_Buffer[nReadHead], nBytesRead - nReadHead);
					n = nBytesRead - nReadHead;
					break;
				}
			}	// while

			// now we got all data waiting for us
			if (nBytesRead < nSize)
				bDone = true;
		}
	}	// while
	return hr;
}	//	Receive

HRESULT ZFXSocketObject::GetNextPkg(ZFXPACKAGE* pPkg)
{
	// anything at all?
	if (m_pInbox->GetCount() > 0) {
		// draw data into the buffer
		m_pInbox->Front(m_Buffer, true);

		// fill structure
		memcpy(pPkg, m_Buffer, g_PkgSize);
		memcpy(pPkg->pData, m_Buffer + g_PkgSize, pPkg->nLength);
		return ZFX_OK;
	}
	return ZFX_FAIL;
}	//	GetNextPkg