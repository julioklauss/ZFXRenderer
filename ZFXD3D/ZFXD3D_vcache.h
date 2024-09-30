#ifndef ZFXD3DVCACHE_H
#define ZFXD3DVCACHE_H

#include "ZFXD3D.h"
#include "ZFXD3D_skinman.h"
#include <ZFX_vcache.h>

#define	NUM_CACHES 10

typedef struct ZFXSTATICBUFFER_TYPE
{
	int		nStride;
	UINT	nSkinID;
	bool	bIndis;
	int		nNumVerts;
	int		nNumIndis;
	int		nNumTris;
	DWORD	dwFVF;
	LPDIRECT3DVERTEXBUFFER9	pVB;
	LPDIRECT3DINDEXBUFFER9	pIB;
} ZFXSTATICBUFFER;

typedef struct ZFXINDEXBUFFER_TYPE
{
	int		nNumIndis;
	int		nNumTris;
	LPDIRECT3DINDEXBUFFER9	pIB;
} ZFXINDEXBUFFER;

class ZFXD3DVCache
{
	public:

		ZFXD3DVCache(UINT nVertsMax, UINT nIndisMax, UINT nStride, ZFXD3DSkinManager* pSkinMan, LPDIRECT3DDEVICE9 pDevice, ZFXD3DVCManager* pDad, DWORD dwID, FILE* pLog);
		
		~ZFXD3DVCache(void);

		HRESULT	Flush(bool bUseShaders);

		HRESULT Add(UINT nVerts, UINT nIndis, const void* pVerts, const WORD* pIndis, bool bUseShaders);

		void	SetSkin(UINT SkinID, bool bUseShader);
		bool	UsesSkin(UINT SkinID) { return (m_SkinID == SkinID); }
		bool	IsEmpty(void) { if (m_nNumVerts > 0) return false; return true; }
		int		NumVerts(void) { return m_nNumVerts; }
		
	private:
		LPDIRECT3DVERTEXBUFFER9	m_pVB;
		LPDIRECT3DINDEXBUFFER9	m_pIB;
		LPDIRECT3DDEVICE9		m_pDevice;
		ZFXD3DSkinManager*		m_pSkinMan;
		ZFXD3DVCManager*		m_pDad;
		ZFXSKIN					m_Skin;
		UINT					m_SkinID;
		DWORD					m_dwID;
		DWORD					m_dwFVF;
		FILE*					m_pLog;

		UINT	m_nNumVertsMax;		// max, vertices in the buffer
		UINT	m_nNumIndisMax;		// max, indices in the buffer
		UINT	m_nNumVerts;		// number in the buffer
		UINT	m_nNumIndis;		// number in the buffer
		UINT	m_nStride;			// stride of a vertex
};	//	class

class ZFXD3DVCManager : public ZFXVertexCacheManager
{
	public:
		ZFXD3DVCManager(ZFXD3DSkinManager* pSkinMan, LPDIRECT3DDEVICE9 pDevice, ZFXD3D* pZFXD3D, UINT nMaxVerts, UINT nMaxIndis, FILE* pLog);
		~ZFXD3DVCManager(void);

		HRESULT CreateStaticBuffer(ZFXVERTEXID VertexID, UINT nSkinID, UINT nVerts, UINT nIndis, const void* pVerts, const WORD* pIndis, UINT* pnID);
		
		HRESULT Render(ZFXVERTEXID VertexID, UINT nVerts, UINT nIndis, const void* pVerts, const WORD* pIndis, UINT SkinID);

		HRESULT Render(UINT nSBufferID);
		HRESULT RenderPoints(ZFXVERTEXID VID, UINT nVerts, const void* pVerts, const ZFXCOLOR* pClr);
		HRESULT RenderLines(ZFXVERTEXID VID, UINT nVerts, const void* pVerts, const ZFXCOLOR* pClr, bool bStrip);
		HRESULT RenderLine(const float* fStart, const float* fEnd, const ZFXCOLOR* pClr);

		HRESULT ForcedFlushAll(void);
		HRESULT ForcedFlush(ZFXVERTEXID VertexID);

		DWORD	GetActiveCache(void)		{ return m_dwActiveCache; }
		void	SetActiveCache(DWORD dwID)	{ m_dwActiveCache = dwID; }
		ZFXD3D* GetZFXD3D(void) { return m_pZFXD3D; }

		void	InvalidateStates(void);

		ZFXRENDERSTATE GetShadeMode(void);

	private:
		ZFXD3DSkinManager*	m_pSkinMan;
		LPDIRECT3DDEVICE9	m_pDevice;
		ZFXD3D*				m_pZFXD3D;

		ZFXSTATICBUFFER*	m_pSB;
		ZFXINDEXBUFFER*		m_pIB;
		UINT				m_nNumSB;
		UINT				m_nNumIB;
		ZFXD3DVCache*		m_CacheUU[NUM_CACHES];
		ZFXD3DVCache*		m_CacheUL[NUM_CACHES];
		DWORD				m_dwActiveCache;
		DWORD				m_dwActiveSB;
		DWORD				m_DwActiveIB;
		FILE*				m_pLog;
};	//	class

#endif // !ZFXD3DVCACHE_H
