#ifndef ZFXSKINMAN_H
#define ZFXSKINMAN_H
#include <Windows.h>
#include "ZFX.h"

class ZFXSkinManager
{
	protected:
		UINT			m_nNumSkins;		// num, skins
		UINT			m_nNumMaterials;	// num, materials
		UINT			m_nNumTextures;		// num, textures
		ZFXSKIN*		m_pSkins;
		ZFXMATERIAL*	m_pMaterials;
		ZFXTEXTURE*		m_pTextures;
	public:
		ZFXSkinManager(void) {};
		virtual	~ZFXSkinManager() {};

		virtual HRESULT	AddSkin(const ZFXCOLOR* pcAmbient,
			const ZFXCOLOR* pcDiffuse,
			const ZFXCOLOR* pcEmissive,
			const ZFXCOLOR* pcSpecular,
			float fSpecPower,
			UINT* nSkinID) = 0;

		virtual HRESULT AddTexture(UINT nSkinID,
			const char* chName,
			bool bAlpha,
			float fAlpha,
			ZFXCOLOR* cColorKeys,
			DWORD dwNumColorKeys) = 0;

		virtual HRESULT		AddTextureHeightmapAsBump(UINT nSkinID, const char* chName) = 0;
		virtual bool		MaterialEqual(const ZFXMATERIAL* pMat0, const ZFXMATERIAL* pMat1) = 0;
		virtual ZFXSKIN		GetSkin(UINT nSkinID) = 0;
		virtual ZFXMATERIAL	GetMaterial(UINT nMatID) = 0;
		virtual const char* GetTextureName(UINT nTextID, float* pfAlpha, ZFXCOLOR* pAK, UCHAR* pNum) = 0;
		virtual void		LogCurrentStatus(char* chLog, bool bDetail) = 0;
};
#endif
