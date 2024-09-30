#include "ZFX.h"
#include "ZFXD3D.h"
#include "ZFXD3D_skinman.h"
#include "ZFXD3D_vcache.h"
#include "resource.h"
#include <Vfw.h>

// variables for callbacks to dialog
ZFXDEVICEINFO	g_xDevice;
D3DDISPLAYMODE	g_Dspmd;
D3DFORMAT		g_fmtA;
D3DFORMAT		g_fmtB;

// Initialization, Enumeration, Shutdown
ZFXD3D* g_ZFXD3D = NULL;

// Initializations Process Chain
HBITMAP g_hBMP;

BOOL WINAPI DllEntryPoint(HINSTANCE hDll, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH	: break;
	case DLL_PROCESS_DETACH	: break;
	default					: break;
	}

	return TRUE;
}	// DllEntryPoint
/*---------------------------------------------------------*/

// Create a new ZFXD3D instance
extern "C" __declspec(dllexport) HRESULT CreateRenderDevice(HINSTANCE hDLL, ZFXRenderDevice * *pDevice)
{
	if (!*pDevice)
	{
		*pDevice = new ZFXD3D(hDLL);
		return ZFX_OK;
	}
	return ZFX_FAIL;
}

// Clears the ZFXD3D instance 
extern "C" __declspec(dllexport) HRESULT ReleaseRenderDevice(ZFXRenderDevice * *pDevice)
{
	if (!*pDevice)
	{
		return ZFX_FAIL;
	}
	delete* pDevice;
	*pDevice = NULL;
	return ZFX_OK;
}

ZFXD3D::ZFXD3D(HINSTANCE hDLL)
{
	m_hDLL				= hDLL;
	m_pEnum				= NULL;
	m_pD3D				= NULL;
	m_pDevice			= NULL;
	m_pLog				= NULL;
	m_ClearColor		= D3DCOLOR_COLORVALUE( 0.0f, 0.0f, 0.0f, 1.0f );
	
	m_bRunning			= false;
	m_bIsScreenRunning	= false;
	
	m_nActiveWnd		= 0;
	
	g_ZFXD3D			= this;
}

ZFXD3D::~ZFXD3D()
{
	if (m_pLog)
		fflush(m_pLog);
	Release();
}

void ZFXD3D::Release()
{
	if (m_pEnum)
	{
		delete m_pEnum;
		m_pEnum = NULL;
	}
	if (m_pDevice)
	{
		m_pDevice->Release();
		m_pDevice = NULL;
	}
	if (m_pD3D)
	{
		m_pD3D->Release();
		m_pD3D = NULL;
	}
	fclose(m_pLog);
}
// End Initialization, Enumeration, Shutdown

BOOL CALLBACK DlgProcWrap(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	return g_ZFXD3D->DlgProc(hDlg, message, wParam, lParam);
}

BOOL CALLBACK ZFXD3D::DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DIBSECTION	dibSection;
	BOOL		bWnd = FALSE;

	// get handlers
	HWND hFULL = GetDlgItem(hDlg, IDC_FULL);
	HWND hWND = GetDlgItem(hDlg, IDC_WND);
	HWND hADAPTER = GetDlgItem(hDlg, IDC_ADAPTER);
	HWND hMODE = GetDlgItem(hDlg, IDC_MODE);
	HWND hADAPTERFMT = GetDlgItem(hDlg, IDC_ADAPTERFMT);
	HWND hBACKFMT = GetDlgItem(hDlg, IDC_BACKFMT);
	HWND hDEVICE = GetDlgItem(hDlg, IDC_DEVICE);

	switch (message)
	{
		// preselect windowed mode
		case WM_INITDIALOG:
		{
			SendMessage(hWND, BM_SETCHECK, BST_CHECKED, 0);
			m_pEnum->Enum(hADAPTER, hMODE, hDEVICE, hADAPTERFMT, hBACKFMT, hWND, hFULL, m_pLog);
			return TRUE;
		}
		// render logo ( g_hBMP is initialized in Init() )
		case WM_PAINT:
		{
			if (g_hBMP)
			{
				GetObject(g_hBMP, sizeof(DIBSECTION), &dibSection);
				HDC			hdc = GetDC(hDlg);
				HDRAWDIB	hdd = DrawDibOpen();
				DrawDibDraw(hdd, hdc, 50, 10, 95, 99, &dibSection.dsBmih, dibSection.dsBm.bmBits, 0, 0, dibSection.dsBmih.biWidth, dibSection.dsBmih.biHeight, 0);
				DrawDibClose(hdd);
				ReleaseDC(hDlg, hdc);
			}
		} break;

		// a control reports a message
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				// OK Button
				case IDOK:
				{
					m_bWindowed = SendMessage(hFULL, BM_GETCHECK, 0, 0) != BST_CHECKED;
					m_pEnum->GetSelections(&g_xDevice, &g_Dspmd, &g_fmtA, &g_fmtB);
					GetWindowText(hADAPTER, m_chAdapter, 256);
					EndDialog(hDlg, 1);
					return TRUE;
				} break;

				// cancel button
				case IDCANCEL:
				{
					EndDialog(hDlg, 0);
					return true;
				} break;

				case IDC_ADAPTER:
				{
					if (HIWORD(wParam) == CBN_SELCHANGE)
						m_pEnum->ChangedAdapter();
				} break;

				case IDC_DEVICE:
				{
					if (HIWORD(wParam) == CBN_SELCHANGE)
						m_pEnum->ChangedDevice();
				} break;

				case IDC_ADAPTERFMT:
				{
					if (HIWORD(wParam) == CBN_SELCHANGE)
						m_pEnum->ChangedAdapterFmt();
				} break;

				case IDC_FULL: case IDC_WND:
				{
					m_pEnum->ChangedWindowMode();
				} break;
			} // switch [CMD]
		} break; // case [CMD]
	}	// swicth [MSG]
	return FALSE;
}

HRESULT ZFXD3D::Init(HWND hWnd, const HWND* hWnd3D, int nNumhWnd, int nMinDepth, int nMinStencil, bool bSaveLog)
{
	int nResult;

	fopen_s(&m_pLog, "log_renderdevice.txt", "w");
	if (!m_pLog) return ZFX_FAIL;

	// Should I use child windows??
	if (nNumhWnd > 0)
	{
		if (nNumhWnd > MAX_3DHWND)
			nNumhWnd = MAX_3DHWND;
		memcpy(&m_hWnd[0], hWnd3D, sizeof(HWND) * nNumhWnd);
		m_nNumhWnd = nNumhWnd;
	} // else use main window handle
	else
	{
		m_hWnd[0] = hWnd;
		m_nNumhWnd = 0;
	}
	m_hWndMain = hWnd;

	if (nMinStencil > 0)
		m_bStencil = true;

	Log("ZFXEngine ZFXD3D-RenderDevice Log File:\n\n");

	// generate enum object
	m_pEnum = new ZFXD3DEnum(nMinDepth, nMinStencil);

	Log("calling dialog... \n");

	// load ZFX logo
	g_hBMP = (HBITMAP)LoadImage(NULL, "zfx.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);

	// open up dialog
	nResult = DialogBox(m_hDLL, "dlgChangeDevice", hWnd, DlgProcWrap);

	// free resources
	if (g_hBMP)
		DeleteObject(g_hBMP);
	
	Log("returning from dialog... \n");

	// error in dialog
	if (nResult == -1)
	{
		Log("selection dialog error \n");
		return ZFX_FAIL;
	}
	// dialog canceled by user
	else if (nResult == 0)
	{
		Log("selection dialog canceled by user\n");
		return ZFX_CANCELED;
	}
	// dialog OK
	else
	{
		Log("selection dialog ok\nFiring up direct3d\n");
		return Go();
	}
}

// Direct3D device
HRESULT ZFXD3D::Go(void)
{
	ZFXCOMBOINFO	xCombo;
	HRESULT			hr;
	HWND			hwnd;

	// create Direct3D main object
	if (m_pD3D)
	{
		m_pD3D->Release();
		m_pD3D = NULL;
	}
	m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);

	if (!m_pD3D)
	{
		Log("error: Direct3DCreate9()\n");
		return ZFX_CREATEAPI;
	}

	// get fitting combo
	for (UINT i = 0; i < g_xDevice.nNumCombo; ++i)
	{
		if ((g_xDevice.d3dCombo[i].bWindowed == m_bWindowed)
			&& (g_xDevice.d3dCombo[i].d3dDevType == g_xDevice.d3dDevType)
			&& (g_xDevice.d3dCombo[i].fmtAdapter == g_fmtA)
			&& (g_xDevice.d3dCombo[i].fmtBackBuffer == g_fmtB))
		{
			xCombo = g_xDevice.d3dCombo[i];
			break;
		}
	}

	// fill in present parameters structure
	ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));
	m_d3dpp.Windowed				= m_bWindowed;
	m_d3dpp.BackBufferCount			= 1;
	m_d3dpp.BackBufferFormat		= g_Dspmd.Format;
	m_d3dpp.EnableAutoDepthStencil	= TRUE;
	m_d3dpp.MultiSampleType			= xCombo.msType;
	m_d3dpp.AutoDepthStencilFormat	= xCombo.fmtDepthStencil;
	m_d3dpp.SwapEffect				= D3DSWAPEFFECT_DISCARD;

	// stencil buffer active?
	if ((xCombo.fmtDepthStencil == D3DFMT_D24S8)
		|| (xCombo.fmtDepthStencil == D3DFMT_D24X4S4)
		|| (xCombo.fmtDepthStencil == D3DFMT_D15S1))
		m_bStencil = true;
	else
		m_bStencil = false;

	// fullscreen mode
	if (!m_bWindowed)
	{
		m_d3dpp.hDeviceWindow		= hwnd = m_hWndMain;
		m_d3dpp.BackBufferWidth		= g_Dspmd.Width;
		m_d3dpp.BackBufferHeight	= g_Dspmd.Height;
		ShowCursor(FALSE);
	}
	// windowed mode
	else
	{
		m_d3dpp.hDeviceWindow		= hwnd = m_hWnd[0];
		m_d3dpp.BackBufferWidth		= GetSystemMetrics(SM_CXSCREEN);
		m_d3dpp.BackBufferHeight	= GetSystemMetrics(SM_CYSCREEN);
	}

	// create direct3d device
	hr = m_pD3D->CreateDevice(g_xDevice.nAdapter, g_xDevice.d3dDevType, hwnd, xCombo.dwBehavior, &m_d3dpp, &m_pDevice);

	// create swap chains if needed
	if ((m_nNumhWnd > 0) && m_bWindowed)
	{
		for (UINT i = 0; i < m_nNumhWnd; ++i)
		{
			m_d3dpp.hDeviceWindow = m_hWnd[i];
			m_pDevice->CreateAdditionalSwapChain(&m_d3dpp, &m_pChain[i]);
		}
	}

	delete m_pEnum;
	m_pEnum = NULL;

	if (FAILED(hr))
	{
		Log("error: IDirect3D::CreateDevice() \n");
		return ZFX_CREATEDEVICE;
	}

	Log("Initialized OK... \n");

	m_bRunning = true;
	m_bIsScreenRunning = false;

	m_dwWidth	= m_d3dpp.BackBufferWidth;
	m_dwHeight	= m_d3dpp.BackBufferHeight;
	return OneTimeInit();
} // Go

HRESULT ZFXD3D::OneTimeInit(void)
{
	ZFX3DInitCPU();

	m_bUseShaders = true;

	m_pSkinMan = new ZFXD3DSkinManager(m_pDevice, m_pLog);

	m_pVertexMan = new ZFXD3DVCManager((ZFXD3DSkinManager*)m_pSkinMan, m_pDevice, this, 3000, 4500, m_pLog);

	// activate render states
	m_pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
	m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	m_pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);

	// create standard-material
	memset(&m_StdMtrl, 0, sizeof(D3DMATERIAL9));
	m_StdMtrl.Ambient.r = 1.0f;
	m_StdMtrl.Ambient.g = 1.0f;
	m_StdMtrl.Ambient.b = 1.0f;
	m_StdMtrl.Ambient.a = 1.0f;

	if (FAILED(m_pDevice->SetMaterial(&m_StdMtrl))) {
		Log("error: set material (OneTimeInit)");
		return ZFX_FAIL;
	}

	// activate texture filtering
	m_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	m_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	m_pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

	ZFXVIEWPORT vpView = { 0, 0, m_dwWidth, m_dwHeight };
	m_Mode		= EMD_PERSPECTIVE;
	m_nStage	= -1;
	SetActiveSkinID(MAX_ID);

	// identity matrix for view matrix
	IDENTITY(m_mView3D);

	// clipping plane values
	SetClippingPlanes(0.1f, 1000.0f);

	// initialize shader stuff
	PrepareShaderStuff();

	// build default shader with ID = 0
	if (m_bUseShaders) {
		const char BaseShader[] =
			"vs.1.1					\n"\
			"dcl_position0	v0		\n"\
			"dcl_normal0	v3		\n"\
			"dcl_texcoord0	v6		\n"\
			"dp4 oPos.x, v0, c0		\n"\
			"dp4 oPos.y, v0, c1		\n"\
			"dp4 oPos.z, v0, c2		\n"\
			"dp4 oPos.w, v0, c3		\n"\
			"mov oD0, c4			\n"
			"mov oT0, v6			\n";

		if (FAILED(CreateVShader((void*)BaseShader, sizeof(BaseShader), false, false, NULL)))
			return ZFX_FAIL;

		if (FAILED(ActivateVShader(0, VID_UU)))
			return ZFX_FAIL;
	}	// default shader

	// set ambient light level
	SetAmbientLight(1.0f, 1.0f, 1.0f);

	// set perspective projection stage 0
	if (FAILED(InitStage(0.8f, &vpView, 0)))
		return ZFX_FAIL;

	// activate perspective projection stage 1
	if (FAILED(SetMode(EMD_PERSPECTIVE, 0)))
		return ZFX_FAIL;

	return ZFX_OK;
}	//	OneTimeInit


void ZFXD3D::Log(const char* chString, ...)
{
	char	ch[256];
	va_list pArgs;

	va_start(pArgs, chString);
	vsprintf_s(ch, 256, chString, pArgs);
	fprintf(m_pLog, ch);

#ifdef  _DEBUGFLUSH_
	fflush(m_pLog)
#endif //  _DEBUGFLUSH_
}