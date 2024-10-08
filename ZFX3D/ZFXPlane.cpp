#include "ZFX3D.h"

// set plane's value but caculate distance to origin
inline void ZFXPlane::Set(const ZFXVector& vcN, const ZFXVector& vcPoint)
{
	m_fD		= -(vcN * vcPoint);
	m_vcN		= vcN;
	m_vcPoint	= vcPoint;
}
/*---------------------------------------------------------*/

// set all plane's values direct;y
inline void ZFXPlane::Set(const ZFXVector& vcN, const ZFXVector& vcPoint, float fD)
{
	m_vcN		= vcN;
	m_fD		= fD;
	m_vcPoint	= vcPoint;
}
/*---------------------------------------------------------*/

// calculate plane from three points forming two vectors
inline void ZFXPlane::Set(const ZFXVector& v0, const ZFXVector& v1, const ZFXVector& v2)
{
	ZFXVector vcEdge1 = v1 - v0;
	ZFXVector vcEdge2 = v2 - v0;

	m_vcN.Cross(vcEdge1, vcEdge2);
	m_fD = m_vcN * v0;
}
/*---------------------------------------------------------*/

// Calculte distance point to plane
// normal vector needs to be normalized
inline float ZFXPlane::Distance(const ZFXVector& vcP)
{
	return (_fabs((m_vcN * vcP) - m_fD));
}
/*---------------------------------------------------------*/

// classify a point with respect to the plane
inline int ZFXPlane::Classify(const ZFXVector& vcP)
{
	float f = (vcP * m_vcN) + m_fD;

	if (f >  0.00001) return ZFXFRONT;
	if (f < -0.00001) return ZFXBACK;
	return ZFXPLANAR;
}
/*---------------------------------------------------------*/

// Classify polygon with respect to this plane
int ZFXPlane::Classify(const ZFXPolygon& poly)
{
	int NumFront = 0, NumBack = 0, NumPlanar = 0;
	int nClass;

	// cast away const
	ZFXPolygon* pPoly = ((ZFXPolygon*)&poly);

	int NumPoints = pPoly->GetNumPoints();

	// loop through all points
	for (int i = 0; i < NumPoints; ++i) {
		nClass = Classify(pPoly->m_pPoints[i]);

		if (nClass == ZFXFRONT)
			++NumFront;
		else if (nClass == ZFXBACK)
			++NumBack;
		else {
			++NumFront;
			++NumBack;
			++NumPlanar;
		}
	}	//	for

	// all points are planar
	if (NumPlanar == NumPoints)
		return ZFXPLANAR;
	// all points are in front of plane
	else if (NumFront == NumPoints)
		return ZFXFRONT;
	// all points are on backside of plane
	else if (NumBack == NumPoints)
		return ZFXBACK;
	// poly is intersecting the plane
	else
		return ZFXCLIPPED;
}	//	Classify
/*---------------------------------------------------------*/

bool ZFXPlane::Intersects(const ZFXVector& vc0, const ZFXVector& vc1, const ZFXVector& vc2)
{
	int n = this->Classify(vc0);

	if ((n == this->Classify(vc1)) && (n == this->Classify(vc2)))
		return false;

	return true;
}	//	Intersescts(Tri)

bool ZFXPlane::Intersects(const ZFXPlane& plane, ZFXRay* pIntersection)
{
	ZFXVector	vcCross;
	float		fSqrLength;

	// if cross product equals 0 planes are parallel
	vcCross.Cross(this->m_vcN, plane.m_vcN);
	fSqrLength = vcCross.GetSqrLength();

	if (fSqrLength < 1e-08f)
		return false;

	// intersection line if needed
	if (pIntersection) {
		float fN00 = this->m_vcN.GetSqrLength();
		float fN01 = this->m_vcN * plane.m_vcN;
		float fN11 = plane.m_vcN.GetSqrLength();
		float fDet = fN00 * fN11 - fN01 * fN01;

		if (_fabs(fDet) < 1e-08f)
			return false;

		float fInvDet = 1.0f / fDet;
		float fC0 = (fN11 * this->m_fD - fN01 * plane.m_fD) * fInvDet;
		float fC1 = (fN00 * plane.m_fD - fN01 * this->m_fD) * fInvDet;

		(*pIntersection).m_vcDir = vcCross;
		(*pIntersection).m_vcOrig = this->m_vcN * fC0 + plane.m_vcN * fC1;
	}
	return true;
}	//	Intersects(Plane)

bool ZFXPlane::Intersects(const ZFXAabb& aabb)
{
	ZFXVector Vmin, Vmax;

	// x-coordinate
	if (m_vcN.x >= 0.0f) {
		Vmin.x = aabb.vcMin.x;
		Vmax.x = aabb.vcMax.x;
	}
	else {
		Vmin.x = aabb.vcMax.x;
		Vmax.x = aabb.vcMin.x;
	}

	// y-coordinate
	if (m_vcN.y >= 0.0f) {
		Vmin.y = aabb.vcMin.y;
		Vmax.y = aabb.vcMax.y;
	}
	else {
		Vmin.y = aabb.vcMax.y;
		Vmax.y = aabb.vcMin.y;
	}

	// z-coordinate
	if (m_vcN.z >= 0.0f) {
		Vmin.z = aabb.vcMin.z;
		Vmax.z = aabb.vcMax.z;
	}
	else {
		Vmin.z = aabb.vcMax.z;
		Vmax.z = aabb.vcMin.z;
	}

	if (((m_vcN * Vmin) + m_fD) > 0.0f)
		return false;

	if (((m_vcN * Vmax) + m_fD) >= 0.0f)
		return true;

	return false;
}	//	Intersects(AABB)

bool ZFXPlane::Intersects(const ZFXObb& obb)
{
	float fRadius = _fabs(obb.fA0 * (m_vcN * obb.vcA0))
		+ _fabs(obb.fA1 * (m_vcN * obb.vcA1))
		+ _fabs(obb.fA2 * (m_vcN * obb.vcA2));

	float fDistance = this->Distance(obb.vcCenter);
	return (fDistance <= fRadius);
}	//	Intersects(OBB)