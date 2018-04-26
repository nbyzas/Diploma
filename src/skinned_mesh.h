#ifndef SKINNED_MESH_H
#define	SKINNED_MESH_H

// Project
#include "ksensor.h"
#include "util.h"

// Assimp
#include <assimp\Importer.hpp>      // C++ importer interface
#include <assimp\scene.h>			 // Output data structure
#include <assimp\postprocess.h>     // Post processing flags

// Qt
#include <QtCore\QVector>

// Standard C/C++
#include <map>
#include <vector>
#include <bitset>

#define ASSIMP_LOAD_FLAGS aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices  | aiProcess_LimitBoneWeights 
#define NUM_PARAMETERS 10

#define NUM_BONES_PER_VERTEX 4
struct VertexBoneData
{
	uint IDs[NUM_BONES_PER_VERTEX];
	float Weights[NUM_BONES_PER_VERTEX];
	float OldWeights[NUM_BONES_PER_VERTEX];

	VertexBoneData()
	{
		Reset();
	};

	void Reset()
	{
		ZERO_MEM(IDs);
		ZERO_MEM(Weights);
		ZERO_MEM(OldWeights); // Not used
	}

	void AddBoneData(uint BoneID, float Weight);
	void AdjustBoneData(); // Not used
	void RestoreBoneData(); // Not used
};
#define INVALID_MATERIAL 0xFFFFFFFF
struct MeshEntry {
	MeshEntry()
	{
		NumIndices = 0;
		BaseVertex = 0;
		BaseIndex = 0;
		MaterialIndex = INVALID_MATERIAL;
	}

	uint NumIndices;
	uint BaseVertex;
	uint BaseIndex;
	uint MaterialIndex;
};
struct BoneInfo
{
	QMatrix4x4 local;
	QMatrix4x4 corrected;
	QMatrix4x4 offset;
	QMatrix4x4 global;
	QMatrix4x4 combined;
	QMatrix4x4 scaling;

	bool visible;
	float xRot, yRot, zRot;
	
	QVector3D endPosition;
	float length;

	BoneInfo()
	{
		xRot = yRot = zRot = 0.f;
		visible = true;
	}
};

class SkinnedMesh
{
public:
	SkinnedMesh();
	~SkinnedMesh();	
	bool loadMesh(const string& basename);
	bool loadMotion(const QString& filename);

	void getBoneTransforms(vector<QMatrix4x4>& transforms);
	void traverseNodeHierarchy(const aiNode* pNode, const QMatrix4x4& P);
	void initCorrectedMatrices();
	void printInfo() const;
	void printNodeHierarchy(const aiNode* pNode) const;
	void printParameters() const;
	void printSceneInfo() const;
	
	bool m_successfullyLoaded;

	float boneRotationX(const QString &boneName) const;
	float boneRotationY(const QString &boneName) const;
	float boneRotationZ(const QString &boneName) const;
	void setBoneRotationX(const QString &boneName, float value);
	void setBoneRotationY(const QString &boneName, float value);
	void setBoneRotationZ(const QString &boneName, float value);
	
	uint numBones() const;
	const map<string, uint>& boneMap() const;
	const QMatrix4x4& boneGlobal(uint boneIndex) const;
	const QVector3D& boneEndPosition(uint boneIndex) const;

	uint findBoneId(const QString &boneName) const;
	bool boneVisibility(uint boneIndex) const;
	bool boneVisibility(const QString &boneName) const;
	void setBoneVisibility(uint boneIndex, bool state);
	void setBoneVisibility(const QString &boneName, bool state);
	QString boneTransformInfo(const QString& boneName) const;
	void flipParameter(uint i);

	// Get vertex attribute functions
	vector<MeshEntry>& entries();
	vector<QVector3D>& positions();
	vector<QVector3D>& normals();
	QVector<QVector2D>& texCoords();
	vector<VertexBoneData>& vertexBoneData();
	vector<uint>& indices();	
	vector<QImage>& images();

	QQuaternion pelvisRotation();
	QVector3D pelvisPosition();
	double timestamp(uint index);
	void setActiveCoordinates(uint frameIndex);
	uint sequenceSize();
	void initCorrectionVecs(const array<KLimb, NUM_LIMBS>& limbs); // correct scaling
	QVector3D getOffset();

private:
	void clear();
	void initMesh(uint MeshIndex, const aiMesh* paiMesh);
	void loadBones(uint MeshIndex, const aiMesh* paiMesh, vector<VertexBoneData>& Bones);
	bool initImages(const aiScene* pScene, const string& Filename);
	bool initFromScene(const aiScene* pScene, const string& Filename);

	// Data loaded in CPU by loadMesh		
	vector<MeshEntry> m_entries;
	// Vertex Attributes
	vector<QVector3D> m_positions;
	vector<QVector3D> m_normals;
	QVector<QVector2D> m_texCoords;
	vector<VertexBoneData> m_vertexBoneData;
	vector<uint> m_indices;
	// Bones
	vector<BoneInfo> m_boneInfo;
	// Textures
	vector<QImage> m_images;

	map<string, uint> m_boneMap; // maps a mesh's bone name to its index (key = bone name, value = index)
	map<string, uint> m_kboneMap; // maps a mesh's bone name to its kinect JointType index
	const KJoint *m_pKBones = NULL;

	// Control pose
	vector<QQuaternion> m_controlQuats; 
	vector<QMatrix4x4> m_controlMats;

	// Correct pose
	QVector<QVector3D> m_correctionVecs; // correct scaling
	QVector<QQuaternion> m_correctionQuats; // correct rotations
	vector<QMatrix4x4> m_correctionMats;

	void initCorrectionQuats();
	void initLocalMatrices(const aiNode* node);

	vector<QString> m_boneTransformInfo;
	vector<bool> m_hasCoordinates; // #?

	enum Coordinates{
		pelvis_tilt, //z
		pelvis_list, //x
		pelvis_rotation, //y
		pelvis_tx,
		pelvis_ty,
		pelvis_tz,

		hip_flexion_r,
		hip_adduction_r,
		hip_rotation_r,
		knee_angle_r,
		ankle_angle_r,
		subtalar_angle_r,
		mtp_angle_r,

		hip_flexion_l,
		hip_adduction_l,
		hip_rotation_l,
		knee_angle_l,
		ankle_angle_l,
		subtalar_angle_l,
		mtp_angle_l,

		lumbar_extension,
		lumbar_bending,
		lumbar_rotation,

		arm_flex_r,
		arm_add_r,
		arm_rot_r,
		elbow_flex_r,
		pro_sup_r,
		wrist_flex_r,
		wrist_dev_r,

		arm_flex_l,
		arm_add_l,
		arm_rot_l,
		elbow_flex_l,
		pro_sup_l,
		wrist_flex_l,
		wrist_dev_l
	};
	static const uint m_numCoordinates = 37;

	// Skinned Mesh Frame
	array<float, m_numCoordinates> m_activeCoordinates;
	QVector<array<float, m_numCoordinates>> m_coordinateSequence;
	QVector<double> m_timestamps;

	void initCoordinates();
	QQuaternion boneOrientation(uint boneIndex);

	uint m_numBones = 0; // crash if not 0
	uint m_numVertices; // total number of vertices
	unsigned long long m_vertexArrayBytes;

	const aiScene* m_pScene;
	Assimp::Importer m_Importer;

	bitset<NUM_PARAMETERS> m_parameters = bitset<NUM_PARAMETERS>().set();
	const string m_parameterInfo[NUM_PARAMETERS] = { "",  "Corrected Pose", "OpenSim Pose", "Controlled Pose" };	

	void calculateBoneLength(const aiNode* pNode);
	uint m_activeFrame = 0;
};

#endif	/* SKINNED_MESH_H */
