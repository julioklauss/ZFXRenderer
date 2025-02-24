#pragma once

#include <windows.h>
#include <ZFX.h>

typedef struct ZFXPACKAGE_TYPE
{
	UINT	nLength;		// length of pData in bytes
	UCHAR	nType;			// message type id
	UINT	nSender;		// sender id where 0 is the server
	void*	pData;			// actual data in this package
} ZFXPACKAGE;
/*	Reserved Values For nType:
0 - First message client receives from server after connection is established. pData should contain client ID as unsigned int
1 - Message from server to all clients that new client has been added to network. pData contains new client's ID
2 - Message from server to all clients that one client has been disconnected. pData contains disconnected client's ID
*/

typedef enum ZFXNETMODE_TYPE
{
	NMD_SERVER = 0,
	NMD_CLIENT = 1
} ZFXNETMODE;

class ZFXNetworkDevice
{
	protected:
		HWND		m_hWndMain;		// window handle
		HINSTANCE	m_hDLL;			// DLL handle
		bool		m_bRunning;
		int			m_nPort;		// port number
		char		m_pIP[256];		// IP address
		UINT		m_nMaxSize;		// buffer size

	public:
		ZFXNetworkDevice(void) {};
		virtual ~ZFXNetworkDevice(void) {};

		// init and release
		virtual	HRESULT	Init(HWND, ZFXNETMODE, int Port, char* IP, UINT size, bool) = 0;
		virtual	void	Release(void) = 0;
		virtual bool	IsRunning(void) = 0;

		// message procedure
		virtual HRESULT	MsgProc(WPARAM, LPARAM) = 0;

		// sending and receiving
		virtual HRESULT	SendToServer(const ZFXPACKAGE*) = 0;
		virtual HRESULT SendToClients(const ZFXPACKAGE*) = 0;

		// information about the inbox
		virtual bool	IsPkgWaiting(void) = 0;
		virtual UINT	GetNextPkgSize(void) = 0;
		virtual HRESULT	GetNextPkg(ZFXPACKAGE*) = 0;
};	//	class
typedef class ZFXNetworkDevice* LPZFXNETWORKDEVICE;

/*----------------------------------------------------------------*/

extern "C"
{
	HRESULT CreateNetworkDevice(HINSTANCE hDLL, ZFXNetworkDevice** pInterface);
	typedef HRESULT(*CREATENETWORKDEVICE) (HINSTANCE hDLL, ZFXNetworkDevice** pInterface);

	HRESULT ReleaseNetworkDevice(ZFXNetworkDevice** pInterface);
	typedef HRESULT(*RELEASENETWORKDEVICE) (ZFXNetworkDevice** pInterface);
}