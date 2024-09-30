#include "ZFX3D.h"

CPUINFO GetCPUInfo() 
{
	CPUINFO		info;
	char*		pStr = info.vendor;
	int			n = 1;
	int*		pn = &n;

	memset(&info, 0, sizeof(CPUINFO));

	// 1: Vendor name, SSE2, SSE, MMX support
	__try {
		__asm {
			mov	eax, 0			// get vendor name
			CPUID

			mov	esi,	pStr
			mov [esi],	ebx		// first 4 Chars
			mov [esi + 4], edx	// next 4 Chars
			mov [esi + 8], ecx	// final 4 Chars

			mov eax, 1			// Feature-List
			CPUID

			test edx, 04000000h	// test SSE2
			jz	 _NOSSE2		// jump if negative
			mov [info.bSSE2], 1	// true

	_NOSSE2:test edx, 02000000h // test SSE
			jz	_NOSSE			// jump if negative
			mov [info.bSSE], 1	// true

	_NOSSE:	test edx, 00800000h	// test MMX
			jz _EXIT1			// jump if negative
			mov	[info.bMMX], 1	// true

	_EXIT1: // done
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (_exception_code() == STATUS_ILLEGAL_INSTRUCTION)
			return info;		// CPU inactive
		return info;			// unexpected error
	}

	// 2: Test Extended Features
	__asm {
		mov eax, 80000000h		// Extended Features?
		CPUID
		cmp eax, 80000000h		// > 0x80?
		jbe	_EXIT2				// jump if negative
		mov [info.bEXT], 1		// true

		mov eax, 80000000h		// Feat-Bits to EDX
		CPUID
		test edx, 80000000h		// test 3DNow!
		jz	_EXIT2				// jump if negative
		mov [info.b3DNOW], 1	// true
_EXIT2:	// fertig
	}

	// 3: vendor dependent things
	//	  INTEL: CPU id
	//	  AMD:	 CPU id, 3dnow_ex, mmx_Ex
	if ((strncmp(info.vendor, "GenuineIntel", 12) == 0) && info.bEXT) {
		// INTEL
		_asm {
			mov	eax, 1			// Feature-List
			CPUID
			mov esi, pn			// Brand-ID
			mov [esi], ebx
		}
		int m = 0;
		memcpy(&m, pn, sizeof(char));	// only lower 8 bit
		n = m;
	}
	else if ((strncmp(info.vendor, "AuthenticAMD", 12) == 0) && info.bEXT) {
		// AMD
		_asm {
			mov eax, 1				// Feature-List
			CPUID
			mov esi, pn				// CPU-Type
			mov [esi], eax

			mov eax, 0x80000001		// Ext.Feat. Bits
			CPUID

			test edx, 0x40000000	// AMD extended 3DNow!
			jz	_AMD1				// jump on error
			mov [info.b3DNOWEX], 1	// true
	_AMD1:	test edx, 0x00400000	// AMD extended MMX
			jz	_AMD2				// jump if negative
			mov [info.bMMXEX], 1	// true
	_AMD2:
		}
	}
	else {
		if (info.bEXT)
			;	/* UNKNOWN VENDOR */
		else
			;	/* NO Extended-Feature-List */
	}

	info.vendor[13] = '\0';
	GetCPUName(info.name, n, info.vendor);
	return info;
}

/**
* Get name to found processor id.
*/
void GetCPUName(char* chName, int n, const char* vendor)
{
	// Intel processors
	if (strncmp(vendor, "GenuineIntel", 12) == 0) {
		switch (n) {
		case 0:		sprintf_s(chName, 200, "< Pentium III/Celeron");
					break;
		case 1:		sprintf_s(chName, 200, "Pentium Celeron (1)");
					break;
		case 2:		sprintf_s(chName, 200, "Pentium III (2)");
					break;
		case 3:		sprintf_s(chName, 200, "Pentium III Xeon/Celeron");
					break;
		case 4:		sprintf_s(chName, 200, "Pentium III (4)");
					break;
		case 6:		sprintf_s(chName, 200, "Pentium III-M");
					break;
		case 7:		sprintf_s(chName, 200, "Pentium Celeron (7)");
					break;
		case 8:		sprintf_s(chName, 200, "Pentium IV (Genuine)");
					break;
		case 9:		sprintf_s(chName, 200, "Pentium IV");
					break;
		case 10:	sprintf_s(chName, 200, "Pentium Celeron (10)");
					break;
		case 11:	sprintf_s(chName, 200, "Pentium Xeon / Xeon-MP");
					break;
		case 12:	sprintf_s(chName, 200, "Pentium Xeon-MP");
					break;
		case 14:	sprintf_s(chName, 200, "Pentium IV-M / Xeon");
					break;
		case 15:	sprintf_s(chName, 200, "Pentium Celeron (15)");
					break;
		default:	sprintf_s(chName, 200, "Unknown Intel");
					break;
		} 
	}
	else if (strncmp(vendor, "AuthenticAMD", 12) == 0) {
		switch (n) {
		case 1660:	sprintf_s(chName, 200, "AthLon / Duron (Model-7)");
					break;
		case 1644:	sprintf_s(chName, 200, "AthLon / Duron (Model-6)");
					break;
		case 1596:	sprintf_s(chName, 200, "AthLon / Duron (Model-3)");
					break;
		case 1612:	sprintf_s(chName, 200, "Athlon (Model-4)");
					break;
		case 1580:	sprintf_s(chName, 200, "AthLon (Model-2)");
					break;
		case 1564:	sprintf_s(chName, 200, "AthLon (Model-1)");
					break;
		case 1463:	sprintf_s(chName, 200, "K6-III (Model-9)");
					break;
		case 1420:	sprintf_s(chName, 200, "K6-2 (Model-8)");
					break;
		case 1404:	sprintf_s(chName, 200, "K6 (Model-7)");
					break;
		case 1388:	sprintf_s(chName, 200, "K6 (Model-6)");
					break;
		case 1340:	sprintf_s(chName, 200, "K5 (Model-3)");
					break;
		case 1324:	sprintf_s(chName, 200, "K5 (Model-2)");
					break;
		case 1308:	sprintf_s(chName, 200, "K5 (Model-1)");
					break;
		case 1292:	sprintf_s(chName, 200, "K5 (Model-0)");
					break;
		default:	sprintf_s(chName, 200, "Unknown AMD");
					break;
		}
	}
	return;
}

bool OSSupportSSE()
{
	__try {
		__asm xorps xmm0, xmm0
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (_exception_code() == STATUS_ILLEGAL_INSTRUCTION)
			return false;		// sse not supported
		return false;			// unexpected error
	}
	return true;
}

bool ZFX3DInitCPU() 
{
	CPUINFO info = GetCPUInfo();
	bool	bOS  = OSSupportSSE();

	if (info.bSSE && bOS)
		g_bSSE = true;

	return g_bSSE;
}