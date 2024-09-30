#ifndef ZFXD3DSKINMAN_H
#define ZFXD3DSKINMAN_H

#include "ZFXD3D.h"
#include <ZFX_skinman.h>

#define MAX_ID 65535

class ZFXD3DSkinManager : public ZFXSkinManager
{
	friend class ZFXD3DVCache;

	public:
		ZFXD3DSkinManager(LPDIRECT3DDEVICE9 pDevice, FILE* pLog);
		~ZFXD3DSkinManager(void);

		HRESULT	AddSkin(const ZFXCOLOR* pcAmbient,
			const ZFXCOLOR* pcDiffuse,
			const ZFXCOLOR* pcEmissive,
			const ZFXCOLOR* pcSpecular,
			float fSpecPower, UINT* nSkinID);

		HRESULT AddTexture(UINT nSkinID, const char* chName,
			bool bAlpha, float fAlpha,
			ZFXCOLOR* cColorKeys,
			DWORD dwNumColorKeys);

		HRESULT AddTextureHeightmapAsBump(UINT nSkinID, const char* chName);
		HRESULT	ConvertToNormalMap(ZFXTEXTURE* pTexture);
		DWORD	VectorToRGBA(ZFXVector* vc, float fHeight);

		bool	MaterialEqual(const ZFXMATERIAL* pMat0, const ZFXMATERIAL* pMat1);

		ZFXSKIN		GetSkin(UINT nSkinID);
		ZFXMATERIAL GetMaterial(UINT nMatID);
		ZFXTEXTURE	GetTexture(UINT nTexID);
		const char* GetTextureName(UINT nTextID, float* pfAlpha, ZFXCOLOR* pAK, UCHAR* pNum);
		void		LogCurrentStatusChar(char* chLog, bool bDetailed) {};
	protected:
		LPDIRECT3DDEVICE9	m_pDevice;
		FILE*				m_pLog;

		inline bool ColorEqual(const ZFXCOLOR* pCol0, const ZFXCOLOR* pCol1);
		HRESULT	CreateTexture(ZFXTEXTURE* pTexture, bool bAlpha);
		HRESULT SetAlphaKey(LPDIRECT3DTEXTURE9* ppTexture, UCHAR R, UCHAR G, UCHAR B, UCHAR A);
		HRESULT	SetTransparency(LPDIRECT3DTEXTURE9* ppTexture, UCHAR Alpha);
		DWORD	MakeD3DColor(UCHAR R, UCHAR G, UCHAR B, UCHAR A);

		void	Log(const char*, ...);
};

#endif
