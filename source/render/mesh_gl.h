#ifndef _MESH_GL_H
#define _MESH_GL_H

#include "utility/common.h"
#include "math/mat4.h"
#include "math/dmat4.h"
#include "material_gl.h"
#include "buffers_gl.h"


namespace render
{
	
	#define MAX_MESHSCENENODE_NAME_SIZE		64
	#define MAX_ANIMATION_NAME_SIZE			64


	enum VertexFlags : u8
	{
		Vertex_None                  = 0x00,
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
		u32			vertexSize;			// size / stride of the vertex
		u32			numElements;		// for GL_TRIANGLES it's number of primitives * 3
		u32			indexBaseOffset;	// base offset into the index buffer
		u32			indexRangeStart;	// range low of indices into the vertex buffer, before vertexBaseOffset is added
		u32			indexRangeEnd;		// range high of indices into the vertex buffer, before vertexBaseOffset is added
		u32			vertexBaseOffset;	// base offset into the vertex buffer
		u32			glPrimitiveType;	// GL_TRIANGLES is the only mode currently supported
		u32			glVAO;				// Vertex Array Object, created during Mesh_GL initialization
		u32			materialIndex;		// index into the mesh's materials array

		u8			vertexFlags;		// bits set from enum VertexFlags, checked against a material's requirements

		// per-vertex offsets
		// position is always at offset 0
		u8			normalOffset;
		u8			texCoordsOffset;
		u8			colorsOffset;
		u8			tangentOffset;
		u8			bitangentOffset;

		u8			numColorChannels;		// how many 4-byte colors are there? Up to 8 supported.
		u8			numTexCoordChannels;	// how many U, UV or UVW coordinate sets are there? Up to 8 supported.
		u8			numTexCoordComponents[MAX_MATERIAL_TEXTURES];	// indexed by channel, how many components in the channel?
	};
	static_assert_aligned_size(DrawSet,4);


	struct MeshSceneNode
	{
		mat4		transform;
		u32			parentIndex;			// index into array of nodes
		u32			numChildren;
		u32			childIndexOffset;		// offset into array of child node indexes, numChildren elements belong to this node
		u32			numMeshes;
		u32			meshIndexOffset;		// offset into array of mesh instances, numMeshes elements belong to this node
		// scene node string name is stored in the metadata buffer with the same index
		u8			_padding[12];			// pad to 96 bytes for 16-byte alignment of MeshSceneNode arrays (supports SIMD _m128 in mat4)
	};
	static_assert_aligned_size(MeshSceneNode,4);


	struct MeshSceneNodeMetaData
	{
		char		name[MAX_MESHSCENENODE_NAME_SIZE];	// name of the scene graph node
	};
	static_assert_aligned_size(MeshSceneNodeMetaData,4);


	struct MeshSceneGraph
	{
		u32			numNodes;
		u32			numChildIndices;
		u32			numMeshIndices;
		u32			childIndicesOffset;		// offset in bytes to start of childIndices array data
		u32			meshIndicesOffset;		// offset in bytes to start of meshIndices array data
		u32			meshMetaDataOffset;		// offset in bytes to start of sceneNodeMetaData array data
		MeshSceneNode* sceneNodes;		// array of nodes starting with root, in breadth-first order, offset is always 0 relative to start of MeshSceneGraph data
		u32*		childIndices;		// combined array of child indices for scene nodes, each an index into sceneNodes array
		u32*		meshIndices;		// combined array of mesh indices for scene nodes, each an index into drawSets array
		MeshSceneNodeMetaData* sceneNodeMetaData;	// metaData is indexed corresponding to sceneNodes
	};


	// Animation Structs

	struct PositionKeyFrame
	{
		r32			time;
		r32			x, y, z;
	};
	static_assert_aligned_size(PositionKeyFrame,4);


	struct RotationKeyFrame
	{
		r32			time;
		r32			w, x, y, z;
		u8			_padding[4];
	};
	static_assert_aligned_size(RotationKeyFrame,4);


	struct ScalingKeyFrame
	{
		r32			time;
		r32			x, y, z;
	};
	static_assert_aligned_size(ScalingKeyFrame,4);


	struct AnimationTrack
	{
		u32			nodeAnimationsIndexOffset;
		u32			numNodeAnimations;
		r32			ticksPerSecond;
		r32			durationTicks;
		r32			durationSeconds;
		r32			durationMilliseconds;
	};
	static_assert_aligned_size(AnimationTrack,4);


	enum AnimationBehavior : u8 {
		AnimationBehavior_Default  = 0,	// The value from the default node transformation is taken
		AnimationBehavior_Constant = 1, // The nearest key value is used without interpolation
		AnimationBehavior_Linear   = 2, // nearest two keys is linearly extrapolated for the current time value
		AnimationBehavior_Repeat   = 3  // The animation is repeated. If the animation key go from n to m and the current time is t, use the value at (t-n) % (|m-n|).
	};


	struct NodeAnimation
	{
		u32			sceneNodeIndex;				// index of sceneNode that this animation controls
		u32			positionKeysIndexOffset;	// offset into positionKeys array of the first position keyframe
		u32			rotationKeysIndexOffset;	// offset into rotationKeys array of the first rotation keyframe
		u32			scalingKeysIndexOffset;		// offset into scalingKeys array of the first scaling keyframe
		u16			numPositionKeys;
		u16			numRotationKeys;
		u16			numScalingKeys;
		u8			preState;					// TODO: define enum for these states, look at assimp values
		u8			postState;
	};
	static_assert_aligned_size(NodeAnimation,4);


	struct AnimationTrackMetaData
	{
		char		name[MAX_ANIMATION_NAME_SIZE];	// name of the animation track
	};
	static_assert_aligned_size(AnimationTrackMetaData,4);


	struct MeshAnimations
	{
		u32			numAnimationTracks;
		u32			nodeAnimationsOffset;	// all offsets in this struct are relative to the start of animation data ...
		u32			positionKeysOffset;		// ... unlike other offsets which are relative to start of full model data buffer
		u32			rotationKeysOffset;
		u32			scalingKeysOffset;
		u32			trackNamesOffset;
		AnimationTrack*			animations;		// animation tracks, offset always 0 relative to start of animation data
		NodeAnimation*			nodeAnimations;
		PositionKeyFrame*		positionKeys;
		RotationKeyFrame*		rotationKeys;
		ScalingKeyFrame*		scalingKeys;
		AnimationTrackMetaData* trackNames;		// array of 64-byte strings
	};


	/**
	 * The Mesh_GL has a trivially-copyable and padded data layout for direct memory-image
	 * serialization. This is contained by a Model_GL, which provides a richer C++ interface
	 * and handles to child resources for the mesh like textures and the material shader.
	 */
	struct Mesh_GL
	{
		u32				sizeBytes;			// contains the size of modelData in bytes

		u32				numDrawSets;
		u32				numMaterials;

		u32				drawSetsOffset;		// offsets into modelData
		u32				materialsOffset;
		u32				meshSceneOffset;
		u32				animationsSize;
		u32				animationsOffset;
		u32				vertexBufferOffset;	// 0 when vertexBuffer contains internal data, > 0 when vertex data is part of modelData
		u32				indexBufferOffset;	// 0 when indexBuffer contains internal data, > 0 when index data is part of modelData

		DrawSet*		drawSets;
		Material_GL*	materials;

		MeshSceneGraph	meshScene;
		MeshAnimations	animations;
		VertexBuffer_GL	vertexBuffer;
		IndexBuffer_GL	indexBuffer;

		u8*				modelData;		// contains data for drawSets, materials, meshScene.sceneNodes,
										// meshScene.childIndices, meshScene.meshIndices


		/**
		 * Constructor used by resource loading system. The modelData buffer contains the entire
		 * mesh data to be deserialized. The createResourcesFromInternalMemory function must be
		 * called from the OpenGL thread after this constructor is used.
		 */
		explicit Mesh_GL(u8* data, size_t size);
		
		/**
		 * Constructor used by import routine. The modelData buffer has a different layout and
		 * is smaller compared to when the mesh is deserialized from disk. The internal pointers
		 * are still able to be hooked up, so it works just fine. This constructor results in a
		 * fully loaded and usable mesh, including initing OpenGL resources. Call this from the
		 * OpenGL thread only.
		 */
		explicit Mesh_GL(size_t sizeBytes, u16 numDrawSets, u16 numMaterials,
							u32 drawSetsOffset, u32 materialsOffset, u32 meshSceneOffset,
							u32 animationsSize, u32 animationsOffset,
							u8* modelData, MeshSceneGraph&& meshScene, MeshAnimations&& meshAnimations,
							VertexBuffer_GL&& vb, IndexBuffer_GL&& ib);
		
		~Mesh_GL();

		// Functions

		/**
		 * Gets the animation track index by the animation name. Do this once after load and
		 * store the result for O(1) lookup later. Assumes there are < UINT32_MAX animations.
		 * @returns	index or UINT32_MAX if name not found
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
		void render(
			u8 viewport,
			int shaderProgramId,
			int ambientLoc,
			int diffuseLoc,
			int specularLoc,
			int shininessLoc,
			int diffuseMapLoc,
			r32 animTime, // TODO: should take an Entity Id instead, to get animation times and blends and material overrides
			const dmat4& viewMat,
			const mat4& projMat/*All TEMP*/) const;

		/**
		 * Creates the Vertex Buffer Object with OpenGL for each draw set
		 */
		void initVAO(int drawSetIndex) const;

		inline void Mesh_GL::initVAOs() const
		{
			for (u32 ds = 0; ds < numDrawSets; ++ds) {
				initVAO(ds);
			}
		}

		/**
		 * Serialization functions, to/from binary stream
		 */
//		void serialize(std::ostream& out);
//		void deserialize(std::istream& in);

		/**
		 * Sets properties and internal pointers based on data loaded into modelData buffer
		 */
		void loadFromInternalMemory();

		/**
		 * Creates index/vertex buffers based on data loaded into modelData buffer. Call this
		 * from the OpenGL thread after calling loadFromInternalMemory.
		 */
		void createBuffersFromInternalMemory();
	};


	/**
	 * Model_GL is the engine's wrapper for a mesh. The mesh contains all relevant data to be
	 * loaded from disk, and is constructed in a way that can be loaded and persisted quickly.
	 * This wrapper holds the mesh and handles to required child resources like textures
	 * materials, and shader programs. Model_GL is the resource constructed by the resource
	 * loading system. The resource size refers to the "on disk" memory which is just the mesh
	 * and doesn't include the space for the materials, shaders and other fields.
	 */
	struct Model_GL {
		struct RenderEntryList {
			RenderQueue::KeyType*	keys;
			RenderEntry*			entries;
		};

		
		Mesh_GL			mesh; // should models have a collection of meshes here?
		RenderEntryList	renderEntries;
		TextureId*		textures;
		ShaderId*		shaderPrograms;
		u32				numTextures;
		u32				numShaders;
		bool			assetsLoaded;	// true when all asset asyncronous loading completed, model can be rendered


		void render(Scene& scene, ComponentId modelInstanceId, u8 viewport);
		void draw(ComponentId modelInstanceId, u32 drawSetIndex);
		
		/**
		 * Builds a list of render keys/entries  for
		 * the owner model.
		 */
		void initRenderEntries(Model_GL& owner);

		/**
		 * Builds list of render keys/entries for optimized submission to the render queue. Can
		 * be called on loading thread since no OpenGL calls are made.
		 */
		void initRenderEntries();

		/**
		 * Causes all mesh material textures and shaders to be queued up for asynchronous
		 * loading, and stores all resource handles containing the futures. Can be called from
		 * the loading thread since no OpenGL calls are directly made, this only creates the
		 * resource loading tasks. Once all resources are loaded, the resourcesLoaded bool is
		 * set to true, and the model is ready to be rendered.
		 */
		void loadMaterialResources(const char* filePath);

		/**
		 * Creates OpenGL buffers in the internal mesh. Call this from the OpenGL thread after
		 * the mesh is constructed.
		 */
		void createBuffers()
		{
			mesh.createBuffersFromInternalMemory();
		}
	};

}


#endif