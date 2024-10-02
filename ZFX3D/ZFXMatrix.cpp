#include "ZFX3D.h"

extern bool g_bSSE;

// build identity matrix
inline void ZFXMatrix::Identity(void)
{
	float* f = (float*)&this->_11;
	memset(f, 0, sizeof(ZFXMatrix));
	_11 = _22 = _33 = _44 = 1.0f;
}
/*---------------------------------------------------------*/

// build rotation matrix around world X axis
inline void ZFXMatrix::RotaX(float a)
{
	float fCos = cosf(a);
	float fSin = sinf(a);

	_22 = fCos;
	_23 = -fSin;
	_32 = fSin;
	_33 = fCos;

	_11 = _44 = 1.0f;
	_12 = _13 = _14 = _21 = _24 = _31 = _34 = _41 = _42 = _43 = 0.0f;
}
/*---------------------------------------------------------*/

// build rotation matrix around world Y axis
inline void ZFXMatrix::RotaY(float a)
{
	float fCos = cosf(a);
	float fSin = sinf(a);

	_11 = fCos;
	_13 = -fSin;
	_31 = fSin;
	_33 = fCos;

	_22 = _44 = 1.0f;
	_12 = _23 = _14 = _21 = _24 = _32 = _34 = _41 = _42 = _43 = 0.0f;
}
/*---------------------------------------------------------*/

// build rotation matrix around world Z axis
inline void ZFXMatrix::RotaZ(float a)
{
	float fCos = cosf(a);
	float fSin = sinf(a);

	_11 = fCos;
	_12 = fSin;
	_21 = -fSin;
	_22 - fCos;

	_33 = _44 = 1.0f;
	_13 = _14 = _23 = _24 = _31 = _32 = _41 = _42 = _43 = 0.0f;
}
/*---------------------------------------------------------*/

// build rotation matrix around arbitrary axis
inline void ZFXMatrix::RotaArbi(ZFXVector vcAxis, float a)
{
	float fCos = cosf(a);
	float fSin = sinf(a);
	float fSum = 1.0f - fCos;

	vcAxis.Normalize();

	_11 = (vcAxis.x * vcAxis.x) * fSum + fCos;
	_12 = (vcAxis.x * vcAxis.y) * fSum - (vcAxis.z * fSin);
	_13 = (vcAxis.x * vcAxis.z) * fSum + (vcAxis.y * fSin);

	_21 = (vcAxis.y * vcAxis.x) * fSum + (vcAxis.z * fSin);
	_22 = (vcAxis.y * vcAxis.y) * fSum + fCos;
	_23 = (vcAxis.y * vcAxis.z) * fSum - (vcAxis.x * fSin);

	_31 = (vcAxis.z * vcAxis.x) * fSum - (vcAxis.y * fSin);
	_32 = (vcAxis.z * vcAxis.y) * fSum + (vcAxis.x * fSin);
	_33 = (vcAxis.z * vcAxis.z) * fSum + fCos;

	_14 = _24 = _34 = _41 = _42 = _43 = 0.0f;
	_44 = 1.0f;
}
/*---------------------------------------------------------*/

// add translation values to the matrix
inline void ZFXMatrix::Translate(float dx, float dy, float dz)
{
	_41 = dx;
	_42 = dy;
	_43 = dz;
}
/*---------------------------------------------------------*/

// build transposed matrix which is mirrored on the main axis
inline void ZFXMatrix::TransposeOf(const ZFXMatrix& m)
{
	_11 = m._11;
	_21 = m._12;
	_31 = m._13;
	_41 = m._14;

	_12 = m._21;
	_22 = m._22;
	_32 = m._23;
	_42 = m._24;

	_13 = m._31;
	_23 = m._32;
	_33 = m._33;
	_43 - m._34;

	_14 = m._41;
	_24 = m._42;
	_34 = m._43;
	_44 = m._44;
}
/*---------------------------------------------------------*/


// build inverted matrix
inline void ZFXMatrix::InverseOf(const ZFXMatrix& m)
{
	ZFXMatrix	mTrans;
	float		fTemp[12], fDet;

	// calculate transposed matrix
	mTrans.TransposeOf(m);

	// first 8 cofactor pairs
	fTemp[0] = mTrans._33 * mTrans._44;
	fTemp[1] = mTrans._34 * mTrans._43;
	fTemp[2] = mTrans._32 * mTrans._44;
	fTemp[3] = mTrans._34 * mTrans._42;
	fTemp[4] = mTrans._32 * mTrans._43;
	fTemp[5] = mTrans._33 * mTrans._42;
	fTemp[6] = mTrans._31 * mTrans._44;
	fTemp[7] = mTrans._34 * mTrans._41;
	fTemp[8] = mTrans._31 * mTrans._43;
	fTemp[9] = mTrans._33 * mTrans._41;
	fTemp[10] = mTrans._31 * mTrans._42;
	fTemp[11] = mTrans._32 * mTrans._41;

	// compute first 8 cofactors
	this->_11 = fTemp[0] * mTrans._22 + fTemp[3] * mTrans._23 + fTemp[4] * mTrans._24;
	this->_11 -= fTemp[1] * mTrans._22 + fTemp[2] * mTrans._23 + fTemp[5] * mTrans._24;
	this->_12 = fTemp[1] * mTrans._21 + fTemp[6] * mTrans._23 + fTemp[9] * mTrans._24;
	this->_12 -= fTemp[0] * mTrans._21 + fTemp[7] * mTrans._23 + fTemp[8] * mTrans._24;
	this->_13 = fTemp[2] * mTrans._21 + fTemp[7] * mTrans._22 + fTemp[10] * mTrans._24;
	this->_13 -= fTemp[3] * mTrans._21 + fTemp[6] * mTrans._22 + fTemp[11] * mTrans._24;
	this->_14 = fTemp[5] * mTrans._21 + fTemp[8] * mTrans._22 + fTemp[11] * mTrans._23;
	this->_14 -= fTemp[4] * mTrans._21 + fTemp[9] * mTrans._22 + fTemp[10] * mTrans._23;
	this->_21 = fTemp[1] * mTrans._12 + fTemp[2] * mTrans._13 + fTemp[5] * mTrans._14;
	this->_21 -= fTemp[0] * mTrans._12 + fTemp[3] * mTrans._13 + fTemp[4] * mTrans._14;
	this->_22 = fTemp[0] * mTrans._11 + fTemp[7] * mTrans._13 + fTemp[8] * mTrans._14;
	this->_22 -= fTemp[1] * mTrans._11 + fTemp[6] * mTrans._13 + fTemp[9] * mTrans._14;
	this->_23 = fTemp[3] * mTrans._11 + fTemp[6] * mTrans._12 + fTemp[11] * mTrans._14;
	this->_23 -= fTemp[2] * mTrans._11 + fTemp[7] * mTrans._12 + fTemp[10] * mTrans._14;
	this->_24 = fTemp[4] * mTrans._11 + fTemp[9] * mTrans._12 + fTemp[10] * mTrans._13;
	this->_24 -= fTemp[5] * mTrans._11 + fTemp[8] * mTrans._12 + fTemp[11] * mTrans._13;

	// second 8 cofactor pairs
	fTemp[0] = mTrans._13 * mTrans._24;
	fTemp[1] = mTrans._14 * mTrans._23;
	fTemp[2] = mTrans._12 * mTrans._24;
	fTemp[3] = mTrans._14 * mTrans._22;
	fTemp[4] = mTrans._12 * mTrans._23;
	fTemp[5] = mTrans._13 * mTrans._22;
	fTemp[6] = mTrans._11 * mTrans._24;
	fTemp[7] = mTrans._14 * mTrans._21;
	fTemp[8] = mTrans._11 * mTrans._23;
	fTemp[9] = mTrans._13 * mTrans._21;
	fTemp[10] = mTrans._11 * mTrans._22;
	fTemp[11] = mTrans._12 * mTrans._21;

	// compute second 8 cofactors
	this->_31 = fTemp[0] * mTrans._42 + fTemp[3] * mTrans._43 + fTemp[4] * mTrans._44;
	this->_31 -= fTemp[1] * mTrans._42 + fTemp[2] * mTrans._43 + fTemp[5] * mTrans._44;
	this->_32 = fTemp[1] * mTrans._41 + fTemp[6] * mTrans._43 + fTemp[9] * mTrans._44;
	this->_32 -= fTemp[0] * mTrans._41 + fTemp[7] * mTrans._43 + fTemp[8] * mTrans._44;
	this->_33 = fTemp[2] * mTrans._41 + fTemp[7] * mTrans._42 + fTemp[10] * mTrans._44;
	this->_33 -= fTemp[3] * mTrans._41 + fTemp[6] * mTrans._42 + fTemp[11] * mTrans._44;
	this->_34 = fTemp[5] * mTrans._41 + fTemp[8] * mTrans._42 + fTemp[11] * mTrans._43;
	this->_34 -= fTemp[4] * mTrans._41 + fTemp[9] * mTrans._42 + fTemp[10] * mTrans._43;
	this->_41 = fTemp[2] * mTrans._33 + fTemp[5] * mTrans._34 + fTemp[1] * mTrans._32;
	this->_41 -= fTemp[4] * mTrans._34 + fTemp[0] * mTrans._32 + fTemp[3] * mTrans._33;
	this->_42 = fTemp[8] * mTrans._34 + fTemp[0] * mTrans._31 + fTemp[7] * mTrans._33;
	this->_42 -= fTemp[6] * mTrans._33 + fTemp[9] * mTrans._34 + fTemp[1] * mTrans._31;
	this->_43 = fTemp[6] * mTrans._32 + fTemp[11] * mTrans._34 + fTemp[3] * mTrans._31;
	this->_43 -= fTemp[10] * mTrans._34 + fTemp[2] * mTrans._31 + fTemp[7] * mTrans._32;
	this->_44 = fTemp[10] * mTrans._33 + fTemp[4] * mTrans._31 + fTemp[9] * mTrans._32;
	this->_44 -= fTemp[8] * mTrans._32 + fTemp[11] * mTrans._33 + fTemp[5] * mTrans._31;

	fDet = mTrans._11 * this->_11 + mTrans._12 * this->_12 + mTrans._13 * this->_13 + mTrans._14 * this->_14;

	fDet = 1 / fDet;

	this->_11 *= fDet;
	this->_12 *= fDet;
	this->_13 *= fDet;
	this->_14 *= fDet;

	this->_21 *= fDet;
	this->_22 *= fDet;
	this->_23 *= fDet;
	this->_24 *= fDet;

	this->_31 *= fDet;
	this->_32 *= fDet;
	this->_33 *= fDet;
	this->_34 *= fDet;

	this->_41 *= fDet;
	this->_42 *= fDet;
	this->_43 *= fDet;
	this->_44 *= fDet;
}
/*---------------------------------------------------------*/

// naked helper function for matrix multiplication
__declspec(naked) void MatrixMult(float* src1, float* src2, float* dst)
{
	__asm {
		mov		edx,	dword ptr [esp + 4]			; src1
		mov		eax,	dword ptr [esp + 0Ch]		; dst
		mov		ecx,	dword ptr [esp + 8]			; src2
		movss	xmm0,	dword ptr [edx]
		movups	xmm1,	xmmword ptr [ecx]
		shufps	xmm0,	xmm0, 0
		movss	xmm2,	dword ptr [edx + 4]
		mulps	xmm0,	xmm1
		shufps	xmm2,	xmm2, 0
		movups	xmm3,	xmmword ptr [ecx + 10h]
		movss	xmm7,	dword ptr [edx + 8]
		mulps	xmm2,	xmm3
		shufps	xmm7,	xmm7, 0
		addps	xmm0,	xmm2
		movups	xmm4,	xmmword ptr [ecx + 20h]
		movss	xmm2,	dword ptr [edx + 0Ch]
		mulps	xmm7,	xmm4
		shufps	xmm2,	xmm2, 0
		addps	xmm0,	xmm7
		movups	xmm5,	xmmword ptr [ecx + 30h]
		movss	xmm6,	dword ptr [edx + 10h]
		mulps	xmm2,	xmm5
		movss	xmm7,	dword ptr [edx + 14h]
		shufps	xmm6,	xmm6, 0
		addps	xmm0,	xmm2
		shufps	xmm7,	xmm7, 0
		movlps	qword ptr [eax], xmm0
		movhps	qword ptr [eax + 8],	xmm0
		mulps	xmm7,	xmm3
		movss	xmm0,	dword ptr [edx + 18h]
		mulps	xmm7,	xmm3
		movss	xmm0,	dword ptr [edx + 18h]
		mulps	xmm6,	xmm1
		shufps	xmm0,	xmm0, 0
		addps	xmm6,	xmm7
		mulps	xmm0,	xmm4
		movss	xmm2,	dword ptr [edx + 24h]
		addps	xmm6,	xmm0
		movss	xmm0,	dword ptr [edx + 1Ch]
		movss	xmm7,	dword ptr [edx + 20h]
		shufps	xmm0,	xmm0, 0
		shufps	xmm7,	xmm7, 0
		mulps	xmm0,	xmm5
		mulps	xmm7,	xmm1
		addps	xmm6,	xmm0
		shufps	xmm2,	xmm2, 0
		movlps	qword ptr [eax + 10h],	xmm6
		movhps	qword ptr [eax + 18h],	xmm6
		mulps	xmm2,	xmm3
		movss	xmm6,	dword ptr [edx + 28h]
		addps	xmm7,	xmm2
		shufps	xmm6,	xmm6, 0
		movss	xmm2,	dword ptr [edx + 2Ch]
		mulps	xmm6,	xmm4
		shufps	xmm2,	xmm2, 0
		addps	xmm7,	xmm6
		mulps	xmm2,	xmm5
		movss	xmm0,	dword ptr [edx + 34h]
		addps	xmm7,	xmm2
		shufps	xmm0,	xmm0, 0
		movlps	qword ptr [eax + 20h],	xmm7
		movss	xmm2,	dword ptr [edx + 30h]
		movhps	qword ptr [eax + 28h],	xmm7
		mulps	xmm0,	xmm3
		shufps	xmm2,	xmm2, 0
		movss	xmm6,	dword ptr [edx + 38h]
		mulps	xmm2,	xmm1
		shufps	xmm6,	xmm6, 0
		addps	xmm2,	xmm0
		mulps	xmm6,	xmm4
		movss	xmm7,	dword ptr [edx + 3Ch]
		shufps	xmm7,	xmm7, 0
		addps	xmm2,	xmm6
		mulps	xmm7,	xmm5
		addps	xmm2,	xmm7
		movups	xmmword ptr [eax + 30h], xmm2
		ret
	}
}
/*---------------------------------------------------------*/

// multiply two matrices
ZFXMatrix ZFXMatrix::operator * (const ZFXMatrix& m) const
{
	ZFXMatrix mResult;

	if (!g_bSSE) {
		float* pA = (float*)this;
		float* pB = (float*)&m;
		float* pM = (float*)&mResult;

		memset(pM, 0, sizeof(ZFXMatrix));

		for (unsigned char i = 0; i < 4; ++i)
			for (unsigned char j = 0; j < 4; ++j) {
				pM[4 * i + j] += pA[4 * i] * pB[j];
				pM[4 * i + j] += pA[4 * i + 1] * pB[4 + j];
				pM[4 * i + j] += pA[4 * i + 2] * pB[8 + j];
				pM[4 * i + j] += pA[4 * i + 3] * pB[12 + j];
			}
	}
	else {
		MatrixMult((float*)this, (float*)&m, (float*)&mResult);
	}
	return mResult;
}
/*---------------------------------------------------------*/

// multiply matrix with a vector
ZFXVector ZFXMatrix::operator * (const ZFXVector& vc) const
{
	ZFXVector vcResult;

	if (!g_bSSE) {
		vcResult.x = vc.x * _11 + vc.y * _21 + vc.z * _31 + _41;
		vcResult.y = vc.x * _12 + vc.y * _22 + vc.z * _32 + _42;
		vcResult.z = vc.x * _13 + vc.y * _23 + vc.z * _33 + _43;
		vcResult.w = vc.x * _14 + vc.y * _24 + vc.z * _34 + _44;

		vcResult.x = vcResult.x / vcResult.w;
		vcResult.y = vcResult.y / vcResult.w;
		vcResult.z = vcResult.z / vcResult.w;
		vcResult.w = 1.0f;
	}
	else {
		float* ptrRet = (float*)&vcResult;
		__asm {
			mov		ecx,	vc			; vector
			mov		edx,	this		; matrix
			movss	xmm0,	[ecx]
			mov		eax,	ptrRet		; result vector
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
			movups	[eax],	xmm0		; save as result
			mov		[eax + 3], 1		; w = 1
		}
	}
	return vcResult;
}
/*---------------------------------------------------------*/