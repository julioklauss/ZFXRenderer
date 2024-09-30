#include "ZFX3D.h"

void ZFXAabb::Construct(const ZFXObb& Obb)
{
	ZFXVector vcA0, vcA1, vcA2;
	ZFXVector _vcMax, _vcMin;

	vcA0 = Obb.vcA0 * Obb.fA0;
	vcA1 = Obb.vcA1 * Obb.fA1;
	vcA2 = Obb.vcA2 * Obb.fA2;

	if (vcA0.x > vcA1.x) {
		if (vcA0.x > vcA2.x) {
			vcMax.x = vcA0.x;
			vcMin.x = -vcA0.x;
		}
		else {
			vcMax.x = vcA2.x;
			vcMin.x = -vcA2.x;
		}
	}
	else {
		if (vcA1.x > vcA2.x) {
			vcMax.x = vcA1.x;
			vcMin.x = -vcA1.x;
		}
		else {
			vcMax.x = vcA2.x;
			vcMin.x = -vcA2.x;
		}
	}
	if (vcA0.y > vcA1.y) {
		if (vcA0.y > vcA2.y) {
			vcMax.y = vcA0.y;
			vcMin.y = -vcA0.y;
		}
		else {
			vcMax.y = vcA2.y;
			vcMin.y = -vcA2.y;
		}
	}
	else {
		if (vcA1.y > vcA2.y) {
			vcMax.y = vcA1.y;
			vcMin.y = -vcA1.y;
		}
		else {
			vcMax.y = vcA2.y;
			vcMin.y = -vcA2.y;
		}
	}
	if (vcA0.z > vcA1.z) {
		if (vcA0.z > vcA2.z) {
			vcMax.z = vcA0.z;
			vcMin.z = -vcA0.z;
		}
		else {
			vcMax.z = vcA2.z;
			vcMin.z = -vcA2.z;
		}
	}
	else {
		if (vcA1.z > vcA2.z) {
			vcMax.z = vcA1.z;
			vcMin.z = -vcA1.z;
		}
		else {
			vcMax.z = vcA2.z;
			vcMin.z = -vcA2.z;
		}
	}

	vcMax += Obb.vcCenter;
	vcMin += Obb.vcCenter;
}	//	construct

int ZFXAabb::Cull(const ZFXPlane* pPlanes, int nNumPlanes)
{
	ZFXVector	vcMin, vcMax;
	bool		bIntersects = false;

	// build and test extreme points
	for (int i = 0; i < nNumPlanes; ++i) {
		if (pPlanes[i].m_vcN.x >= 0.0f) {
			vcMin.x = this->vcMin.x;
			vcMax.x = this->vcMax.x;
		}
		else {
			vcMin.x = this->vcMax.x;
			vcMax.x = this->vcMin.x;
		}
		if (pPlanes[i].m_vcN.y >= 0.0f) {
			vcMin.y = this->vcMin.y;
			vcMax.y = this->vcMax.y;
		}
		else {
			vcMin.y = this->vcMax.y;
			vcMax.y = this->vcMax.y;
		}
		if (pPlanes[i].m_vcN.z >= 0.0f) {
			vcMin.z = this->vcMin.z;
			vcMax.z = this->vcMax.z;
		}
		else {
			vcMin.z = this->vcMax.z;
			vcMax.z = this->vcMin.z;
		}

		if (((pPlanes[i].m_vcN * vcMin) + pPlanes[i].m_fD) > 0.0f)
			return ZFXCULLED;

		if (((pPlanes[i].m_vcN * vcMax) + pPlanes[i].m_fD) >= 0.0f)
			bIntersects = true;
	}	//	for
	
	if (bIntersects)
		return ZFXCLIPPED;
	return ZFXVISIBLE;
}	//	cull

void ZFXAabb::GetPlanes(ZFXPlane* pPlanes)
{
	ZFXVector vcN;
	if (!pPlanes)
		return;

	// right
	vcN.Set(1.0f, 0.0f, 0.0f);
	pPlanes[0].Set(vcN, vcMax);

	// left
	vcN.Set(-1.0f, 0.0f, 0.0f);
	pPlanes[1].Set(vcN, vcMin);

	// front
	vcN.Set(0.0f, 0.0f, -1.0f);
	pPlanes[2].Set(vcN, vcMin);

	// back
	vcN.Set(0.0f, 0.0f, 1.0f);
	pPlanes[3].Set(vcN, vcMax);

	// top
	vcN.Set(0.0f, 1.0f, 0.0f);
	pPlanes[4].Set(vcN, vcMax);

	// bottom
	vcN.Set(0.0f, -1.0f, 0.0f);
	pPlanes[5].Set(vcN, vcMin);
}	//	Intersects(point)

bool ZFXAabb::Contains(const ZFXRay& Ray, float fL)
{
	ZFXVector vcEnd = Ray.m_vcOrig + (Ray.m_vcDir * fL);

	return (Intersects(Ray.m_vcOrig) && Intersects(vcEnd));
}	//	Contains

// intersection between two aabbs
bool ZFXAabb::Intersects(const ZFXAabb& aabb)
{
	if ((vcMin.x > aabb.vcMax.x) || (aabb.vcMin.x > vcMax.x))
		return false;
	if ((vcMin.y > aabb.vcMax.y) || (aabb.vcMin.y > vcMax.y))
		return false;
	if ((vcMin.z > aabb.vcMax.z) || (aabb.vcMin.z > vcMax.z))
		return false;
	return true;
}	//	Intersects(Aabb)
/*---------------------------------------------------------*/

bool ZFXAabb::Intersects(const ZFXVector& vc)
{
	if (vc.x > vcMax.x) return false;
	if (vc.y > vcMax.y) return false;
	if (vc.z > vcMax.z) return false;
	if (vc.x < vcMin.x) return false;
	if (vc.y < vcMin.y) return false;
	if (vc.z < vcMin.z) return false;
	return true;
}	//	Intersect(point)