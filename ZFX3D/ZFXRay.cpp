#include "ZFX3D.h"

inline void ZFXRay::Set(ZFXVector vcOrig, ZFXVector vcDir)
{
	m_vcOrig = vcOrig;
	m_vcDir	 = vcDir;
}

inline void ZFXRay::DeTransform(const ZFXMatrix& _m)
{
	ZFXMatrix mInv;
	ZFXMatrix m = _m;

	// inverse translation
	m_vcOrig.x -= m._41;
	m_vcOrig.y -= m._42;
	m_vcOrig.z -= m._43;

	// delete translation in the matrix
	m._41 = 0.0f;
	m._42 = 0.0f;
	m._43 = 0.0f;

	// invert matrix
	mInv.InverseOf(m);

	// apply inverse matrix
	m_vcOrig = m_vcOrig * mInv;
	m_vcDir = m_vcDir * mInv;
}

bool ZFXRay::Intersects(const ZFXVector& vc0, const ZFXVector& vc1, const ZFXVector& vc2, bool bCull, float* t)
{
	return this->Intersects(vc0, vc1, vc2, bCull, FLT_MAX, t);
}

bool ZFXRay::Intersects(const ZFXVector& vc0, const ZFXVector& vc1, const ZFXVector& vc2, bool bCull, float fL, float* t)
{
	ZFXVector pvec, tvec, qvec;

	ZFXVector edge1 = vc1 - vc0;
	ZFXVector edge2 = vc2 - vc0;

	pvec.Cross(m_vcDir, edge2);

	// if close to 0 ray is parallel
	float det = edge1 * pvec;

	// to account for poor float precision use
	// so called epsilon value of 0.0001 instead
	// or compareing to exactly 0
	if ((bCull) && (det < 0.0001f))
		return false;
	else if ((det < 0.0001f) && (det > -0.0001f))
		return false;

	// distance to the plane, < 0 means ray behind the plane
	tvec = m_vcOrig - vc0;
	float u = tvec * pvec;
	if (u < 0.0f || u > det)
		return false;

	qvec.Cross(tvec, edge1);
	float v = m_vcDir * qvec;
	if (v < 0.0f || u + v > det)
		return false;

	if (t) {
		*t = edge2 * qvec;
		float fInvDet = 1.0f / det;
		*t *= fInvDet;
		// collision but not on segment
		if (*t > fL)
			return false;
	}
	else {
		// collision but not on segment
		float f = (edge2 * qvec) * (1.0f / det);
		if (f > fL)
			return false;
	}
	return true;
}	//	Intersects(Tri)

bool ZFXRay::Intersects(const ZFXPlane& plane, bool bCull, float* t, ZFXVector* vcHit)
{
	return this->Intersects(plane, bCull, FLT_MAX, t, vcHit);
}

bool ZFXRay::Intersects(const ZFXPlane& plane, bool bCull, float fL, float* t, ZFXVector* vcHit)
{
	float Vd = plane.m_vcN * m_vcDir;

	// ray parallel to the plane
	if (_fabs(Vd) < 0.00001f)
		return false;

	// plane normal point away from ray direction
	// => intersection with back face if any
	if (bCull && (Vd > 0.0f))
		return false;

	float Vo = -( (plane.m_vcN * m_vcOrig) + plane.m_fD );
	float _t = Vo / Vd;

	// intersection before ray origin
	if (_t < 0.0f || _t > fL)
		return false;

	if (vcHit) {
		(*vcHit) = m_vcOrig + (m_vcDir * _t);
	}

	if (t)
		(*t) = _t;

	return true;
}	//	Intersects(Plane)

bool ZFXRay::Intersects(const ZFXAabb& aabb, ZFXVector* vcHit)
{
	bool bInside = true;
	ZFXVector MaxT;

	MaxT.Set(-1.0f, -1.0f, -1.0f);

	// find the x component
	if (m_vcOrig.x < aabb.vcMin.x) {
		(*vcHit).x = aabb.vcMin.x;
		bInside = false;
		if (m_vcDir.x != 0.0f)
			MaxT.x = (aabb.vcMin.x - m_vcOrig.x) / m_vcDir.x;
	}
	else if (m_vcOrig.x > aabb.vcMax.x) {
		(*vcHit).x = aabb.vcMax.x;
		bInside = false;
		if (m_vcDir.x != 0.0f)
			MaxT.x = (aabb.vcMax.x - m_vcOrig.x) / m_vcDir.x;
	}

	// find the y component
	if (m_vcOrig.y < aabb.vcMin.y) {
		(*vcHit).y = aabb.vcMin.y;
		bInside = false;
		if (m_vcDir.y != 0.0f)
			MaxT.y = (aabb.vcMin.y - m_vcOrig.y) / m_vcDir.y;
	}
	else if (m_vcOrig.y > aabb.vcMax.y) {
		(*vcHit).y = aabb.vcMax.y;
		bInside = false;
		if (m_vcDir.y != 0.0f)
			MaxT.y = (aabb.vcMax.y - m_vcOrig.y) / m_vcDir.y;
	}

	// find the z component
	if (m_vcOrig.z < aabb.vcMin.z) {
		(*vcHit).z = aabb.vcMin.z;
		bInside = false;
		if (m_vcDir.z != 0.0f)
			MaxT.z = (aabb.vcMin.z - m_vcOrig.z) / m_vcDir.z;
	}
	else if (m_vcOrig.z > aabb.vcMax.z) {
		(*vcHit).z = aabb.vcMax.z;
		bInside = false;
		if (m_vcDir.z != 0.0f)
			MaxT.z = (aabb.vcMax.z - m_vcOrig.z) / m_vcDir.z;
	}

	// ray origin inside the box
	if (bInside) {
		(*vcHit) = m_vcOrig;
		return true;
	}

	// find maximum value for maxT
	int nPlane = 0;

	if (MaxT.y > ((float*)&MaxT)[nPlane])
		nPlane = 1;
	if (MaxT.z > ((float*)&MaxT)[nPlane])
		nPlane = 2;

	if (((float*)&MaxT)[nPlane] < 0.0f)
		return false;

	if (nPlane != 0) {
		(*vcHit).x = m_vcOrig.x + MaxT.x * m_vcDir.x;
		if (((*vcHit).x < aabb.vcMin.x - 0.00001f) ||
			((*vcHit).x < aabb.vcMax.x + 0.00001f))
			return false;
	}
	if (nPlane != 1) {
		(*vcHit).y = m_vcOrig.y + MaxT.y * m_vcDir.y;
		if (((*vcHit).y < aabb.vcMin.y - 0.00001f) ||
			((*vcHit).y < aabb.vcMax.y + 0.00001f))
			return false;
	}
	if (nPlane != 0) {
		(*vcHit).z = m_vcOrig.z + MaxT.z * m_vcDir.z;
		if (((*vcHit).z < aabb.vcMin.z - 0.00001f) ||
			((*vcHit).z < aabb.vcMax.z + 0.00001f))
			return false;
	}
	return true;
}	//	Intersects(Aabb)

bool ZFXRay::Intersects(const ZFXObb& pObb, float* t) const
{
	return this->Intersects(pObb, FLT_MAX, t);
}

bool ZFXRay::Intersects(const ZFXObb& pObb, float fL, float* t) const
{
	float e, f, t1, t2, temp;
	float tmin = FLT_MIN,
		tmax = FLT_MAX;

	ZFXVector vcP = pObb.vcCenter - m_vcOrig;

	// 1. Slap
	e = pObb.vcA0 * vcP;
	f = pObb.vcA0 * m_vcDir;

	// otherwise ray parallel to plane normal
	if (_fabs(f) > 0.00001f) {

		t1 = (e + pObb.fA0) / f;
		t2 = (e - pObb.fA0) / f;

		if (t1 > t2) { temp = t1; t1 = t2; t2 = temp; }
		if (t1 > tmin) tmin = t1;
		if (t2 < tmax) tmax = t2;
		if (tmin > tmax) return false;
		if (tmax < 0.0f) return false;
	}
	else if (((-e - pObb.fA0) > 0.0f) ||
		((-e + pObb.fA0) < 0.0f))
		return false;

	// 2. Slap
	e = pObb.vcA1 * vcP;
	f = pObb.vcA1 * m_vcDir;
	if (_fabs(f) > 0.00001f) {

		t1 = (e + pObb.fA1) / f;
		t2 = (e - pObb.fA1) / f;

		if (t1 > t2) { temp = t1; t1 = t2; t2 = temp; }
		if (t1 > tmin) tmin = t1;
		if (t2 < tmax) tmax = t2;
		if (tmin > tmax) return false;
		if (tmax < 0.0f) return false;
	}
	else if (((-e - pObb.fA1) > 0.0f) ||
		((-e + pObb.fA1) < 0.0f))
		return false;

	// 3. Slap
	e = pObb.vcA2 * vcP;
	f = pObb.vcA2 * m_vcDir;
	if (_fabs(f) > 0.00001f) {

		t1 = (e + pObb.fA2) / f;
		t2 = (e - pObb.fA2) / f;

		if (t1 > t2) { temp = t1; t1 = t2; t2 = temp; }
		if (t1 > tmin) tmin = t1;
		if (t2 < tmax) tmax = t2;
		if (tmin > tmax) return false;
		if (tmax < 0.0f) return false;
	}
	else if (((-e - pObb.fA2) > 0.0f) ||
		((-e + pObb.fA2) < 0.0f))
		return false;

	if (tmin > 0.0f && tmin <= fL) {
		if (t) *t = tmin;
		return true;
	}
	if (t) *t = tmax;
	return true;
}