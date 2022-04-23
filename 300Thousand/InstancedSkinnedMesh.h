#ifndef InstancedSkinnedMesh_H
#define	InstancedSkinnedMesh_H

#include <map>
#include <vector>
#include <assert.h>
#include <GL/glew.h>
#include <assimp/Importer.hpp>      
#include <assimp/scene.h>       
#include <assimp/postprocess.h> 

#include <assimp/vector3.h>
#include <assimp/matrix3x3.h>
#include <assimp/matrix4x4.h>
#include "FreeImage.h"
#include <glm/ext/vector_float2.hpp>

#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define INVALID_MATERIAL 0xFFFFFFFF

using namespace std;

struct MeshEntry
{
    MeshEntry()
    {
        NumIndices = 0;
        BaseVertex = 0;
        BaseIndex = 0;
        MaterialIndex = INVALID_MATERIAL;
    }

    aiVector3D mBbMin, mBbMax;

    unsigned int NumIndices;
    unsigned int BaseVertex;
    unsigned int BaseIndex;
    unsigned int MaterialIndex;
};

class InstancedSkinnedMesh
{
   public:
       InstancedSkinnedMesh();
       ~InstancedSkinnedMesh();

       bool LoadMesh(const string& Filename);

       void Update(float deltaSeconds, int animationIndex = -1);
       void UpdateFrame(int frameNumber, int bits, int animationIndex = 0);
       void Render();
       void RenderInstanced(int instanceCount);
	
       unsigned int NumBones() const {return m_NumBones;}
    
       void BoneTransform(float TimeInSeconds, vector<aiMatrix4x4>& Transforms, int animationIndex);
       void BoneTransformFrame(int frame_number, vector<aiMatrix4x4>& Transforms, int bits, int animationIndex);
       void generateAnimTextures(unsigned int height, unsigned int width, int bits);
       void setCurrentAnimationIndex(int animationIndex);
       int getCurrentAnimationIndexFrames();
       
       FIBITMAP* m_img;
       vector<MeshEntry> m_Entries;
       GLuint m_VAO;
    
   private:
       const static int NUM_BONES_PER_VERTEX = 4;

       struct BoneInfo
       {
           aiMatrix4x4 BoneOffset;
           aiMatrix4x4 FinalTransformation;        

           BoneInfo()
           {
               aiMatrix4x4::Scaling(aiVector3D(0.0f), BoneOffset);
               aiMatrix4x4::Scaling(aiVector3D(0.0f), FinalTransformation);            
           }
       };
    
       struct VertexBoneData
       {        
           unsigned char IDs[NUM_BONES_PER_VERTEX];
           float Weights[NUM_BONES_PER_VERTEX];

           VertexBoneData()
           {
               memset(IDs, 0, sizeof(IDs));
               memset(Weights, 0, sizeof(Weights));
           };
        
           void AddBoneData(unsigned int BoneID, float Weight);
       };

       void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
       void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
       void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);    
       unsigned int FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
       unsigned int FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
       unsigned int FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
       const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const string& NodeName);
       void ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const aiMatrix4x4& ParentTransform, int animationIndex);
       bool InitFromScene(const aiScene* pScene, const string& Filename);
       void InitMesh(unsigned int MeshIndex,
                     const aiMesh* paiMesh,
                     vector<aiVector3D>& Positions,
                     vector<aiVector3D>& Normals,
                     vector<aiVector2D>& TexCoords,
                     vector<VertexBoneData>& Bones,
                     vector<unsigned int>& Indices);
       void LoadBones(unsigned int MeshIndex, const aiMesh* paiMesh, vector<VertexBoneData>& Bones);
       bool InitMaterials(const aiScene* pScene, const string& Filename);
       void Clear();
       int setMatrixInImage(aiMatrix4x4 boneTransform, FIBITMAP* img, int height, int width, unsigned int& currentX, unsigned int& currentY, int bits);
       void getRowAsColor(float elem1, float elem2, float elem3, float elem4, RGBQUAD & color, int bits);
       void getRowAsColor(float elem1, float elem2, float elem3, float elem4, FIRGBAF& vector, int bits);

       int setColorAsRow(float elem1, float elem2, float elem3, float elem4, FIBITMAP* img, int height, int width, unsigned int& currentX, unsigned int& currentY, int bits);
       int getCellIndex(int frame_number, int bone_id, int row);
       glm::vec2 getTexCoord(int frame_number, int bone_id, int row);
       void getPixel128bit(FIBITMAP* img, int x, int y, FIRGBAF & pixel);
       void generateAnimTexture(unsigned int height, unsigned int width, int bits, int animationIndex);
       glm::vec3* createMatPosInstanceArray(int instanceCount);
       GLuint createMatPosVBO(int instanceCount);

   
  
      enum VB_TYPES 
      {
          INDEX_BUFFER,
          POS_VB,
          NORMAL_VB,
          TEXCOORD_VB,
          BONE_VB,
          NUM_VBs            
      };

      GLuint m_Buffers[NUM_VBs];

      

      static const unsigned int MAX_BONES = 100;

      vector<GLuint> m_Textures;

      vector<GLuint> animTextures;
     
      map<string, unsigned int> m_BoneMapping; // maps a bone name to its index
      unsigned int m_NumBones;
      unsigned int m_currentAnimationIndex;
      vector<BoneInfo> m_BoneInfo;
      aiMatrix4x4 m_GlobalInverseTransform;
    
      const aiScene* m_pScene;
      Assimp::Importer m_Importer;

      unsigned int animTexHeight;
      unsigned int animTexWidth;
      int m_currentAnimationFrames;
      vector<int> textureBindValues;
};


#endif	/* InstancedSkinnedMesh_H */

