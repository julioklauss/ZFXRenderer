#include "ZFXD3D_skinman.h"

#define MAX_ID 65535
#define RGB16BIT(r, g, b) ((b % 32) + ((g % 64) << 5) + ((r % 32) << 11))

static ZFXMATERIAL EmptyMaterial;
static ZFXSKIN EmptySkin;
static ZFXTEXTURE EmptyTexture;

bool g_bLF;

void ZFXD3DSkinManager::Log(const char* chString, ...)
{
	char	ch[256];
	char*	pArgs;

	pArgs = (char*)&chString + sizeof(chString);
	vsprintf(ch, chString, pArgs);
	fprintf(m_pLog, "[ ZFXD3DSkinManager ]: ");
	fprintf(m_pLog, ch);
	fprintf(m_pLog, "\n");

	if (g_bLF)
		fflush(m_pLog);
}	//	Log
/*---------------------------------------------------------*/

ZFXD3DSkinManager::ZFXD3DSkinManager(LPDIRECT3DDEVICE9 pDevice, FILE* pLog)
{
	m_nNumMaterials = 0;
	m_nNumTextures	= 0;
	m_nNumSkins		= 0;
	m_pMaterials	= NULL;
	m_pTextures		= NULL;
	m_pSkins		= NULL;
	m_pLog			= pLog;
	m_pDevice		= pDevice;
	Log("online");
}

ZFXD3DSkinManager::~ZFXD3DSkinManager(void)
{
	// release Direct3D texture objects
	if (m_pTextures) {
		for (UINT i = 0; i < m_nNumTextures; ++i) {
			if (m_pTextures[i].pData) {
				((LPDIRECT3DTEXTURE9)(m_pTextures[i].pData))->Release();
				m_pTextures[i].pData = NULL;
			}
			if (m_pTextures[i].pClrKeys) {
				delete[] m_pTextures[i].pClrKeys;
				m_pTextures[i].pClrKeys = NULL;
			}
			if (m_pTextures[i].chName) {
				delete[] m_pTextures[i].chName;
				m_pTextures[i].chName = NULL;
			}
		}
		free(m_pTextures);
		m_pTextures = NULL;
	}

	// free memory
	if (m_pTextures) {
		free(m_pTextures);
		m_pTextures = NULL;
	}

	if (m_pSkins) {
		free(m_pSkins);
		m_pSkins = NULL;
	}

	Log("offline (ok)");
}

inline bool ZFXD3DSkinManager::ColorEqual(const ZFXCOLOR* pCol0, const ZFXCOLOR* pCol1)
{
	if ((pCol0->fA != pCol1->fA) ||
		(pCol0->fR != pCol1->fR) ||
		(pCol0->fG != pCol1->fG) ||
		(pCol0->fB != pCol1->fB))
		return false;

	return true;
}	// ColorEqual
/*---------------------------------------------------------*/

bool ZFXD3DSkinManager::MaterialEqual(const ZFXMATERIAL* pMat0, const ZFXMATERIAL* pMat1)
{
	if (!ColorEqual(&pMat0->cAmbient, &pMat1->cAmbient) ||
		!ColorEqual(&pMat0->cDiffuse, &pMat1->cDiffuse) ||
		!ColorEqual(&pMat0->cEmissive, &pMat1->cEmissive) ||
		!ColorEqual(&pMat0->cSpecular, &pMat1->cSpecular) ||
		(pMat0->fPower != pMat1->fPower))
		return false;

	return true;
}	// MaterialEqual
/*---------------------------------------------------------*/

ZFXSKIN ZFXD3DSkinManager::GetSkin(UINT nSkinID)
{
	if (nSkinID < m_nNumSkins)
		return m_pSkins[nSkinID];
	else {
		return EmptySkin;
	}
}	// GetSkin
/*---------------------------------------------------------*/

ZFXMATERIAL ZFXD3DSkinManager::GetMaterial(UINT nMatID)
{
	if (nMatID < m_nNumMaterials)
		return m_pMaterials[nMatID];
	else {
		return EmptyMaterial;
	}
}	// GetMaterial
/*---------------------------------------------------------*/

ZFXTEXTURE ZFXD3DSkinManager::GetTexture(UINT nTexID)
{
	if (nTexID < m_nNumTextures)
		return m_pTextures[nTexID];
	else
		return EmptyTexture;
}	//	GetTexture
/*---------------------------------------------------------*/

const char* ZFXD3DSkinManager::GetTextureName(UINT nID, float* pfAlpha, ZFXCOLOR* pAK, UCHAR* pNum)
{
	if (nID >= m_nNumTextures) return NULL;
	if (pfAlpha) *pfAlpha = m_pTextures[nID].fAlpha;
	if (pNum) *pNum = m_pTextures[nID].dwNum;

	if (m_pTextures[nID].pClrKeys && pAK) {
		memcpy(pAK, m_pTextures[nID].pClrKeys, sizeof(ZFXCOLOR) * m_pTextures[nID].dwNum);
	}

	return m_pTextures[nID].chName;
}	// GetTextureName
/*---------------------------------------------------------*/

HRESULT ZFXD3DSkinManager::AddSkin(const ZFXCOLOR* pcAmbient, const ZFXCOLOR* pcDiffuse, const ZFXCOLOR* pcEmissive, const ZFXCOLOR* pcSpecular, float fSpecPower, UINT* nSkinID)
{
	UINT	nMat, n;
	bool	bMat = false;

	// allocate memory for 50 objects if needed
	if ((m_nNumSkins % 50) == 0) {
		n = (m_nNumSkins + 50) * sizeof(ZFXSKIN);
		m_pSkins = (ZFXSKIN*)realloc(m_pSkins, n);
		if (!m_pSkins)
			return ZFX_OUTOFMEMORY;
	}

	ZFXMATERIAL mat;
	mat.cAmbient	= *pcAmbient;
	mat.cDiffuse	= *pcDiffuse;
	mat.cEmissive	= *pcEmissive;
	mat.cSpecular	= *pcSpecular;
	mat.fPower		= fSpecPower;

	// do we already have such a material?
	for (nMat = 0; nMat < m_nNumMaterials; ++nMat) {
		if (MaterialEqual(&mat, &m_pMaterials[nMat])) {
			bMat = true;
			break;
		}
	}	//	for [MATERIALS]

	// if yes save its id if not create a new one
	if (bMat) 
		m_pSkins[m_nNumSkins].nMaterial = nMat;
	else {
		m_pSkins[m_nNumSkins].nMaterial = m_nNumMaterials;

		// allocate memory for 50 objects if needed
		if (m_nNumMaterials % 50 == 0) {
			n = (m_nNumMaterials + 50) % sizeof(ZFXMATERIAL);
			m_pMaterials = (ZFXMATERIAL*)realloc(m_pMaterials, n);
			if (!m_pMaterials) 
				return ZFX_OUTOFMEMORY;
		}
		memcpy(&m_pMaterials[m_nNumMaterials], &mat, sizeof(ZFXMATERIAL));
		++m_nNumMaterials;
	}

	m_pSkins[m_nNumSkins].bAlpha = false;
	for (int i = 0; i < 8; ++i)
		m_pSkins[m_nNumSkins].nTexture[i] = MAX_ID;

	// save id and incremental counter
	(*nSkinID) = m_nNumSkins;
	++m_nNumSkins;

	return ZFX_OK;
}	//	AddSkin
/*---------------------------------------------------------*/

HRESULT ZFXD3DSkinManager::AddTexture(UINT nSkinID, const char* chName, bool bAlpha, float fAlpha, ZFXCOLOR* cColorKeys, DWORD dwNumColorKeys)
{
	ZFXTEXTURE* pZFXTex = NULL;		// helper pointer
	HRESULT		hr;
	UINT		nTex, n;
	bool		bTex = false;

	// is skin id valid at all?
	if (nSkinID >= m_nNumSkins)
		return ZFX_INVALIDID;

	// any of the eight texture slots free?
	if (m_pSkins[nSkinID].nTexture[7] != MAX_ID)
		return ZFX_BUFFERSIZE;

	// is this texture already loaded?
	for (nTex = 0; nTex < m_nNumTextures; ++nTex) {
		if (strcmp(chName, m_pTextures[nTex].chName) == 0) {
			bTex = true;
			break;
		}
	}	//	for [TEXTURES]

	// load new texture if needed
	if (!bTex) {
		// allocate memory for 50 objects if needed
		if ((m_nNumTextures % 50) == 0) {
			n = (m_nNumTextures + 50) * sizeof(ZFXTEXTURE);
			m_pTextures = (ZFXTEXTURE*)realloc(m_pTextures, n);
			if (!m_pTextures)
				return ZFX_OUTOFMEMORY;
		}

		// alphablending needed?
		if (bAlpha)
			m_pSkins[nSkinID].bAlpha = true;
		else
			m_pTextures[m_nNumTextures].fAlpha = 1.0f;

		m_pTextures[m_nNumTextures].pClrKeys = NULL;

		// save texture name
		m_pTextures[m_nNumTextures].chName = new char[strlen(chName) + 1];
		memcpy(m_pTextures[m_nNumTextures].chName, chName, strlen(chName) + 1);

		// create new Direct3D texture object
		hr = CreateTexture(&m_pTextures[m_nNumTextures], bAlpha);
		if (FAILED(hr))
			return hr;

		// add alpha channel if needed
		if (bAlpha) {
			pZFXTex = &m_pTextures[m_nNumTextures];
			pZFXTex->dwNum = dwNumColorKeys;
			pZFXTex->pClrKeys = new ZFXCOLOR[dwNumColorKeys];
			memcpy(pZFXTex->pClrKeys, cColorKeys, sizeof(ZFXCOLOR) * pZFXTex->dwNum);
			LPDIRECT3DTEXTURE9 pTex = (LPDIRECT3DTEXTURE9)pZFXTex->pData;

			// alpha keys are first
			for (DWORD dw = 0; dw < dwNumColorKeys; ++dw) {
				hr = SetAlphaKey(&pTex,
					UCHAR(cColorKeys[dw].fR * 255),
					UCHAR(cColorKeys[dw].fG * 255),
					UCHAR(cColorKeys[dw].fB * 255),
					UCHAR(cColorKeys[dw].fA * 255));
				if (FAILED(hr))
					return hr;
			}

			if (fAlpha < 1.0f) {
				// now general transparency
				m_pTextures[m_nNumTextures].fAlpha = fAlpha;
				hr = SetTransparency(&pTex, UCHAR(fAlpha * 255));
				if (FAILED(hr))
					return hr;
			}
		}

		// save id and increment counter
		nTex = m_nNumTextures;
		++m_nNumTextures;
	}

	// save id to first free texture slot of the skin
	for (int i = 0; i < 8; ++i) {
		if (m_pSkins[nSkinID].nTexture[i] == MAX_ID) {
			m_pSkins[nSkinID].nTexture[i] = nTex;
			break;
		}
	}

	return ZFX_OK;
}	//	AddTexture
/*---------------------------------------------------------*/

HRESULT	ZFXD3DSkinManager::ConvertToNormalMap(ZFXTEXTURE* pTexture)
{
	HRESULT				hr = ZFX_OK;
	D3DLOCKED_RECT		d3dRect;
	D3DSURFACE_DESC		desc;
	LPDIRECT3DTEXTURE9	pTex = ((LPDIRECT3DTEXTURE9)pTexture->pData);

	if (FAILED(pTex->LockRect(0, &d3dRect, NULL, 0)))
		return ZFX_BUFFERLOCK;

	// pointer on pixel data
	DWORD* pPixel = (DWORD*)d3dRect.pBits;

	// build normal vector for each pixel
	for (DWORD i = 0; i < desc.Width; ++i)
	{
		for (DWORD j = 0; j < desc.Height; ++j)
		{
			DWORD color00 = pPixel[0];
			DWORD color10 = pPixel[1];
			DWORD color01 = pPixel[d3dRect.Pitch / sizeof(DWORD)];

			float fHeight00 = (float)((color00 & 0x00ff0000) >> 16) / 255.0f;
			float fHeight10 = (float)((color10 & 0x00ff0000) >> 16) / 255.0f;
			float fHeight01 = (float)((color01 & 0x00ff0000) >> 16) / 255.0f;

			ZFXVector vcPoint00(i + 0.0f, j + 0.0f, fHeight00);
			ZFXVector vcPoint10(i + 1.0f, j + 0.0f, fHeight10);
			ZFXVector vcPoint01(i + 0.0f, j + 1.0f, fHeight01);
			ZFXVector vc10 = vcPoint10 - vcPoint00;
			ZFXVector vc01 = vcPoint01 - vcPoint00;

			ZFXVector vcNormal;
			vcNormal.Cross(vc10, vc01);
			vcNormal.Normalize();

			*pPixel++ = VectorToRGBA(&vcNormal, fHeight00);
		}
	}

	pTex->UnlockRect(0);
	return ZFX_OK;
}	//	ConvertToNormalMap
/*---------------------------------------------------------*/

DWORD ZFXD3DSkinManager::VectorToRGBA(ZFXVector* vc, float fHeight)
{
	DWORD r = (DWORD)(127.0f * vc->x + 128.0f);
	DWORD g = (DWORD)(127.0f * vc->y + 128.0f);
	DWORD b = (DWORD)(127.0f * vc->z + 128.0f);
	DWORD a = (DWORD)(127.0f * fHeight);

	return (a << 24) + (r << 16) + (g << 8) + (b << 0);
}	//	VectorToRGBA
/*---------------------------------------------------------*/

HRESULT ZFXD3DSkinManager::AddTextureHeightmapAsBump(UINT nSkinID, const char* chName)
{
	ZFXTEXTURE* pZFXTex = NULL;
	HRESULT		hr;
	UINT		nTex, n;
	bool		bTex = false;

	// is skin ID valid at all
	if (nSkinID >= m_nNumSkins)
		return ZFX_INVALIDID;

	// all 8 stages for this skin already set?
	if (m_pSkins[nSkinID].nTexture[7] != MAX_ID) {
		Log("error: AddTexture() failed, all 8 stages set");
		return ZFX_BUFFERSIZE;
	}

	// do we already have this texture
	for (nTex = 0; nTex < m_nNumTextures; ++nTex) {
		if (strcmp(chName, m_pTextures[nTex].chName) == 0) {
			bTex = true;
			break;
		}
	}	//	for [TEXTURES]

	// load new texture if not yet done
	if (!bTex) {
		// allocate 50 new memory slots for textures if necessary
		if (m_nNumTextures % 50 == 0) {
			n = (m_nNumTextures + 50) * sizeof(ZFXTEXTURE);
			m_pTextures = (ZFXTEXTURE*)realloc(m_pTextures, n);
			if (!m_pTextures) {
				Log("error: AddTexture() failed, realloc()");
				return ZFX_OUTOFMEMORY;
			}
		}

		// no alpha blending needed
		m_pTextures[m_nNumTextures].fAlpha = 1.0f;
		m_pTextures[m_nNumTextures].pClrKeys = NULL;

		// save texture name
		m_pTextures[m_nNumTextures].chName = new char[strlen(chName) + 1];
		memcpy(m_pTextures[m_nNumTextures].chName, chName, strlen(chName) + 1);

		// create d3d texture from that pointer
		hr = CreateTexture(&m_pTextures[m_nNumTextures], true);
		if (FAILED(hr)) {
			Log("error: CreateTexture() failed");
			return hr;
		}

		// build normals from  heightvalues
		hr = ConvertToNormalMap(&m_pTextures[m_nNumTextures]);
		if (FAILED(hr)) {
			Log("error: ConvertToNormalmap() failed");
			return hr;
		}

		// save ID and add to count
		nTex = m_nNumTextures;
		++m_nNumTextures;
	}	// load texture 

	// put texture ID to skin ID
	for (int i = 0; i < 8; ++i) {
		if (m_pSkins[nSkinID].nTexture[i] == MAX_ID) {
			m_pSkins[nSkinID].nTexture[i] = nTex;
			break;
		}
	}

	return ZFX_OK;
}	//	AddTextureHeightmapAsBump
/*---------------------------------------------------------*/

HRESULT ZFXD3DSkinManager::CreateTexture(ZFXTEXTURE* pTexture, bool bAlpha)
{
	D3DLOCKED_RECT	d3dRect;
	D3DFORMAT		fmt;
	DIBSECTION		dibS;
	HRESULT			hr;
	int				LineWidth;
	void*			pMemory = NULL;

	HBITMAP hBMP = (HBITMAP)LoadImage(NULL, pTexture->chName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
	if (!hBMP)
		return ZFX_FILENOTFOUND;

	GetObject(hBMP, sizeof(DIBSECTION), &dibS);

	// we support only 24-bit bitmaps
	if (dibS.dsBmih.biBitCount != 24) {
		DeleteObject(hBMP);
		return ZFX_INVALIDFILE;
	}

	if (bAlpha) fmt = D3DFMT_A8R8G8B8;
	else fmt = D3DFMT_R5G6B5;

	long	lWidth		= dibS.dsBmih.biWidth;
	long	lHeight		= dibS.dsBmih.biHeight;
	BYTE*	pBMPBits	= (BYTE*)dibS.dsBm.bmBits;

	hr = m_pDevice->CreateTexture(lWidth, lHeight, 1, 0, fmt, D3DPOOL_MANAGED, (LPDIRECT3DTEXTURE9*)(&(pTexture->pData)), NULL);
	if (FAILED(hr))
		return ZFX_FAIL;

	// set dummy pointer
	LPDIRECT3DTEXTURE9 pTex = ((LPDIRECT3DTEXTURE9)pTexture->pData);
	if (FAILED(pTex->LockRect(0, &d3dRect, NULL, 0)))
		return ZFX_BUFFERLOCK;

	if (bAlpha) {
		LineWidth = d3dRect.Pitch >> 2;		// 32 Bit = 4 Byte
		pMemory = (DWORD*)d3dRect.pBits;
	}
	else {
		LineWidth = d3dRect.Pitch >> 1;		// 16 Bit = 2 Byte
		pMemory = (USHORT*)d3dRect.pBits;
	}

	// copy each pixel
	for (int cy = 0; cy < lHeight; ++cy) 
	{
		for (int cx = 0; cx < lWidth; ++cx) 
		{
			if (bAlpha) 
			{
				DWORD	Color = 0xff000000;
				int		i = (cy * lWidth + cx) * 3;
				memcpy(&Color, &pBMPBits[i], sizeof(BYTE) * 3);

				((DWORD*)pMemory)[cx + (cy * LineWidth)] = Color;
			}	//	32 Bit
			else
			{
				// convert 24 bit into 16 bit
				UCHAR B = (pBMPBits[(cy * lWidth + cx) * 3 + 0]) >> 3,
					G = (pBMPBits[(cy * lWidth + cx) * 3 + 1]) >> 3,
					R = (pBMPBits[(cy * lWidth + cx) * 3 + 2]) >> 3;

				// map value to 5, 6, and 5 bits respectively and call macro
				USHORT Color = RGB16BIT((int)(((float)R / 255.0f) * 32.0f),
					(int)(((float)G / 255.0f) * 64.0f),
					(int)(((float)B / 255.0f) * 32.0f));

				// write pixel as 16-bit color
				((USHORT*)pMemory)[cx + (cy * LineWidth)] = Color;
			}	// 16 Bit
		}	// for
	}	// for

	pTex->UnlockRect(0);
	DeleteObject(hBMP);
	return ZFX_OK;
}	//	CreateTexture
/*---------------------------------------------------------*/

DWORD ZFXD3DSkinManager::MakeD3DColor(UCHAR R, UCHAR G, UCHAR B, UCHAR A)
{
	return (A << 24) | (R << 16) | (G << 8) | B;
}	//	MakeD3DColor
/*---------------------------------------------------------*/

HRESULT ZFXD3DSkinManager::SetAlphaKey(LPDIRECT3DTEXTURE9* ppTexture, UCHAR R, UCHAR G, UCHAR B, UCHAR A)
{
	D3DSURFACE_DESC	d3dDesc;
	D3DLOCKED_RECT	d3dRect;
	DWORD			dwKey, Color;

	// must be 32 bit ARGB format
	(*ppTexture)->GetLevelDesc(0, &d3dDesc);
	if (d3dDesc.Format != D3DFMT_A8R8G8B8)
		return ZFX_INVALIDPARAM;

	// color to be replaced
	dwKey = MakeD3DColor(R, G, B, 255);

	// color to replace old one with
	if (A > 0) Color = MakeD3DColor(R, G, B, A);
	else Color = MakeD3DColor(0, 0, 0, A);
	
	if (FAILED((*ppTexture)->LockRect(0, &d3dRect, NULL, 0)))
		return ZFX_BUFFERLOCK;

	// overwrite all pixels to be replaced
	for (DWORD y = 0; y < d3dDesc.Height; ++y)
	{
		for (DWORD x = 0; x < d3dDesc.Width; ++x) 
		{
			if (((DWORD*)d3dRect.pBits)[d3dDesc.Width * y + x] == dwKey)
			{
				((DWORD*)d3dRect.pBits)[d3dDesc.Width * y + x] = Color;
			}
		}
	}
	(*ppTexture)->UnlockRect(0);

	return ZFX_OK;
}	//	SetAlphaKey
/*---------------------------------------------------------*/

HRESULT ZFXD3DSkinManager::SetTransparency(LPDIRECT3DTEXTURE9* ppTexture, UCHAR Alpha)
{
	D3DSURFACE_DESC	d3dDesc;
	D3DLOCKED_RECT	d3dRect;
	DWORD			Color;
	UCHAR			A, R, G, B;

	// must be 32 bit format
	(*ppTexture)->GetLevelDesc(0, &d3dDesc);
	if (d3dDesc.Format != D3DFMT_A8R8G8B8)
		return ZFX_INVALIDPARAM;

	if (FAILED((*ppTexture)->LockRect(0, &d3dRect, NULL, 0)))
		return ZFX_BUFFERLOCK;

	// loop through all pixels
	for (DWORD y = 0; y < d3dDesc.Height; ++y) {
		for (DWORD x = 0; x < d3dDesc.Width; ++x) {
			// get color from the pixel
			Color = ((DWORD*)d3dRect.pBits)[d3dDesc.Width * y + x];

			// calculate new ARGB value
			A = (UCHAR)((Color & 0xff000000) >> 24);
			B = (UCHAR)((Color & 0x00ff0000) >> 16);
			G = (UCHAR)((Color & 0x0000ff00) >> 8);
			B = (UCHAR)((Color & 0x000000ff) >> 0);

			// set only if new alpha value is lower
			if (A >= Alpha)
				A = Alpha;

			((DWORD*)d3dRect.pBits)[d3dDesc.Width * y + x] = MakeD3DColor(R, G, B, A);
		}
	}
	(*ppTexture)->UnlockRect(0);

	return ZFX_OK;
}	//	SetTransparency
/*---------------------------------------------------------*/