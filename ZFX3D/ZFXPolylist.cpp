#include "ZFX3D.h"
#include <memory>

ZFXPolylist::ZFXPolylist(void)
{
	m_pPolys	= NULL;
	m_Num		= 0;
	m_Max		= 0;
}	//	constructor
/*---------------------------------------------------------*/

ZFXPolylist::~ZFXPolylist(void)
{
	if (m_pPolys) {
		free(m_pPolys);
		m_pPolys = NULL;
	}
}	//	destructor
/*---------------------------------------------------------*/

bool ZFXPolylist::AddPolygon(const ZFXPolygon& Poly)
{
	if (!CheckMem())
		return false;
	m_pPolys[m_Num].CopyOf(Poly);
	++m_Num;
	return true;
}	//	AddPolygon
/*---------------------------------------------------------*/

void ZFXPolylist::Reset(void)
{
	if (m_pPolys) {
		free(m_pPolys);
		m_pPolys = NULL;
	}
	m_Num = 0;
	m_Max = 0;
}	//	Reset
/*---------------------------------------------------------*/

bool ZFXPolylist::CheckMem(void)
{
	if (m_Num < m_Max)
		return true;
	m_Max += 100;
	int nSize = sizeof(ZFXPolygon) * m_Max;
	m_pPolys = (ZFXPolygon*)realloc(m_pPolys, nSize);
	memset(&m_pPolys[m_Num], 0, sizeof(ZFXPolygon) * 100);
	return (m_pPolys != NULL);
}	//	CheckMem
/*---------------------------------------------------------*/