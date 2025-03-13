#include "ZFX3D.h"
#include <memory>

#define SAFE_FREE(p) if ((p) != NULL) { free(p); (p) = NULL; }
#define SAFE_DELETE(p) if ((p) != NULL) { delete (p); (p) = NULL; }

ZFXBspTree::ZFXBspTree(void)
{
	m_NumPolys	= 0;
	m_pBack		= NULL;
	m_pFront	= NULL;
	m_pRoot		= NULL;
	m_pParent	= NULL;
	m_pPolys	= NULL;
}	//	constructor
/*---------------------------------------------------------*/

ZFXBspTree::~ZFXBspTree(void)
{
	m_NumPolys = 0;

	SAFE_FREE(m_pPolys);
	SAFE_DELETE(m_pFront);
	SAFE_DELETE(m_pBack);
}	//	destuctor
/*---------------------------------------------------------*/

void ZFXBspTree::BuildTree(const ZFXPolygon* pPolys, UINT Num)
{
	m_pRoot		= this;
	m_pParent	= NULL;
	if (Num < 1) return;

	// allocate memory
	int nSize = sizeof(ZFXPolygon) * Num;
	m_pPolys = (ZFXPolygon*)malloc(nSize);
	memset(m_pPolys, 0, nSize);
	m_NumPolys = Num;

	for (UINT i = 0; i < Num; ++i)
		m_pPolys[i].CopyOf(pPolys[i]);

	// start recursion
	CreateChilds();
}	//	BuildTree

void ZFXBspTree::CreateChilds(void)
{
	float		fDot = 0.0f;
	ZFXPolygon	plyFront, plyBack;
	int			nFront = 0, nBack = 0, nClass = 0;

	CalcBoundingBox(m_pPolys, m_NumPolys);

	// if no splitter found, this is a leaf
	if (!FindBestSplitter()) {
		ZFXBspTree::m_sNum += m_NumPolys;
		return;
	}

	// create objects for the two children
	m_pFront	= new ZFXBspTree;
	m_pBack		= new ZFXBspTree;
	m_pFront->SetRelationship(m_pRoot, this);
	m_pBack->SetRelationship(m_pRoot, this);

	// sort polygon into the children
	for (UINT i = 0; i < m_NumPolys; ++i)
	{
		nClass = m_Plane.Classify(m_pPolys[i]);

		if (nClass == ZFXFRONT) {
			m_pFront->AddPolygon(m_pPolys[i]);
		} 
		else if (nClass == ZFXBACK) {
			m_pBack->AddPolygon(m_pPolys[i]);
		} 
		else if (nClass == ZFXCLIPPED) {
			// split polygon at the splitting plane
			m_pPolys[i].Clip(m_Plane, &plyFront, &plyBack);

			m_pFront->AddPolygon(plyFront);
			m_pBack->AddPolygon(plyBack);
		}
		else if (nClass == ZFXPLANAR) {
			fDot = m_Plane.m_vcN * m_pPolys[i].GetPlane().m_vcN;
			if (fDot >= 0.0f) {
				m_pFront->AddPolygon(m_pPolys[i]);
			}
			else {
				m_pBack->AddPolygon(m_pPolys[i]);
			}
		}
	}	//	for

	// delete polygon list on inner leaves
	SAFE_FREE(m_pPolys);

	// RECURSION
	m_pFront->CreateChilds();
	m_pBack->CreateChilds();
}	//	CreateChilds
/*---------------------------------------------------------*/

void ZFXBspTree::AddPolygon(const ZFXPolygon& Poly)
{
	m_pPolys = (ZFXPolygon*)realloc(m_pPolys, sizeof(ZFXPolygon) * (m_NumPolys + 1));

	memset(&m_pPolys[m_NumPolys], 0, sizeof(ZFXPolygon));

	m_pPolys[m_NumPolys].CopyOf(Poly);
	++m_NumPolys;
}	//	AddPolygon
/*---------------------------------------------------------*/

void ZFXBspTree::CalcBoundingBox(const ZFXPolygon* _pPolys_, UINT Num)
{
	ZFXVector	vcMax, vcMin, vcTemp;
	ZFXAabb		Aabb;

	// cast away const
	ZFXPolygon* pPolys = (ZFXPolygon*)_pPolys_;

	if (Num < 1)
		return;

	// get arbitrary sub bounding box
	Aabb = pPolys[0].GetAabb();
	vcMax = vcMin = Aabb.vcCenter;

	for (unsigned int i = 0; i < Num; ++i)
	{
		Aabb = pPolys[i].GetAabb();

		// get obb one side's extreme values
		vcTemp = Aabb.vcMax;

		if (vcTemp.x > vcMax.x)
			vcMax.x = vcTemp.x;
		else if (vcTemp.x < vcMin.x)
			vcMin.x = vcTemp.x;

		if (vcTemp.y > vcMax.y)
			vcMax.y = vcTemp.y;
		else if (vcTemp.y < vcMin.y)
			vcMin.y = vcTemp.y;

		if (vcTemp.z > vcMax.z)
			vcMax.z = vcTemp.z;
		else if (vcTemp.z < vcMin.z)
			vcMin.z = vcTemp.z;

		// get obb other side's extreme values
		vcTemp = Aabb.vcMin;

		if (vcTemp.x > vcMax.x)
			vcMax.x = vcTemp.x;
		else if (vcTemp.x < vcMin.x)
			vcMin.x = vcTemp.x;

		if (vcTemp.y > vcMax.y)
			vcMax.y = vcTemp.y;
		else if (vcTemp.y > vcMin.y)
			vcMin.y = vcTemp.y;

		if (vcTemp.z > vcMax.z)
			vcMax.z = vcTemp.z;
		else if (vcTemp.z < vcMin.z)
			vcMin.z = vcTemp.z;

		// now calculate maximum extension
		float fMax = vcMax.x - vcMin.x;
		if (fMax < (vcMax.y - vcMin.y))
			fMax = vcMax.y - vcMin.y;
		if (fMax < (vcMax.z - vcMin.z))
			fMax = vcMax.z - vcMin.z;

		// make box cubic
		m_Aabb.vcCenter = (vcMax + vcMin) / 2.0f;
		m_Aabb.vcMax = m_Aabb.vcCenter + (fMax / 2.0f);
		m_Aabb.vcMin = m_Aabb.vcCenter - (fMax / 2.0f);
	}
}	//	CalcBoundingBox
/*---------------------------------------------------------*/

bool ZFXBspTree::FindBestSplitter(void)
{
	ZFXPolygon* pBestSplitter = NULL, * pSplitter = NULL;
	ZFXPlane	Plane;
	LONG		lFront = 0,		// how many polygons lay in
				lBack = 0,		// front, back, and planar
				lPlanar = 0,	// or spanning with regard
				lSplits = 0;	// to each possible splitter
	int			nClass;
	LONG		lScore, lBestScore = 1000000;
	bool		bFound = false;

	for (UINT i = 0; i < m_NumPolys; ++i)
	{
		pSplitter	= &m_pPolys[i];
		Plane		= pSplitter->GetPlane();

		// reset counters
		lFront = lBack = lPlanar = lSplits = 0;

		// has been used as splitter already?
		if (pSplitter->GetFlag() == 1)
			continue;

		// test all polygons as splitter
		for (UINT j = 0; j < m_NumPolys; ++j)
		{
			if (i == j)
				continue;

			nClass = Plane.Classify(m_pPolys[j]);
			if (nClass == ZFXFRONT)
				++lFront;
			else if (nClass == ZFXBACK)
				++lBack;
			else if (nClass == ZFXPLANAR)
				++lPlanar;
			else
				++lSplits;
		}	// for
	
		// CALCULATE SCORE
		lScore = abs(lFront - lBack) + (lSplits * 3);
		if (lScore < lBestScore)
		{
			if (((lFront > 0) && (lBack > 0)) || (lSplits > 0)) {
				lBestScore = lScore;
				pBestSplitter = pSplitter;
				bFound = true;
			}
		}	// if [ulScore]
	}	// for

	// no splitter can be found
	if (!bFound)
		return false;

	// mark polygon, save splitter plane
	pBestSplitter->SetFlag(1);
	m_Plane = pBestSplitter->GetPlane();
	return true;
}	//	FindBestSplitter
/*---------------------------------------------------------*/

void ZFXBspTree::TraverseFtB(ZFXPolylist* pList, ZFXVector vcPos, const ZFXPlane* Frustum)
{
	// frustum culling for this mode
	if (m_Aabb.Cull(Frustum, 6) == ZFXCULLED)
		return;

	// leaves contain polygons
	if (IsLeaf()) {
		for (UINT i = 0; i < m_NumPolys; ++i)
			pList->AddPolygon(m_pPolys[i]);
	}
	else {
		int nClass = m_Plane.Classify(vcPos);

		if (nClass == ZFXBACK) {
			m_pBack->TraverseFtB(pList, vcPos, Frustum);
			m_pFront->TraverseFtB(pList, vcPos, Frustum);
		}
		else {
			m_pFront->TraverseFtB(pList, vcPos, Frustum);
			m_pBack->TraverseFtB(pList, vcPos, Frustum);
		}
	}
}	//	TraverseFtB
/*---------------------------------------------------------*/

void ZFXBspTree::TraverseBtF(ZFXPolylist* pList, ZFXVector vcPos, const ZFXPlane* Frustum)
{
	// frustum culling for this node
	if (m_Aabb.Cull(Frustum, 6) == ZFXCULLED)
		return;

	// leaves contains polygons
	if (IsLeaf()) {
		for (UINT i = 0; i < m_NumPolys; ++i)
			pList->AddPolygon(m_pPolys[i]);
	}
	else {
		int nClass = m_Plane.Classify(vcPos);

		if (nClass == ZFXBACK) {
			m_pFront->TraverseBtF(pList, vcPos, Frustum);
			m_pBack->TraverseBtF(pList, vcPos, Frustum);
		}
		else {
			m_pBack->TraverseBtF(pList, vcPos, Frustum);
			m_pFront->TraverseBtF(pList, vcPos, Frustum);
		}
	}
}	//	TraverseBtF
/*---------------------------------------------------------*/

bool ZFXBspTree::TestCollision(const ZFXRay& Ray, float fL, float* pfD, ZFXVector* pvcN)
{
	ZFXRay	rayFront, rayBack;
	int		nFront = 0;

	// THIS IS A LEAF
	if (IsLeaf()) {
		for (UINT i = 0; i < m_NumPolys; ++i)
		{
			// collision with a polygon?
			if (m_pPolys[i].Intersects(Ray, false, fL, 0)) {
				if (pvcN)
					*pvcN = m_pPolys[i].GetPlane().m_vcN;
				return true;
			}
		}	// for
		return false;
	}

	// ELSE THIS IS AN INNER NODE
	int nClass = m_Plane.Classify(Ray.m_vcOrig);

	// ray intersects split plane?
	if (m_Plane.Clip(&Ray, fL, &rayFront, &rayBack)) {
		// search in front-to-back order
		if (nClass == ZFXBACK)
			return m_pBack->TestCollision(rayBack, fL, pfD, pvcN) || m_pFront->TestCollision(rayFront, fL, pfD, pvcN);
		else
			return m_pFront->TestCollision(rayFront, fL, pfD, pvcN) || m_pBack->TestCollision(rayBack, fL, pfD, pvcN);
	}
	else {
		if (nClass == ZFXBACK)
			return m_pBack->TestCollision(Ray, fL, pfD, pvcN);
		else
			return m_pFront->TestCollision(Ray, fL, pfD, pvcN);
	}
}	//	TestCollision [ray]
/*---------------------------------------------------------*/

bool ZFXBspTree::LineOfSight(const ZFXVector& vcA, const ZFXVector& vcB)
{
	ZFXRay Ray;

	// ray from A to B
	ZFXVector vcDir = vcB - vcA;
	vcDir.Normalize();
	Ray.Set(vcA, vcDir);

	// test for collision
	return !TestCollision(Ray, vcDir.GetLength(), 0, 0);
}	//	LineOfSight
/*---------------------------------------------------------*/