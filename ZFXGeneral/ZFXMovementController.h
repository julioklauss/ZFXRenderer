#pragma once

#include <ZFX3D.h>

class ZFXMovementController
{
	public:
		ZFXMovementController();
		virtual ~ZFXMovementController();

		virtual void Update(float fElapsedTime) = 0;

		// Accessor Methods
		ZFXVector GetPos(void)		{ return m_vcPos; }
		ZFXVector GetRight(void)	{ return m_vcRight; }
		ZFXVector GetUp(void)		{ return m_vcUp; }
		ZFXVector GetDir(void)		{ return m_vcDir; }
		ZFXVector GetVelocity(void) { return m_vcV; }

	protected:
		ZFXVector	m_vcPos;		// position
		ZFXVector	m_vcRight;		// right vector
		ZFXVector	m_vcUp;			// up vector
		ZFXVector	m_vcDir;		// direction vector
		ZFXVector	m_vcV;			// speed vector
		ZFXQuat		m_Quat;			// quaternion for rotation

		// rotation speed on local axis
		float		m_fRollSpd;
		float		m_fPitchSpd;
		float		m_fYawSpd;

		float		m_fRollSpdMax;
		float		m_fPitchSpdMax;
		float		m_fYawSpdMax;

		// rotation angle on local axis
		float		m_fRotX;
		float		m_fRotY;
		float		m_fRotZ;

		float		m_fThrust;

		// methods
		virtual void RecalcAxes(void);
		virtual void Init(void);
};