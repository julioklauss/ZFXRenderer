#include "CZFXModel.h"

void CZFXModel::Init()
{
	// Settings
	m_pFile					= NULL;				// File
	m_pVertices				= NULL;				// Vertices
	m_pVertices_Orig		= NULL;				// Vertices
	m_pFaces				= NULL;				// Faces
	m_pMeshes				= NULL;				// Meshes
	m_pMaterials			= NULL;				// Materials
	m_ppIndices				= NULL;				// Indices
	m_pIndices				= NULL;				// Indices
	m_pAnimations			= NULL;				// Animations
	m_pRenderDevice			= NULL;				// Renderdevice
	strcpy_s(m_cLogFileName, "MODELLOG.TXT");	// Log filename
	m_bLog					= true;				// Logging on
	m_uiLogLevel			= 10;				// Log level
	m_fTime					= 0.0;				// Time
	m_fStartTime			= -1.0f;			// Time
	m_uiCurrentAnimation	= 8;				// Animation
	m_bAnimationComplete	= false;			// Animation completed
	m_bAnimationRunOnce		= true;				// Single animation
	m_bRenderBones			= false;			// Render bones
	m_bRenderNormals		= true;				// Normal vectoren
}

// --------------------------
// Name: Prepare( void )
// 
// Return		= (HRESULT)		Status
// --------------------------
HRESULT CZFXModel::Prepare(void)
{
	// Variablen init
	ULONG		ulNumIndices	= 0;
	ULONG		ulNumVertices	= 0;
	UINT		uiCurrentMat	= 0;
	PWORD		pIndex			= NULL;			// Index
	ULONG		ulCounter		= 0;
	LPMATERIAL	pMaterial		= NULL;
	char		cTexture[256]	= { 0 };
	PCHAR		pcSeperator		= NULL;
	ULONG		ulIndexCount	= 0;

	// 1. setup of the bones
	SetupBones();

	LOG(20, false, "Sort Indices by Material [%d]", m_sHeader.uiNumMaterials);

	// calculate maximum memory needed
	m_sHeader.ulNumIndices	= m_sHeader.ulNumFaces * 3;
	pIndex					= new WORD[m_sHeader.ulNumIndices];

	m_ppIndices				= new PVOID[m_sHeader.uiNumMaterials];
	ZeroMemory(m_ppIndices, sizeof(PVOID) * m_sHeader.uiNumMaterials);

	m_pIndices				= new WORD[m_sHeader.ulNumIndices];
	ZeroMemory(m_pIndices, sizeof(WORD) * m_sHeader.ulNumIndices);

	m_puiNumIndices			= new UINT[m_sHeader.uiNumMaterials];
	ZeroMemory(m_puiNumIndices, sizeof(UINT) * m_sHeader.uiNumMaterials);

	m_puiSkinBuffer			= new UINT[m_sHeader.uiNumMaterials];
	ZeroMemory(m_puiSkinBuffer, sizeof(UINT) * m_sHeader.uiNumMaterials);

	// sort all faces into the index array
	do {
		
		ZeroMemory(pIndex, sizeof(WORD) * m_sHeader.ulNumIndices);

		// reset counter
		ulNumIndices = 0;

		// loop through all faces
		for (ulCounter = 0; ulCounter < m_sHeader.ulNumFaces; ++ulCounter)
		{
			// still the same material
			if (m_pFaces[ulCounter].uiMaterialID == uiCurrentMat) {
				m_pIndices[ulIndexCount++] = pIndex[ulNumIndices++] = (WORD)m_pFaces[ulCounter].ulIndices[0];

				m_pIndices[ulIndexCount++] = pIndex[ulNumIndices++] = (WORD)m_pFaces[ulCounter].ulIndices[1];

				m_pIndices[ulIndexCount++] = pIndex[ulNumIndices++] = (WORD)m_pFaces[ulCounter].ulIndices[2];
			}
		}
		// enough indices?
		if (!ulNumIndices) {
			// new material
			++uiCurrentMat;

			LOG(1, true, "STOP Error: Not Enough Indices...");

			continue;
		}
		m_puiNumIndices[uiCurrentMat] = ulNumIndices;
		m_ppIndices[uiCurrentMat] = new WORD[ulNumIndices];
		memcpy(m_ppIndices[uiCurrentMat], pIndex, sizeof(WORD) * ulNumIndices);

		// set current material
		pMaterial = &m_pMaterials[uiCurrentMat];

		// read material
		if (FAILED(m_pRenderDevice->GetSkinManager()->AddSkin((ZFXCOLOR*)&pMaterial->fAmbient, 
																(ZFXCOLOR*)&pMaterial->fDiffuse,		
																(ZFXCOLOR*)&pMaterial->fEmissive, 
																(ZFXCOLOR*)&pMaterial->fSpecular, 
																pMaterial->fSpecularPower, 
																&m_puiSkinBuffer[uiCurrentMat])))
			LOG(1, true, "FAILED [LOAD SKIN %d]", uiCurrentMat);

		// prepare textures
		ZeroMemory(cTexture, sizeof(char) * 256);
		pcSeperator = strchr(strrev(strdup(m_pcFileName)), '/');

		if (!pcSeperator)
			pcSeperator = strchr(strrev(strdup(m_pcFileName)), 92);

		if (pcSeperator)
			strcpy(cTexture, strrev(pcSeperator));

		strcat(cTexture, pMaterial->cTexture_1);

		// load textures
		if (FAILED(m_pRenderDevice->GetSkinManager()->AddTexture(m_puiSkinBuffer[uiCurrentMat], cTexture, false, 0, NULL, 0)))
			LOG(1, true, "FAILED [LOAD TEXTURE %s]", pMaterial->cTexture_1);

		// new material
		++uiCurrentMat;
	} while (uiCurrentMat != m_sHeader.uiNumMaterials);

	// clean up
	delete[] pIndex;

	LOG(20, true, "done");

	return S_OK;
}
// --------------------------

// --------------------------
// Name: GetNextChunk( CHUNK_S &pChunk )
// Info: Reads the next ChunkID
// Return		= (WORD)		next chunk id
// pChunk		= (CHUNK_S&)	Pointer to Chunk
// --------------------------
WORD CZFXModel::GetNextChunk(CHUNK_S& pChunk)
{
	// read the next chunk
	fread(&pChunk, sizeof(CHUNK_S), 1, m_pFile);

	// return chunk id 
	return pChunk.wIdentifier;
}
// --------------------------

// --------------------------
// Name: CheckForChunks( void )
// Info: Checks for Chunks
// Return		= (HRESULT)		Status
// --------------------------
HRESULT CZFXModel::CheckForChunks(void)
{
	bool  bLoop = true;

	// loop until end chunk is found
	do {
		// seek the next Chunk
		switch (GetNextChunk(m_sChunk))
		{
			case V1_HEADER:		ReadHeader();		break;
			case V1_VERTEX:		ReadVertices();		break;
			case V1_FACE:		ReadFaces();		break;
			case V1_MESH:		ReadMeshes();		break;
			case V1_MATERIAL:	ReadMaterials();	break;
			case V1_JOINT:		ReadJoints();		break;
			case V1_ANIMATION:	ReadAnimations();	break;
			case V1_END:		bLoop = false;		break;
			default:	break;
		}
	} while (bLoop);

	// are there animations at all?
	if (m_sHeader.uiNumJoints == 0) {
		// if not we don't need this member
		delete[] m_pVertices_Orig;
		m_pVertices_Orig = NULL;
	}

	// return OK
	return S_OK;
}
// --------------------------

// --------------------------
// Name: ReadHeader( void )
// Info: Reads header from open file
//
// Return					= (HRESULT)		Status
// --------------------------
HRESULT	CZFXModel::ReadHeader(void)
{
	// write to log file
	LOG(20, false, "Reading Header...");

	// clear memory area
	ZeroMemory(&m_sHeader, sizeof(CHUNKHEAD_S));

	// read the header
	fread(&m_sHeader, sizeof(CHUNKHEAD_S), 1, m_pFile);

	// seek the end chunk
	if (GetNextChunk(m_sChunk) == V1_END) {
		LOG(20, true, "OK");		// log it
		return S_OK;				// bye
	}

	// no end chunk found
	LOG(1, true, "FAILED [Header]");
	
	// return
	return E_FAIL;
}
// --------------------------

// --------------------------
// Name: ReadVertices( void )
// Info: reads vertices from the open file
//
// Return		= (HRESULT)		Status
// --------------------------
HRESULT CZFXModel::ReadVertices(void)
{
	// initialize the variables
	ULONG		ulNumVertices	= m_sHeader.ulNumVertices;
	LPVERTEX_3F pVertices		= NULL;
	
	LOG(20, false, "Read Vertices [%d]", ulNumVertices);

	// allocate memory
	pVertices = new VERTEX_3F_S[ulNumVertices];
	if (!pVertices) {
		LOG(1, true, "FAILED [VERTICES]");		// log it
		return E_FAIL;							// bye
	}

	// read all vertices
	fread(pVertices, sizeof(VERTEX_3F_S), ulNumVertices, m_pFile);

	// allocate memory
	m_pVertices			= new CVERTEX[ulNumVertices];
	m_pVertices_Orig	= new CVERTEX[ulNumVertices];
	ZeroMemory(m_pVertices, sizeof(CVERTEX) * ulNumVertices);
	ZeroMemory(m_pVertices_Orig, sizeof(CVERTEX) * ulNumVertices);

	// convert the vertices
	for (ULONG ulCounter = 0; ulCounter < m_sHeader.ulNumVertices; ++ulCounter)
	{
		// copy the vertices
		memcpy(&m_pVertices[ulCounter].x, &pVertices[ulCounter].fXYZ, sizeof(float) * 3);
		memcpy(&m_pVertices[ulCounter].vcN, &pVertices[ulCounter].fNormal, sizeof(float) * 3);
		memcpy(&m_pVertices[ulCounter].tu, &pVertices[ulCounter].fUV0, sizeof(float) * 2);
		m_pVertices[ulCounter].fBone1	= (float)pVertices[ulCounter].uiBoneID_A;
		m_pVertices[ulCounter].fWeight1 = (float)pVertices[ulCounter].fWeight_A;
		m_pVertices[ulCounter].fBone2	= (float)pVertices[ulCounter].uiBoneID_B;
		m_pVertices[ulCounter].fWeight2 = (float)pVertices[ulCounter].fWeight_B;
	}
	// free memory
	delete[] pVertices;

	// search the end chunk
	if (GetNextChunk(m_sChunk) == V1_END) {
		LOG(20, true, "OK");		// log it
		return S_OK;				// bye
	}

	LOG(1, true, "FAILED [VERTICES]");
	return E_FAIL;
}
// --------------------------

// --------------------------
// Name: ReadFaces( void )
//
// Return		= (HRESULT)		Status
// --------------------------
HRESULT CZFXModel::ReadFaces(void)
{
	ULONG	ulNumFaces = m_sHeader.ulNumFaces;	// temp Var
	
	LOG(20, false, "Reading Faces [%d] ...", ulNumFaces);

	// allocate memory
	m_pFaces = new FACE_S[ulNumFaces];
	if (!m_pFaces) {
		LOG(1, true, "FAILED [FACES]");
		return E_FAIL;
	}

	// read all faces
	fread(m_pFaces, sizeof(FACE_S), ulNumFaces, m_pFile);

	// read end chunk
	if (GetNextChunk(m_sChunk) == V1_END) {
		LOG(20, true, "OK");
		return S_OK;
	}

	LOG(1, true, "FAILED [FACES]");

	return E_FAIL;
}
// --------------------------

// --------------------------
// Name: ReadMesh( void )
//
// Return		= (HRESULT)		Status
// --------------------------
HRESULT CZFXModel::ReadMeshes(void)
{
	ULONG	ulNumMesh = m_sHeader.ulNumMeshes;

	LOG(20, false, "Reading Meshes[%d] ...", ulNumMesh);

	// allocate memory
	m_pMeshes = new MESH_S[ulNumMesh];
	if (!m_pMeshes) {
		LOG(1, TRUE, "FAILED [MESH]");
		return E_FAIL;
	}

	// clear memory
	ZeroMemory(m_pMeshes, sizeof(MESH_S) * ulNumMesh);

	// read the whole mesh
	fread(m_pMeshes, sizeof(MESH_S), ulNumMesh, m_pFile);

	// check for end chunk
	if (GetNextChunk(m_sChunk) == V1_END) {
		LOG(20, true, "OK");		// log it
		return S_OK;				// bye
	}

	LOG(1, true, "FAILED [MESH]");

	return E_FAIL;
}
// --------------------------

// --------------------------
// Name: ReadMaterials( void )
//
// Return		= (HRESULT)		Status
// --------------------------
HRESULT CZFXModel::ReadMaterials(void)
{
	UINT	uiNumMat = m_sHeader.uiNumMaterials;

	LOG(20, false, "Reading Materials [%d] ...", uiNumMat);

	// allocate memory
	m_pMaterials = new MATERIAL_S[uiNumMat];
	if (!m_pMaterials) {
		LOG(1, true, "FAILED [MATERIALS]");
		return E_FAIL;
	}

	// read the materials
	fread(m_pMaterials, sizeof(MATERIAL_S), uiNumMat, m_pFile);

	// check for end chunk
	if (GetNextChunk(m_sChunk) == V1_END) {
		LOG(20, true, "OK");
		return S_OK;
	}

	LOG(1, true, "FAILED [MATERIALS]");
	return E_FAIL;
}
// --------------------------

// --------------------------
// Name: ReadJoints( void )
//
// Return		= (HRESULT)		Status
// --------------------------
HRESULT CZFXModel::ReadJoints(void)
{
	bool	bLoop = true;
	UINT	uiLoop = 0;
	UINT	uiNumJoints = m_sHeader.uiNumJoints;
	LPJOINT	pJoint = NULL;

	LOG(20, false, "Reading Joints [%d] ...", uiNumJoints);

	// allocate memory
	m_pJoints = new JOINT_S[uiNumJoints];
	if (!m_pJoints) {
		LOG(1, true, "FAILED [JOINTS]");
		return E_FAIL;						// bye
	}

	// loop until end chunk found
	do {
		// find the next chunk
		switch (GetNextChunk(m_sChunk))
		{
			case V1_JOINT_MAIN:
				pJoint = &m_pJoints[uiLoop];
				ReadJoint_Main(pJoint);
				++uiLoop;
				break;

			case V1_JOINT_KEYFRAME_ROT:
				ReadJoint_KeyFrame_Rot(pJoint);	break;

			case V1_JOINT_KEYFRAME_POS:
				ReadJoint_KeyFrame_Pos(pJoint);	break;

			case V1_END:
				bLoop = false;					break;
		}
	} while (bLoop);

	// check for end chunk
	if (!bLoop) {
		LOG(20, true, "OK");
		return S_OK;
	}

	LOG(1, true, "FAILED [JOINTS]");
	return E_FAIL;
}
// --------------------------

// --------------------------
// Name: ReadJoint_Main( LPJOINT pJoint )
//
// Return		= (HRESULT)		Status
// 
// pJoint		= (LPJOINT)		Parent-Joint
// --------------------------
HRESULT CZFXModel::ReadJoint_Main(LPJOINT pJoint)
{
	// log start
	LOG(20, false, "Reading Joint ");

	// read joints
	fread(pJoint, sizeof(JOINT_S), 1, m_pFile);

	// check for the end chunk
	if (GetNextChunk(m_sChunk) == V1_END) {
		LOG(20, true, "OK");
		return S_OK;
	}

	LOG(1, true, "FAILED [JOINT_MAIN]");
	return E_FAIL;
}
// --------------------------

// --------------------------
// Name: ReadJoint_KeyFrame_Rot( LPJOINT pJoint )
//
// Return		= (HRESULT)		Status
// 
// pJoint		= (LPJOINT)		Parent-Joint
// --------------------------
HRESULT CZFXModel::ReadJoint_KeyFrame_Rot(LPJOINT pJoint)
{
	UINT	uiNumKeys = pJoint->wNumKF_Rotation;

	LOG(20, false, "Reading KF Rot, [%d] ...", uiNumKeys);

	// allocate memory
	pJoint->pKF_Rotation = new KF_ROT_S[uiNumKeys];
	if (!pJoint->pKF_Rotation) {
		LOG(1, true, "FAILED [JOINT_KEYFRAME_ROTATIONS]");
		return E_FAIL;
	}

	// clear memory
	ZeroMemory(pJoint->pKF_Rotation, sizeof(KF_ROT_S) * uiNumKeys);

	// read the rotations
	fread(pJoint->pKF_Rotation, sizeof(KF_ROT_S), uiNumKeys, m_pFile);

	// check for the end chunk
	if (GetNextChunk(m_sChunk) == V1_END) {
		LOG(20, true, "OK");
		return S_OK;
	}

	LOG(1, true, "FAILED [JOINT_KEYFRAME_ROTATIONS]");
	return E_FAIL;
}
// --------------------------

// --------------------------
// Name: ReadJoint_KeyFrame_Pos( LPJOINT pJoint )
//
// Return		= (HRESULT)		Status
// 
// pJoint		= (LPJOINT)		Parent-Joint
// --------------------------
HRESULT CZFXModel::ReadJoint_KeyFrame_Pos(LPJOINT pJoint)
{
	UINT	uiNumKeys = pJoint->wNumKF_Position;

	LOG(20, false, "Reading KeyFrame Positions [%d] ...", uiNumKeys);

	// allocate memory
	pJoint->pKF_Position = new KF_POS_S[uiNumKeys];
	if (!pJoint->pKF_Position) {
		LOG(1, true, "FAILED [JOINT_KEYFRAME_POSITIONS]");
		return E_FAIL;
	}

	// clear memory
	ZeroMemory(pJoint->pKF_Position, sizeof(KF_POS_S) * uiNumKeys);

	// read the positions
	fread(pJoint->pKF_Position, sizeof(KF_POS_S), uiNumKeys, m_pFile);

	// check for the end chunk
	if (GetNextChunk(m_sChunk) == V1_END) {
		LOG(20, true, "OK");
		return S_OK;
	}

	LOG(1, true, "FAILED [JOINT_KEYFRAME_POSITIONS]");
	return E_FAIL;
}
// --------------------------

// --------------------------
// Name: ReadAnimations( void )
//
// Return		= (HRESULT)		Status
// --------------------------
HRESULT CZFXModel::ReadAnimations(void)
{
	UINT	uiNumAnim = m_sHeader.uiNumAnimations;	// tmp

	LOG(20, false, "Reading Animations [%d] ...", uiNumAnim);

	// allocate memory
	m_pAnimations = new ANIMATION_S[uiNumAnim];
	if (!m_pAnimations) {
		LOG(1, true, "FAILED [ANIMATIONS]");
		return E_FAIL;
	}

	// clear memory
	ZeroMemory(m_pAnimations, sizeof(ANIMATION_S) * uiNumAnim);

	// read the animations
	fread(m_pAnimations, sizeof(ANIMATION_S), uiNumAnim, m_pFile);

	// check for the end chunk
	if (GetNextChunk(m_sChunk) == V1_END) {
		LOG(20, true, "OK");
		return S_OK;
	}

	LOG(1, true, "FAILED [ANIMATIONS]");
	return E_FAIL;
}
// --------------------------

// --------------------------
// Name: SetScaling( float fScale /* = 0.0f */ )
//
// --------------------------
void CZFXModel::SetScaling(float fScale /* = 0.0f */)
{
	ULONG		ulCounter	= 0;		// Counter
	ULONG		ulInner		= 0;		// Counter
	CVERTEX*	pVertex		= NULL;		// temporarily
	float		fScaling	= 0.0f;		// scaling
	LPJOINT		pJoint		= NULL;		// Joint

	// do we need to scale?
	if (fScale == 0.0f) return;

	// calculate bounding box
	m_sBBoxMin.x = 999999.0f; m_sBBoxMax.x = -999999.0f;
	m_sBBoxMin.y = 999999.0f; m_sBBoxMax.y = -999999.0f;
	m_sBBoxMin.z = 999999.0f; m_sBBoxMax.z = -999999.0f;
	
	// calculate the bounding box
	for (ulCounter = 0; ulCounter < m_sHeader.ulNumVertices; ++ulCounter)
	{
		pVertex = &m_pVertices[ulCounter];

		// enlarge box if needed
		m_sBBoxMax.x = max(m_sBBoxMax.x, pVertex->x);
		m_sBBoxMax.y = max(m_sBBoxMax.y, pVertex->y);
		m_sBBoxMax.z = max(m_sBBoxMax.z, pVertex->z);
		m_sBBoxMin.x = min(m_sBBoxMin.x, pVertex->x);
		m_sBBoxMin.y = min(m_sBBoxMin.y, pVertex->y);
		m_sBBoxMin.z = min(m_sBBoxMin.z, pVertex->z);
	}

	// scale bounding box
	fScaling = (m_sBBoxMax.y - m_sBBoxMin.y) / fScale;

	// scale the vertex data
	for (ulCounter = 0; ulCounter < m_sHeader.ulNumVertices; ++ulCounter)
	{
		pVertex		= &m_pVertices[ulCounter];

		pVertex->x /= fScaling;
		pVertex->y /= fScaling;
		pVertex->z /= fScaling;
	}

	// copy to back up array if animation is present
	if (m_sHeader.uiNumJoints > 0) 
		memcpy(m_pVertices_Orig, m_pVertices, sizeof(CVERTEX) * m_sHeader.ulNumVertices);

	// scale the bones
	for (ulCounter = 0; ulCounter < m_sHeader.uiNumJoints; ++ulCounter)
	{
		pJoint		= &m_pJoints[ulCounter];

		pJoint->vPosition.x /= fScaling;
		pJoint->vPosition.y /= fScaling;
		pJoint->vPosition.z /= fScaling;

		// scale key frame positions for this bone
		for (ulInner = 0; ulInner < pJoint->wNumKF_Position; ++ulInner)
		{
			pJoint->pKF_Position[ulInner].vPosition.x /= fScaling;

			pJoint->pKF_Position[ulInner].vPosition.y /= fScaling;

			pJoint->pKF_Position[ulInner].vPosition.z /= fScaling;
		}

		// build ZFXEngine aabb
		m_sAabb.vcMin.x		= m_sBBoxMin.x;
		m_sAabb.vcMin.y		= m_sBBoxMin.y;
		m_sAabb.vcMin.z		= m_sBBoxMin.z;
		m_sAabb.vcMax.x		= m_sBBoxMax.x;
		m_sAabb.vcMax.y		= m_sBBoxMax.y;
		m_sAabb.vcMax.z		= m_sBBoxMax.z;
		m_sAabb.vcCenter.x	= (m_sBBoxMax.x - m_sBBoxMin.x) / 2;
		m_sAabb.vcCenter.y	= (m_sBBoxMax.y - m_sBBoxMin.y) / 2;
		m_sAabb.vcCenter.z	= (m_sBBoxMax.z - m_sBBoxMin.z) / 2;
	}
}
// --------------------------

// --------------------------
// Name: SetupBones( void )
//
// Return		= (HRESULT)		Status
// --------------------------
HRESULT CZFXModel::SetupBones(void)
{
	// Variablen
	LPJOINT		pJoint		= NULL;				// joint
	ULONG		ulCounter	= 0;				// counter
	UINT		uiLoop		= 0;				// counter
	UINT		uiParentID	= 0;				// parent ID
	ZFXVector	sVector_A;						// vector
	ZFXVector	sVector_B;						// vector
	CVERTEX*	pVertex		= NULL;				// vector
	ZFXMatrix	matTemp;	matTemp.Identity();	// temporary

	// are there bones at all?
	if (m_sHeader.uiNumJoints == 0) return S_OK;

	// build the matrix
	for (ulCounter = 0; ulCounter < m_sHeader.uiNumJoints; ++ulCounter)
	{
		// get the joint
		pJoint = &m_pJoints[ulCounter];

		// set rotation to matrix
		pJoint->sMatrix_relative = CreateRotationMatrix(&pJoint->vRotation);

		// set position to matrix
		pJoint->sMatrix_relative._14 = pJoint->vPosition.x;

		pJoint->sMatrix_relative._24 = pJoint->vPosition.y;

		pJoint->sMatrix_relative._34 = pJoint->vPosition.z;

		// find the parent...
		for (uiLoop = 0; uiLoop < m_sHeader.uiNumJoints; ++uiLoop)
		{
			// remember parent id
			uiParentID = 255;

			if (strcmp(m_pJoints[uiLoop].cName, pJoint->cParentName) == 0) {
				// found
				uiParentID = uiLoop;
				break;
			}
		}

		// remember found id
		pJoint->wParentID = uiParentID;

		// is there a parent
		if (uiParentID != 255) {
			// parent found so we need to multiply its absolute matrix with
			// the relative matrix of this joint to get its absolute matrix
			pJoint->sMatrix_absolute = m_pJoints[uiParentID].sMatrix_absolute * pJoint->sMatrix_relative;
		}
		else {
			// no parent found so relative matrix equals absolute matrix
			pJoint->sMatrix_absolute = pJoint->sMatrix_relative;
		}

		// final matrix
		pJoint->sMatrix.TransposeOf(pJoint->sMatrix_absolute);

		// transposed matrix
		matTemp = pJoint->sMatrix_relative;
		pJoint->sMatrix_relative.TransposeOf(matTemp);
	}

	// vertices Setup
	for (ulCounter = 0; ulCounter < m_sHeader.ulNumVertices; ++ulCounter)
	{
		// get the current vertex
		pVertex = &m_pVertices_Orig[ulCounter];

		// continue only if there is a bone
		if (pVertex->fBone1 != 255.0f) {
			// get current matrix
			matTemp.Identity();
			matTemp = m_pJoints[(UINT)pVertex->fBone1].sMatrix;

			// 1. rotate matrix
			sVector_A.x = pVertex->x;
			sVector_A.y = pVertex->y;
			sVector_A.z = pVertex->z;
			sVector_A -= matTemp.GetTranslation();
			sVector_A.InvRotateWith(matTemp);
			pVertex->x = sVector_A.x;
			pVertex->y = sVector_A.y;
			pVertex->z = sVector_A.z;

			// 2. rotate normals
			sVector_A.x = pVertex->vcN[0];
			sVector_A.y = pVertex->vcN[1];
			sVector_A.z = pVertex->vcN[2];
			sVector_A.InvRotateWith(matTemp);
			pVertex->vcN[0] = sVector_A.x;
			pVertex->vcN[1] = sVector_A.y;
			pVertex->vcN[2] = sVector_A.z;
		}
	}
	return S_OK;
}
// --------------------------

// --------------------------
// Name: Animation( void )
//
// Return		= (HRESULT)		Status
// --------------------------
HRESULT	CZFXModel::Animation(void)
{
	float		fElapsed	= -1.0f;		// time
	float		fStart		= -1.0f;		// start
	float		fEnd		= -1.0f;		// end
	LPANIMATION	pAnimation	= NULL;			// animation

	// is there an animation at all?
	if (m_sHeader.uiNumJoints == 0)
		return S_OK;

	// run only once?
	if (m_bAnimationRunOnce && m_bAnimationComplete && !m_bAnimationChanged)
		return S_OK;

	// check time
	m_fTime = (float)GetTickCount();

	// if new then this is the new start time
	if (m_fStartTime == -1.0f)
		m_fStartTime = m_fTime;

	// calculate elapsed time
	fElapsed = m_fTime - m_fStartTime;

	// get current animation
	pAnimation = &m_pAnimations[m_uiCurrentAnimation];

	fStart	= pAnimation->fStartFrame;
	fEnd	= pAnimation->fEndFrame;

	// calculate frame position
	m_fFrame = fStart + (m_sHeader.fAnimationFPS / 2048) * fElapsed;

	// set new start frame
	if (m_fFrame <= fStart)
		m_fFrame = fStart;

	// animation ended?
	if (m_fFrame >= fEnd) {
		m_fStartTime	= m_fTime;
		m_fFrame		= fStart;
		m_bAnimationComplete = true;
	}
	else {
		// prepare animation
		AnimationPrepare();

		// setup vertices
		AnimationVertices();
		m_bAnimationComplete	= false;	// set Flag
		m_bAnimationChanged		= false;
	}
	return S_OK;
}
// --------------------------

// --------------------------
// Name: AnimationPrepare( void )
//
// Return		= (HRESULT)		Status
// --------------------------
HRESULT CZFXModel::AnimationPrepare(void)
{
	// Initialize Variables
	LPJOINT		pJoint		= NULL;			// joint
	ULONG		ulCounter	= 0;			// counter
	UINT		uiLoop		= 0;			// counter
	ZFXVector	sPosition;					// vector
	ZFXVector	sRotation;					// vector
	UINT		uiKeyPos	= 0;			// key-position
	UINT		uiKeyRot	= 0;			// key-rotation
	LPKF_ROT	pLastRot	= NULL;			// rotation
	LPKF_ROT	pThisRot	= NULL;			// rotation
	LPKF_ROT	pKeyRot		= NULL;			// rotation
	LPKF_POS	pLastPos	= NULL;			// position
	LPKF_POS	pThisPos	= NULL;			// position
	LPKF_POS	pKeyPos		= NULL;			// position
	float		fScale		= 0.0f;			// scaling
	ZFXMatrix	matTemp;	matTemp.Identity();
	ZFXMatrix	matFinal;	matFinal.Identity();

	// clip the animation
	if (m_fFrame > m_sHeader.uiNumFrames)
		m_fFrame = 0;

	// calculate matrix
	for (ulCounter = 0; ulCounter < m_sHeader.uiNumJoints; ++ulCounter)
	{
		// get current joint
		pJoint		= &m_pJoints[ulCounter];

		// get data
		uiKeyPos	= pJoint->wNumKF_Position;	// position
		uiKeyRot	= pJoint->wNumKF_Rotation;	// rotation

		// recalculation necessary?
		if ((uiKeyRot + uiKeyPos) != 0) {
			// yes new position or rotation
			pLastPos	= NULL;
			pThisPos	= NULL;
			pKeyPos		= NULL;

			for (uiLoop = 0; uiLoop < uiKeyPos; ++uiLoop)
			{
				// get current position
				pKeyPos = &pJoint->pKF_Position[uiLoop];

				// check time
				if (pKeyPos->fTime >= m_fFrame) {
					pThisPos = pKeyPos;
					break;
				}
				// nothing found
				pLastPos = pKeyPos;
			}

			// interpolation the two positions
			if (pLastPos && pThisPos) {
				// calculate scaling
				fScale = (m_fFrame - pLastPos->fTime) / (pThisPos->fTime - pLastPos->fTime);

				// interpolation
				sPosition = pLastPos->vPosition + (pThisPos->vPosition - pLastPos->vPosition) * fScale;
			}
			else if (!pLastPos) {
				// copy the position
				sPosition = pThisPos->vPosition;
			}
			else {
				// copy the position
				sPosition = pLastPos->vPosition;
			}

			// apply rotation
			pLastRot	= NULL;
			pThisRot	= NULL;
			pKeyRot		= NULL;

			for (uiLoop = 0; uiLoop < uiKeyRot; ++uiLoop)
			{
				// get current rotation
				pKeyRot = &pJoint->pKF_Rotation[uiLoop];

				// check time
				if (pKeyRot->fTime >= m_fFrame) {
					pThisRot = pKeyRot;
					break;
				}
				// nothing found
				pLastRot = pKeyRot;
			} // all Rotations

			// interpolate the rotations
			if (pLastRot && pThisRot) {
				sRotation = pLastRot->vRotation + (pThisRot->vRotation - pLastRot->vRotation) * fScale;
			}
			else if (!pLastRot) {
				// copy rotation
				sRotation = pThisRot->vRotation;
			}
			else {
				// copy rotation
				sRotation = pLastRot->vRotation;
			}

			// joint matrix setup
			matTemp.SetTranslation(sPosition);
			matTemp.Rota(sRotation);

			// calculate relative matrix
			matFinal = matTemp * pJoint->sMatrix_relative;

			// is there a parent
			if (pJoint->wParentID != 255) {
				// take parent matrix into account
				pJoint->sMatrix = matFinal * m_pJoints[pJoint->wParentID].sMatrix;
			}
			else {
				pJoint->sMatrix = matFinal;
			}
		}
		else {
			// no new matrix, copy old one
			pJoint->sMatrix = pJoint->sMatrix_relative;
		}
	}
	return S_OK;
}
// --------------------------

// --------------------------
// Name: AnimationVertices( void )
//
// Return		= (HRESULT)		Status
// --------------------------
HRESULT CZFXModel::AnimationVertices(void)
{
	// Variablen init
	ULONG	ulCounter = 0;		// counter
	CVERTEX* pVertex = NULL;	// temporary
	CVERTEX* pVertex_Orig = NULL;	// temporary
	ZFXVector sVector_A, sVector_B;	// vector

	// reset bounding box
	m_sBBoxMin.x = 999999.0f; m_sBBoxMax.x = -999999.0f;
	m_sBBoxMin.y = 999999.0f; m_sBBoxMax.y = -999999.0f;
	m_sBBoxMin.z = 999999.0f; m_sBBoxMax.z = -999999.0f;

	// setup the vertices
	for (ulCounter = 0; ulCounter < m_sHeader.ulNumVertices; ++ulCounter)
	{
		// get current vertex
		pVertex			= &m_pVertices[ulCounter];
		pVertex_Orig	= &m_pVertices_Orig[ulCounter];

		// only do this if a bone is found
		if (pVertex->fBone1 != 255.0f) {
			// 1. get original (non-animated) vertex
			sVector_A.x = pVertex_Orig->x;
			sVector_A.y = pVertex_Orig->y;
			sVector_A.z = pVertex_Orig->z;

			// 2. rotate the vertex
			sVector_A.RotateWith(m_pJoints[(UINT)pVertex_Orig->fBone1].sMatrix);

			// 3. get position
			sVector_A += m_pJoints[(UINT)pVertex_Orig->fBone1].sMatrix.GetTranslation();

			// 4. calculate new position
			pVertex->x = sVector_A.x;
			pVertex->y = sVector_A.y;
			pVertex->z = sVector_A.z;

			// 5. animate the normals
			sVector_A.x = pVertex_Orig->vcN[0];
			sVector_A.y = pVertex_Orig->vcN[1];
			sVector_A.z = pVertex_Orig->vcN[2];
			sVector_A.RotateWith(m_pJoints[(UINT)pVertex_Orig->fBone1].sMatrix);
			pVertex->vcN[0] = sVector_A.x;
			pVertex->vcN[1] = sVector_A.y;
			pVertex->vcN[2] = sVector_A.z;

			// 6. calculate bounding box
			m_sBBoxMax.x = max(m_sBBoxMax.x, pVertex->x);
			m_sBBoxMax.y = max(m_sBBoxMax.y, pVertex->y);
			m_sBBoxMax.z = max(m_sBBoxMax.z, pVertex->z);
			m_sBBoxMin.x = min(m_sBBoxMin.x, pVertex->x);
			m_sBBoxMin.y = min(m_sBBoxMin.y, pVertex->y);
			m_sBBoxMin.z = min(m_sBBoxMin.z, pVertex->z);
		}
	}
	// 7. create aabb
	m_sAabb.vcMin.x = m_sBBoxMin.x;
	m_sAabb.vcMin.y = m_sBBoxMin.y;
	m_sAabb.vcMin.z = m_sBBoxMin.z;
	m_sAabb.vcMax.x = m_sBBoxMax.x;
	m_sAabb.vcMax.y = m_sBBoxMax.y;
	m_sAabb.vcMax.z = m_sBBoxMax.z;
	m_sAabb.vcCenter.x = (m_sBBoxMax.x - m_sBBoxMin.x) / 2;
	m_sAabb.vcCenter.y = (m_sBBoxMax.y - m_sBBoxMin.y) / 2;
	m_sAabb.vcCenter.z = (m_sBBoxMax.z - m_sBBoxMin.z) / 2;
	return S_OK;
}
// --------------------------

// --------------------------
// Name: Update( float fTime )
//
// fTime		= DeltaTime
// 
// Return		= (HRESULT)		Status
// --------------------------
HRESULT CZFXModel::Update(float fTime)
{
	// set the time
	m_fTime = fTime;

	// do the animation
	return Animation();
}
// --------------------------

// --------------------------
// Name: Render( void )
// 
// Return		= (HRESULT)		Status
// --------------------------
HRESULT CZFXModel::Render(void)
{
	UINT	uiCounter = 0;

	// set culling
	m_pRenderDevice->SetBackfaceCulling(RS_CULL_CCW);

	// render vertex buffer
	for (uiCounter = 0; uiCounter < m_sHeader.uiNumMaterials; ++uiCounter)
		if (FAILED(m_pRenderDevice->GetVertexCacheManager()->Render(VID_CA, m_sHeader.ulNumVertices, m_puiNumIndices[uiCounter], m_pVertices, (PWORD)m_ppIndices[uiCounter], m_puiSkinBuffer[uiCounter])))
			LOG(1, true, "ERROR Failed to Render VB: %d [%d]", m_puiSkinBuffer[uiCounter], uiCounter);

	// render other data if requested
	if (m_bRenderBones)
		RenderBones();			// bones
	if (m_bRenderNormals)
		RenderNormals();		// normals
	return S_OK;
}
// --------------------------

void CZFXModel::SetAnimation(UINT uiAnim)
{
	// In Range?
	if (uiAnim > m_sHeader.uiNumAnimations)
		uiAnim = 0;
	if (uiAnim < 0)
		uiAnim = m_sHeader.uiNumAnimations;

	// Set Animation
	m_uiCurrentAnimation = uiAnim;

	// Multiply Animations
	m_bAnimationRunOnce = false;
}
// --------------------------

void CZFXModel::SetAnimation(bool bSingle, UINT uiAnim)
{
	// set the wanted Animation
	SetAnimation(uiAnim);

	// Multiply Animations
	m_bAnimationChanged = true;
	m_bAnimationRunOnce = bSingle;
	m_bAnimationComplete = false;
}
// --------------------------

// --------------------------
// Name: RenderBones( float fTime )
//
// fTime		= DeltaTime
// 
// Return		= (HRESULT)		Status
// --------------------------
HRESULT CZFXModel::RenderBones(void)
{
	UINT	uiCounter	= 0;			// counter
	LVERTEX	pLine[3];					// joints
	WORD	pIndis[3]	= { 0, 1, 2 };	// indices
	DWORD	dwColor		= 0x00ffff;		// color

	// are there bones at all
	if (m_sHeader.uiNumJoints == 0)
		return S_OK;

	m_pRenderDevice->SetBackfaceCulling(RS_CULL_NONE);
	m_pRenderDevice->SetDepthBufferMode(RS_DEPTH_NONE);

	// render the bones
	for (uiCounter = 0; uiCounter < m_sHeader.uiNumJoints; ++uiCounter)
	{
		// first vertex
		pLine[0].x = m_pJoints[uiCounter].sMatrix._41;
		pLine[0].y = m_pJoints[uiCounter].sMatrix._42;
		pLine[0].z = m_pJoints[uiCounter].sMatrix._43;
		pLine[0].Color = dwColor;

		if (m_pJoints[uiCounter].wParentID != 255)
		{
			// second vertex
			pLine[1].x = m_pJoints[m_pJoints[uiCounter].wParentID].sMatrix._41;
			pLine[1].y = m_pJoints[m_pJoints[uiCounter].wParentID].sMatrix._42;
			pLine[1].z = m_pJoints[m_pJoints[uiCounter].wParentID].sMatrix._43;
			pLine[1].Color = dwColor;

			// third vertex
			pLine[2].x = pLine[1].x + 1.0f;
			pLine[2].y = pLine[1].y + 1.0f;
			pLine[2].z = pLine[1].z + 1.0f;
			pLine[2].Color = dwColor;

			// render
			m_pRenderDevice->GetVertexCacheManager()->Render(VID_UL, 3, 3, pLine, pIndis, 0);
		}
	}
	m_pRenderDevice->SetDepthBufferMode(RS_DEPTH_READWRITE);
	m_pRenderDevice->SetBackfaceCulling(RS_CULL_CCW);
	return S_OK;
}
// --------------------------

// --------------------------
// Name: RenderNormals( void )
//
// fTime		= DeltaTime
// 
// Return		= (HRESULT)		Status
// --------------------------
HRESULT CZFXModel::RenderNormals(void)
{
	ULONG		ulCounter	= 0;
	float		fStart[3]	= { 0, 0, 0 };
	float		fEnd[3]		= { 0, 0, 0 };
	ZFXCOLOR	sColor		= { 1.0f, 0, 0, 0 };
	CVERTEX*	pVertex		= NULL;

	// render the normals
	for (ulCounter = 0; ulCounter < m_sHeader.ulNumVertices; ++ulCounter)
	{
		// get current vertex
		pVertex = &m_pVertices[ulCounter];

		// set starting point
		memcpy(fStart, &pVertex->x, sizeof(float) * 3);

		// set end point
		fEnd[0] = fStart[0] + (pVertex->vcN[0] * 2.0f);
		fEnd[1] = fStart[1] + (pVertex->vcN[1] * 2.0f);
		fEnd[2] = fStart[2] + (pVertex->vcN[2] * 2.0f);

		// render normal vector
		m_pRenderDevice->GetVertexCacheManager()->RenderLine(fStart, fEnd, &sColor);
	}
	return S_OK;
}
// --------------------------

ZFXMatrix CZFXModel::CreateRotationMatrix(ZFXVector* pVector)
{
	// Init variables
	float		sr, sp, sy, cr, cp, cy;
	ZFXMatrix	matRet;

	matRet.Identity();

	sy = (float)sin(pVector->z);
	cy = (float)cos(pVector->z);
	sp = (float)sin(pVector->y);
	cp = (float)cos(pVector->y);
	sr = (float)sin(pVector->x);
	cr = (float)cos(pVector->x);

	matRet._11 = cp * cy;
	matRet._21 = cp * sy;
	matRet._31 = -sp;
	matRet._12 = sr * sp * cp + cr * -sy;
	matRet._22 = sr * sp * sy + cr * cy;
	matRet._32 = sr * cp;
	matRet._13 = (cr * sp * cy + -sr * -sy);
	matRet._23 = (cr * sp * sy + -sr * cy);
	matRet._33 = cr * cp;
	matRet._14 = 0.0f;
	matRet._24 = 0.0f;
	matRet._34 = 0.0f;

	return matRet;
}
// --------------------------

void CZFXModel::LOG(UINT iLevel, bool bCR, const char* pcText, ...)
{
	// Variablen init
	va_list	args;				// Arguments
	char	cBuffer[4096];		// linebuffer
	FILE*	pLog;				// file

	// do logging?
	if (!m_bLog)
		return;

	// correct log level?
	if (iLevel <= m_uiLogLevel) {
		// something to log?
		if (pcText) {
			// set the pointer to the beginning
			va_start(args, pcText);

			// copy all data to cBuffer
			vsprintf_s(cBuffer, pcText, args);

			// reset the list
			va_end(args);
		}

		// open the logfile
		fopen_s(&pLog, m_cLogFileName, "a");

		// file opened?
		if (!pLog)
			return;

		// write the wanted text into the log file
		if (bCR)
			fprintf(pLog, "%s\n", cBuffer);
		else
			fprintf(pLog, "%s", cBuffer);
		
		// file close
		fclose(pLog);
	}
}
// --------------------------