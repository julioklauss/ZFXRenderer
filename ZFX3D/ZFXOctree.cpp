#include "ZFX3D.h"
#include <memory>

#define SAFE_FREE(p) if ((p) != NULL) { free(p); (p) = NULL; }
#define SAFE_DELETE(p) if ((p) != NULL) { delete (p); (p) = NULL; }
#define SAFE_DELETE_A(p) if ((p) != NULL) { delete[] (p); (p) = NULL; }

ZFXOctree::ZFXOctree(void)
{
	m_NumPolys	= 0;
	m_Pos		= -1;
	m_pPolys	= NULL;
	m_pRoot		= NULL;
	m_pParent	= NULL;

	for (int i = 0; i < 8; ++i)
		m_pChild[i] = NULL;

	memset(&m_Aabb, 0, sizeof(ZFXAabb));
}	//	constructor
/*---------------------------------------------------------*/

ZFXOctree::~ZFXOctree(void)
{
	m_NumPolys = 0;

	SAFE_DELETE_A(m_pPolys);

	for (int i = 0; i < 8; ++i)
	{
		SAFE_DELETE(m_pChild[i]);
	}
}	//	destructor
/*---------------------------------------------------------*/

void ZFXOctree::InitChildObject(int ChildID, ZFXOctree* pParent)
{
	ZFXAabb aabb;

	float xmin = m_Aabb.vcMin.x, xcen = m_Aabb.vcCenter.x, xmax = m_Aabb.vcMax.x;

	float ymin = m_Aabb.vcMin.y, ycen = m_Aabb.vcCenter.y, ymax = m_Aabb.vcMax.y;

	float zmin = m_Aabb.vcMin.z, zcen = m_Aabb.vcCenter.z, zmax = m_Aabb.vcMax.z;

	switch (ChildID)
	{
		case UP_NW:
			aabb.vcMax = ZFXVector(xcen, ymax, zmax);
			aabb.vcMin = ZFXVector(xmin, ycen, zcen);
			break;
		case UP_NE:
			aabb.vcMax = m_Aabb.vcMax;
			aabb.vcMin = m_Aabb.vcCenter;
			break;
		case UP_SW:
			aabb.vcMax = ZFXVector(xcen, ymax, zcen);
			aabb.vcMin = ZFXVector(xmin, ycen, zmin);
			break;
		case UP_SE:
			aabb.vcMax = ZFXVector(xmax, ymax, zcen);
			aabb.vcMin = ZFXVector(xcen, ycen, zmin);
			break;
		case LW_NW:
			aabb.vcMax = ZFXVector(xcen, ycen, zmax);
			aabb.vcMin = ZFXVector(xmin, ymin, zcen);
			break;
		case LW_NE:
			aabb.vcMax = ZFXVector(xmax, ycen, zmax);
			aabb.vcMin = ZFXVector(xcen, ymin, zcen);
			break;
		case LW_SW:
			aabb.vcMax = m_Aabb.vcCenter;
			aabb.vcMin = m_Aabb.vcMin;
			break;
		case LW_SE:
			aabb.vcMax = ZFXVector(xmax, ycen, zcen);
			aabb.vcMin = ZFXVector(xcen, ymin, zmin);
			break;
		default:
			break;
	}

	aabb.vcCenter = (aabb.vcMax + aabb.vcMin) / 2.0f;

	m_pChild[ChildID] = new ZFXOctree();
	m_pChild[ChildID]->SetBoundingBox(aabb);
	m_pChild[ChildID]->m_Pos		= ChildID;
	m_pChild[ChildID]->m_pParent	= pParent;
}	//	InitChildObjects
/*---------------------------------------------------------*/

void ZFXOctree::BuildTree(const ZFXPolygon* pPolys, UINT Num)
{
	m_pRoot = this;

	if (Num < 1)
		return;

	// calculate AABB for the root node
	CalcBoundingBox(pPolys, Num);

	m_pPolys = new ZFXPolygon[Num];
	m_NumPolys = Num;

	for (UINT i = 0; i < Num; ++i)
		m_pPolys[i].CopyOf(pPolys[i]);

	// calculate the children
	CreateChilds(this);

	SAFE_DELETE_A(m_pPolys);
}	//	BuildTree
/*---------------------------------------------------------*/

void ZFXOctree::CreateChilds(ZFXOctree* pRoot)
{
	// save address of the root node
	m_pRoot = pRoot;

	// go on?
	if ((pRoot == this) || (m_NumPolys > POLYS_PER_LEAF)) {
		// initialize children
		for (int i = 0; i < 8; ++i)
		{
			InitChildObject(i, this);

			// build polygonlist for the child
			m_pChild[i]->ChopListToMe(m_pPolys, m_NumPolys);
			m_pChild[i]->CreateChilds(pRoot);
		}
		SAFE_DELETE_A(m_pPolys);
	}
	// no, this is a leaf
	else return;
}	//	CreateChilds
/*---------------------------------------------------------*/

void ZFXOctree::ChopListToMe(ZFXPolygon* pList, UINT Num)
{
	ZFXPolygon ChoppedPoly;
	int nClass = 0;
	
	if (Num < 1)
		return;

	// better safe than sorry ...
	SAFE_DELETE_A(m_pPolys);
	m_NumPolys = 0;

	ZFXPolygon* TempMem = new ZFXPolygon[Num];

	// loop through the list
	for (UINT i = 0; i < Num; ++i)
	{
		if (pList[i].GetFlag() == 1)
			continue;

		ChoppedPoly.CopyOf(pList[i]);

		nClass = ChoppedPoly.Cull(m_Aabb);

		// polygon outside the AABB
		if (nClass == ZFXCULLED) 
			continue;
		// polygon contain in or intersecting the AABB
		else {
			// flag if contained
			if (nClass != ZFXCLIPPED)
				pList[i].SetFlag(1);
			// else clip polygon
			else ChoppedPoly.Clip(m_Aabb);

			// add to temporary list
			TempMem[m_NumPolys].CopyOf(ChoppedPoly);
			++m_NumPolys;
		}
	}

	// copy temporary list to this node
	m_pPolys = new ZFXPolygon[m_NumPolys];

	for (UINT j = 0; j < m_NumPolys; ++j)
		m_pPolys[j].CopyOf(TempMem[j]);

	delete[] TempMem;
}	//	ChopListToAabb
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

bool ZFXOctree::TestCollision(const ZFXAabb& aabb, ZFXPlane* pP)
{
	// test collision in this node
	if (this != m_pRoot) {
		if (!m_Aabb.Intersects(aabb))
			return false;
	}

	// no geometry, just a node
	if (!IsLeaf()) {
		for (int i = 0; i < 8; ++i)
		{
			// check children for collision
			if (m_pChild[i]->TestCollision(aabb, pP))
				return true;
		}	// for

		// none of the children collided either
		return false;
	}	// if [!leaf]

	// this is a leaf with geometry
	else {
		if (!m_pPolys)
			return false;

		// test all polygons for collision
		for (UINT i = 0; i < m_NumPolys; ++i)
		{
			if (m_pPolys[i].GetAabb().Intersects(aabb)) {
				if (pP) *pP = m_pPolys[i].GetPlane();
				return true;
			}
		}
		// no collision in this leaf
		return false;
	}
}	//	TestCollision [aabb]
/*---------------------------------------------------------*/

bool ZFXOctree::TestCollision(const ZFXRay& Ray, float fL, float* pfD)
{
	bool blnCollision = false;
	float _fD = 0.0f;

	// collision in this node
	if (this != m_pRoot) {
		if (!m_Aabb.Intersects(Ray, fL, pfD) && !m_Aabb.Contains(Ray, fL))
			return false;
	}
	// no geometry, just a node
	if (!IsLeaf()) {
		for (int i = 0; i < 8; ++i)
		{
			// check the children
			if (m_pChild[i]->TestCollision(Ray, fL, pfD))
				return true;
		}	// for

		// none of the children collided
		return false;
	}	// if [!leaf]

	// this is a leaf with geometry
	else {
		if (!m_pPolys)
			return false;

		// test all polygons for collision
		for (UINT i = 0; i < m_NumPolys; ++i)
		{
			if (m_pPolys[i].Intersects(Ray, false, fL, &_fD)) {
				blnCollision = true;
				if (!pfD)
					return true;

				// get the closes collision point
				if ((*pfD <= 0.0f) || (_fD < *pfD))
					*pfD = _fD;
			}
		}
		return blnCollision;
	}
	return false;
}	//	TestCollision [ray]
/*---------------------------------------------------------*/

bool ZFXOctree::IntersectsDownwardsRay(const ZFXVector& vcOrig, float f)
{
	// ray origin below this node
	if (vcOrig.y < m_Aabb.vcMin.y) return false;

	// on x-axis outside this node
	if (vcOrig.x < m_Aabb.vcMin.x) return false;
	if (vcOrig.x > m_Aabb.vcMax.x) return false;

	// on z-axis outside this node
	if (vcOrig.z < m_Aabb.vcMin.z) return false;
	if (vcOrig.z > m_Aabb.vcMax.z) return false;

	// minimal possible distance to this node is already
	// greater than current intersection found in "f"
	if (f < (fabs(m_Aabb.vcMax.y - vcOrig.y)))
		return false;

	return true;
}	//	IntersectsDownwardRay
/*---------------------------------------------------------*/

bool ZFXOctree::GetFloor(const ZFXVector& vcPos, float* pf, ZFXPlane* pPlane)
{
	float	fAabbDist = 0, fHitDist = 0;
	bool	bHit = false;
	ZFXAabb	aabb;
	ZFXRay	Ray;

	// if this is the root node
	if (this == m_pRoot)
		*pf = 99999.0f;

	// no geometry just a node
	if (!IsLeaf()) {
		for (int i = 0; i < 8; ++i)
		{
			// is ray intersecting any child at all?
			if (m_pChild[i]->IntersectsDownwardsRay(vcPos, *pf)) {
				// intersection closer than current one?
				if (m_pChild[i]->GetFloor(vcPos, pf, pPlane))
					bHit = true;
			}
		}	// for
		return bHit;
	}	// if [!leaf]
	// this is a leaf with geometry
	else {
		if (!m_pPolys)
			return false;

		Ray.Set(vcPos, ZFXVector(0.0f, -1.0f, 0.0f));

		for (UINT i = 0; i < m_NumPolys; ++i)
		{
			aabb = m_pPolys[i].GetAabb();

			// quick-test ray besides the polygon
			if ((Ray.m_vcOrig.x < aabb.vcMin.x) ||
				(Ray.m_vcOrig.x > aabb.vcMax.x) ||
				(Ray.m_vcOrig.z < aabb.vcMin.z) ||
				(Ray.m_vcOrig.z > aabb.vcMax.z) ||
				(Ray.m_vcOrig.y < aabb.vcMin.y))
				continue;

			// full blown collision test
			if (m_pPolys[i].Intersects(Ray, true, *pf, &fHitDist)) {
				*pf = fHitDist;
				bHit = true;
			}
		}	// for
		return bHit;
	}
}	//	GetFloor
/*---------------------------------------------------------*/

void ZFXOctree::Traverse(ZFXPolylist* pList, ZFXPolylist* pAabbList, const ZFXPlane* pFrustum)
{
	if (m_Aabb.Cull(pFrustum, 6) == ZFXCULLED)
		return;

	if (IsLeaf()) {
		if (pList) {
			for (unsigned int i = 0; i < m_NumPolys; ++i)
				pList->AddPolygon(m_pPolys[i]);
		}
		if (pAabbList)
			GetAabbAsPolygons(pAabbList);
	}
	else {
		m_pChild[0]->Traverse(pList, pAabbList, pFrustum);
		m_pChild[1]->Traverse(pList, pAabbList, pFrustum);
		m_pChild[2]->Traverse(pList, pAabbList, pFrustum);
		m_pChild[3]->Traverse(pList, pAabbList, pFrustum);
		m_pChild[4]->Traverse(pList, pAabbList, pFrustum);
		m_pChild[5]->Traverse(pList, pAabbList, pFrustum);
		m_pChild[6]->Traverse(pList, pAabbList, pFrustum);
		m_pChild[7]->Traverse(pList, pAabbList, pFrustum);
	}
}	//	Traverse
/*---------------------------------------------------------*/

void ZFXOctree::GetAabbAsPolygons(ZFXPolylist* pList)
{
	ZFXPolygon		Poly;
	ZFXVector		vcPoints[24];
	unsigned int	nIndis[6] = { 0, 1, 2, 2, 3, 0 };

	float fW = m_Aabb.vcMax.x - m_Aabb.vcMin.x;
	float fH = m_Aabb.vcMax.y - m_Aabb.vcMin.y;
	float fD = m_Aabb.vcMax.z - m_Aabb.vcMin.z;

	// top rectangle
	vcPoints[0].Set(m_Aabb.vcCenter.x - (fW / 2.0f),
		m_Aabb.vcCenter.y + (fH / 2.0f),
		m_Aabb.vcCenter.z - (fD / 2.0f));
	vcPoints[1].Set(m_Aabb.vcCenter.x - (fW / 2.0f),
		m_Aabb.vcCenter.y + (fH / 2.0f),
		m_Aabb.vcCenter.z + (fD / 2.0f));
	vcPoints[2].Set(m_Aabb.vcCenter.x + (fW / 2.0f),
		m_Aabb.vcCenter.y + (fH / 2.0f),
		m_Aabb.vcCenter.z + (fD / 2.0f));
	vcPoints[3].Set(m_Aabb.vcCenter.x + (fW / 2.0f),
		m_Aabb.vcCenter.y + (fH / 2.0f),
		m_Aabb.vcCenter.z - (fD / 2.0f));
	Poly.Set(&vcPoints[0], 4, nIndis, 6);
	pList->AddPolygon(Poly);

	// right rectangle
	vcPoints[4] = vcPoints[3];
	vcPoints[5] = vcPoints[2];
	vcPoints[6].Set(m_Aabb.vcCenter.x + (fW / 2.0f),
		m_Aabb.vcCenter.y - (fH / 2.0f),
		m_Aabb.vcCenter.z + (fD / 2.0f));
	vcPoints[7].Set(m_Aabb.vcCenter.x + (fW / 2.0f),
		m_Aabb.vcCenter.y - (fH / 2.0f),
		m_Aabb.vcCenter.z - (fD / 2.0f));
	Poly.Set(&vcPoints[4], 4, nIndis, 6);
	pList->AddPolygon(Poly);

	// left rectangle
	vcPoints[8] = vcPoints[0];
	vcPoints[9] = vcPoints[1];
	vcPoints[10].Set(m_Aabb.vcCenter.x - (fW / 2.0f),
		m_Aabb.vcCenter.y - (fH / 2.0f),
		m_Aabb.vcCenter.z + (fD / 2.0f));
	vcPoints[11].Set(m_Aabb.vcCenter.x - (fW / 2.0f),
		m_Aabb.vcCenter.y - (fH / 2.0f),
		m_Aabb.vcCenter.z - (fD / 2.0f));
	Poly.Set(&vcPoints[8], 4, nIndis, 6);
	pList->AddPolygon(Poly);

	// backside rectangle
	vcPoints[12] = vcPoints[2];
	vcPoints[13] = vcPoints[1];
	vcPoints[14] = vcPoints[10];
	vcPoints[15] = vcPoints[6];
	Poly.Set(&vcPoints[12], 4, nIndis, 6);
	pList->AddPolygon(Poly);

	// frontside rectangle
	vcPoints[16] = vcPoints[0];
	vcPoints[17] = vcPoints[3];
	vcPoints[18] = vcPoints[7];
	vcPoints[19] = vcPoints[1];
	Poly.Set(&vcPoints[16], 4, nIndis, 6);
	pList->AddPolygon(Poly);

	// bottom rectangle
	vcPoints[20] = vcPoints[7];
	vcPoints[21] = vcPoints[6];
	vcPoints[22] = vcPoints[10];
	vcPoints[23] = vcPoints[11];
	Poly.Set(&vcPoints[20], 4, nIndis, 6);
	pList->AddPolygon(Poly);
}	//	GetAabbPolygons
/*---------------------------------------------------------*/