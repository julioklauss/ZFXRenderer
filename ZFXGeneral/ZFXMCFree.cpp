#include "ZFXMCFree.h"

void ZFXMCFree::SetRotation(float x, float y, float z)
{
	m_fRotX = x;
	m_fRotY = y;
	m_fRotZ = z;
	RecalcAxes();
}	//	SetRotation
/*----------------------------------------------------------------*/

void ZFXMCFree::AddRotationSpeed(float sx, float sy, float sz)
{
	m_fPitchSpd += sx;
	m_fYawSpd	+= sy;
	m_fRollSpd	+= sz;
}	//	AddRotationSpeed
/*----------------------------------------------------------------*/

void ZFXMCFree::SetRotationSpeed(float sx, float sy, float sz)
{
	m_fPitchSpd = sx;
	m_fYawSpd	= sy;
	m_fRollSpd	= sz;
}	//	SetRotationSpeed
/*----------------------------------------------------------------*/

void ZFXMCFree::Update(float fET)
{
	// adding rotation speed
	m_fRotX = (m_fPitchSpd * fET);
	m_fRotY = (m_fPitchSpd * fET);
	m_fRotZ = (m_fPitchSpd * fET);

	// recalc speed vector
	m_vcV = m_vcDir * m_fThrust * fET;

	// move position
	m_vcPos += m_vcV;

	// recalc axis
	RecalcAxes();
}	//	Update
/*----------------------------------------------------------------*/