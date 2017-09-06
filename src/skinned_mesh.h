#ifndef SKINNED_MESH_H
#define	SKINNED_MESH_H

#include "sensor.h"
#include "texture.h"
#include "util.h"
#include "math_3d.h"
#include <map>
#include <vector>
#include <bitset>
#include <GL/glew.h>
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>			 // Output data structure
#include <assimp/postprocess.h>     // Post processing flags


#define ASSIMP_LOAD_FLAGS aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_FlipWindingOrder | aiProcess_LimitBoneWeights 
#define NUM_PARAMETERS 10

using namespace std;

class SkinnedMesh
{
public:
	SkinnedMesh();
	SkinnedMesh(const KSensor &ks);
	~SkinnedMesh();
	bool LoadMesh(const string& Filename);
	void Render();
	void BoneTransform(float TimeInSeconds, vector<Matrix4f>& Transforms);
	void GetBoneTransforms(vector<Matrix4f>& Transforms);
	void TraverseNodeHierarchy(const aiNode* pNode, const Matrix4f& P);
	void SetConQuats();
	void PrintInfo();
	void PrintNodeHierarchy(const aiNode* pNode);
	void PrintNodeMatching(const aiNode* pNode);
	void PrintParameters();
	void PrintSceneInfo();
	void KBoneMapper();
	void AdjustBones();
	void ToggleSkinning();
	void NextModel(int step);
	void NextJoint(int step);
	void FlipParameter(uint i);	
	void NextBoneTransformInfo(int step);
	bool m_Skinned;
	bool m_BindPose;
	bool m_SuccessfullyLoaded;
private:

#define NUM_BONES_PER_VERTEX 4

	struct BoneInfo
	{
		Matrix4f BoneOffset;
		Matrix4f FinalTransformation;
		bool Visible;
		bool Offset;
		bool BindPose;
		BoneInfo()
		{
			BoneOffset.SetZero();
			FinalTransformation.SetZero();
			Visible = true;
			Offset = false;
			BindPose = false;
		}
	};

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
			ZERO_MEM(OldWeights);
		}

		void AddBoneData(uint BoneID, float Weight);
		void AdjustBoneData();
		void RestoreBoneData();
	};

	void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	uint FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
	uint FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
	uint FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
	const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const string NodeName);
	void ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const Matrix4f& ParentTransform);
	bool InitFromScene(const aiScene* pScene, const string& Filename);
	void InitMesh(uint MeshIndex,
		const aiMesh* paiMesh,
		vector<Vector3f>& Positions,
		vector<Vector3f>& Normals,
		vector<Vector2f>& TexCoords,
		vector<VertexBoneData>& Bones,
		vector<uint>& Indices);
	void LoadBones(uint MeshIndex, const aiMesh* paiMesh, vector<VertexBoneData>& Bones);
	bool InitMaterials(const aiScene* pScene, const string& Filename);
	void Clear();

#define INVALID_MATERIAL 0xFFFFFFFF

	enum VB_TYPES {
		INDEX_BUFFER,
		POS_VB,
		NORMAL_VB,
		TEXCOORD_VB,
		BONE_VB,
		NUM_VBs
	};

	GLuint m_VAO;
	GLuint m_Buffers[NUM_VBs];

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

	vector<MeshEntry> m_Entries;
	vector<Texture*> m_Textures;

	map<string, uint> m_BoneMapping; // maps a model bone name to its index (key = bone name, value = index)
	map<string, uint> m_KBoneMapping; // maps a model bone name to its kinect JointType index
	map<string, uint> m_BoneFlags;
	const KJoint *m_pKBones;

	vector<Vector3f> m_relVecs; // relative vectors (for hierarchical translation)
	vector<Quaternion> m_relQuats; // relative quaternions (for hierarchical rotation)
	vector<Matrix4f> m_relMats; // relative matrices (for hierarchical transforms)
	vector<Vector3f> m_absVecs; 
	vector<Quaternion> m_absQuats; 
	vector<Matrix4f> m_absMats; 	
	vector<Vector3f> m_conVecs; // control vectors (for controlled translation)
	vector<Quaternion> m_conQuats; // control quaternions (for controlled rotation)
	vector<Matrix4f> m_conMats;
	vector<Vector3f> m_corVecs; // correction vectors
	vector<Quaternion> m_corQuats; // correction quaternions 
	vector<Matrix4f> m_corMats;
	uint m_NumBones;
	uint m_NumNodes;
	uint m_NumVertices; // total number of vertices

	vector<BoneInfo> m_BoneInfo;
	Matrix4f m_GlobalInverseTransform;

	const aiScene* m_pScene;
	Assimp::Importer m_Importer;

	std::map<std::string, uint>::iterator m_BoneMapIterator;
	uint m_ActiveModel;
	bitset<NUM_PARAMETERS> m_Parameters;
	const string m_ParametersStringTrue[NUM_PARAMETERS] = { "Invisible parts", "Composed local", "My quaternion", "My matrix", "qRel=qAbs*qAbsParInv","qAbs=qAbsPar*qRel" };
	const string m_ParametersStringFalse[NUM_PARAMETERS] = { "Visible parts", "Ready local", "AI quaternion", "AI matrix", "qRel=qAbsParInv*qAbs","qAbs=qRel*qAbsPar" };
	uint m_ActiveBoneTransformInfo;
	vector<string> m_BoneTransformInfo;
};


#endif	/* SKINNED_MESH_H */

