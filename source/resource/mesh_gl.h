#ifndef _MESH_GL_H_
#define _MESH_GL_H_

#include "../utility/common.h"
//#include "render/VertexBuffer_GL.h"
//#include "render/IndexBuffer_GL.h"
#include "material_gl.h"


#define MAX_MESHSCENENODE_NAME_SIZE		64
#define MAX_ANIMATION_NAME_SIZE			64


enum VertexFlags : u8
{
	Vertex_None                  = 0,
	Vertex_Positions             = 0x01,
	Vertex_Normals               = 0x02,
	Vertex_TangentsAndBitangents = 0x04,
	Vertex_Colors                = 0x08,
	Vertex_TextureCoords         = 0x10
};

/**
 * DrawSet stores the properties to render a single sub-mesh (usually one per material)
 * within the greater mesh. It includes the information necessary to interpret the vertex
 * data within the specified range (indexed into the Mesh_GL's vertex buffer), and the set
 * of faces indexed into the Mesh_GL's index buffer. This object describes the vertex data
 * available. DrawSet bit flags are checked against Material_GL bit flag requirements to
 * ensure the mesh has the required components to be rendered. The material and DrawSet
 * combine to build the VAO for rendering, where layout attribute locations are
 * predetermined and are enabled on an as-needed basis.
 */
struct DrawSet
{
	u32	    vertexSize;				// size / stride of the vertex
	u32	    numElements;			// for GL_TRIANGLES it's number of primitives * 3
	u32	    indexBaseOffset;		// base offset into the index buffer
	u32	    indexRangeStart;		// range low of indices into the vertex buffer, before vertexBaseOffset is added
	u32	    indexRangeEnd;			// range high of indices into the vertex buffer, before vertexBaseOffset is added
	u32	    vertexBaseOffset;		// base offset into the vertex buffer
	u32	    glPrimitiveType;		// GL_TRIANGLES is the only mode currently supported
	u32	    glVAO;					// Vertex Array Object, created during Mesh_GL initialization
	u32	    materialIndex;			// index into the mesh's materials array

	u8		vertexFlags;			// bits set from enum VertexFlags, checked against a material's requirements

	// per-vertex offsets
	// position is always at offset 0
	u8		normalOffset;
	u8		texCoordsOffset;
	u8		colorsOffset;
	u8		tangentOffset;
	u8		bitangentOffset;

	u8		numColorChannels;		// how many 4-byte colors are there? Up to 8 supported.
	u8		numTexCoordChannels;	// how many S, ST or STP coordinate sets are there? Up to 8 supported.
	u8		numTexCoordComponents[MAX_MATERIAL_TEXTURES];	// indexed by channel, how many components in the channel?
};
static_assert_aligned_size(DrawSet,4);


struct MeshSceneNode
{
	mat4	transform;
	u32	    parentIndex;			// index into array of nodes
	u32	    numChildren;
	u32	    childIndexOffset;		// offset into array of child node indexes, numChildren elements belong to this node
	u32	    numMeshes;
	u32	    meshIndexOffset;		// offset into array of mesh instances, numMeshes elements belong to this node
	// scene node string name is stored in the metadata buffer with the same index
	u8		_padding_end[12];		// pad to 96 bytes for 16-byte alignment of MeshSceneNode arrays (supports SIMD _m128 in mat4)
};
static_assert_aligned_size(MeshSceneNode,4);


struct MeshSceneNodeMetaData
{
	char	name[MAX_MESHSCENENODE_NAME_SIZE];	// name of the scene graph node
};
static_assert_aligned_size(MeshSceneNodeMetaData,4);


struct MeshSceneGraph
{
	u32		numNodes;
	u32		numChildIndices;
	u32		numMeshIndices;
	u32		childIndicesOffset;		// offset in bytes to start of childIndices array data
	u32		meshIndicesOffset;		// offset in bytes to start of meshIndices array data
	u32		meshMetaDataOffset;		// offset in bytes to start of sceneNodeMetaData array data
	MeshSceneNode*  sceneNodes;		// array of nodes starting with root, in breadth-first order, offset is always 0 relative to start of MeshSceneGraph data
	u32*	childIndices;			// combined array of child indices for scene nodes, each an index into sceneNodes array
	u32*	meshIndices;			// combined array of mesh indices for scene nodes, each an index into drawSets array
	MeshSceneNodeMetaData* sceneNodeMetaData;	// metaData is indexed corresponding to sceneNodes
};


// Animation Structs

struct PositionKeyFrame
{
	r32		time;
	r32		x, y, z;
};
static_assert_aligned_size(PositionKeyFrame,4);


struct RotationKeyFrame
{
	r32		time;
	r32		w, x, y, z;
	u8		_padding_end[4];
};
static_assert_aligned_size(RotationKeyFrame,4);


struct ScalingKeyFrame
{
	r32		time;
	r32		x, y, z;
};
static_assert_aligned_size(ScalingKeyFrame,4);


struct AnimationTrack
{
	u32		nodeAnimationsIndexOffset;
	u32		numNodeAnimations;
	r32		ticksPerSecond;
	r32		durationTicks;
	r32		durationSeconds;
	r32		durationMilliseconds;
};
static_assert_aligned_size(AnimationTrack,4);


enum AnimationBehavior : u8
{
	AnimationBehavior_Default = 0,	// The value from the default node transformation is taken
	AnimationBehavior_Constant,		// The nearest key value is used without interpolation
	AnimationBehavior_Linear,		// nearest two keys is linearly extrapolated for the current time value
	AnimationBehavior_Repeat		// The animation is repeated. If the animation key go from n to m and the current time is t, use the value at (t-n) % (|m-n|).
};


struct NodeAnimation
{
	u32		sceneNodeIndex;				// index of sceneNode that this animation controls
	u32		positionKeysIndexOffset;	// offset into positionKeys array of the first position keyframe
	u32		rotationKeysIndexOffset;	// offset into rotationKeys array of the first rotation keyframe
	u32		scalingKeysIndexOffset;		// offset into scalingKeys array of the first scaling keyframe
	u16		numPositionKeys;
	u16		numRotationKeys;
	u16		numScalingKeys;
	u8		preState;					// TODO: define enum for these states, look at assimp values
	u8		postState;
};
static_assert_aligned_size(NodeAnimation,4);


struct AnimationTrackMetaData
{
	char	name[MAX_ANIMATION_NAME_SIZE];	// name of the animation track
};
static_assert_aligned_size(AnimationTrackMetaData,4);


struct MeshAnimations {
	u32		numAnimationTracks;
	u32		nodeAnimationsOffset;		// all offsets in this struct are relative to the start of animation data ...
	u32		positionKeysOffset;			// ... unlike other offsets which are relative to start of full model data buffer
	u32		rotationKeysOffset;
	u32		scalingKeysOffset;
	u32		trackNamesOffset;
	AnimationTrack*			animations;	// animation tracks, offset always 0 relative to start of animation data
	NodeAnimation*			nodeAnimations;
	PositionKeyFrame*		positionKeys;
	RotationKeyFrame*		rotationKeys;
	ScalingKeyFrame*		scalingKeys;
	AnimationTrackMetaData* trackNames;	// array of 64-byte strings
};


/**
 * The Mesh_GL has a trivially-copyable and padded data layout for direct memory-image
 * serialization. This is contained by a Model_GL, which provides handles to child resources for
 * the mesh like textures and the material shader.
 */
struct Mesh_GL
{
	/**
	 * Gets the animation track index by the animation name. Do this once after load and
	 * store the result for O(1) lookup later. Assumes there are < UINT32_MAX animations,
	 * a safe bet. Returns index or UINT32_MAX if name not found.
	 */
	u32 getAnimationTrackIndexByName(const char* name) const;

	/**
	 * Binds the vertex + index buffers, and enables vertex attrib pointers
	 */
	void bind(int drawSetIndex) const;

	/**
	 * Disables vertex attrib pointers, does not unbind the vertex + index buffers since the
	 * next draw call will just bind its own buffers.
	 */
	void unbind(int drawSetIndex) const;

	/**
	 * Draws a single DrawSet
	 */
	void drawMesh(int drawSetIndex) const;
	
	/**
	 * Adds all DrawSets to render queue
	 */
	// void render(Engine& engine, u8 viewport,
				// int shaderProgramId,
				// int ambientLoc, int diffuseLoc, int specularLoc, int shininessLoc,
				// int diffuseMapLoc, float animTime, // TODO: should take an Entity Id instead, to get animation times and blends and material overrides
				// const glm::dmat4& viewMat, const glm::mat4& projMat/*All TEMP*/) const;
	
	/**
	 * Creates the Vertex Buffer Object with OpenGL for each draw set
	 */
	void initVAO(int drawSetIndex) const;
	void initVAOs() const;

	/**
	 * Serialization functions, to/from binary stream
	 */
	void serialize(u8* out);
	void deserialize(u8* in);

	/**
	 * Sets properties and internal pointers based on data loaded into modelData buffer
	 */
	void loadFromInternalMemory();

	/**
	 * Creates index/vertex buffers based on data loaded into modelData buffer. Call this
	 * from the OpenGL thread after calling loadFromInternalMemory.
	 */
	void createBuffersFromInternalMemory();


	// Variables

	u32		sizeBytes;			// contains the size of modelData in bytes

	u32		numDrawSets;
	u32		numMaterials;

	u32		drawSetsOffset;		// offsets into modelData
	u32		materialsOffset;
	u32		meshSceneOffset;
	u32		animationsSize;
	u32		animationsOffset;
	u32		vertexBufferOffset;	// 0 when vertexBuffer contains internal data, > 0 when vertex data is part of modelData
	u32		indexBufferOffset;	// 0 when indexBuffer contains internal data, > 0 when index data is part of modelData

	DrawSet*		drawSets;
	Material_GL*	materials;

	MeshSceneGraph	meshScene;
	MeshAnimations	animations;
//	VertexBuffer_GL	vertexBuffer;
//	IndexBuffer_GL	indexBuffer;
};


// Inline Functions

inline void Mesh_GL::initVAOs() const
{
	for (u32 ds = 0; ds < numDrawSets; ++ds) {
		initVAO(ds);
	}
}


#endif