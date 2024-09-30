#include "ZFXD3D.h"

extern bool g_bLF;

HRESULT ZFXD3D::SetView3D(const ZFXVector& vcRight, const ZFXVector& vcUp, const ZFXVector& vcDir, const ZFXVector& vcPos)
{
	if (!m_bRunning) 
		return E_FAIL;

	m_mView3D._14 = m_mView3D._21 = m_mView3D._34 = 0.0f;
	m_mView3D._44 = 1.0f;

	m_mView3D._11 = vcRight.x;
	m_mView3D._21 = vcRight.y;
	m_mView3D._31 = vcRight.z;
	m_mView3D._41 = -(vcRight * vcPos);

	m_mView3D._12 = vcUp.x;
	m_mView3D._22 = vcUp.y;
	m_mView3D._32 = vcUp.z;
	m_mView3D._42 = -(vcUp * vcPos);

	m_mView3D._13 = vcDir.x;
	m_mView3D._23 = vcDir.y;
	m_mView3D._33 = vcDir.z;
	m_mView3D._32 = -(vcDir * vcPos);

	if (!m_bUseShaders)
	{
		if (FAILED(m_pDevice->SetTransform(D3DTS_VIEW, &m_mView3D)))
			return ZFX_FAIL;
	}

	CalcViewProjMatrix();
	CalcWorldViewProjMatrix();
	return ZFX_OK;
}
/*---------------------------------------------------------*/

HRESULT ZFXD3D::SetViewLookAt(const ZFXVector& vcPos, const ZFXVector& vcPoint, const ZFXVector& vcWorldUp)
{
	ZFXVector vcDir, vcTemp, vcUp;

	vcDir = vcPoint - vcPos;
	vcDir.Normalize();

	// calculate up vector
	float fDot = vcWorldUp * vcDir;
	vcTemp = vcDir * fDot;
	vcUp = vcWorldUp - vcTemp;
	float fL = vcUp.GetLength();

	// if too short take y axis
	if (fL < 1e-6f) {
		ZFXVector vcY;
		vcY.Set(0.0f, 1.0f, 0.0f);

		vcTemp = vcDir * vcDir.y;
		vcUp = vcY - vcTemp;

		fL = vcUp.GetLength();

		// take z axis if still too short
		if (fL < 1e-6f) {
			vcY.Set(0.0f, 0.0f, 1.0f);

			vcTemp = vcDir * vcDir.z;
			vcUp = vcY - vcTemp;

			fL = vcUp.GetLength();

			// we tried our best
			if (fL < 1e-6f)
				return ZFX_FAIL;
		}
	}
	vcUp /= fL;

	// build right vector
	ZFXVector vcRight;
	vcRight.Cross(vcUp, vcDir);

	// create and activate final view matrix
	return SetView3D(vcRight, vcUp, vcDir, vcPos);
}
/*---------------------------------------------------------*/

HRESULT ZFXD3D::GetFrustum(ZFXPlane* p)
{
	// left plane
	p[0].m_vcN.x	= -(m_mViewProj._14 + m_mViewProj._11);
	p[0].m_vcN.y	= -(m_mViewProj._24 + m_mViewProj._21);
	p[0].m_vcN.z	= -(m_mViewProj._34 + m_mViewProj._31);
	p[0].m_fD		= -(m_mViewProj._44 + m_mViewProj._41);

	// right plane
	p[1].m_vcN.x	= -(m_mViewProj._14 - m_mViewProj._11);
	p[1].m_vcN.y	= -(m_mViewProj._24 - m_mViewProj._21);
	p[1].m_vcN.z	= -(m_mViewProj._34 - m_mViewProj._31);
	p[1].m_fD		= -(m_mViewProj._44 - m_mViewProj._41);

	// top plane
	p[2].m_vcN.x	= -(m_mViewProj._14 - m_mViewProj._12);
	p[2].m_vcN.y	= -(m_mViewProj._24 - m_mViewProj._22);
	p[2].m_vcN.z	= -(m_mViewProj._34 - m_mViewProj._32);
	p[2].m_fD		= -(m_mViewProj._44 - m_mViewProj._42);

	// bottom plane
	p[3].m_vcN.x	= -(m_mViewProj._14 + m_mViewProj._12);
	p[3].m_vcN.y	= -(m_mViewProj._24 + m_mViewProj._22);
	p[3].m_vcN.z	= -(m_mViewProj._34 + m_mViewProj._32);
	p[3].m_fD		= -(m_mViewProj._44 + m_mViewProj._42);

	// near plane
	p[4].m_vcN.x	= -m_mViewProj._13;
	p[4].m_vcN.y	= -m_mViewProj._23;
	p[4].m_vcN.z	= -m_mViewProj._33;
	p[4].m_fD		= -m_mViewProj._43;

	// far plane
	p[5].m_vcN.x	= -(m_mViewProj._14 - m_mViewProj._13);
	p[5].m_vcN.y	= -(m_mViewProj._24 - m_mViewProj._23);
	p[5].m_vcN.z	= -(m_mViewProj._34 - m_mViewProj._33);
	p[5].m_fD		= -(m_mViewProj._44 - m_mViewProj._43);

	// normalize normals
	for (int i = 0; i < 6; ++i)
	{
		float fL = p[i].m_vcN.GetLength();
		p[i].m_vcN	/= fL;
		p[i].m_fD	/= fL;
	}
	return ZFX_OK;
}
/*---------------------------------------------------------*/

void ZFXD3D::SetClippingPlanes(float fNear, float fFar)
{
	m_fNear = fNear;
	m_fFar	= fFar;

	if (m_fNear <= 0.0f) m_fNear = 0.01;
	if (m_fFar <= 1.0f) m_fFar = 1.00f;

	if (m_fNear >= m_fFar) {
		m_fNear = m_fFar;
		m_fFar	= m_fNear + 1.0f;
	}

	// adjust 2d matrices
	Prepare2D();

	// adjust orthogonal projection
	float Q = 1.0f / (m_fFar - m_fNear);
	float X = -Q * m_fNear;
	m_mProj0[0]._33 = m_mProj0[1]._33 = Q;
	m_mProj0[2]._33 = m_mProj0[3]._33 = Q;
	m_mProj0[0]._43 = m_mProj0[1]._43 = X;
	m_mProj0[2]._43 = m_mProj0[3]._43 = X;

	// adjust perspective projection
	Q *= m_fFar;
	X = -Q * m_fNear;
	m_mProj0[0]._33 = m_mProj0[1]._33 = Q;
	m_mProj0[2]._33 = m_mProj0[3]._33 = Q;
	m_mProj0[0]._43 = m_mProj0[1]._43 = X;
	m_mProj0[2]._43 = m_mProj0[3]._43 = X;
}
/*---------------------------------------------------------*/

void ZFXD3D::Prepare2D(void)
{
	// make identity matrix
	memset(&m_mProj2D, 0, sizeof(float) * 16);
	memset(&m_mView2D, 0, sizeof(float) * 16);
	m_mView2D._11 = m_mView2D._33 = m_mView2D._44 = 1.0f;

	// orthogonal projection matrix
	m_mProj2D._11 = 2.0f / (float)m_dwWidth;
	m_mProj2D._22 = 2.0f / (float)m_dwHeight;
	m_mProj2D._33 = 1.0f / (m_fFar - m_fNear);
	m_mProj2D._43 = -m_fNear * (1.0f / (m_fFar - m_fNear));
	m_mProj2D._44 = 1.0f;

	// 2d view matrix
	float tx, ty, tz;
	tx = -((int)m_dwWidth + m_dwWidth * 0.5f);
	ty = m_dwHeight - m_dwHeight * 0.5f;
	tz = m_fNear + 0.1f;

	m_mView2D._22 = -1.0f;
	m_mView2D._41 = tx;
	m_mView2D._42 = ty;
	m_mView2D._43 = tz;
}
/*---------------------------------------------------------*/

HRESULT ZFXD3D::CalcPerspProjMatrix(float fFOV, float fAspect, D3DMATRIX* m)
{
	if (fabs(m_fFar - m_fNear) < 0.01)
		return ZFX_FAIL;

	float sinFOV2 = sinf(fFOV / 2);

	if (fabs(sinFOV2) < 0.01f)
		return ZFX_FAIL;

	float cosFOV2 = cosf(fFOV / 2);

	float w = fAspect * (cosFOV2 / sinFOV2);
	float h = 1.0f * (cosFOV2 / sinFOV2);
	float Q = m_fFar / (m_fFar - m_fNear);

	memset(m, 0, sizeof(D3DMATRIX));
	(*m)._11 = w;
	(*m)._22 = h;
	(*m)._33 = Q;
	(*m)._34 = 1.0f;
	(*m)._43 = -Q * m_fNear;
	return ZFX_OK;
}	//	CalcPerspProjMatrix
/*---------------------------------------------------------*/

void ZFXD3D::CalcViewProjMatrix(void)
{
	ZFXMatrix* pA;
	ZFXMatrix* pB;

	// 2D, perspective, or orthogonal
	if (m_Mode == EMD_TWOD)
	{
		pA = (ZFXMatrix*)&m_mProj2D;
		pB = (ZFXMatrix*)&m_mView2D;
	}
	else
	{
		pB = (ZFXMatrix*)&m_mView3D;
		if (m_Mode == EMD_PERSPECTIVE)
			pA = (ZFXMatrix*)&(m_mProjP[m_nStage]);
		else
			pA = (ZFXMatrix*)&(m_mProj0[m_nStage]);
	}
	ZFXMatrix* pM = (ZFXMatrix*)&m_mViewProj;
	(*pM) = (*pA) * (*pB);
}	//	CalcViewProjMatrix
/*---------------------------------------------------------*/

void ZFXD3D::CalcWorldViewProjMatrix(void)
{
	ZFXMatrix* pProj;
	ZFXMatrix* pView;
	ZFXMatrix* pWorld;

	pWorld = (ZFXMatrix*)&m_mWorld;

	// 2D, perspective, or orthogonal
	if (m_Mode == EMD_TWOD)
	{
		pProj = (ZFXMatrix*)&m_mProj2D;
		pView = (ZFXMatrix*)&m_mView2D;
	}
	else
	{
		pView = (ZFXMatrix*)&m_mView3D;
		if (m_Mode == EMD_PERSPECTIVE)
			pProj = (ZFXMatrix*)&(m_mProjP[m_nStage]);
		else
			pProj = (ZFXMatrix*)&(m_mProj0[m_nStage]);
	}
	ZFXMatrix* pCombo = (ZFXMatrix*)&m_mWorldViewProj;
	(*pCombo) = ((*pWorld) * (*pView) * (*pProj));
}	//	CalcViewProjMatrix
/*---------------------------------------------------------*/

HRESULT ZFXD3D::SetMode(ZFXENGINEMODE Mode, int nStage)
{
	D3DVIEWPORT9 d3dVP;

	if (!m_bRunning) return E_FAIL;
	if ((nStage > 3) || (nStage < 0)) nStage = 0;

	if (m_Mode != Mode)
		m_Mode = Mode;

	// flush all caches prior to changing mode
	m_pVertexMan->ForcedFlushAll();

	// if 2d use its matrices
	if (Mode == EMD_TWOD) {
		d3dVP.X			= 0;
		d3dVP.Y			= 0;
		d3dVP.Width		= m_dwWidth;
		d3dVP.Height	= m_dwHeight;
		d3dVP.MinZ		= 0.0f;
		d3dVP.MaxZ		= 1.0f;

		if (FAILED(m_pDevice->SetViewport(&d3dVP)))
			return ZFX_FAIL;

		if (!m_bUseShaders) {
			if (FAILED(m_pDevice->SetTransform(D3DTS_PROJECTION, &m_mProj2D)))
				return ZFX_FAIL;
			if (FAILED(m_pDevice->SetTransform(D3DTS_VIEW, &m_mView2D)))
				return ZFX_FAIL;
		}
	}
	// perspective or orthogonal projection
	else {
		m_nStage = nStage;

		// set viewport
		d3dVP.X			= m_VP[nStage].X;
		d3dVP.Y			= m_VP[nStage].Y;
		d3dVP.Width		= m_VP[nStage].Width;
		d3dVP.Height	= m_VP[nStage].Height;
		d3dVP.MinZ		= 0.0f;
		d3dVP.MaxZ		= 1.0f;

		if (FAILED(m_pDevice->SetViewport(&d3dVP)))
			return ZFX_FAIL;

		if (!m_bUseShaders) {
			if (m_Mode == EMD_PERSPECTIVE) {
				if (FAILED(m_pDevice->SetTransform(D3DTS_PROJECTION, &m_mProjP[nStage])))
					return ZFX_FAIL;
			}
			else {		// EMD_ORTHOGONAL
				if (FAILED(m_pDevice->SetTransform(D3DTS_PROJECTION, &m_mProj0[nStage])))
					return ZFX_FAIL;
			}
		}
		CalcViewProjMatrix();
		CalcWorldViewProjMatrix();
	}
	return ZFX_OK;
}	//	SetMode
/*---------------------------------------------------------*/

HRESULT ZFXD3D::InitStage(float fFOV, ZFXVIEWPORT* pView, int nStage)
{
	float	fAspect;
	bool	bOwnRect = false;

	if (!pView)
	{
		ZFXVIEWPORT vpOwn = { 0, 0, m_dwWidth, m_dwHeight };
		memcpy(&m_VP[nStage], &vpOwn, sizeof(RECT));
	}
	else
		memcpy(&m_VP[nStage], pView, sizeof(RECT));

	if ((nStage > 3) || (nStage < 0)) nStage = 0;

	fAspect = ((float)(m_VP[nStage].Height)) / (m_VP[nStage].Width);

	// perspective projection matrix
	if (FAILED(this->CalcPerspProjMatrix(fFOV, fAspect, &m_mProjP[nStage])))
	{
		return ZFX_FAIL;
	}

	// orthogonal projection matrix
	memset(&m_mProj0[nStage], 0, sizeof(float) * 16);
	m_mProj0[nStage]._11 = 2.0f / m_VP[nStage].Width;
	m_mProj0[nStage]._22 = 2.0f / m_VP[nStage].Height;
	m_mProj0[nStage]._33 = 1.0f / (m_fFar - m_fNear);
	m_mProj0[nStage]._43 = -m_fNear * m_mProj0[nStage]._33;
	m_mProj0[nStage]._44 = 1.0f;
	
	return ZFX_OK;
}	//	InitStage
/*---------------------------------------------------------*/

POINT ZFXD3D::Transform3DTo2D(const ZFXVector& vcPoint)
{
	POINT pt;
	float fClip_x, fClip_y;
	float fXp, fYp, fWp;
	DWORD dwWidth, dwHeight;

	// if 2d mode take the whole screen
	if (m_Mode == EMD_TWOD) {
		dwWidth		= m_dwWidth;
		dwHeight	= m_dwHeight;
	}
	// else take viewport dimensions
	else {
		dwWidth		= m_VP[m_nStage].Width;
		dwHeight	= m_VP[m_nStage].Height;
	}
	fClip_x = (float)(dwWidth >> 1);
	fClip_y = (float)(dwHeight >> 1);

	// transformation & projection
	fXp = (m_mViewProj._11 * vcPoint.x) + (m_mViewProj._21 * vcPoint.y)
		+ (m_mViewProj._31 * vcPoint.z) + m_mViewProj._41;
	fYp = (m_mViewProj._12 * vcPoint.x) + (m_mViewProj._22 * vcPoint.y)
		+ (m_mViewProj._32 * vcPoint.z) + m_mViewProj._42;
	fXp = (m_mViewProj._14 * vcPoint.x) + (m_mViewProj._24 * vcPoint.y)
		+ (m_mViewProj._34 * vcPoint.z) + m_mViewProj._44;

	float fWpInv = 1.0f / fWp;

	// converting from [-1, 1] to viewport size
	pt.x = (LONG)((1.0f + (fXp * fWpInv)) * fClip_x);
	pt.y = (LONG)((1.0f + (fYp * fWpInv)) * fClip_y);
	return pt;
}
/*---------------------------------------------------------*/

void ZFXD3D::Transform2DTo3D(const POINT& pt, ZFXVector* vcOrig, ZFXVector* vcDir)
{
	D3DMATRIX*	pView = NULL, * pProj = NULL;
	ZFXMatrix	mInvView;
	ZFXVector	vcS;
	DWORD		dwWidth,
				dwHeight;

	// 2d mode
	if (m_Mode == EMD_TWOD) {
		dwWidth		= m_dwWidth;
		dwHeight	= m_dwHeight;

		pView = &m_mView2D;
	}
	// else orthogonal or perspective projection
	else {
		dwWidth		= m_VP[m_nStage].Width;
		dwHeight	= m_VP[m_nStage].Height;

		pView = &m_mView3D;

		if (m_Mode == EMD_PERSPECTIVE)
			pProj = &m_mProjP[m_nStage];
		else
			pProj = &m_mProj0[m_nStage];
	}

	// scale to viewport and inverse projection
	vcS.x = (((pt.x * 2.0f) / dwWidth) - 1.0f) / m_mProjP[m_nStage]._11;
	vcS.y = -(((pt.y * 2.0f) / dwHeight) - 1.0f) / m_mProjP[m_nStage]._22;
	vcS.z = 1.0f;

	// invert view matrix
	mInvView.InverseOf(*((ZFXMatrix*)&m_mView3D._11));

	// ray from screen to world coordinates
	(*vcDir).x = (vcS.x * mInvView._11) + (vcS.y * mInvView._21)
				+ (vcS.z * mInvView._31);
	(*vcDir).y = (vcS.x * mInvView._12) + (vcS.y * mInvView._22)
				+ (vcS.z * mInvView._32);
	(*vcDir).z = (vcS.x * mInvView._13) + (vcS.y * mInvView._23)
				+ (vcS.z * mInvView._33);

	// inverse translation
	(*vcOrig).x = mInvView._41;
	(*vcOrig).y = mInvView._42;
	(*vcOrig).z = mInvView._43;
	// normalizing
	(*vcDir).Normalize();
}
/*---------------------------------------------------------*/

void ZFXD3D::SetWorldTransform(const ZFXMatrix* mWorld)
{
	// last chance check
	m_pVertexMan->ForcedFlushAll();

	// class attribute 'world matrix'
	if (!mWorld) {
		ZFXMatrix m;
		m.Identity();
		memcpy(&m_mWorld, &m, sizeof(D3DMATRIX));
	}
	else
		memcpy(&m_mWorld, mWorld, sizeof(D3DMATRIX));

	// recalculating depending values
	CalcWorldViewProjMatrix();

	// if shader set a constant
	if (m_bUseShaders) {
		ZFXMatrix mTranspose;
		mTranspose.TransposeOf(*(ZFXMatrix*)&m_mWorldViewProj);
		m_pDevice->SetVertexShaderConstantF(0, (float*)&mTranspose, 4);
	}
	else
		m_pDevice->SetTransform(D3DTS_WORLD, &m_mWorld);
}	//	SetWorldTransform

// SHADER FUNCTIONS:
// ----------------

void ZFXD3D::PrepareShaderStuff()
{
	D3DCAPS9 d3dCaps;

	if (FAILED(m_pDevice->GetDeviceCaps(&d3dCaps)))
	{
		m_bUseShaders = false;
		return;
	}

	if (d3dCaps.VertexShaderVersion < D3DVS_VERSION(1, 1))
	{
		m_bUseShaders = false;
		return;
	}

	if (d3dCaps.PixelShaderVersion < D3DPS_VERSION(1, 1))
	{
		m_bUseShaders = false;
		return;
	}

	// vertex declaration for vertex shaders
	D3DVERTEXELEMENT9 declVertex[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	D3DVERTEXELEMENT9 declLVertex[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
		{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	// Create the vertex declarations
	m_pDevice->CreateVertexDeclaration(declVertex, &m_pDeclVertex);
	m_pDevice->CreateVertexDeclaration(declLVertex, &m_pDeclLVertex);
	m_pDevice->SetFVF(NULL);

	m_bUseShaders = true;
}	//	PrepareShaderStuff
/*---------------------------------------------------------*/

HRESULT ZFXD3D::CreateVShader(const void* pData, UINT nSize, bool bLoadFromFile, bool bIsCompiled, UINT* pID)
{
	LPD3DXBUFFER	pCode = NULL;
	LPD3DXBUFFER	pDebug = NULL;
	HRESULT			hrC = ZFX_OK, hrA = ZFX_OK;
	DWORD*			pVS = NULL;
	HANDLE			hFile, hMap;

	// is there storage room for one more?
	if (m_nNumVShaders >= (MAX_SHADER - 1))
		return ZFX_OUTOFMEMORY;

	// (1): ALREADY ASSEMBLED SHADER
	if (bIsCompiled) {

		// from file
		if (bLoadFromFile) {
			hFile = CreateFile((LPCTSTR)pData, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			if (hFile == INVALID_HANDLE_VALUE)
				return ZFX_FILENOTFOUND;

			hMap = CreateFileMapping(hFile, 0, PAGE_READONLY, 0, 0, 0);
			pVS = (DWORD*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
		}

		// from RAM pointer
		else {
			pVS = (DWORD*)pData;
		}
	}	// if

	// (2): NEEDS TO ASSEMBLED
	else {
		// from file pointer
		if (bLoadFromFile) {
			hrA = D3DXAssembleShaderFromFile((char*)pData, NULL, NULL, 0, &pCode, &pDebug);
		}
		// from RAM pointer
		else {
			hrA = D3DXAssembleShader((char*)pData, nSize - 1, NULL, NULL, 0, &pCode, &pDebug);
		}

		// check error
		if (SUCCEEDED(hrA)) {
			pVS = (DWORD*)pCode->GetBufferPointer();
		}
		else {
			Log("error: AssembleShader[FromFile]() failed");
			if (pDebug->GetBufferPointer())
				Log("Shader debugger says: %s", (char*)pDebug->GetBufferPointer());
			return ZFX_FAIL;
		}	
	}	// else
		
	// create the shader object
	if (FAILED(hrC = m_pDevice->CreateVertexShader(pVS, &m_pVShader[m_nNumVShaders]))) {
		Log("error: CreateVertexShader() failed");
		return ZFX_FAIL;
	}

	// save id of this shader
	if (pID) (*pID) = m_nNumVShaders;

	// free resource
	if (bIsCompiled && bLoadFromFile) {
		UnmapViewOfFile(pVS);
		CloseHandle(hMap);
		CloseHandle(hFile);
	}

	++m_nNumVShaders;
	return ZFX_OK;
}	//	CreateVShader
/*---------------------------------------------------------*/

HRESULT ZFXD3D::ActivateVShader(UINT nID, ZFXVERTEXID VertexID)
{
	if (!m_bUseShaders)
		return ZFX_NOSHADERSUPPOT;
	if (nID >= m_nNumVShaders)
		return ZFX_INVALIDID;

	// write out vertex caches
	m_pVertexMan->ForcedFlushAll();

	// get vertex size and format
	switch (VertexID) {
		case VID_UU: {
			if (FAILED(m_pDevice->SetVertexDeclaration(m_pDeclVertex)))
				return ZFX_FAIL;
		}	break;
		case VID_UL: {
			if (FAILED(m_pDevice->SetVertexDeclaration(m_pDeclLVertex)))
				return ZFX_FAIL;
		}	break;
		default: return ZFX_INVALIDID;
	}	// switch

	if (FAILED(m_pDevice->SetVertexShader(m_pVShader[nID])))
		return ZFX_FAIL;

	return ZFX_OK;
}	//	ActivateVShader
/*---------------------------------------------------------*/

HRESULT ZFXD3D::CreatePShader(const void* pData, UINT nSize, bool bLoadFromFile, bool bIsCompiled, UINT* pID)
{
	LPD3DXBUFFER	pCode = NULL;
	LPD3DXBUFFER	pDebug = NULL;
	HRESULT			hrC = ZFX_OK, hrA = ZFX_OK;
	DWORD*			pPS = NULL;
	HANDLE			hFile, hMap;

	// is there storage room for one more?
	if (m_nNumPShaders >= (MAX_SHADER - 1))
	{
		return ZFX_OUTOFMEMORY;
	}

	// (1): ALREADY ASSEMBLED SHADER
	if (bIsCompiled)
	{
		// from file
		if (bLoadFromFile)
		{
			hFile = CreateFile((LPCSTR)pData, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				return ZFX_FILENOTFOUND;
			}

			hMap = CreateFileMapping(hFile, 0, PAGE_READONLY, 0, 0, 0);
			pPS = (DWORD*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
		}
		//from RAM pointer
		else
		{
			pPS = (DWORD*)pData;
		}
	}
	// (2): NEEDS TO BE ASSEMBLED
	else
	{
		// from file
		if (bLoadFromFile)
		{
			hrA = D3DXAssembleShaderFromFile((char*)pData, NULL, NULL, 0, &pCode, &pDebug);
		}
		//from RAM pointer
		else
		{
			hrA = D3DXAssembleShader((char*)pData, nSize - 1, NULL, NULL, 0, &pCode, &pDebug);
		}

		// check error
		if (SUCCEEDED(hrA))
		{
			pPS = (DWORD*)pCode->GetBufferPointer();
		}
		else
		{
			Log("error: AssembleShader() failed");
			if (pDebug->GetBufferPointer())
			{
				Log("Shader debugger says: %s", (char*)pDebug->GetBufferPointer());
			}
			return ZFX_FAIL;
		}
	}

	//create the shader object
	if (FAILED(hrC = m_pDevice->CreatePixelShader(pPS, &m_pPShader[m_nNumPShaders])))
	{
		Log("error: CreatePixelShader() failed");
		return ZFX_FAIL;
	}

	//save ID of this shader
	if (pID)
	{
		(*pID) = m_nNumPShaders;
	}

	//free resources
	if (bIsCompiled && bLoadFromFile)
	{
		UnmapViewOfFile(pPS);
		CloseHandle(hMap);
		CloseHandle(hFile);
	}

	m_nNumPShaders++;
	return ZFX_OK;
}	//	CreatePShader
/*---------------------------------------------------------*/

HRESULT ZFXD3D::ActivatePShader(UINT nID)
{
	if (!m_bUseShaders) 
		return ZFX_NOSHADERSUPPOT;
	if (nID >= m_nNumPShaders)
		return ZFX_INVALIDID;

	// write out vertex caches
	m_pVertexMan->ForcesFlushAll();

	if (FAILED(m_pDevice->SetPixelShader(m_pPShader[nID])))
		return ZFX_FAIL;

	return ZFX_OK;
}	//	ActivatePShader
/*---------------------------------------------------------*/

void ZFXD3D::SetBackfaceCulling(ZFXRENDERSTATE rs)
{
	m_pVertexMan->ForcedFlushAll();
	if (rs == RS_CULL_CW)
		m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	else if (rs == RS_CULL_CCW)
		m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	else if (rs == RS_CULL_NONE)
		m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
}	//	SetBackfaceCulling
/*---------------------------------------------------------*/

void ZFXD3D::SetDepthBufferMode(ZFXRENDERSTATE rs)
{
	m_pVertexMan->ForcedFlushAll();
	if (rs == RS_DEPTH_READWRITE) {
		m_pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
		m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	}
	else if (rs == RS_DEPTH_READONLY) {
		m_pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
		m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	}
	else if (rs == RS_DEPTH_NONE) {
		m_pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
		m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	}
}	//	SetDepthBufferMode
/*---------------------------------------------------------*/

inline DWORD FtoDW(FLOAT f) { return *((DWORD*)&f); }

void ZFXD3D::SetShadeMode(ZFXRENDERSTATE smd, float f, const ZFXCOLOR* pClr)
{
	m_pVertexMan->ForcedFlushAll();

	// copy new color if any
	if (pClr) {
		memcpy(&m_clrWire, pClr, sizeof(ZFXCOLOR));
		m_pVertexMan->InvalidateStates();
	}

	// no changes in mode
	if (smd == m_ShadeMode) {
		// maybe change in size
		if (smd == RS_SHADE_POINTS)
			m_pDevice->SetRenderState(D3DRS_POINTSIZE, FtoDW(f));

		return;
	}

	if (smd == RS_SHADE_TRIWIRE) {
		// real Direct3D wireframe mode
		m_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
		m_ShadeMode = smd;
	}
	else {
		if (smd != RS_SHADE_SOLID) {
			m_pDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);
		}
		m_pDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
		m_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		m_ShadeMode = smd;
	}

	if (smd == RS_SHADE_POINTS) {
		if (f > 0.0f) {
			m_pDevice->SetRenderState(D3DRS_POINTSPRITEENABLE, TRUE);
			m_pDevice->SetRenderState(D3DRS_POINTSCALEENABLE, TRUE);
			m_pDevice->SetRenderState(D3DRS_POINTSIZE, FtoDW(f));
			m_pDevice->SetRenderState(D3DRS_POINTSIZE_MIN, FtoDW(0.00f));
			m_pDevice->SetRenderState(D3DRS_POINTSCALE_A, FtoDW(0.00f));
			m_pDevice->SetRenderState(D3DRS_POINTSCALE_B, FtoDW(0.00f));
			m_pDevice->SetRenderState(D3DRS_POINTSCALE_C, FtoDW(1.00f));
		}
		else {
			m_pDevice->SetRenderState(D3DRS_POINTSPRITEENABLE, FALSE);
			m_pDevice->SetRenderState(D3DRS_POINTSCALEENABLE, FALSE);
		}
	}
	else {
		m_pDevice->SetRenderState(D3DRS_POINTSPRITEENABLE, FALSE);
		m_pDevice->SetRenderState(D3DRS_POINTSPRITEENABLE, FALSE);
	}

	// update depending states
	m_pVertexMan->InvalidateStates();
}	//	SetShadeMode
/*---------------------------------------------------------*/

HRESULT ZFXD3D::CreateFont(const char* chType, int nWeight, bool bItalic, bool bUnderline, bool bStrike, DWORD dwSize, UINT* pID)
{
	HRESULT hr;
	HDC		hDC;
	int		nHeight;

	if (!pID) return ZFX_INVALIDPARAM;

	hDC = GetDC(NULL);
	nHeight = -MulDiv(dwSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	ReleaseDC(NULL, hDC);

	m_pFont = (LPD3DXFONT*)realloc(m_pFont, sizeof(LPD3DXFONT) * (m_nNumFonts + 1));

	// D3DXCreateFont with HFONT is deprecated
	hr = D3DXCreateFont(m_pDevice, 
		nHeight, nHeight / 2,		// height width
		nWeight,					// thickness
		1,
		bItalic,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		"Arial",
		& m_pFont[m_nNumFonts]);

	if (SUCCEEDED(hr)) {
		(*pID) = m_nNumFonts;
		++m_nNumFonts;
		return ZFX_OK;
	}
	else {
		return ZFX_FAIL;
	}
}	//	CreateFont
/*---------------------------------------------------------*/

HRESULT ZFXD3D::DrawText(UINT nID, int x, int y, UCHAR r, UCHAR g, UCHAR b, char* ch, ...)
{
	RECT	rc = { x, y, 0, 0 };
	char	cch[1024];
	char*	pArgs;

	// move optional parameters into the string
	pArgs = (char*)&ch + sizeof(ch);
	vsprintf(cch, ch, pArgs);
	
	if (nID >= m_nNumFonts) return ZFX_INVALIDPARAM;

	// Begin transfer to LPD3DXSPRITE
	// m_pFont[nID]->Begin();

	// calculate bounding rect for the size
	m_pFont[nID]->DrawText(NULL, cch, -1, &rc, DT_SINGLELINE | DT_CALCRECT, 0);

	// now draw the text
	m_pFont[nID]->DrawText(NULL, cch, -1, &rc, DT_SINGLELINE, D3DCOLOR_ARGB(255, r, g, b));

	// End transfer to LPD3DXPRITE
	// m_pFont[nID]->End();
	return ZFX_OK;
}	//	DrawText
/*---------------------------------------------------------*/

