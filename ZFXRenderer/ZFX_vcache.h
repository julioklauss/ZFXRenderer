#ifndef ZFXVCACHE_H
#define ZFXVACHE_H
#include "ZFX.h"

class ZFXVertexCacheManager
{
	public:
		ZFXVertexCacheManager(void) {};
		virtual ~ZFXVertexCacheManager(void) {};

		virtual HRESULT	CreateStaticBuffer(ZFXVERTEXID VertexID, UINT nSkinID, UINT nVerts, UINT nIndis, const void* pVerts, const WORD* pIndis, UINT* pnID) = 0;

		virtual HRESULT Render(ZFXVERTEXID VertexID, UINT nVerts, UINT nIndis, const void* pVerts, const WORD* pIndis, UINT SkinID) = 0;

		virtual HRESULT	Render(UINT nID) = 0;

		virtual HRESULT ForcedFlushAll(void) = 0;

		virtual HRESULT ForcedFlush(ZFXVERTEXID) = 0;

		virtual HRESULT RenderPoints(ZFXVERTEXID VertexID, UINT nVerts, const void* pVerts, const ZFXCOLOR* pClr) = 0;

		virtual HRESULT RenderLines(ZFXVERTEXID VertexID, UINT nVerts, const void* pVerts, const ZFXCOLOR* pClr, bool bStrip) = 0;

		virtual void	InvalidateStates(void) = 0;

		virtual ZFXRENDERSTATE GetShadeMode(void) = 0;
};
#endif // !ZFXVCACHE_H

