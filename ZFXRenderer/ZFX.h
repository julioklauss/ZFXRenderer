#pragma once
#ifndef ZFX_H
#define ZFX_H

#include <windows.h>
extern bool g_blF;

// no error
#define ZFX_OK				S_OK

// reports no error
#define ZFX_CANCELED		0x02000001

// generall error message
#define ZFX_FAIL			0x82000001

// specific error message
#define ZFX_CREATEAPI		0x82000002
#define ZFX_CREATEDEVICE	0x82000003
#define ZFX_CREATEBUFFER	0x82000004
#define ZFX_INVALIDPARAM	0x82000005
#define ZFX_INVALIDID		0x82000006
#define ZFX_BUFFERSIZE		0x82000007
#define ZFX_BUFFERLOCK		0x82000008
#define ZFX_NOTCOMPATIBLE	0x82000009
#define ZFX_OUTOFMEMORY		0x8200000a
#define ZFX_FILENOTFOUND	0x8200000b
#define	ZFX_INVALIDFILE		0x8200000c
#define ZFX_NOSHADERSUPPOT	0x8200000d
#define ZFX_FAILa			0x8200000c
#define ZFX_FAILb			0x8200000d
#define ZFX_FAILc			0x8200000e
#define ZFX_FAILd			0x8200000f

typedef struct ZFXCOLOR_TYPE
{
	union {
		struct {
			float fR;
			float fG;
			float fB;
			float fA;
		};
		float c[4];
	};
} ZFXCOLOR;

typedef struct ZFXMATERIAL_TYPE
{
	ZFXCOLOR	cDiffuse;	// RGBA diffuse light
	ZFXCOLOR	cAmbient;	// RGBA ambient light
	ZFXCOLOR	cSpecular;	// RGBA specular light
	ZFXCOLOR	cEmissive;	// RGBA emissive light
	float		fPower;		// Specular power
} ZFXMATERIAL;

typedef struct ZFXTEXTURE_TYPE
{
	float		fAlpha;		// overall transparency value
	char*		chName;		// texture filename
	void*		pData;		// texture data
	ZFXCOLOR*	pClrKeys;	// color key array
	DWORD		dwNum;		
} ZFXTEXTURE;


typedef struct ZFXSKIN_TYPE
{
	bool	bAlpha;
	UINT	nMaterial;
	UINT	nTexture[8];
} ZFXSKIN;

// simple viewport type
typedef struct ZFXVIEWPORT_TYPE
{
	DWORD	X;		// position of upper ...
	DWORD	Y;		// ... left corner
	DWORD	Width;
	DWORD	Height;
} ZFXVIEWPORT;

typedef enum ZFXENGINEMODE_TYPE
{
	EMD_PERSPECTIVE,	// perspective projection
	EMD_TWOD,			// world equals screen coordinates
	EMD_ORTHOGONAL		// orthogonal projection
} ZFXENGINEMODE;

typedef enum ZFXVERTEXID
{
	VID_PS,				// unstransformed position only
	VID_UU,				// untransformed and unlit
	VID_UL,				// untransformed and lit
	VID_CA,				// used for character animation
	VID_3T,				// three texture coord pairs
	VID_TV,				// like UU but with tangent vector
} ;

typedef struct PVERTEX_TYPE
{
	float	x, y, z;
} PVERTEX;

typedef struct VERTEX_TYPE 
{
	float	x, y, z;
	float	vcN[3];
	float	tu, tv;
} VERTEX;

typedef struct LVERTEX_TYPE 
{
	float	x, y, z;
	DWORD	Color;
	float	tu, tv;
} LVERTEX;

typedef struct CVERTEX_TYPE
{
	float	x, y, z;
	float	vcN[3];
	float	tu, tv;
	float	fBone1, fWeight1;
	float	tBone2, fWeight2;
} CVERTEX;

typedef struct VERTEX3T_TYPE
{
	float	x, y, z;
	float	vcN[3];
	float	tu0, tv0;
	float	tu1, tv1;
	float	tu2, tv2;
} VERTEX3T;

typedef struct TVERTEX_TYPE 
{
	float	x, y, z;
	float	vcN[3];
	float	tu, tv;
	float	vcU[3];
} TVERTEX;

typedef enum ZFXRENDERSTATE_TYPE
{
	RS_CULL_CW,			// culling clockwise
	RS_CULL_CCW,		// culling counterclockwise
	RS_CULL_NONE,		// render frontface and backface
	RS_DEPTH_READWRITE,	// read and write depth buffer
	RS_DEPTH_READONLY,	// no writes to depth buffer
	RS_DEPTH_NONE,		// no reads or writes with depth buffer
	RS_SHADE_POINTS,	// render vertices as points
	RS_SHADE_TRIWIRE,	// render wireframe triangles
	RS_SHADE_HULLWIRE,	// render wireframe polygons
	RS_SHADE_SOLID		// solid triangles
} ZFXRENDERSTATE;
#endif