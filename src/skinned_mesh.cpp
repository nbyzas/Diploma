// Own
#include "skinned_mesh.h"

// Qt
#include <QtGui\QVector2D>
#include <QtGui\QImage>
#include <QtGui\QMatrix4x4>

// Standard C/C++
#include <cassert>
#include <sstream>
#include <iomanip>

void VertexBoneData::AddBoneData(uint BoneID, float Weight)
{
	for (uint i = 0; i < ARRAY_SIZE_IN_ELEMENTS(IDs); i++) {
		if (Weights[i] == 0.0) {
			IDs[i] = BoneID;
			Weights[i] = Weight;
			return;
		}
	}
	cout << "Should never get here!" << endl;
	system("PAUSE");
	// should never get here - more vertexBoneData than we have space for
	assert(0);
}
void VertexBoneData::AdjustBoneData()
{
	float max = 0.0f;
	for (uint i = 0; i < ARRAY_SIZE_IN_ELEMENTS(IDs); i++) {
		if (Weights[i] > max) max = Weights[i];
		OldWeights[i] = Weights[i]; // backup weights
	}

	for (uint i = 0; i < ARRAY_SIZE_IN_ELEMENTS(IDs); i++) {
		if (Weights[i] >= max) {
			Weights[i] = 1;
			for (uint j = 0; j < ARRAY_SIZE_IN_ELEMENTS(IDs); j++)
			{
				if (j != i) Weights[j] = 0.0f;
			}
			return;
		}
	}
	cout << "Should never get here!" << endl;
	system("PAUSE");
	// should never get here
	assert(0);
}
void VertexBoneData::RestoreBoneData()
{
	for (uint i = 0; i < ARRAY_SIZE_IN_ELEMENTS(IDs); i++)
	{
		Weights[i] = OldWeights[i];
	}
}

SkinnedMesh::SkinnedMesh()
	:
	m_successfullyLoaded(false),
	m_pScene(NULL)
{
	cout << "SkinnedMesh constructor start." << endl;
	cout << "SkinnedMesh constructor end.\n" << endl;
}
SkinnedMesh::~SkinnedMesh()
{
    clear();
}
// delete container contents and resize to 0
void SkinnedMesh::clear()
{	
	m_meshEntries.clear();
	m_positions.clear();
	m_normals.clear();
	m_texCoords.clear();
	m_vertexBoneData.clear();
	m_indices.clear();
	m_images.clear();
	m_boneInfo.clear();
}
bool SkinnedMesh::loadFromFile(const string& fileName)
{
	clear();

    bool ret = false;
	string filePath = "models/" + fileName;
    m_pScene = m_Importer.ReadFile(filePath.c_str(), ASSIMP_LOAD_FLAGS);    
    if (m_pScene) {  
        ret = initFromScene(m_pScene, filePath);
    }
    else {
       cout << "Error parsing '" << filePath << "' -> '\n" << string(m_Importer.GetErrorString()) << endl;
    }
	
	m_successfullyLoaded = ret;
	if (m_successfullyLoaded) {
		cout << "Loaded SkinnedMesh from " << filePath << endl;
	}
    return ret;
}
bool SkinnedMesh::initFromScene(const aiScene* pScene, const string& filename)
{  
	// Update mesh entries
	uint numVertices = 0;
    uint numIndices = 0;
	m_meshEntries.resize(pScene->mNumMeshes);
	for (uint i = 0 ; i < m_meshEntries.size() ; i++) {
        m_meshEntries[i].materialIndex = pScene->mMeshes[i]->mMaterialIndex;        
        m_meshEntries[i].numIndices    = pScene->mMeshes[i]->mNumFaces * 3;
        m_meshEntries[i].baseVertex    = numVertices;
        m_meshEntries[i].baseIndex     = numIndices;	
        numVertices += pScene->mMeshes[i]->mNumVertices;
        numIndices  += m_meshEntries[i].numIndices;
    }
	m_numVertices = numVertices;

	// Reserve space in the vectors for the vertex attributes and indices
	m_positions.reserve(numVertices);
	m_normals.reserve(numVertices);
	m_texCoords.reserve(numVertices);
	m_indices.reserve(numIndices);
	m_vertexBoneData.resize(numVertices); // elements will be accessed without being push_backed first

	// Initialize the meshes in the scene one by one
	for (uint i = 0; i < m_meshEntries.size(); i++) {
		const aiMesh* paiMesh = pScene->mMeshes[i];
		initMesh(i, paiMesh);
	}

	m_controlQuats.resize(m_numBones);
	m_controlMats.resize(m_numBones);
	m_boneInfo.resize(m_numBones);
	m_boneTransformInfo.resize(m_numBones);

	initDefaultLocalMatrices(m_pScene->mRootNode);
	correctLocalMatrices();
	initImages(m_pScene, filename);

	return true;
}
void SkinnedMesh::initMesh(uint meshIndex, const aiMesh* paiMesh)
{
	const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

	// Populate the vertex attribute vectors
	for (uint i = 0; i < paiMesh->mNumVertices; i++) {
		const aiVector3D* pPos = &(paiMesh->mVertices[i]);
		const aiVector3D* pNormal = &(paiMesh->mNormals[i]);
		const aiVector3D* pTexCoord = paiMesh->HasTextureCoords(0) ? &(paiMesh->mTextureCoords[0][i]) : &Zero3D;
		m_positions.push_back(QVector3D(pPos->x, pPos->y, pPos->z));
		m_normals.push_back(QVector3D(pNormal->x, pNormal->y, pNormal->z));
		m_texCoords.push_back(QVector2D(pTexCoord->x, pTexCoord->y));
	}

	loadBones(meshIndex, paiMesh, m_vertexBoneData);
	checkWeights(meshIndex, paiMesh);

	// Populate the index buffer
	for (uint i = 0; i < paiMesh->mNumFaces; i++) {
		const aiFace& face = paiMesh->mFaces[i];
		assert(face.mNumIndices == 3);
		m_indices.push_back(face.mIndices[0]);
		m_indices.push_back(face.mIndices[1]);
		m_indices.push_back(face.mIndices[2]);
	}
}
void SkinnedMesh::loadBones(uint meshIndex, const aiMesh* pMesh, QVector<VertexBoneData>& vertexBoneData)
{
	for (uint i = 0; i < pMesh->mNumBones; i++) {
		uint BoneIndex = 0;
		string BoneName(pMesh->mBones[i]->mName.data);
		if (m_boneMap.find(BoneName) == m_boneMap.end()) {
			BoneIndex = m_numBones;
			m_numBones++;
			BoneInfo bi;
			m_boneInfo.push_back(bi);
		}
		else {
			BoneIndex = m_boneMap[BoneName];
		}

		m_boneMap[BoneName] = BoneIndex;
		m_boneInfo[BoneIndex].offset = toQMatrix(pMesh->mBones[i]->mOffsetMatrix);

		for (uint j = 0; j < pMesh->mBones[i]->mNumWeights; j++) {
			uint VertexID = m_meshEntries[meshIndex].baseVertex + pMesh->mBones[i]->mWeights[j].mVertexId;
			float Weight = pMesh->mBones[i]->mWeights[j].mWeight;
			vertexBoneData[VertexID].AddBoneData(BoneIndex, Weight);
		}
	}
}
void SkinnedMesh::checkWeights(uint meshIndex, const aiMesh* pMesh) {
	bool SumNot1 = false;
	for (uint i = 0; i < pMesh->mNumVertices; i++) {
		float sum = 0;
		uint VertexID = m_meshEntries[meshIndex].baseVertex + i;
		for (uint i = 0; i < NUM_BONES_PER_VERTEX; i++) {
			sum += m_vertexBoneData[VertexID].Weights[i];
		}
		if (sum<0.99 || sum >1.01) SumNot1 = true; // Must be equal to 1
	}
	if (SumNot1) cout << "Sum of weights is not 1 for all vertices!" << endl;
}
bool SkinnedMesh::initImages(const aiScene* pScene, const string& fileName)
{
	// Extract the directory part from the file name
	string::size_type slashIndex = fileName.find_last_of("/");
	string directory;

	if (slashIndex == string::npos) {
		directory = ".";
	}
	else if (slashIndex == 0) {
		directory = "/";
	}
	else {
		directory = fileName.substr(0, slashIndex);
	}

	bool ret;
	// Initialize the materials
	for (uint i = 0; i < pScene->mNumMaterials; i++) {
		ret = false;
		const aiMaterial* pMaterial = pScene->mMaterials[i];
		if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			aiString path;
			if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
				string p(path.data);
				if (p.substr(0, 2) == ".\\") {
					p = p.substr(2, p.size() - 2);
				}
				string fullPath = string(directory + "\\" + p).c_str();
				cout << "Loading image: " << fullPath << endl;
				m_images.push_back(QImage(QString::fromStdString(fullPath)));
				ret = true;
			}
		}
	}
	return ret;
}
void SkinnedMesh::printInfo() const
{
	if (m_pScene) {
		cout << endl;
		printSceneInfo();
		if (m_pScene->mRootNode) {
			cout << endl;
			cout << "Model node hierarchy:" << endl;
			printNodeHierarchy(m_pScene->mRootNode);
			cout << endl;
		}
	}
}
void SkinnedMesh::printSceneInfo() const
{
	cout << "Scene info:";
	cout << " MeshEntries:"	 << setw( 3) << m_pScene->mNumMeshes;
	cout << " Materials:"	 << setw( 3) << m_pScene->mNumMaterials;
	cout << " Textures:"	 << setw( 3) << m_pScene->mNumTextures;
	cout << " Lights:"		 << setw( 3) << m_pScene->mNumLights;
	cout << " Animations:"   << setw( 3) << m_pScene->mNumAnimations;
	cout << endl;

	cout << "Meshes:" << endl;
	for (uint i = 0; i < m_pScene->mNumMeshes; i++) {
		cout << "Id:"	       << i;
		cout << " Name:"	   << setw(20) << m_pScene->mMeshes[i]->mName.C_Str();
		cout << " Vertices:"   << setw( 6) << m_pScene->mMeshes[i]->mNumVertices;
		cout << " Faces:"	   << setw( 6) << m_pScene->mMeshes[i]->mNumFaces;
		cout << " Bones:"	   << setw( 6) << m_pScene->mMeshes[i]->mNumBones;
		cout << " MaterialId:" << setw( 6) << m_pScene->mMeshes[i]->mMaterialIndex;
		cout << endl;
	}
	cout << "Total: Vertices:" << m_numVertices << " Bones:" << m_numBones;
	cout << endl;
}
void SkinnedMesh::printNodeHierarchy(const aiNode* pNode) const
{
	static uint counter = 0;
	if (!pNode->mParent) counter = 0;
	string NodeName(pNode->mName.data);
	string ParentName(pNode->mParent ? pNode->mParent->mName.data : "Root");
	cout << "Node " << setw(2) << left << counter << ": " << setw(25) << left << NodeName << setw(10) << left;
	if (m_kboneMap.find(NodeName) != m_kboneMap.end()) cout << "(KBone)";
	else if (m_boneMap.find(NodeName) != m_boneMap.end()) cout << " (Bone)";
	else cout << "";
	counter++;
	cout << " Parent:" << setw(20) << left << ParentName;
	cout << " Children (" << pNode->mNumChildren << "): ";
	for (uint i = 0; i < pNode->mNumChildren; i++) {
		string ChildName(pNode->mChildren[i]->mName.data);
		cout << ChildName << " ";	
	}
	cout << endl;
	for (uint i = 0; i < pNode->mNumChildren; i++) {
		printNodeHierarchy(pNode->mChildren[i]);
	}
	
}
QVector3D SkinnedMesh::getPelvisOffset()
{
	float x = m_boneInfo[findBoneId("pelvis")].defaultLocal(0, 3);
	float y = m_boneInfo[findBoneId("pelvis")].defaultLocal(1, 3);
	float z = m_boneInfo[findBoneId("pelvis")].defaultLocal(2, 3);
	return QVector3D(x, y, z);
}
bool SkinnedMesh::parameter(uint i) const
{
	return m_parameters[i];
}
void SkinnedMesh::initDefaultLocalMatrices(const aiNode* node)
{
	const auto& it = m_boneMap.find(node->mName.data);
	if (it != m_boneMap.cend()) {
		m_boneInfo[it->second].defaultLocal = toQMatrix(node->mTransformation);
	}
	else {
		QString nodeName(node->mName.data);
		QMatrix4x4 L = toQMatrix(node->mTransformation);
		cout << nodeName.toStdString() << " transformation:" << endl;
		cout << toString(L).toStdString() << endl;
	}
	for (uint i = 0; i < node->mNumChildren; i++) {
		initDefaultLocalMatrices(node->mChildren[i]);
	}
}
void SkinnedMesh::correctLocalMatrices()
{
	// Init all as identity
	for (int i = 0; i < m_numBones; i++) {
		m_boneInfo[i].localCorrection = QMatrix4x4();
	}

	// core
	m_boneInfo[findBoneId("pelvis")].localCorrection = fromRotation(0.f, 0.f, 0.f);
	m_boneInfo[findBoneId("spine_01")].localCorrection = fromRotation(0.f, 0.f, 0.f);
	m_boneInfo[findBoneId("spine_02")].localCorrection = fromRotation(0.f, 0.f, 0.f);
	m_boneInfo[findBoneId("spine_03")].localCorrection = fromRotation(0.f, 0.f, 0.f);
	m_boneInfo[findBoneId("neck_01")].localCorrection = fromRotation(0.f, 0.f, 0.f);
	m_boneInfo[findBoneId("head")].localCorrection = fromRotation(0.f, 0.f, 0.f);

	// arms
	m_boneInfo[findBoneId("clavicle_l")].localCorrection = fromRotation(0.f, 0.f, 0.f);
	m_boneInfo[findBoneId("clavicle_r")].localCorrection = fromRotation(0.f, 0.f, 0.f);
	m_boneInfo[findBoneId("upperarm_l")].localCorrection = fromRotation(0.f, 90.f, 0.f);
	m_boneInfo[findBoneId("upperarm_r")].localCorrection = fromRotation(0.f, -90.f, 0.f);
	m_boneInfo[findBoneId("lowerarm_l")].localCorrection = fromRotation(0.f, 180.f, 0.f);
	m_boneInfo[findBoneId("lowerarm_r")].localCorrection = fromRotation(0.f, -90.f, 0.f);
	m_boneInfo[findBoneId("hand_l")].localCorrection = fromRotation(0.f, -90.f, 0.f);
	m_boneInfo[findBoneId("hand_r")].localCorrection = fromRotation(0.f, 0.f, 0.f);

	// legs
	m_boneInfo[findBoneId("thigh_l")].localCorrection = fromRotation(0.f, -90.f, 0.f);
	m_boneInfo[findBoneId("thigh_r")].localCorrection = fromRotation(0.f, 90.f, 0.f);
	m_boneInfo[findBoneId("calf_l")].localCorrection = fromRotation(0.f, -90.f, 0.f);
	m_boneInfo[findBoneId("calf_r")].localCorrection = fromRotation(0.f, 90.f, 0.f);
	
	m_boneInfo[findBoneId("foot_l")].defaultLocal =
		getTranslationPart(m_boneInfo[findBoneId("foot_l")].defaultLocal)*
		fromRotation(0.f, 0.f, 0.f);
	m_boneInfo[findBoneId("foot_r")].defaultLocal =
		getTranslationPart(m_boneInfo[findBoneId("foot_r")].defaultLocal)*
		fromRotation(0.f, 0.f, 0.f);
	m_boneInfo[findBoneId("foot_l")].localCorrection = fromRotation(-90.f, 0.f, 0.f);
	m_boneInfo[findBoneId("foot_r")].localCorrection = fromRotation(-90.f, 0.f, 0.f);

	for (int i = 0; i < m_numBones; i++) {
		m_boneInfo[i].correctedLocal = m_boneInfo[i].defaultLocal * m_boneInfo[i].localCorrection;
	}
}
void SkinnedMesh::initKBoneMap()
{
	// Core
	m_kboneMap["pelvis"    ] = JointType_SpineMid     ;
	m_kboneMap["spine_01"  ] = JointType_SpineMid     ;
	m_kboneMap["spine_02"  ] = JointType_SpineMid     ;
	m_kboneMap["spine_03"  ] = JointType_SpineShoulder;
	m_kboneMap["neck_01"   ] = JointType_Neck         ;
	//m_kboneMap["head"    ] = JointType_Head           ;

	// arms
	m_kboneMap["clavicle_l"] = JointType_ShoulderLeft ;
	m_kboneMap["clavicle_r"] = JointType_ShoulderRight;
	m_kboneMap["upperarm_l"] = JointType_ElbowLeft    ;
	m_kboneMap["upperarm_r"] = JointType_ElbowRight   ;
	m_kboneMap["lowerarm_l"] = JointType_WristLeft    ;
	m_kboneMap["lowerarm_r"] = JointType_WristRight   ;
	m_kboneMap["hand_l"    ] = JointType_HandLeft     ;
	m_kboneMap["hand_r"    ] = JointType_HandRight    ;
	
	// legs
	m_kboneMap["thigh_l"   ] = JointType_KneeLeft     ;
	m_kboneMap["thigh_r"   ] = JointType_KneeRight    ;
	m_kboneMap["calf_l"    ] = JointType_AnkleLeft    ;
	m_kboneMap["calf_r"    ] = JointType_AnkleRight   ;
	//m_kboneMap["foot_l"    ] = JointType_FootLeft     ;
	//m_kboneMap["foot_r"    ] = JointType_FootRight    ;
}
void SkinnedMesh::calculateBoneTransforms(const aiNode* pNode, const QMatrix4x4& P, const array<KJoint, JointType_Count>& joints)
{
	QString nodeName(pNode->mName.data);
	QMatrix4x4 L = toQMatrix(pNode->mTransformation);
	QMatrix4x4 G;

	const auto& it = m_boneMap.find(nodeName.toStdString());
	if (it == m_boneMap.end()){ // is not a bone
		G = P * L;
	}
	else { // is a bone
		uint i = it->second;
		QQuaternion q;
		QString qs;
		QTextStream qts(&qs);

		qts << "\nBoneName=" << nodeName << " Index=" << i << endl;
		
		q = extractQuaternion(L);
		qts << "Default local quaternion: " << toString(q) << toStringEulerAngles(q) << toStringAxisAngle(q) << endl;
		qts << "Default local transformation:\n" << toString(L);

		QMatrix4x4 localTransformation(L);
		const auto& kit = m_kboneMap.find(nodeName.toStdString());
		if (m_parameters[1] && kit != m_kboneMap.end()) {
			uint k = kit->second;
			// calculate scaling from Kinect
			QMatrix4x4 kinectScaling = QMatrix();
			
			// calculate rotation from Kinect
			QQuaternion absQ = joints[k].orientation;
			QQuaternion parQ = extractQuaternion(P);
			QQuaternion relQ = parQ.inverted() * absQ;
			QMatrix4x4 kinectRotation = fromRotation(relQ);
			qts << "Kinect rotation abs: " << toString(absQ) << toStringEulerAngles(absQ) << toStringAxisAngle(absQ) << endl;
			qts << "Parent rotation abs: " << toString(parQ) << toStringEulerAngles(parQ) << toStringAxisAngle(parQ) << endl;
			qts << "Parent rotation inv: " << toString(parQ.inverted()) << toStringEulerAngles(parQ.inverted()) << toStringAxisAngle(parQ.inverted()) << endl;
			qts << "Kinect rotation rel: " << toString(relQ) << toStringEulerAngles(relQ) << toStringAxisAngle(relQ) << endl;
			qts << "Kinect local orientation: " << toString(relQ) << toStringEulerAngles(relQ) << toStringAxisAngle(relQ) << endl;

			// calculate translation from Kinect
			QMatrix4x4 kinectTranslation = getTranslationPart(L);
			if (nodeName == "pelvis") kinectTranslation = fromTranslation(QVector3D(0.f, 0.f, 0.f));
			QVector3D v = QVector3D(kinectTranslation(0, 3), kinectTranslation(1, 3), kinectTranslation(2, 3));
			qts << "Kinect local translation: " << toStringCartesian(v) << endl;

			localTransformation = kinectTranslation * kinectRotation * kinectScaling;
			qts << "Kinect local transformation:\n" << toString(localTransformation);
		}
		
		if (m_parameters[2]) {
			q = extractQuaternion(m_boneInfo[i].localCorrection);
			qts << "Correction quaternion: " << toString(q) << toStringEulerAngles(q) << toStringAxisAngle(q) << endl;
			qts << "Correction transformation:\n" << toString(m_boneInfo[i].localCorrection);
			
			if (m_parameters[1] && kit!=m_kboneMap.end()) {
				localTransformation = localTransformation * m_boneInfo[i].localCorrection;
			}
			else {
				localTransformation = m_boneInfo[i].correctedLocal;
			}
		}

		q = extractQuaternion(localTransformation);
		qts << "Local quaternion: " << toString(q) << toStringEulerAngles(q) << toStringAxisAngle(q) << endl;
		qts << "Local transformation:\n" << toString(localTransformation);

		QMatrix4x4 controlRot; // initialized as identity matrix
		if (m_parameters[3]) {
			controlRot = m_controlMats[i];

			q = m_controlQuats[i];
			qts << "Control quaternion: " << toString(q) << toStringEulerAngles(q) << toStringAxisAngle(q) << endl;
			qts << "Control transformation:\n" << toString(m_controlMats[i]);
		}
		if (!m_parameters[4]) localTransformation = QMatrix4x4();

		// localTransformation next to P, 
		// control next to localTransformation
		// (so that changing the control matrix is like changing the correction matrix)
		G = P * localTransformation * controlRot;

		m_boneInfo[i].global = G;
		m_boneInfo[i].combined = G * m_boneInfo[i].offset;
		if (!m_parameters[0]) m_boneInfo[i].combined = m_boneInfo[i].offset;
		
		qts << "Parent's Global transformation:\n" << toString(P);
		qts << "Global transformation:\n" << toString(G);
		qts << "Offset transformation:\n" << toString(m_boneInfo[i].offset);
		qts << "Combined transformation:\n" << toString(m_boneInfo[i].combined);

		QVector3D positionGlobal = m_boneInfo[i].global * QVector3D(QVector4D(0.f, 0.f, 0.f, 1.f));
		m_boneInfo[i].endPosition = positionGlobal;
		qts << "Bone position (from global): " << toStringCartesian(positionGlobal);

		qts << flush;
		m_boneTransformInfo[findBoneId(nodeName)] = qs;
	}

	for (uint i = 0; i < pNode->mNumChildren; i++) {
		calculateBoneTransforms(pNode->mChildren[i], G, joints);
	}
}
float SkinnedMesh::boneRotationX(const QString &boneName) const
{
	return m_boneInfo[findBoneId(boneName)].xRot;
}
float SkinnedMesh::boneRotationY(const QString &boneName) const
{
	return m_boneInfo[findBoneId(boneName)].yRot;
}
float SkinnedMesh::boneRotationZ(const QString &boneName) const
{
	return m_boneInfo[findBoneId(boneName)].zRot;
}
void SkinnedMesh::setBoneRotationX(const QString &boneName, float value)
{
	uint boneId = findBoneId(boneName);
	assert(boneId < m_boneInfo.size());
	BoneInfo &bi = m_boneInfo[boneId];
	bi.xRot = value;
	QQuaternion q;
	q  = QQuaternion::fromEulerAngles(bi.xRot, 0, 0);
	q *= QQuaternion::fromEulerAngles(0, bi.yRot, 0);
	q *= QQuaternion::fromEulerAngles(0, 0, bi.zRot);
	m_controlQuats[boneId] = q;
	m_controlMats[boneId] = fromRotation(q);
}
void SkinnedMesh::setBoneRotationY(const QString &boneName, float value)
{
	uint boneId = findBoneId(boneName);
	assert(boneId < m_boneInfo.size());
	BoneInfo &bi = m_boneInfo[boneId];
	bi.yRot = value;
	QQuaternion q;
	q = QQuaternion::fromEulerAngles(bi.xRot, 0, 0);
	q *= QQuaternion::fromEulerAngles(0, bi.yRot, 0);
	q *= QQuaternion::fromEulerAngles(0, 0, bi.zRot);
	m_controlQuats[boneId] = q;
	m_controlMats[boneId] = fromRotation(q);
}
void SkinnedMesh::setBoneRotationZ(const QString &boneName, float value)
{
	uint boneId = findBoneId(boneName);
	assert(boneId < m_boneInfo.size());
	BoneInfo &bi = m_boneInfo[boneId];
	bi.zRot = value;
	QQuaternion q;
	q = QQuaternion::fromEulerAngles(bi.xRot, 0, 0);
	q *= QQuaternion::fromEulerAngles(0, bi.yRot, 0);
	q *= QQuaternion::fromEulerAngles(0, 0, bi.zRot);
	m_controlQuats[boneId] = q;
	m_controlMats[boneId] = fromRotation(q);
}
void SkinnedMesh::flipParameter(uint i)
{
	m_parameters[i].flip();
	cout << "Parameter " << i << ": " << m_parameterInfo[i] << (m_parameters[i] ? " ON" : " OFF") << endl;
}
void SkinnedMesh::printParameters() const
{
	for (uint i = 0; i < NUM_PARAMETERS; i++) {
		cout << "Parameter " << i << ": " << m_parameterInfo[i] << (m_parameters[i] ? " ON" : " OFF") << endl;
	}
}
uint SkinnedMesh::findBoneId(const QString &boneName) const
{
	const auto& it = m_boneMap.find(boneName.toStdString());
	if (it == m_boneMap.cend()) {
		cerr << "findBoneId() could not locate bone " << boneName.toStdString() << endl;
		return m_numBones;
	}
	return it->second;
}
bool SkinnedMesh::boneVisibility(uint boneIndex) const
{
	return m_boneInfo[boneIndex].visible;
}
bool SkinnedMesh::boneVisibility(const QString &boneName) const
{
	return m_boneInfo[findBoneId(boneName)].visible;
}
void SkinnedMesh::setBoneVisibility(uint boneIndex, bool state)
{
	if (boneIndex > m_boneInfo.size() || boneIndex < 0) {
		cout << "setBoneVisibility: bone index out of bounds! Index=" << boneIndex << endl;
		return;
	}
	m_boneInfo[boneIndex].visible = state;
}
void SkinnedMesh::setBoneVisibility(const QString &boneName, bool state)
{
	m_boneInfo[findBoneId(boneName)].visible = state;
}
QString SkinnedMesh::boneTransformInfo(const QString& boneName) const
{
	return m_boneTransformInfo[findBoneId(boneName)];
}
QVector<QVector3D>& SkinnedMesh::positions()
{
	return m_positions;
}
QVector<QVector3D>& SkinnedMesh::normals()
{
	return m_normals;
}
QVector<QVector2D>& SkinnedMesh::texCoords()
{
	return m_texCoords;
}
QVector<VertexBoneData>& SkinnedMesh::vertexBoneData()
{
	return m_vertexBoneData;
}
QVector<uint>& SkinnedMesh::indices()
{
	return m_indices;
}
QVector<QImage>& SkinnedMesh::images()
{
	return m_images;
}
QVector<MeshEntry>& SkinnedMesh::meshEntries()
{
	return m_meshEntries;
}
uint SkinnedMesh::numBones() const
{
	return m_numBones;
}
const map<string, uint>& SkinnedMesh::boneMap() const
{
	return m_boneMap;
}
const QMatrix4x4& SkinnedMesh::boneGlobal(uint boneIndex) const
{
	if (boneIndex > m_boneInfo.size() || boneIndex < 0) {
		cout << "boneGlobal: bone index out of bounds! Index=" << boneIndex << endl;
		return QMatrix4x4();
	}
	return m_boneInfo[boneIndex].global;
}
const QVector3D& SkinnedMesh::boneEndPosition(uint boneIndex) const
{
	if (boneIndex > m_boneInfo.size() || boneIndex < 0) {
		cout << "boneEndPosition: bone index out of bounds! Index=" << boneIndex << endl;
		return QVector3D();
	}
	return m_boneInfo[boneIndex].endPosition;
}
const BoneInfo& SkinnedMesh::boneInfo(uint boneIndex) const
{
	if (boneIndex > m_boneInfo.size() || boneIndex < 0) {
		cout << "SkinnedMesh::boneInfo: bone index out of bounds! Index=" << boneIndex << endl;
	}
	return m_boneInfo[boneIndex];
}