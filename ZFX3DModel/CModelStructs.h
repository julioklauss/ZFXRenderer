#pragma once

#include <Windows.h>
#include <ZFX3D.h>

// chunk types 
#define	V1_HEADER				0x0100	// Header
#define	V1_VERTEX				0x0200	// Vertices
#define	V1_FACE					0x0300	// Faces
#define	V1_MESH					0x0400	// Meshes
#define V1_MATERIAL				0x0500	// Materials
#define V1_JOINT				0x0600	// Joints
#define	V1_JOINT_MAIN			0x0610	// Joints Main
#define V1_JOINT_KEYFRAME_ROT	0x0620	// Keyframe Rotation
#define V1_JOINT_KEYFRAME_POS	0x0630	// Keyframe Position
#define V1_ANIMATION			0x0700	// Animations
#define V1_END					0x9999	// End Chunk

// chunk
struct CHUNK_S
{
	WORD	wIdentifier;
	ULONG	ulSize;
};
typedef CHUNK_S* LPCHUNK;

struct CHUNKHEAD_S
{
	UCHAR	ucIdentifier[4];		// identifier
	UCHAR	ucName[32];				// name
	UCHAR	ucAuthor[32];			// author
	UCHAR	ucEmail[32];			// E-mail
	UCHAR	ucType;					// type
	UCHAR	ucVersion;				// version
	ULONG	ulNumVertices;			// number vertices
	ULONG	ulNumIndices;			// number indices
	ULONG	ulNumFaces;				// number faces
	ULONG	ulNumMeshes;			// number meshes
	UINT	uiNumMaterials;			// number materials
	UINT	uiNumJoints;			// number joints
	float	fAnimationFPS;			// FPS
	float	fCurrentTime;			// current time
	UINT	uiNumFrames;			// number frames
	UINT	uiNumAnimations;		// number animations
};
typedef CHUNKHEAD_S* LPCHUNKHEAD;

// -- Vertex --
typedef struct _VERTEX
{
	float		fXYZ[3];			// spatial coordinates
	float		fUV0[2];			// texture coords 1
	float		fUV1[2];			// texture coords 2
	ZFXVector	fNormal;			// normal vector
	USHORT		usReferences;		// references
	UINT		uiBoneID_A;			// bone ID 1
	float		fWeight_A;			// weight 1
	UINT		uiBoneID_B;			// bone ID 2
	float		fWeight_B;			// weight 2
	BYTE		byFlags;			// flags
} VERTEX_3F_S;
typedef VERTEX_3F_S* LPVERTEX_3F;

// -- Face --
typedef struct _FACE
{
	ULONG		ulIndices[32];		// name
	ZFXVector	fNormal;			// normal vector
	ULONG		ulMeshID;			// mesh ID
	UINT		uiMaterialID;		// material ID
	BYTE		byFlags;			// flags
} FACE_S;
typedef FACE_S* LPFACE;

// -- Mesh --
typedef struct _MESH
{
	char		cName[32];			// name
	WORD		wNumFaces;			// number of faces
	PWORD		pIndices;			// face indices
	UINT		uiMaterialID;		// material ID
	BYTE		byFlags;			// flags
} MESH_S;
typedef MESH_S* LPMESH;

// -- Material --
typedef struct _MATERIAL
{
	char		cName[32];			// name
	float		fAmbient[4];		// ambient color
	float		fDiffuse[4];		// diffuse color
	float		fSpecular[4];		// specular color
	float		fEmissive[4];		// emissive color
	float		fSpecularPower;		// specular power
	float		fTransparency;		// transparency
	char		cTexture_1[128];	// texture name
	char		cTexture_2[128];	// texture name
	BYTE		byFlags;			// flags
} MATERIAL_S; 
typedef MATERIAL_S* LPMATERIAL;

// -- Animations --
typedef struct _ANIMATION 
{
	TCHAR		cName[64];			// name
	float		fStartFrame;		// start frame
	float		fEndFrame;			// end frame
	bool		bActive;			// is this animation active?
} ANIMATION_S;
typedef ANIMATION_S* LPANIMATION;

// -- Keyframe-Rotation --
typedef struct _KF_ROT
{
	float		fTime;				// time
	ZFXVector	vRotation;			// rotation
} KF_ROT_S;
typedef KF_ROT_S* LPKF_ROT;

// -- Keyframe-Position --
typedef struct _KF_POS
{
	float		fTime;				// time
	ZFXVector	vPosition;			// position
} KF_POS_S;
typedef KF_POS_S* LPKF_POS;

// -- Joints --
typedef struct _JOINT
{
	char		cName[32];			// name
	char		cParentName[32];	// parent joint's name
	WORD		wParentID;			// parent joint's ID
	ZFXVector	vRotation;			// rotation
	ZFXVector	vPosition;			// position
	WORD		wNumKF_Rotation;	// number of rotation keyframes
	WORD		wNumKF_Position;	// number of position keyframes
	LPKF_ROT	pKF_Rotation;		// rotation keyframes
	LPKF_POS	pKF_Position;		// position keyframes
	bool		bAnimated;			// is this joint animated
	BYTE		byFlags;			// flags
	ZFXMatrix	sMatrix;			// vertex transform matrix - tranpose of absolute matrix
	ZFXMatrix	sMatrix_absolute;	// matrix relative to world coordinate system - relative multiplied by absolute of parent
	ZFXMatrix	sMatrix_relative;	// matrix relative to parent of this joint
} JOINT_S;
typedef JOINT_S* LPJOINT;
