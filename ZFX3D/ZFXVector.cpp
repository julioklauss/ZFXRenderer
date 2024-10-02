#include "ZFX3D.h"

extern bool g_bSSE;

float _fabs(float f) 
{
	if (f < 0.0f)
		return -f;
	return f;
}

inline void ZFXVector::Set(float _x, float _y, float _z, float _w)
{
	x = _x;
	y = _y;
	z = _z;
	w = _w;
}
/*---------------------------------------------------------*/

void ZFXVector::operator += (const ZFXVector& v)
{
	x += v.x;
	y += v.y;
	z += v.z;
}
/*---------------------------------------------------------*/

ZFXVector ZFXVector::operator + (const ZFXVector& v) const
{
	return ZFXVector(x + v.x, y + v.y, z + v.z);
}
/*---------------------------------------------------------*/

void ZFXVector::operator -= (const ZFXVector& v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
}
/*---------------------------------------------------------*/

ZFXVector ZFXVector::operator - (const ZFXVector& v) const
{
	return ZFXVector(x - v.x, y - v.y, z - v.z);
}
/*---------------------------------------------------------*/

void ZFXVector::operator *= (float f) 
{
	x *= f;
	y *= f;
	z *= f;
}
/*---------------------------------------------------------*/

void ZFXVector::operator /= (float f)
{
	x /= f;
	y /= f;
	z /= f;
}
/*---------------------------------------------------------*/

ZFXVector ZFXVector::operator * (float f) const
{
	return ZFXVector(x * f, y * f, z * f);
}
/*---------------------------------------------------------*/

float ZFXVector::operator * (const ZFXVector& v) const
{
	return (v.x * x + v.y * y + v.z * z);
}
/*---------------------------------------------------------*/

ZFXVector ZFXVector::operator + (float f) const
{
	return ZFXVector(x + f, y + f, z + f);
}
/*---------------------------------------------------------*/

ZFXVector ZFXVector::operator - (float f) const
{
	return ZFXVector(x - f, y - f, z - f);
}

ZFXVector ZFXVector::operator / (float f) const
{
	return ZFXVector(x / f, y / f, z / f);
}

inline float ZFXVector::GetSqrLength(void) const
{
	return (x * x + y * y + z * z);
}
/*---------------------------------------------------------*/

inline void ZFXVector::Negate(void)
{
	x = -x;
	y = -y;
	z = -z;
}
/*---------------------------------------------------------*/

inline void ZFXVector::Difference(const ZFXVector& u, const ZFXVector& v)
{
	x = v.x - u.x;
	y = v.y - u.y;
	z = v.z - u.z;
	w = 1.0f;
}
/*---------------------------------------------------------*/

inline float ZFXVector::AngleWith(ZFXVector& v)
{
	return (float)acos(((*this) * v) / (this->GetLength() * v.GetLength()));
}
/*---------------------------------------------------------*/

inline float ZFXVector::GetLength(void)
{
	float f;

	if (!g_bSSE) {
		f = (float)sqrt(x * x + y * y + z * z);
	}
	else {
		float* pf = &f;
		w = 0.0f;
		__asm {
			mov		ecx,	pf			; point to the result register
			mov		esi,	this		; vector U
			movups	xmm0,	[esi]		; vector U in XMM0
			mulps	xmm0,	xmm0		; multiply
			movaps	xmm1,	xmm0		; copy result
			shufps	xmm1,	xmm1,	4Eh	; shuffle: f1, f0, f3, f2
			addps	xmm0,	xmm1
			movaps	xmm1,	xmm0		; copy result
			shufps	xmm1,	xmm1,	11h
			addps	xmm0,	xmm1
			sqrtss	xmm0,	xmm0		; square root
			movss	[ecx],	xmm0		; move result to ECX which is f
		}
		w = 1.0f;
	}
	return f;
}
/*---------------------------------------------------------*/

inline void ZFXVector::Normalize(void)
{
	if (!g_bSSE) {
		float f = (float)sqrt(x * x + y * y + z * z);

		if (f != 0.0f) {
			x /= f;
			y /= f;
			z /= f;
		}
	}
	else {
		w = 0.0f;
		__asm {
			mov		esi,	this
			movups	xmm0,	[esi]
			movaps	xmm2,	xmm0
			mulps	xmm0,	xmm0
			movaps	xmm1,	xmm0
			shufps	xmm1,	xmm1,	4Eh
			addps	xmm0,	xmm1
			movaps	xmm1,	xmm0
			shufps	xmm1,	xmm1,	11h
			addps	xmm0,	xmm1

			rsqrtps	xmm0,	xmm0		; reciprocal square root
			mulps	xmm2,	xmm0		; multiply
			movups	[esi],	xmm2
		}
		w = 1.0f;
	}
}
/*---------------------------------------------------------*/

inline void ZFXVector::Cross(const ZFXVector& u, const ZFXVector& v)
{
	if (!g_bSSE) {
		x = u.y * v.z - u.z * v.y;
		y = u.z * v.x - u.x * v.z;
		z = u.x * v.y - u.y * v.x;
		w = 1.0f;
	}
	else {
		__asm {
			mov esi, u
			mov edi, v

			movups xmm0, [esi]
			movups xmm1, [edi]
			movaps xmm2, xmm0
			movaps xmm3, xmm1

			shufps xmm0, xmm0, 0xc9
			shufps xmm1, xmm1, 0xd2
			mulps  xmm0, xmm1

			shufps xmm2, xmm2, 0xd2
			shufps xmm3, xmm3, 0xc9
			mulps  xmm2, xmm3

			subps  xmm0, xmm2

			mov	   esi,  this
			movups [esi], xmm0
		}
		w = 1.0f;
	}
}
/*---------------------------------------------------------*/

ZFXVector ZFXVector::operator * (const ZFXMatrix& m) const
{
	ZFXVector vcResult;

	if (!g_bSSE) {
		vcResult.x = x * m._11 + y * m._21 + z * m._31 + m._41;
		vcResult.y = x * m._12 + y * m._22 + z * m._32 + m._42;
		vcResult.z = x * m._13 + y * m._23 + z * m._33 + m._43;
		vcResult.w = x * m._14 + y * m._24 + z * m._34 + m._44;

		vcResult.x = vcResult.x / vcResult.w;
		vcResult.y = vcResult.y / vcResult.w;
		vcResult.z = vcResult.z / vcResult.w;
		vcResult.w = 1.0f;
	}
	else {
		float* ptrRet = (float*)&vcResult;
		__asm {
			mov		ecx,	this	; vector
			mov		edx,	m		; matrix
			movss	xmm0,	[ecx]
			mov		eax,	ptrRet	; result vector
			shufps	xmm0,	xmm0, 0
			movss	xmm1,	[ecx + 4]
			mulps	xmm0,	[edx]
			shufps	xmm1,	xmm1, 0
			movss	xmm2,	[ecx + 8]
			mulps	xmm1,	[edx + 16]
			shufps	xmm2,	xmm2, 0
			movss	xmm3,	[ecx + 12]
			mulps	xmm2,	[edx + 32]
			shufps	xmm3,	xmm3, 0
			addps	xmm0,	xmm1
			mulps	xmm3,	[edx + 48]
			addps	xmm2,	xmm3
			addps	xmm0,	xmm2
			movups	[eax],	xmm0	; store as result
			mov		[eax + 3], 1	; w = 1
		}
	}
	return vcResult;
}
/*---------------------------------------------------------*/
