#pragma once
#include "AABB.h";
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "SceneObject.h"
#include "Constants.hpp"
#include "ShaderLocs.h"

using namespace std;
using namespace glm;

/*
 when node is root, its parentNode = -1, when node is leaf, its childnode = -1, -1 equals null
*/

struct BVHNode
{
	BVHNode(AABB aabb, int parenetNode, int leftChildNode, int rightChildNode, int index, int indexInMapToScene)
		: aabb(aabb), parentNode(parenetNode), leftChildNode(leftChildNode), rightChildNode(rightChildNode), index(index), indexMapToScene(indexInMapToScene) {};

	AABB aabb;
	int parentNode;
	int leftChildNode;
	int rightChildNode;
	int index;
	int indexMapToScene;
};

class BVH
{
public:
	BVH(const vector<SceneObject>);
	~BVH();
	void addNode(SceneObject object);
	void updateNode(int addIndex, int parnetIndex);
	void updateBVH();
	void traverseBVHByLayer(int index, int curr_layer, int target_layer);
	void traverseBVH(int index);
	int findClosestNode(AABB aabb, int nodeIndex);
	void refitParentAABBInBVH(int node_2_parent_index);
	void drawBVH();
	void drawBVHInLayer(int layer);
	vector<int> CollisionDetection(AABB aabb, int sceneIndex);
	void searchCollision(AABB aabb, int nodeIndex, int searchNodeIndex, vector<int>& collisions);
	int getRootIndex();
	static GLuint createAABBVbo(AABB aabb);
	static vector<vec3> generateAABBvertices(AABB aabb);

	GLuint vaos[INSTANCE_NUM - 1];
	GLuint vbos[INSTANCE_NUM - 1];
	int draw_status[2 * INSTANCE_NUM - 1] = {0};

private:
	vector<BVHNode> bvhNodes;
	//vector<SceneObject> objects;
	int rootIndex;
};

