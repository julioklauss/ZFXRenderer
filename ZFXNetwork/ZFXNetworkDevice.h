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
