// Based on the tutorial at
// https://ogldev.org/www/tutorial38/tutorial38.html


//Modifications
// Speed up FindAnimNode with std::map
// Use aiProcess_LimitBoneWeights when importing, to limit bones per vertex to 4
// Use unsigned byte for bone IDs
// Use layout qualifiers for attribs and uniforms
// Eliminate SetBoneTransform() - send all matrices in one glUniform call
// Pass strings by reference

#include <assert.h>

#include "InstancedSkinnedMesh.h"
#include "LoadTexture.h"
#include "ShaderLocs.h"
#include <iostream>
#include "Constants.hpp"


void InstancedSkinnedMesh::VertexBoneData::AddBoneData(unsigned int BoneID, float Weight)
{
   for (unsigned int i = 0 ; i < NUM_BONES_PER_VERTEX ; i++) 
   {
      if (Weights[i] == 0.0) 
      {
         IDs[i]     = BoneID;
         Weights[i] = Weight;
         return;
      }        
   }
    
   // should never get here - more bones than we have space for
   printf("more bones than we have space for, %d, %d\n", BoneID, this);
   assert(0);
}

InstancedSkinnedMesh::InstancedSkinnedMesh()
{
   m_VAO = 0;
   memset(m_Buffers, 0, sizeof(m_Buffers));
   m_NumBones = 0;
   m_pScene = NULL;
}


InstancedSkinnedMesh::~InstancedSkinnedMesh()
{
   Clear();
}


void InstancedSkinnedMesh::Clear()
{

   for (unsigned int i = 0 ; i < m_Textures.size() ; i++) 
   {
      if(m_Textures[i])
      {
         glDeleteTextures(	1, &m_Textures[i]);
      }
   }

   if (m_Buffers[0] != 0) 
   {
      glDeleteBuffers(NUM_VBs, m_Buffers);
   }
       
   if (m_VAO != 0) 
   {
      glDeleteVertexArrays(1, &m_VAO);
      m_VAO = 0;
   }
}


bool InstancedSkinnedMesh::LoadMesh(const string& Filename)
{
   // Release the previously loaded mesh (if it exists)
   Clear();
 
   // Create the VAO
   glGenVertexArrays(1, &m_VAO);   
   glBindVertexArray(m_VAO);
    
   // Create the buffers for the vertices attributes
   glGenBuffers(NUM_VBs, m_Buffers);

   bool ret = false;    
  
   //aiProcessPreset_TargetRealtime_Quality includes aiProcess_LimitBoneWeights which restricts bones per vertex to 4
   m_pScene = m_Importer.ReadFile(Filename.c_str(), aiProcessPreset_TargetRealtime_Quality | aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);
    
   if (m_pScene) 
   {  
      m_GlobalInverseTransform = m_pScene->mRootNode->mTransformation;
      m_GlobalInverseTransform.Inverse();
      ret = InitFromScene(m_pScene, Filename);
      
   }
   else 
   {
      printf("Error parsing '%s': '%s'\n", Filename.c_str(), m_Importer.GetErrorString());
   }

   // Make sure the VAO is not changed from the outside
   glBindVertexArray(0);	

   return ret;
}

void InstancedSkinnedMesh::Update(float deltaSeconds, int animationIndex)
{
    m_currentAnimationIndex = animationIndex;
    glUniform1i(UniformLoc::AnimationIndex, m_currentAnimationIndex);
    glUniform1i(UniformLoc::NumBones, m_NumBones);

    static vector<aiMatrix4x4> Transforms;
    BoneTransform(deltaSeconds, Transforms, animationIndex);
    glUniformMatrix4fv(UniformLoc::Bones, Transforms.size(), GL_TRUE, &Transforms[0].a1);     
}

void InstancedSkinnedMesh::UpdateFrame(int frameNumber, int bits, int animationIndex)
{
    m_currentAnimationIndex = animationIndex;
    glUniform1i(UniformLoc::AnimationIndex, m_currentAnimationIndex);
    glUniform1i(UniformLoc::NumBones, m_NumBones);

    /*static vector<aiMatrix4x4> Transforms;
    BoneTransformFrame(frameNumber, Transforms, bits);
    glUniformMatrix4fv(UniformLoc::Bones, Transforms.size(), GL_TRUE, &Transforms[0].a1);*/
}


void GetBoundingBox(const aiMesh* mesh, aiVector3D* min, aiVector3D* max)
{
    min->x = min->y = min->z = 1e10f;
    max->x = max->y = max->z = -1e10f;

    for (unsigned int t = 0; t < mesh->mNumVertices; ++t)
    {
        aiVector3D tmp = mesh->mVertices[t];

        min->x = std::min(min->x, tmp.x);
        min->y = std::min(min->y, tmp.y);
        min->z = std::min(min->z, tmp.z);

        max->x = std::max(max->x, tmp.x);
        max->y = std::max(max->y, tmp.y);
        max->z = std::max(max->z, tmp.z);
    }
}

bool InstancedSkinnedMesh::InitFromScene(const aiScene* pScene, const string& Filename)
{  
    m_Entries.resize(pScene->mNumMeshes);
    m_Textures.resize(pScene->mNumMaterials);

    vector<aiVector3D> Positions;
    vector<aiVector3D> Normals;
    vector<aiVector2D> TexCoords;
    vector<VertexBoneData> Bones;
    vector<unsigned int> Indices;
       
    unsigned int NumVertices = 0;
    unsigned int NumIndices = 0;
    
   // Count the number of vertices and indices
   for (unsigned int i = 0 ; i < m_Entries.size() ; i++) 
   {
      m_Entries[i].MaterialIndex = pScene->mMeshes[i]->mMaterialIndex;        
      m_Entries[i].NumIndices    = pScene->mMeshes[i]->mNumFaces * 3;
      m_Entries[i].BaseVertex    = NumVertices;
      m_Entries[i].BaseIndex     = NumIndices;
        
      NumVertices += pScene->mMeshes[i]->mNumVertices;
      NumIndices  += m_Entries[i].NumIndices;
   }
    
    // Reserve space in the vectors for the vertex attributes and indices
    Positions.reserve(NumVertices);
    Normals.reserve(NumVertices);
    TexCoords.reserve(NumVertices);
    Bones.resize(NumVertices);
    Indices.reserve(NumIndices);
        
   // Initialize the meshes in the scene one by one
   for (unsigned int i = 0 ; i < m_Entries.size() ; i++) 
   {
      const aiMesh* pMesh = pScene->mMeshes[i];
      InitMesh(i, pMesh, Positions, Normals, TexCoords, Bones, Indices);

      // get bounding box of each mesh
      GetBoundingBox(pScene->mMeshes[i], &m_Entries[i].mBbMin, &m_Entries[i].mBbMax);
      /*std::cout << m_Entries[i].mBbMin.x << ", " << m_Entries[i].mBbMax.x << std::endl;
      std::cout << m_Entries[i].mBbMin.y << ", " << m_Entries[i].mBbMax.y << std::endl;
      std::cout << m_Entries[i].mBbMin.z << ", " << m_Entries[i].mBbMax.z << std::endl;*/
   }

   if (!InitMaterials(pScene, Filename)) 
   {
      return false;
   }

   // Generate and populate the buffers with vertex attributes and the indices
   glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[POS_VB]);
   glBufferData(GL_ARRAY_BUFFER, sizeof(Positions[0]) * Positions.size(), &Positions[0], GL_STATIC_DRAW);
   glEnableVertexAttribArray(AttribLoc::Pos);
   glVertexAttribPointer(AttribLoc::Pos, 3, GL_FLOAT, GL_FALSE, 0, 0);    

   glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[TEXCOORD_VB]);
   glBufferData(GL_ARRAY_BUFFER, sizeof(TexCoords[0]) * TexCoords.size(), &TexCoords[0], GL_STATIC_DRAW);
   glEnableVertexAttribArray(AttribLoc::TexCoord);
   glVertexAttribPointer(AttribLoc::TexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[NORMAL_VB]);
   glBufferData(GL_ARRAY_BUFFER, sizeof(Normals[0]) * Normals.size(), &Normals[0], GL_STATIC_DRAW);
   glEnableVertexAttribArray(AttribLoc::Normal);
   glVertexAttribPointer(AttribLoc::Normal, 3, GL_FLOAT, GL_FALSE, 0, 0);

   glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[BONE_VB]);
   glBufferData(GL_ARRAY_BUFFER, sizeof(Bones[0]) * Bones.size(), &Bones[0], GL_STATIC_DRAW);
   glEnableVertexAttribArray(AttribLoc::BoneIds);
   glVertexAttribIPointer(AttribLoc::BoneIds, 4, GL_UNSIGNED_BYTE, sizeof(VertexBoneData), (const GLvoid*)0);
   glEnableVertexAttribArray(AttribLoc::BoneWeights);    
   glVertexAttribPointer(AttribLoc::BoneWeights, 4, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData), (const GLvoid*)(NUM_BONES_PER_VERTEX*sizeof(unsigned char)));
    
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[INDEX_BUFFER]);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices[0]) * Indices.size(), &Indices[0], GL_STATIC_DRAW);

   /*GLuint mat_pos_buffer = createMatPosVBO(INSTANCE_NUM);

   glBindBuffer(GL_ARRAY_BUFFER, mat_pos_buffer);

   glVertexAttribPointer(AttribLoc::matPosInstance, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), NULL);
   glEnableVertexAttribArray(AttribLoc::matPosInstance);
   glVertexAttribDivisor(AttribLoc::matPosInstance, 1);*/

   return true;
}

void InstancedSkinnedMesh::InitMesh(unsigned int MeshIndex,
                    const aiMesh* pMesh,
                    vector<aiVector3D>& Positions,
                    vector<aiVector3D>& Normals,
                    vector<aiVector2D>& TexCoords,
                    vector<VertexBoneData>& Bones,
                    vector<unsigned int>& Indices)
{    
   const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

   
    
   // Populate the vertex attribute vectors
   for (unsigned int i = 0 ; i < pMesh->mNumVertices ; i++) 
   {
      const aiVector3D* pPos      = &(pMesh->mVertices[i]);
      const aiVector3D* pNormal   = &(pMesh->mNormals[i]);
      const aiVector3D* pTexCoord = pMesh->HasTextureCoords(0) ? &(pMesh->mTextureCoords[0][i]) : &Zero3D;

      Positions.push_back(*pPos);
      Normals.push_back(*pNormal);
      TexCoords.push_back(aiVector2D(pTexCoord->x, pTexCoord->y));      
   }
    
   LoadBones(MeshIndex, pMesh, Bones);
    
   // Populate the index buffer
   for (unsigned int i = 0 ; i < pMesh->mNumFaces ; i++) 
   {
      const aiFace& Face = pMesh->mFaces[i];
      assert(Face.mNumIndices == 3);
      Indices.push_back(Face.mIndices[0]);
      Indices.push_back(Face.mIndices[1]);
      Indices.push_back(Face.mIndices[2]);
   }

    //textureBindValues.resize(m_pScene->mNumAnimations);
    textureBindValues.push_back(GL_TEXTURE1);
    textureBindValues.push_back(GL_TEXTURE2);
    textureBindValues.push_back(GL_TEXTURE3);
    textureBindValues.push_back(GL_TEXTURE4);
    textureBindValues.push_back(GL_TEXTURE5);
}


void InstancedSkinnedMesh::LoadBones(unsigned int MeshIndex, const aiMesh* pMesh, vector<VertexBoneData>& Bones)
{
   for (unsigned int i = 0 ; i < pMesh->mNumBones ; i++) 
   {                
      unsigned int BoneIndex = 0;        
      string BoneName(pMesh->mBones[i]->mName.data);
        
      auto iter = m_BoneMapping.find(BoneName);
      if (iter == m_BoneMapping.end()) 
      {
         // Allocate an index for a new bone
         BoneIndex = m_NumBones;
         m_NumBones++;            
         BoneInfo bi;			
         m_BoneInfo.push_back(bi);
         m_BoneInfo[BoneIndex].BoneOffset = pMesh->mBones[i]->mOffsetMatrix;            
         m_BoneMapping[BoneName] = BoneIndex;
      }
      else 
      {
         BoneIndex = iter->second;
      }                      
        
      for (unsigned int j = 0 ; j < pMesh->mBones[i]->mNumWeights ; j++) 
      {
         unsigned int VertexID = m_Entries[MeshIndex].BaseVertex + pMesh->mBones[i]->mWeights[j].mVertexId;
         float Weight  = pMesh->mBones[i]->mWeights[j].mWeight;                   
         Bones[VertexID].AddBoneData(BoneIndex, Weight);
      }
   }    
}


bool InstancedSkinnedMesh::InitMaterials(const aiScene* pScene, const string& Filename)
{
   // Extract the directory part from the file name
   string::size_type SlashIndex = Filename.find_last_of("/");
   string Dir;

   if (SlashIndex == string::npos) 
   {
      Dir = ".";
   }
   else if (SlashIndex == 0) 
   {
      Dir = "/";
   }
   else 
   {
      Dir = Filename.substr(0, SlashIndex);
   }

   bool Ret = true;

   // Initialize the materials
   for (unsigned int i = 0 ; i < pScene->mNumMaterials ; i++) 
   {
      const aiMaterial* pMaterial = pScene->mMaterials[i];

      m_Textures[i] = NULL;

      aiColor4D color (0.f,0.f,0.f,0.0f);
      aiGetMaterialColor(pMaterial,AI_MATKEY_COLOR_DIFFUSE,&color);

      if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) 
      {
         aiString Path;

         if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) 
         {
            string p(Path.data);
                
            if (p.substr(0, 2) == ".\\") 
            {                    
               p = p.substr(2, p.size() - 2);
            }
                               
            string FullPath = Dir + "/" + p;
                    
            m_Textures[i] = LoadTexture(FullPath.c_str());
            
         }
      }
   }

   return Ret;
}


void InstancedSkinnedMesh::Render()
{
   //glBindVertexArray(m_VAO);

   for (int i = 0; i < m_pScene->mNumAnimations; i++) {
       glActiveTexture(textureBindValues[i]);
       glBindTexture(GL_TEXTURE_2D, animTextures[i]);
   }

   for (unsigned int i = 0 ; i < m_Entries.size() ; i++) 
   {
      const unsigned int MaterialIndex = m_Entries[i].MaterialIndex;
      
      assert(MaterialIndex < m_Textures.size());
        
      if (m_Textures[MaterialIndex]) 
      {
         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D, m_Textures[MaterialIndex]);
      }

      glDrawElementsBaseVertex(GL_TRIANGLES, 
         m_Entries[i].NumIndices, 
         GL_UNSIGNED_INT, 
         (void*)(sizeof(unsigned int) * m_Entries[i].BaseIndex), 
         m_Entries[i].BaseVertex);
   }

   // Make sure the VAO is not changed from the outside    
   //glBindVertexArray(0);
}

glm::vec3* InstancedSkinnedMesh::createMatPosInstanceArray(int instanceCount)
{
    glm::vec3* matPos = new glm::vec3[instanceCount];

    int rows = (int)std::sqrt(instanceCount);

    for (int i = 0; i < instanceCount; i++)
    {
        glm::vec3 tran = glm::vec3(((i % rows) - 1) * 10, ((i / rows) - 1) * 10, 0);
        matPos[i] = tran;// glm::vec4((i % rows) * 0.3f, (i / rows) * 0.3f, i / (float)instanceCount, 1);
    }

    return matPos;
}

GLuint InstancedSkinnedMesh::createMatPosVBO(int instanceCount) {
    glm::vec3* matPos = createMatPosInstanceArray(instanceCount);

    GLuint instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * instanceCount, &matPos[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return instanceVBO;
}

void InstancedSkinnedMesh::RenderInstanced(int instanceCount)
{
    //glBindVertexArray(m_VAO);

    for (int i = 0; i < m_pScene->mNumAnimations; i++) {
        glActiveTexture(textureBindValues[i]);
        glBindTexture(GL_TEXTURE_2D, animTextures[i]);
    }

    for (unsigned int i = 0; i < m_Entries.size(); i++)
    {
        const unsigned int MaterialIndex = m_Entries[i].MaterialIndex;

        assert(MaterialIndex < m_Textures.size());

        if (m_Textures[MaterialIndex])
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_Textures[MaterialIndex]);
        }

        glDrawElementsInstancedBaseVertex(GL_TRIANGLES,
            m_Entries[i].NumIndices,
            GL_UNSIGNED_INT,
            (void*)(sizeof(unsigned int) * m_Entries[i].BaseIndex), 
            instanceCount,
            m_Entries[i].BaseVertex);
    }

    // Make sure the VAO is not changed from the outside    
    //glBindVertexArray(0);
}

unsigned int InstancedSkinnedMesh::FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
{    
   for (unsigned int i = 0 ; i < pNodeAnim->mNumPositionKeys - 1 ; i++) 
   {
      if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) 
      {
         return i;
      }
   }
    
   assert(0);

   return 0;
}


unsigned int InstancedSkinnedMesh::FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
   assert(pNodeAnim->mNumRotationKeys > 0);

   for (unsigned int i = 0 ; i < pNodeAnim->mNumRotationKeys - 1 ; i++) 
   {
      if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime) 
      {
         return i;
      }
   }
    
   assert(0);

   return 0;
}


unsigned int InstancedSkinnedMesh::FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
   assert(pNodeAnim->mNumScalingKeys > 0);
    
   for (unsigned int i = 0 ; i < pNodeAnim->mNumScalingKeys - 1 ; i++) 
   {
      if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) 
      {
         return i;
      }
   }
    
   assert(0);

   return 0;
}


void InstancedSkinnedMesh::CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
   if (pNodeAnim->mNumPositionKeys == 1) 
   {
      Out = pNodeAnim->mPositionKeys[0].mValue;
      return;
   }
            
   const unsigned int PositionIndex = FindPosition(AnimationTime, pNodeAnim);
   const unsigned int NextPositionIndex = (PositionIndex + 1);
   assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
   const float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime);
   const float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
   //assert(Factor >= 0.0f && Factor <= 1.0f);
   const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
   const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
   const aiVector3D Delta = End - Start;
   Out = Start + Factor * Delta;
}


void InstancedSkinnedMesh::CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
   // we need at least two values to interpolate...
   if (pNodeAnim->mNumRotationKeys == 1) 
   {
      Out = pNodeAnim->mRotationKeys[0].mValue;
      return;
   }
    
   const unsigned int RotationIndex = FindRotation(AnimationTime, pNodeAnim);
   const unsigned int NextRotationIndex = (RotationIndex + 1);
   assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
   const float DeltaTime = (float)(pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime);
   const float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
   //assert(Factor >= 0.0f && Factor <= 1.0f);
   const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
   const aiQuaternion& EndRotationQ   = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;    
   aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
   Out = Out.Normalize();
}


void InstancedSkinnedMesh::CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
   if (pNodeAnim->mNumScalingKeys == 1) 
   {
      Out = pNodeAnim->mScalingKeys[0].mValue;
      return;
   }

   const unsigned int ScalingIndex = FindScaling(AnimationTime, pNodeAnim);
   const unsigned int NextScalingIndex = (ScalingIndex + 1);
   assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
   const float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime);
   const float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
   //assert(Factor >= 0.0f && Factor <= 1.0f);
   const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
   const aiVector3D& End   = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
   const aiVector3D Delta = End - Start;
   Out = Start + Factor * Delta;
}


void InstancedSkinnedMesh::ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const aiMatrix4x4& ParentTransform, int animationIndex)
{    
   const string NodeName(pNode->mName.data);
    
   const aiAnimation* pAnimation = m_pScene->mAnimations[animationIndex];
        
   aiMatrix4x4 NodeTransformation(pNode->mTransformation);
     
   const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName);
    
   if (pNodeAnim) 
   {
      // Interpolate scaling and generate scaling transformation matrix
      aiVector3D Scaling;
      CalcInterpolatedScaling(Scaling, AnimationTime, pNodeAnim);
      aiMatrix4x4 ScalingM;
      aiMatrix4x4::Scaling(Scaling, ScalingM);
        
      // Interpolate rotation and generate rotation transformation matrix
      aiQuaternion RotationQ;
      CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim);        
      aiMatrix4x4 RotationM = aiMatrix4x4(RotationQ.GetMatrix());

      // Interpolate translation and generate translation transformation matrix
      aiVector3D Translation;
      CalcInterpolatedPosition(Translation, AnimationTime, pNodeAnim);
      aiMatrix4x4 TranslationM;
      aiMatrix4x4::Translation(Translation, TranslationM);
        
      // Combine the above transformations
      NodeTransformation = TranslationM * RotationM * ScalingM;
   }
       
   aiMatrix4x4 GlobalTransformation = ParentTransform * NodeTransformation;
    
   auto iter = m_BoneMapping.find(NodeName);
   if (iter != m_BoneMapping.end()) 
   {
      const unsigned int BoneIndex = iter->second;
      m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform * GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;
   }
    
   for (unsigned int i = 0 ; i < pNode->mNumChildren ; i++) 
   {
      ReadNodeHierarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation, animationIndex);
   }
}

void InstancedSkinnedMesh::BoneTransform(float TimeInSeconds, vector<aiMatrix4x4>& Transforms, int animationIndex)
{
   aiMatrix4x4 Identity;
   
   if(m_pScene->mNumAnimations < (animationIndex + 1))
   {
      return;
   }

   float TicksPerSecond = (float)(m_pScene->mAnimations[animationIndex]->mTicksPerSecond != 0 ? m_pScene->mAnimations[animationIndex]->mTicksPerSecond : 25.0f);
   float TimeInTicks = TimeInSeconds * TicksPerSecond;
   float AnimationTime = fmod(TimeInTicks, (float)m_pScene->mAnimations[animationIndex]->mDuration);

   ReadNodeHierarchy(AnimationTime, m_pScene->mRootNode, Identity, animationIndex);

   //cout << "NumBones " << m_NumBones << std::endl;
   Transforms.resize(m_NumBones);

   for (unsigned int i = 0; i < m_NumBones; i++)
   {
      Transforms[i] = m_BoneInfo[i].FinalTransformation;
   }

}

int InstancedSkinnedMesh::getCellIndex(int frame_number, int bone_id, int row) {
    return (frame_number * m_NumBones * 4) + (bone_id * 4) + row;
}

glm::vec2 InstancedSkinnedMesh::getTexCoord(int frame_number, int bone_id, int row) {
    int cellIndex = getCellIndex(frame_number, bone_id, row);

    float y = float(cellIndex / animTexWidth);
    float x = cellIndex % animTexWidth;

    return glm::vec2(x, y);
}

void InstancedSkinnedMesh::getPixel128bit(FIBITMAP * img, int x, int y, FIRGBAF & pixel) {
    FIRGBAF* bits = (FIRGBAF*)FreeImage_GetScanLine(img, y);

    pixel = bits[x];
}

void InstancedSkinnedMesh::BoneTransformFrame(int frame_number, vector<aiMatrix4x4>& Transforms, int bits, int animationIndex)
{
    aiMatrix4x4 Identity;

    if (m_pScene->mNumAnimations < (animationIndex + 1))
    {
        return;
    }

    Transforms.resize(m_NumBones);

    float row11, row12, row13, row14, row21, row22, row23, row24, row31, row32, row33, row34, row41, row42, row43, row44;

    for (unsigned int i = 0; i < m_NumBones; i++)
    {
        if (bits == 32) 
        {
            RGBQUAD pixelRow1;
            RGBQUAD pixelRow2;
            RGBQUAD pixelRow3;
            RGBQUAD pixelRow4;

            glm::vec2 row1TexCoord = getTexCoord(frame_number, i, 0);
            glm::vec2 row2TexCoord = getTexCoord(frame_number, i, 1);
            glm::vec2 row3TexCoord = getTexCoord(frame_number, i, 2);
            glm::vec2 row4TexCoord = getTexCoord(frame_number, i, 3);
            FreeImage_GetPixelColor(m_img, row1TexCoord.x, row1TexCoord.y, &pixelRow1);
            FreeImage_GetPixelColor(m_img, row2TexCoord.x, row2TexCoord.y, &pixelRow2);
            FreeImage_GetPixelColor(m_img, row3TexCoord.x, row3TexCoord.y, &pixelRow3);
            FreeImage_GetPixelColor(m_img, row4TexCoord.x, row4TexCoord.y, &pixelRow4);
            float bitDivisor = pow(2, (bits / 4)) - 1;
            row11 = (int)pixelRow1.rgbRed / bitDivisor;
            row12 = (int)pixelRow1.rgbGreen / bitDivisor;
            row13 = (int)pixelRow1.rgbBlue / bitDivisor;
            row14 = (int)pixelRow1.rgbReserved / bitDivisor;

            row21 = (int)pixelRow2.rgbRed / bitDivisor;
            row22 = ((int)pixelRow2.rgbGreen) / bitDivisor;
            row23 = (int)pixelRow2.rgbBlue / bitDivisor;
            row24 = (int)pixelRow2.rgbReserved / bitDivisor;

            row31 = (int)pixelRow3.rgbRed / bitDivisor;
            row32 = (int)pixelRow3.rgbGreen / bitDivisor;
            row33 = (int)pixelRow3.rgbBlue / bitDivisor;
            row34 = (int)pixelRow3.rgbReserved / bitDivisor;

            row41 = (int)pixelRow4.rgbRed / bitDivisor;
            row42 = (int)pixelRow4.rgbGreen / bitDivisor;
            row43 = (int)pixelRow4.rgbBlue / bitDivisor;
            row44 = (int)pixelRow4.rgbReserved / bitDivisor;
        }
        else if(bits == 128) 
        {
            FIRGBAF pixelRow1;
            FIRGBAF pixelRow2;
            FIRGBAF pixelRow3;
            FIRGBAF pixelRow4;

            glm::vec2 row1TexCoord = getTexCoord(frame_number, i, 0);
            glm::vec2 row2TexCoord = getTexCoord(frame_number, i, 1);
            glm::vec2 row3TexCoord = getTexCoord(frame_number, i, 2);
            glm::vec2 row4TexCoord = getTexCoord(frame_number, i, 3);

            getPixel128bit(m_img, row1TexCoord.x, row1TexCoord.y, pixelRow1);
            getPixel128bit(m_img, row2TexCoord.x, row2TexCoord.y, pixelRow2);
            getPixel128bit(m_img, row3TexCoord.x, row3TexCoord.y, pixelRow3);
            getPixel128bit(m_img, row4TexCoord.x, row4TexCoord.y, pixelRow4);
            float bitDivisor = 1.0f;// pow(2, (bits / 4)) - 1;
            row11 = pixelRow1.red / bitDivisor;
            row12 = pixelRow1.green / bitDivisor;
            row13 = pixelRow1.blue / bitDivisor;
            row14 = pixelRow1.alpha / bitDivisor;

            row21 = pixelRow2.red / bitDivisor;
            row22 = pixelRow2.green / bitDivisor;
            row23 = pixelRow2.blue / bitDivisor;
            row24 = pixelRow2.alpha / bitDivisor;

            row31 = pixelRow3.red / bitDivisor;
            row32 = pixelRow3.green / bitDivisor;
            row33 = pixelRow3.blue / bitDivisor;
            row34 = pixelRow3.alpha / bitDivisor;

            row41 = pixelRow4.red / bitDivisor;
            row42 = pixelRow4.green / bitDivisor;
            row43 = pixelRow4.blue / bitDivisor;
            row44 = pixelRow4.alpha / bitDivisor;
        }
        
        aiMatrix4x4 newMat = aiMatrix4x4(row11, row12, row13, row14,
            row21, row22, row23, row24,
            row31, row32, row33, row34,
            row41, row42, row43, row44);

        Transforms[i] = newMat;
    }
}


const aiNodeAnim* InstancedSkinnedMesh::FindNodeAnim(const aiAnimation* pAnimation, const string& NodeName)
{
   //avoid search by using map
   static std::map<std::string, aiNodeAnim*> node_map;
   
   auto it = node_map.find(NodeName);
   if(it != node_map.end())
   {
      return it->second;
   }

   for (unsigned int i = 0 ; i < pAnimation->mNumChannels ; i++) 
   {
      aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];
        
      if (string(pNodeAnim->mNodeName.data) == NodeName) 
      {
         //insert in map, so next Find will be fast
         node_map[NodeName] = pNodeAnim;
         return pNodeAnim;
      }
   }
   node_map[NodeName] = NULL; 
   return NULL;
}

void InstancedSkinnedMesh::getRowAsColor(float elem1, float elem2, float elem3, float elem4, RGBQUAD &vector, int bits) {
    float multiplier = pow(2, (bits / 4)) - 1;
    vector.rgbRed = elem1 * multiplier;
    vector.rgbGreen = elem2 * multiplier;
    vector.rgbBlue = elem3 * multiplier;
    vector.rgbReserved = elem4 * multiplier;
}

void InstancedSkinnedMesh::getRowAsColor(float elem1, float elem2, float elem3, float elem4, FIRGBAF& vector, int bits) {
    float multiplier = pow(2, (bits / 4)) - 1;
    vector.red = elem1 * multiplier;
    vector.green = elem2 * multiplier;
    vector.blue = elem3 * multiplier;
    vector.alpha = elem4 * multiplier;
}

int InstancedSkinnedMesh::setColorAsRow(float elem1, float elem2, float elem3, float elem4, FIBITMAP* img, int height, int width, unsigned int& currentX, unsigned int& currentY, int bits) {
    if (bits == 32) {
        RGBQUAD color;
        getRowAsColor(elem1, elem2, elem3, elem4, color, bits);
        FreeImage_SetPixelColor(img, currentX, currentY, &color);
    }
    else if (bits == 128) {
        FIRGBAF* color = (FIRGBAF*)FreeImage_GetScanLine(img, currentY);
        //float multiplier = pow(2, (bits / 4)) - 1;
        color[currentX].red = elem1;// *multiplier;
        color[currentX].green = elem2;// *multiplier;
        color[currentX].blue = elem3;// *multiplier;
        color[currentX].alpha = elem4;// *multiplier;
    }
    
    currentX += 1;
    if (currentX >= width) {
        currentY += 1;
        currentX = 0;
    }
    if (currentY >= height) {
        return 0;
    }

    return 1;
}

int InstancedSkinnedMesh::setMatrixInImage(aiMatrix4x4 boneTransform, FIBITMAP * img, int height, int width, unsigned int & currentX, unsigned int & currentY, int bits) {
    if (setColorAsRow(boneTransform.a1, boneTransform.a2, boneTransform.a3, boneTransform.a4, img, height, width, currentX, currentY, bits)) {
        if (setColorAsRow(boneTransform.b1, boneTransform.b2, boneTransform.b3, boneTransform.b4, img, height, width, currentX, currentY, bits)) {
            if (setColorAsRow(boneTransform.c1, boneTransform.c2, boneTransform.c3, boneTransform.c4, img, height, width, currentX, currentY, bits)) {
                if (setColorAsRow(boneTransform.d1, boneTransform.d2, boneTransform.d3, boneTransform.d4, img, height, width, currentX, currentY, bits)) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

void InstancedSkinnedMesh::generateAnimTextures(unsigned int height, unsigned int width, int bits) {
    FreeImage_Initialise();
    animTexWidth = width;
    animTexHeight = height;

    int animationCount = m_pScene->mNumAnimations;
    for (int i = 0; i < animationCount; i++) {
        generateAnimTexture(height, width, bits, i);
    }
}

void InstancedSkinnedMesh::generateAnimTexture(unsigned int height, unsigned int width, int bits, int animationIndex) {
    float deltaTimeIncrements = 0.01666f;

    float currentTime = 0;

    float TicksPerSecond = (float)(m_pScene->mAnimations[animationIndex]->mTicksPerSecond != 0 ? m_pScene->mAnimations[animationIndex]->mTicksPerSecond : 25.0f);
    float animationTime = (float)m_pScene->mAnimations[animationIndex]->mDuration / TicksPerSecond;

    unsigned int currentRow = 0;
    unsigned int currentColumn = 0;

    FIBITMAP * img = FreeImage_AllocateT(((bits == 128)? FIT_RGBAF: FIT_BITMAP), width, height, bits);
    m_img = img;
    
    bool lastFrameAdded = false;
    while (currentTime < animationTime) {
        //cout << "Current time " << currentTime << " animationTime " << animationTime << std::endl;
        vector<aiMatrix4x4> Transforms;
        BoneTransform(currentTime, Transforms, animationIndex);

        for (int i = 0; i < m_NumBones; i++) {
            if (!setMatrixInImage(Transforms[i], img, height, width, currentColumn, currentRow, bits)) {
                cout << "Out of space in texture row=" << currentRow << " column= " << currentColumn << std::endl;
                return;
            }
        }

        currentTime += deltaTimeIncrements;
        if (currentTime > animationTime && !lastFrameAdded) {
            currentTime = animationTime - 0.001f;
            lastFrameAdded = true;
        }
        m_currentAnimationFrames += 1;
    }

    cout << "Total frames encoded " << m_currentAnimationFrames << " for bones " << m_NumBones << std::endl;
    
    //FreeImage_Save(FIF_PNG, img, "test.png", 0);
    animTextures.push_back(createTexture(img, ((bits == 128)?GL_RGBA32F: GL_RGBA), bits, false));
}

void InstancedSkinnedMesh::setCurrentAnimationIndex(int animationIndex) {
    m_currentAnimationIndex = animationIndex;
}

int InstancedSkinnedMesh::getCurrentAnimationIndexFrames() {
    return m_currentAnimationFrames;
}