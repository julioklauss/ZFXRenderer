#include "ZFXMCEgo.h"

void ZFXMCEgo::SetRotation(float rx, float ry, float rz)
{
	m_fRotX = rx;
	m_fRotY = ry;
	m_fRotZ = rz;
	RecalcAxes();
}	//	SetRotation
/*----------------------------------------------------------------*/

void ZFXMCEgo::GetRotation(float* pfX, float* pfY, float* pfZ)
{
	if (pfX) *pfX = m_fRotX;
	if (pfY) *pfY = m_fRotY;
	if (pfZ) *pfZ = m_fRotZ;
}	//	GetRotation
/*----------------------------------------------------------------*/

ZFXVector ZFXMCEgo::GetRotation(void)
{
	return ZFXVector(m_fRotX, m_fRotY, m_fRotZ);
}	//	GetRotation
/*----------------------------------------------------------------*/

void ZFXMCEgo::RecalcAxes(void)
{
	ZFXMatrix	mat;

	static float f2PI = 6.283185f;

	// keep in range of 2 PI = 360 degree
	if (m_fRotY > f2PI)
		m_fRotY -= f2PI;
	else if (m_fRotY < -f2PI)
		m_fRotY += f2PI;

	// up/down max 80 degree
	if (m_fRotX > 1.4f)
		m_fRotX = 1.4f;
	else if (m_fRotX < -1.4f)
		m_fRotX = -1.4f;

	// initalizing axis
	m_vcRight	= ZFXVector(1.0f, 0.0f, 0.0f);
	m_vcUp		= ZFXVector(0.0f, 1.0f, 0.0f);
	m_vcDir		= ZFXVector(0.0f, 0.0f, 1.0f);

	// rotate around y-axis
	mat.RotaArbi(m_vcUp, m_fRotY);
	m_vcRight	= m_vcRight * mat;
	m_vcDir		= m_vcDir * mat;
	
	// rotate arond x-axis
	mat.RotaArbi(m_vcRight, m_fRotX);
	m_vcUp		= m_vcUp * mat;
	m_vcDir		= m_vcDir * mat;

	// correct rounding errors
	m_vcDir.Normalize();
	m_vcRight.Cross(m_vcUp, m_vcDir);
	m_vcRight.Normalize();
	m_vcUp.Cross(m_vcDir, m_vcRight);
	m_vcUp.Normalize();
}	//	RecalcAxes
/*----------------------------------------------------------------*/

void ZFXMCEgo::Update(float fET)
{
	ZFXVector vcS;
	// add rotation speed
	m_fRotX += (m_fPitchSpd * fET);
	m_fRotY += (m_fYawSpd * fET);
	m_fRotZ += (m_fRollSpd * fET);

	// calculate axis
	RecalcAxes();

	// calculate speed vector
	m_vcV	= m_vcDir * m_fSpeed * fET;
	vcS		= m_vcRight * m_fSlide * fET;

	// move position
	m_vcPos += m_vcV + vcS;
}	//	Update
/*----------------------------------------------------------------*/
