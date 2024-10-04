#include "ZFXD3D_vcache.h"

// ZFXD3DVCache
// ----------------

ZFXD3DVCache::ZFXD3DVCache(UINT nVertsMax, UINT nIndisMax, UINT nStride, ZFXD3DSkinManager* pSkinMan, LPDIRECT3DDEVICE9 pDevice, ZFXD3DVCManager* pDad, DWORD dwID, DWORD dwFVF, FILE* pLog)
{
	HRESULT hr;

	m_pDevice		= pDevice;
	m_pSkinMan		= pSkinMan;
	m_pDad			= pDad;
	m_nNumVertsMax	= nVertsMax;
	m_nNumIndisMax	= nIndisMax;
	m_nNumVerts		= 0;
	m_nNumIndis		= 0;
	m_dwID			= dwID;
	m_dwFVF			= dwFVF;
	m_nStride		= nStride;
	m_pLog			= pLog;

	memset(&m_Skin, MAX_ID, sizeof(ZFXSKIN));
	m_SkinID = MAX_ID;

	// create the buffer
	m_pVB = NULL;
	m_pIB = NULL;

	hr = pDevice->CreateVertexBuffer(nVertsMax * nStride, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &m_pVB, NULL);

	if (FAILED(hr)) m_pVB = NULL;

	hr = pDevice->CreateIndexBuffer(nIndisMax * sizeof(WORD), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_pIB, NULL);

	if (FAILED(hr)) m_pIB = NULL;
}

ZFXD3DVCache::~ZFXD3DVCache(void)
{
	if (m_pVB) {
		m_pVB->Release();
		m_pVB = NULL;
	}
	if (m_pIB) {
		m_pIB->Release();
		m_pIB = NULL;
	}
}

void ZFXD3DVCache::SetSkin(UINT SkinID, bool bUseShaders)
{
	if (!UsesSkin(SkinID)) {
		ZFXSKIN		tmpSkin = m_pSkinMan->GetSkin(SkinID);
		ZFXSKIN*	pSkin = &tmpSkin;

		if (!IsEmpty()) Flush(bUseShaders);

		memcpy(&m_Skin, pSkin, sizeof(ZFXSKIN));
		m_SkinID = SkinID;

		m_pDad->SetActiveCache(MAX_ID);
	}
}

HRESULT ZFXD3DVCache::Add(UINT nVerts, UINT nIndis, const void* pVerts, const WORD* pIndices, bool bUseShaders)
{
	BYTE*	tmp_pVerts = NULL;
	WORD*	tmp_pIndis = NULL;
	int		nSizeV = m_nStride * nVerts;
	int		nSizeI = sizeof(WORD) * nIndis;
	int		nPosV;
	int		nPosI;
	DWORD	dwFlags;

	// is buffer size sufficient?
	if (nVerts > m_nNumVertsMax || nIndis > m_nNumIndisMax)
		return ZFX_BUFFERSIZE;

	// cache is full so empty out
	if ((nVerts + m_nNumVerts > m_nNumVertsMax) || (nIndis + m_nNumIndis > m_nNumIndisMax)) {
		if (Flush(bUseShaders) != ZFX_OK)
			return ZFX_FAIL;
	}

	// DISCARD flag if buffer is empty
	if (m_nNumVerts == 0) {
		nPosV = nPosI = 0;
		dwFlags = D3DLOCK_DISCARD;
	}
	// else append with NOOVERWRITE flag
	else {
		nPosV = m_nStride * m_nNumVerts;
		nPosI = sizeof(WORD) * m_nNumIndis;
		dwFlags = D3DLOCK_NOOVERWRITE;
	}

	// lock buffers
	if (FAILED(m_pVB->Lock(nPosV, nSizeV, (void**)&tmp_pVerts, dwFlags)))
		return ZFX_BUFFERLOCK;

	if (FAILED(m_pIB->Lock(nPosI, nSizeI, (void**)&tmp_pIndis, dwFlags))) {
		m_pVB->Unlock();
		return ZFX_BUFFERLOCK;
	}

	// copy vertices
	memcpy(tmp_pVerts, pVerts, nSizeV);

	// copy indices
	int nBase = m_nNumVerts;
	if (!pIndices) nIndis = nVerts;

	for (UINT i = 0; i < nIndis; ++i)
	{
		if (pIndices != NULL)
			tmp_pIndis[i] = pIndices[i] + nBase;
		else
			tmp_pIndis[i] = i + nBase;
		++m_nNumIndis;
	}

	// increment counter
	m_nNumVerts += nVerts;

	m_pVB->Unlock();
	m_pIB->Unlock();
	return ZFX_OK;
}

HRESULT ZFXD3DVCache::Flush(bool bUseShaders)
{
	ZFXRENDERSTATE sm;
	HRESULT	hr = ZFX_FAIL;
	if (m_nNumVerts <= 0) return ZFX_OK;

	// if this cache is not active
	if (m_pDad->GetActiveCache() != m_dwID) {
		// no shaders
		if (!bUseShaders) m_pDevice->SetFVF(m_dwFVF);
		
		m_pDevice->SetIndices(m_pIB);
		m_pDevice->SetStreamSource(0, m_pVB, 0, m_nStride);
		
		m_pDad->SetActiveCache(m_dwID);
	}	//	[device->cache]

	// if this skin is not yet active
	if (m_pDad->GetZFXD3D()->GetActiveSkinID() != m_SkinID) {
		LPDIRECT3DTEXTURE9 pTex = NULL;
		ZFXMATERIAL* pMat = &m_pSkinMan->m_pMaterials[m_Skin.nMaterial];

		// WIREFRAME-MODE; SPECIAL CASE
		if (!m_pDad->GetZFXD3D()->GetShadeMode() == RS_SHADE_SOLID) {
			// set the material
			D3DMATERIAL9 mat = {
				pMat->cDiffuse.fR,	pMat->cDiffuse.fG,
				pMat->cDiffuse.fB,	pMat->cDiffuse.fA,
				pMat->cAmbient.fR,	pMat->cAmbient.fG,
				pMat->cAmbient.fB,	pMat->cAmbient.fA,
				pMat->cSpecular.fR,	pMat->cSpecular.fG,
				pMat->cSpecular.fB,	pMat->cSpecular.fA,
				pMat->cEmissive.fR,	pMat->cEmissive.fG,
				pMat->cEmissive.fB,	pMat->cEmissive.fA,
				pMat->fPower };
			m_pDevice->SetMaterial(&mat);

			// set the texture
			for (int i = 0; i < 8; ++i)
			{
				if (m_Skin.nTexture[i] != MAX_ID) {
					pTex = (LPDIRECT3DTEXTURE9)m_pSkinMan->m_pTextures[m_Skin.nTexture[i]].pData;
					m_pDevice->SetTexture(i, pTex);
				}
				else break;
			}	// for
		}
		else {
			ZFXCOLOR clrWire = m_pDad->GetZFXD3D()->GetWireColor();
			// set material
			D3DMATERIAL9 matW = {
				clrWire.fR, clrWire.fG, clrWire.fB, clrWire.fA,
				clrWire.fR, clrWire.fG, clrWire.fB, clrWire.fA,
				0.0f,		0.0f,		0.0f,		1.0f,
				0.0f,		0.0f,		0.0f,		1.0f,
				1.0f };
			m_pDevice->SetMaterial(&matW);

			// no texture for the device
			m_pDevice->SetTexture(0, NULL);
		}

		// activate alpha blending
		if (m_Skin.bAlpha) {
			m_pDevice->SetRenderState(D3DRS_ALPHAREF, 50);
			m_pDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
			m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			m_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
			m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		}
		else {
			m_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
			m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		}
		// mark skin as active
		m_pDad->GetZFXD3D()->SetActiveSkinID(m_SkinID);
	}	// [device->skin]

	// FINALLY RENDER
	sm = m_pDad->GetZFXD3D()->GetShadeMode();

	// POINT SPRITES
	if (sm == RS_SHADE_POINTS) {
		hr = m_pDevice->DrawPrimitive(D3DPT_POINTLIST, 0, m_nNumVerts);
	}
	// LINESTRIP
	else if (sm == RS_SHADE_HULLWIRE) {
		hr = m_pDevice->DrawIndexedPrimitive(D3DPT_LINESTRIP, 0, 0, m_nNumVerts, 0, m_nNumIndis / 3);
	}
	// POLYGONLIST
	else {	// RS_SHADE_SOLID || RS_SHADE_TRIWIRE
		hr = m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, m_nNumVerts, 0, m_nNumIndis / 3);
	}

	if (FAILED(hr)) return ZFX_FAIL;

	// reset counters
	m_nNumVerts = 0;
	m_nNumIndis = 0;
	return ZFX_OK;
}

// ZFXD3DVCManager
// ----------------
ZFXD3DVCManager::ZFXD3DVCManager(ZFXD3DSkinManager* pSkinMan, LPDIRECT3DDEVICE9 pDevice, ZFXD3D* pZFXD3D, UINT nMaxVerts, UINT nMaxIndis, FILE* pLog)
{
	DWORD	dwID = 1;
	int		i = 0;

	m_pSB		= NULL;
	m_pIB		= NULL;
	m_nNumSB	= 0;
	m_nNumIB	= 0;

	m_pLog			= pLog;
	m_pDevice		= pDevice;
	m_pZFXD3D		= pZFXD3D;
	m_pSkinMan		= pSkinMan;
	m_dwActiveCache = MAX_ID;
	m_dwActiveSB	= MAX_ID;
	m_DwActiveIB	= MAX_ID;

	for (i = 0; i < NUM_CACHES; ++i) 
	{
		m_CachePS[i] = new ZFXD3DVCache(nMaxVerts, nMaxIndis, sizeof(PVERTEX), pSkinMan, pDevice, this, ++dwID, FVF_PVERTEX, pLog);
		m_CacheUU[i] = new ZFXD3DVCache(nMaxVerts, nMaxIndis, sizeof(VERTEX), pSkinMan, pDevice, this, ++dwID, FVF_VERTEX, pLog);
		m_CacheUL[i] = new ZFXD3DVCache(nMaxVerts, nMaxIndis, sizeof(LVERTEX), pSkinMan, pDevice, this, ++dwID, FVF_LVERTEX, pLog);
		m_CacheCA[i] = new ZFXD3DVCache(nMaxVerts, nMaxIndis, sizeof(CVERTEX), pSkinMan, pDevice, this, ++dwID, FVF_CVERTEX, pLog);
		m_Cache3T[i] = new ZFXD3DVCache(nMaxVerts, nMaxIndis, sizeof(VERTEX3T), pSkinMan, pDevice, this, ++dwID, FVF_T3VERTEX, pLog);
		m_CacheTV[i] = new ZFXD3DVCache(nMaxVerts, nMaxIndis, sizeof(TVERTEX), pSkinMan, pDevice, this, ++dwID, FVF_TVERTEX, pLog);
	}	// for	
}	//	constructor
/*---------------------------------------------------------*/

ZFXD3DVCManager::~ZFXD3DVCManager(void)
{
	UINT	n = 0;
	int		i = 0;

	// release the memory in the static buffers
	if (m_pSB) {
		for (n = 0; n < m_nNumSB; ++n)
		{
			if (m_pSB[n].pVB) {
				m_pSB[n].pVB->Release();
				m_pSB[n].pVB = NULL;
			}
			if (m_pSB[n].pIB) {
				m_pSB[n].pIB->Release();
				m_pSB[n].pIB = NULL;
			}
		}
		free(m_pSB);
		m_pSB = NULL;
	}

	if (m_pIB) {
		for (n = 0; n < m_nNumIB; ++n)
		{
			if (m_pIB[n].pIB) {
				m_pIB[n].pIB->Release();
				m_pIB[n].pIB = NULL;
			}
		}
		free(m_pIB);
		m_pIB = NULL;
	}

	// release the vertex cache objects
	for (i = 0; i < NUM_CACHES; ++i)
	{
		if (m_CachePS[i]) {
			delete m_CachePS[i];
			m_CachePS[i] = NULL;
		}
		if (m_CacheUU[i]) {
			delete m_CacheUU[i];
			m_CacheUU[i] = NULL;
		}
		if (m_CacheUL[i]) {
			delete m_CacheUL[i];
			m_CacheUL[i] = NULL;
		}
		if (m_CacheCA[i]) {
			delete m_CacheCA[i];
			m_CacheCA[i] = NULL;
		}
		if (m_Cache3T[i]) {
			delete m_Cache3T[i];
			m_Cache3T[i] = NULL;
		}
		if (m_CacheTV[i]) {
			delete m_CacheTV[i];
			m_CacheTV[i] = NULL;
		}
	}	// for
}	//	destructor
/*---------------------------------------------------------*/

HRESULT ZFXD3DVCManager::Render(ZFXVERTEXID VertexID, UINT nVerts, UINT nIndis, const void* pVerts, const WORD* pIndis, UINT SkinID)
{
	ZFXD3DVCache** pCache = NULL,
		* pCacheEmpty = NULL,
		* pCacheFullest = NULL;
	int nEmptyVC	= -1;
	int nFullestVC	= 0;

	bool bShaders = m_pZFXD3D->UsesShaders();

	// which vertex type is used?
	switch (VertexID)
	{
		case VID_PS:	{ pCache = m_CachePS; }
						break;
		case VID_UU:	{ pCache = m_CacheUU; }
						break;
		case VID_UL:	{ pCache = m_CacheUL; }
						break;
		case VID_CA:	{ pCache = m_CacheCA; }
						break;
		case VID_3T:	{ pCache = m_Cache3T; }
						break;
		case VID_TV:	{ pCache = m_CacheTV; }
						break;
		default:	return ZFX_INVALIDID;
	}	// switch

	pCacheFullest = pCache[0];

	// active buffer get invalid
	m_dwActiveSB = MAX_ID;

	// SEARCH THE MOST APPROPRIATE POT

	// is there a cache with this skin?
	for (int i = 0; i < NUM_CACHES; ++i)
	{
		// we got one so add data
		if (pCache[i]->UsesSkin(SkinID)) {
			return pCache[i]->Add(nVerts, nIndis, pVerts, pIndis, bShaders);
		}

		// save an empty cache
		if (pCache[i]->IsEmpty()) {
			pCacheEmpty = pCache[i];
		}

		// save the fullest cache
		if (pCache[i]->NumVerts() > pCacheFullest->NumVerts()) {
			pCacheFullest = pCache[i];
		}
	}

	// no luck finding a cache with the skin, is there an empty one?
	if (pCacheEmpty) {
		pCacheEmpty->SetSkin(SkinID, bShaders);
		return pCacheEmpty->Add(nVerts, nIndis, pVerts, pIndis, bShaders);
	}

	// again no luck so use the fullest cache
	pCacheFullest->Flush(bShaders);
	pCacheFullest->SetSkin(SkinID, bShaders);
	return pCacheFullest->Add(nVerts, nIndis, pVerts, pIndis, bShaders);
}	//	Render
/*---------------------------------------------------------*/

HRESULT ZFXD3DVCManager::ForcedFlush(ZFXVERTEXID VertexID)
{
	ZFXD3DVCache**	pCache = NULL;
	HRESULT			hr = ZFX_OK;
	int				i = 0;

	switch (VertexID)
	{
		case VID_PS:	{ pCache = m_CachePS; }
			   break;
		case VID_UU:	{ pCache = m_CacheUU; }
						break;
		case VID_UL:	{ pCache = m_CacheUL; }
						break;
		case VID_CA:	{ pCache = m_CacheCA; }
						break;
		case VID_3T:	{ pCache = m_Cache3T; }
						break;
		case VID_TV:	{ pCache = m_CacheTV; }
						break;

		// unknown Vertex-Type
		default:	return ZFX_INVALIDID;
	}	// switch

	for (i = 0; i < NUM_CACHES; ++i)
		if (FAILED(pCache[i]->Flush(m_pZFXD3D->UsesShaders())))
			hr = ZFX_FAIL;
	
	return hr;
}	//	ForcedFlush
/*---------------------------------------------------------*/

HRESULT ZFXD3DVCManager::ForcedFlushAll(void)
{
	HRESULT	hr = ZFX_OK;
	bool	bShaders = m_pZFXD3D->UsesShaders();
	int		i;

	for (i = 0; i < NUM_CACHES; ++i)
		if (!m_CachePS[i]->IsEmpty())
			if (FAILED(m_CachePS[i]->Flush(bShaders)))
				hr = ZFX_FAIL;

	for (i = 0; i < NUM_CACHES; ++i)
		if (!m_CacheUU[i]->IsEmpty())
			if (FAILED(m_CacheUU[i]->Flush(bShaders)))
				hr = ZFX_FAIL;

	for (i = 0; i < NUM_CACHES; ++i)
		if (!m_CacheUL[i]->IsEmpty())
			if (FAILED(m_CacheUL[i]->Flush(bShaders)))
				hr = ZFX_FAIL;
	
	for (i = 0; i < NUM_CACHES; ++i)
		if (!m_CacheCA[i]->IsEmpty())
			if (FAILED(m_CacheCA[i]->Flush(bShaders)))
				hr = ZFX_FAIL;

	for (i = 0; i < NUM_CACHES; ++i)
		if (!m_Cache3T[i]->IsEmpty())
			if (FAILED(m_Cache3T[i]->Flush(bShaders)))
				hr = ZFX_FAIL;

	for (i = 0; i < NUM_CACHES; ++i)
		if (!m_CacheTV[i]->IsEmpty())
			if (FAILED(m_CacheTV[i]->Flush(bShaders)))
				hr = ZFX_FAIL;

	return hr;
}	//	ForcedFlushAll
/*---------------------------------------------------------*/

HRESULT ZFXD3DVCManager::CreateStaticBuffer(ZFXVERTEXID VertexID, UINT nSkinID, UINT nVerts, UINT nIndis, const void* pVerts, const WORD* pIndis, UINT* pnID)
{
	HRESULT	hr;
	DWORD	dwActualFVF;
	void*	pData;

	if (m_nNumSB >= (MAX_ID - 1)) return ZFX_OUTOFMEMORY;

	// allocate memory if needed
	if ((m_nNumSB % 50) == 0) {
		int n = (m_nNumSB + 50) * sizeof(ZFXSTATICBUFFER);
		m_pSB = (ZFXSTATICBUFFER*)realloc(m_pSB, n);
		if (!m_pSB)
			return ZFX_OUTOFMEMORY;
	}

	m_pSB[m_nNumSB].nNumVerts	= nVerts;
	m_pSB[m_nNumSB].nNumIndis	= nIndis;
	m_pSB[m_nNumSB].nSkinID		= nSkinID;

	// size and format of the vertices
	switch (VertexID)
	{
		case VID_PS: {
			m_pSB[m_nNumSB].nStride = sizeof(PVERTEX);
			m_pSB[m_nNumSB].dwFVF = FVF_PVERTEX;
		}	break;
		case VID_UU: {
			m_pSB[m_nNumSB].nStride = sizeof(VERTEX);
			m_pSB[m_nNumSB].dwFVF = FVF_VERTEX;
		}	break;
		case VID_UL: {
			m_pSB[m_nNumSB].nStride = sizeof(LVERTEX);
			m_pSB[m_nNumSB].dwFVF = FVF_LVERTEX;
		}	break;
		case VID_CA: {
			m_pSB[m_nNumSB].nStride = sizeof(CVERTEX);
			m_pSB[m_nNumSB].dwFVF = FVF_CVERTEX;
		}	break;
		case VID_3T: {
			m_pSB[m_nNumSB].nStride = sizeof(VERTEX3T);
			m_pSB[m_nNumSB].dwFVF = FVF_T3VERTEX;
		}	break;
		case VID_TV: {
			m_pSB[m_nNumSB].nStride = sizeof(TVERTEX);
			m_pSB[m_nNumSB].dwFVF = FVF_TVERTEX;
		}	break;
		default:	return ZFX_INVALIDID;
	}	// switch

	// create index buffer if needed
	if (nIndis > 0) {
		m_pSB[m_nNumSB].bIndis = true;
		m_pSB[m_nNumSB].nNumTris = int(nIndis / 3.0f);

		hr = m_pDevice->CreateIndexBuffer(nIndis * sizeof(WORD), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_pSB[m_nNumSB].pIB, NULL);
		if (FAILED(hr)) return ZFX_CREATEBUFFER;

		// fill the index buffer
		if (SUCCEEDED(m_pSB[m_nNumSB].pIB->Lock(0, 0, (void**)(&pData), 0))) {
			memcpy(pData, pIndis, nIndis * sizeof(WORD));
			m_pSB[m_nNumSB].pIB->Unlock();
		}
		else return ZFX_BUFFERLOCK;
	}
	else {
		m_pSB[m_nNumSB].bIndis = false;
		m_pSB[m_nNumSB].nNumTris = int(nVerts / 3.0f);
		m_pSB[m_nNumSB].pIB = NULL;
	}

	// no need for FVF if shaders are used
	if (m_pZFXD3D->UsesShaders()) dwActualFVF = 0;
	else dwActualFVF = m_pSB[m_nNumSB].dwFVF;

	// create vertex buffer
	hr = m_pDevice->CreateVertexBuffer(nVerts * m_pSB[m_nNumSB].nStride, D3DUSAGE_WRITEONLY, dwActualFVF, D3DPOOL_DEFAULT, &m_pSB[m_nNumSB].pVB, NULL);
	if (FAILED(hr)) return ZFX_CREATEBUFFER;

	// fill vertex buffer
	if (SUCCEEDED(m_pSB[m_nNumSB].pVB->Lock(0, 0, (void**)(&pData), 0))) {
		memcpy(pData, pVerts, nVerts * m_pSB[m_nNumSB].nStride);
		m_pSB[m_nNumSB].pVB->Unlock();
	}
	else return ZFX_BUFFERLOCK;

	(*pnID) = m_nNumSB;
	++m_nNumSB;
	return ZFX_OK;
}	//	CreateStaticBuffer
/*---------------------------------------------------------*/

HRESULT ZFXD3DVCManager::Render(UINT nID)
{
	HRESULT hr = ZFX_OK;

	ZFXRENDERSTATE sm = m_pZFXD3D->GetShadeMode();

	// active vertex cache gets invalid
	m_dwActiveCache = MAX_ID;

	// active this static buffer if not active yet
	if (m_dwActiveSB != nID) {
		// using indices?
		if (m_pSB[nID].bIndis)
			m_pDevice->SetIndices(m_pSB[nID].pIB);

		m_pDevice->SetStreamSource(0, m_pSB[nID].pVB, 0, m_pSB[nID].nStride);
		m_dwActiveSB = nID;
	}
	// skin already active?
	if (m_pZFXD3D->GetActiveSkinID() != m_pSB[nID].nSkinID) {
		// mark as active noew
		ZFXSKIN	tmpSkin = m_pSkinMan->GetSkin(m_pSB[nID].nSkinID);
		ZFXSKIN* pSkin = &tmpSkin;

		// SPECIAL CASE WIREFRAME-MODE
		if (sm == RS_SHADE_SOLID) {
			// set material with wireframe color
			ZFXMATERIAL	tmpMat = m_pSkinMan->GetMaterial(pSkin->nMaterial);
			ZFXMATERIAL* pMat = &tmpMat;
			D3DMATERIAL9 mat = {
				pMat->cDiffuse.fR,	pMat->cDiffuse.fG,
				pMat->cDiffuse.fB,	pMat->cDiffuse.fA,
				pMat->cAmbient.fR,	pMat->cAmbient.fG,
				pMat->cAmbient.fB,	pMat->cAmbient.fA,
				pMat->cSpecular.fR,	pMat->cSpecular.fG,
				pMat->cSpecular.fB,	pMat->cSpecular.fA,
				pMat->cEmissive.fR,	pMat->cEmissive.fG,
				pMat->cEmissive.fB,	pMat->cEmissive.fA,
				pMat->fPower };
			m_pDevice->SetMaterial(&mat);

			// set texture for the device
			for (int i = 0; i < 8; ++i)
			{
				if (pSkin->nTexture[i] != MAX_ID)
					m_pDevice->SetTexture(i, (LPDIRECT3DTEXTURE9)m_pSkinMan->GetTexture(pSkin->nTexture[i]).pData);
			}
		}
		else {
			ZFXCOLOR clrWire = m_pZFXD3D->GetWireColor();

			// set material for the device
			D3DMATERIAL9 matW = {
				clrWire.fR, clrWire.fG, clrWire.fB, clrWire.fA,
				clrWire.fR, clrWire.fG, clrWire.fB, clrWire.fA,
				0.0f,		0.0f,		0.0f,		1.0f,
				0.0f,		0.0f,		0.0f,		1.0f,
				1.0f };
			m_pDevice->SetMaterial(&matW);

			// use no texture
			m_pDevice->SetTexture(0, NULL);
		}

		// set alpha states if needed
		if (pSkin->bAlpha) {
			m_pDevice->SetRenderState(D3DRS_ALPHAREF, 50);
			m_pDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
			m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			m_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
			m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		}
		else {
			m_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
			m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		}

		// active skin has changed
		m_pZFXD3D->SetActiveSkinID(m_pSB[nID].nSkinID);
	}	// [device->skin]

	// if no shader is used activate the approprite FVF
	if (!m_pZFXD3D->UsesShaders())
		m_pDevice->SetFVF(m_pSB[nID].dwFVF);

	// indexed primitives
	if (m_pSB[nID].bIndis) {
		if (sm == RS_SHADE_POINTS) {
			hr = m_pDevice->DrawPrimitive(D3DPT_POINTLIST, 0, m_pSB[nID].nNumVerts);
		}
		else if (sm == RS_SHADE_HULLWIRE) {
			hr = m_pDevice->DrawIndexedPrimitive(D3DPT_LINESTRIP, 0, 0, m_pSB[nID].nNumVerts, 0, m_pSB[nID].nNumVerts);
		}
		else {		// RS_SHADE_SOLID || RS_SHADE_TRIWIRE
			hr = m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, m_pSB[nID].nNumVerts, 0, m_pSB[nID].nNumTris);
		}
	}
	else {
		if (sm == RS_SHADE_POINTS) {
			hr = m_pDevice->DrawPrimitive(D3DPT_POINTLIST, 0, m_pSB[nID].nNumVerts);
		}
		else if (sm == RS_SHADE_HULLWIRE) {
			hr = m_pDevice->DrawPrimitive(D3DPT_LINESTRIP, m_pSB[nID].nNumVerts, m_pSB[nID].nNumVerts);
		}
		else {		// RS_SHADE_SOLID || RS_SHDE_TRIWIRE
			hr = m_pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, m_pSB[nID].nNumVerts, m_pSB[nID].nNumTris);
		}
	}
	return hr;
}	//	Render
/*---------------------------------------------------------*/

void ZFXD3DVCManager::InvalidateStates(void)
{
	m_pZFXD3D->SetActiveSkinID(MAX_ID);
	m_dwActiveSB	= MAX_ID;
	m_DwActiveIB	= MAX_ID;
	m_dwActiveCache = MAX_ID;
}

HRESULT ZFXD3DVCManager::RenderPoints(ZFXVERTEXID VID, UINT nVerts, const void* pVerts, const ZFXCOLOR* pClr)
{
	D3DMATERIAL9	mtrl;
	DWORD			dwFVF;
	int				nStride;

	// invalidate active settings
	InvalidateStates();

	memset(&mtrl, 0, sizeof(D3DMATERIAL9));
	mtrl.Diffuse.r = mtrl.Ambient.r = pClr->fR;
	mtrl.Diffuse.g = mtrl.Ambient.g = pClr->fG;
	mtrl.Diffuse.b = mtrl.Ambient.b = pClr->fB;
	mtrl.Diffuse.a = mtrl.Ambient.a = pClr->fA;

	m_pDevice->SetMaterial(&mtrl);
	m_pDevice->SetTexture(0, NULL);

	switch (VID)
	{
		case VID_PS: {
			nStride = sizeof(PVERTEX);
			dwFVF = FVF_PVERTEX;
		}	break;
		case VID_UU: {
			nStride = sizeof(VERTEX);
			dwFVF = FVF_VERTEX;
		}	break;
		case VID_UL: {
			nStride = sizeof(LVERTEX);
			dwFVF = FVF_LVERTEX;
		}	break;
		case VID_CA: {
			nStride = sizeof(CVERTEX);
			dwFVF = FVF_CVERTEX;
		}	break;
		case VID_3T: {
			nStride = sizeof(VERTEX3T);
			dwFVF = FVF_T3VERTEX;
		}	break;
		case VID_TV: {
			nStride = sizeof(TVERTEX);
			dwFVF = FVF_TVERTEX;
		}	break;
		default:	return ZFX_INVALIDID;
	}	// switch

	// shader of FVF
	if (m_pZFXD3D->UsesShaders()) {
		m_pZFXD3D->ActivateVShader(0, VID);
		m_pZFXD3D->ActivatePShader(0);
	}
	else m_pDevice->SetFVF(dwFVF);

	// finally render list of points
	if (FAILED(m_pDevice->DrawPrimitiveUP(D3DPT_POINTLIST, nVerts, pVerts, nStride)))
		return ZFX_FAIL;
	return ZFX_OK;
}	//	RenderPoints
/*---------------------------------------------------------*/

HRESULT ZFXD3DVCManager::RenderLines(ZFXVERTEXID VID, UINT nVerts, const void* pVerts, const ZFXCOLOR* pClr, bool bStrip)
{
	D3DMATERIAL9	mtrl;
	DWORD			dwFVF;
	int				nStride;

	// mark all current states as invalid
	InvalidateStates();

	if (pClr) {
		memset(&mtrl, 0, sizeof(D3DMATERIAL9));
		mtrl.Diffuse.r = mtrl.Ambient.r = pClr->fR;
		mtrl.Diffuse.g = mtrl.Ambient.g = pClr->fG;
		mtrl.Diffuse.b = mtrl.Ambient.b = pClr->fB;
		mtrl.Diffuse.a = mtrl.Ambient.a = pClr->fA;
		m_pDevice->SetMaterial(&mtrl);
	}
	m_pDevice->SetTexture(0, NULL);

	switch (VID)
	{
		case VID_PS: {
			nStride = sizeof(PVERTEX);
			dwFVF = FVF_PVERTEX;
		}	break;
		case VID_UU:	{
			nStride = sizeof(VERTEX);
			dwFVF	= FVF_VERTEX;
		}	break;
		case VID_UL: {
			nStride = sizeof(LVERTEX);
			dwFVF = FVF_LVERTEX;
		}	break;
		case VID_CA: {
			nStride = sizeof(CVERTEX);
			dwFVF = FVF_CVERTEX;
		}	break;
		case VID_3T: {
			nStride = sizeof(VERTEX3T);
			dwFVF = FVF_T3VERTEX;
		}	break;
		case VID_TV: {
			nStride = sizeof(TVERTEX);
			dwFVF = FVF_TVERTEX;
		}	break;
		default:	return ZFX_INVALIDID;
	}	// switch

	if (m_pZFXD3D->UsesShaders()) {
		m_pZFXD3D->ActivateVShader(0, VID);
		m_pZFXD3D->ActivatePShader(0);
	}
	else m_pDevice->SetFVF(dwFVF);

	if (!bStrip) {
		if (FAILED(m_pDevice->DrawPrimitiveUP(D3DPT_LINELIST, nVerts / 2, pVerts, nStride)))
			return ZFX_FAIL;
	}
	else {
		if (FAILED(m_pDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, nVerts - 1, pVerts, nStride)))
			return ZFX_FAIL;
	}
	return ZFX_OK;
}	//	RenderLines
/*---------------------------------------------------------*/

HRESULT ZFXD3DVCManager::RenderLine(const float* fStart, const float* fEnd, const ZFXCOLOR* pClr)
{
	D3DMATERIAL9	mtrl;
	LVERTEX			pVerts[2];

	if (!pClr)
		return ZFX_INVALIDPARAM;

	// change states
	ForcedFlushAll();

	// active skin and static buffer will become invalid
	InvalidateStates();

	// make sure state is switched off
	m_pZFXD3D->UsesShaders(false);

	// set coordinates
	pVerts[0].x = fStart[0];
	pVerts[0].y = fStart[1];
	pVerts[0].z = fStart[2];
	pVerts[1].x = fEnd[0];
	pVerts[1].y = fEnd[1];
	pVerts[1].z = fEnd[2];

	// set prelit material and color
	pVerts[0].Color = pVerts[1].Color = D3DCOLOR_COLORVALUE(pClr->fR, pClr->fG, pClr->fB, pClr->fA);

	memset(&mtrl, 0, sizeof(D3DMATERIAL9));
	mtrl.Diffuse.r = mtrl.Ambient.r = pClr->fR;
	mtrl.Diffuse.g = mtrl.Ambient.g = pClr->fG;
	mtrl.Diffuse.b = mtrl.Ambient.b = pClr->fB;
	mtrl.Diffuse.a = mtrl.Ambient.a = pClr->fA;

	m_pDevice->SetMaterial(&mtrl);
	m_pDevice->SetTexture(0, NULL);

	m_pDevice->SetFVF(FVF_LVERTEX);
	m_pDevice->SetVertexShader(NULL);
	m_pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);

	// render line
	if (FAILED(m_pDevice->DrawPrimitiveUP(D3DPT_LINELIST, 1, pVerts, sizeof(LVERTEX)))) {
		m_pDevice->SetFVF(NULL);
		m_pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
		return ZFX_FAIL;
	}

	m_pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
	m_pDevice->SetFVF(NULL);
	return ZFX_OK;
}	//	RenderLine
/*---------------------------------------------------------*/

ZFXRENDERSTATE ZFXD3DVCManager::GetShadeMode(void)
{
	return m_pZFXD3D->GetShadeMode();
}