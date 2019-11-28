#include "mesh_gl.h"
//#include <render/model/Mesh_GL.h>
//#include <application/Engine.h>
#include <GL/glew.h>
//#include <render/ShaderProgramLayouts_GL.h>

//#include <render/RenderResources.h> // TEMP (these two headers needed only for temp render function)
//#include <resource/ResourceLoader.h>


#define Meters_to_Feet 3.2808398950131233595800524934383f

namespace render
{

	void Mesh_GL::bind(int drawSetIndex) const
	{
		glBindVertexArray(drawSets[drawSetIndex].glVAO);
		ASSERT_GL_ERROR;
	}


	void Mesh_GL::drawMesh(int drawSetIndex) const
	{
		assert(drawSetIndex >= 0 && drawSetIndex < (int)numDrawSets && "drawSetIndex out of range");
		bind(drawSetIndex);
		
		GLenum indexType = indexBuffer.getIndexType();
		DrawSet& drawSet = drawSets[drawSetIndex];

		glDrawRangeElements(drawSet.glPrimitiveType, drawSet.indexRangeStart, drawSet.indexRangeEnd,
							drawSet.numElements, indexType, (const GLvoid*)drawSet.indexBaseOffset);
		
		//unbind(drawSetIndex);

		ASSERT_GL_ERROR;
	}


	// TODO: convert this to per-update "animate" function, operating on animation components
	void Mesh_GL::render(
		u8 viewport,
		int shaderProgramId,
		int ambientLoc,
		int diffuseLoc,
		int specularLoc,
		int shininessLoc,
		int diffuseMapLoc,
		r32 animTime,
		const dmat4& viewMat,
		const mat4& projMat/*All TEMP*/) const
	{
		auto& renderSystem = *engine.renderSystem;

		dmat4 modelToWorld;
		dmat4 nodeTransform;
		
		ObjectUniformsUBO objectUniformsUBO{};
		glBindBuffer(GL_UNIFORM_BUFFER, renderSystem.getUBOHandle(ObjectUniforms));

		// temp
		modelToWorld = rotate(translate(modelToWorld, /*normalize(dvec3(-1, 1, 1)) * 5700000.0*/dvec3(0.0, -50.0, 0.0)),
								radians(90.0),
								dvec3(1.0, 0.0, 0.0));
		modelToWorld = scale(modelToWorld, dvec3(Meters_to_Feet));

		struct BFSQueueItem {
			u32 nodeIndex;
			dmat4    toWorld;
		};
		// TODO: lot of memory being created and destroyed every frame, should all be pre-calculated and stored for non-animated meshes?
		// Do this on mesh init and save a list of RenderEntries with matching index to the DrawSet
		// during the init, decompose the nodeTransform into separate translation, orientation, scale
		// Move this RenderEntry list to the Model_GL?
		vector_queue<BFSQueueItem> bfsQueue;
		bfsQueue.reserve(meshScene.numNodes);

		bfsQueue.push({ 0, modelToWorld }); // push root node to start traversal

		while (!bfsQueue.empty()) {
			auto& thisItem = bfsQueue.front();

			u32 nodeIndex = thisItem.nodeIndex;
			assert(nodeIndex >= 0 && nodeIndex < meshScene.numNodes && "node index out of range");

			const auto& node = meshScene.sceneNodes[thisItem.nodeIndex];
			nodeTransform = node.transform;

			// Get this node's animation, if any. Alter the node transform matrix.
			// TODO: make sure animation takes place only when instance is going to be visible after early frustum cull
			/*if (animations.numAnimationTracks > 0) {
				auto& anim = animations.animations[0];
				for (u32 na = anim.nodeAnimationsIndexOffset; na < anim.nodeAnimationsIndexOffset + anim.numNodeAnimations; ++na) {
					// TODO: move some of this out of the scene graph traversal? We know the active animations
					// ahead of time, do we need to check on every node?
					auto& nodeAnim = animations.nodeAnimations[na];
					if (nodeAnim.sceneNodeIndex == nodeIndex) {
						vec3 nodeTranslation;
						quat nodeRotation;
						vec3 nodeScale;

						// This code requires at least one key from each of position, rotation and scaling so we avoid having to
						// decompose the default node matrix. Luckily it appears that assimp always gives us at least one key for
						// each channel, but that could also be from Blender specifically. This assertion tells us if there is a
						// missing channel in the animation.
						assert(nodeAnim.numPositionKeys > 0 && nodeAnim.numRotationKeys > 0 && nodeAnim.numScalingKeys > 0 &&
								"animation requires at least one key per channel");
						
						// Translation keyframes
						{
							int key1 = -1;
							int key2 = -1;
							// get nearest two key frames
							for (u32 pk = nodeAnim.positionKeysIndexOffset; pk < nodeAnim.positionKeysIndexOffset + nodeAnim.numPositionKeys; ++pk) {
								auto& posKey = animations.positionKeys[pk];
								if (animTime < posKey.time) {
									key1 = (pk == nodeAnim.positionKeysIndexOffset ? pk : pk - 1);
									key2 = pk;
									break;
								}
							}
							// went past the last key
							if (key1 == -1) {
								key1 = nodeAnim.positionKeysIndexOffset + nodeAnim.numPositionKeys - 1;
								key2 = key1;
							}

							// TODO: look at pre/post state, we may be able to exit early and accept the default modelToWorld when key1 == key2, depending on the state
							// Also, the key1 or key2 at either end of the animation may have to be set to default node transform instead of clamping the animations frame
							r32 time1 = animations.positionKeys[key1].time;
							vec3 pos1(animations.positionKeys[key1].x, animations.positionKeys[key1].y, animations.positionKeys[key1].z);
							r32 time2 = animations.positionKeys[key2].time;
							vec3 pos2(animations.positionKeys[key2].x, animations.positionKeys[key2].y, animations.positionKeys[key2].z);
							
							r32 interp = 0.0f;
							if (key1 != key2) {
								interp = (animTime - time1) / (time2 - time1);
							}

							// TODO: allow interpolation curves other than linear... hermite, cubic, spring system, etc.
							nodeTranslation = mix(pos1, pos2, interp);
						}
						// Rotation keyframes
						{
							int key1 = -1;
							int key2 = -1;

							// get nearest two key frames
							for (u32 k = nodeAnim.rotationKeysIndexOffset; k < nodeAnim.rotationKeysIndexOffset + nodeAnim.numRotationKeys; ++k) {
								auto& rotKey = animations.rotationKeys[k];
								if (animTime < rotKey.time) {
									key1 = (k == nodeAnim.rotationKeysIndexOffset ? k : k - 1);
									key2 = k;
									break;
								}
							}

							// went past the last key
							if (key1 == -1) {
								key1 = nodeAnim.rotationKeysIndexOffset + nodeAnim.numRotationKeys - 1;
								key2 = key1;
							}

							r32 time1 = animations.rotationKeys[key1].time;
							quat rot1(animations.rotationKeys[key1].w, animations.rotationKeys[key1].x, animations.rotationKeys[key1].y, animations.rotationKeys[key1].z);
							r32 time2 = animations.rotationKeys[key2].time;
							quat rot2(animations.rotationKeys[key2].w, animations.rotationKeys[key2].x, animations.rotationKeys[key2].y, animations.rotationKeys[key2].z);

							r32 interp = 0.0f;
							if (key1 != key2) {
								interp = (animTime - time1) / (time2 - time1);
							}

							nodeRotation = normalize(lerp(rot1, rot2, interp));
						}
						// Scaling keyframes
						{
							int key1 = -1;
							int key2 = -1;

							// get nearest two key frames
							for (u32 k = nodeAnim.scalingKeysIndexOffset; k < nodeAnim.scalingKeysIndexOffset + nodeAnim.numScalingKeys; ++k) {
								auto& scaleKey = animations.scalingKeys[k];
								if (animTime < scaleKey.time) {
									key1 = (k == nodeAnim.scalingKeysIndexOffset ? k : k - 1);
									key2 = k;
									break;
								}
							}

							// went past the last key
							if (key1 == -1) {
								key1 = nodeAnim.scalingKeysIndexOffset + nodeAnim.numScalingKeys - 1;
								key2 = key1;
							}

							r32 time1 = animations.scalingKeys[key1].time;
							vec3 scale1(animations.scalingKeys[key1].x, animations.scalingKeys[key1].y, animations.scalingKeys[key1].z);
							r32 time2 = animations.scalingKeys[key2].time;
							vec3 scale2(animations.scalingKeys[key2].x, animations.scalingKeys[key2].y, animations.scalingKeys[key2].z);

							r32 interp = 0.0f;
							if (key1 != key2) {
								interp = (animTime - time1) / (time2 - time1);
							}

							nodeScale = mix(scale1, scale2, interp);
						}
						nodeTransform = mat4_cast(nodeRotation);
						nodeTransform[3].xyz = nodeTranslation;
						nodeTransform = scale(nodeTransform, dvec3(nodeScale));
					}
				}
			}*/

			modelToWorld = thisItem.toWorld * nodeTransform;
			
			// original code, jitters far from origin
			//mat4 modelView(viewMat * modelToWorld);
			//mat4 mvp(projMat * modelView);
			//mat4 normalMat(transpose(inverse(mat3(modelView))));

			// transform world space to camera space on CPU in double precision, then send single to GPU
			// see http://blogs.agi.com/insight3d/index.php/2008/09/03/precisions-precisions/
			dmat4 modelView_World(viewMat * modelToWorld);

			// camera space is defined as world space rotation without the translation component
			mat4 modelView_Camera = make_mat4(modelView_World);
			modelView_Camera[0][3] = 0;
			modelView_Camera[1][3] = 0;
			modelView_Camera[2][3] = 0;

			mat4 mvp(projMat * modelView_Camera);
			mat4 normalMat(transpose(inverse(make_mat3(modelView_Camera))));
			
			// set the object UBO values
			objectUniformsUBO.modelToWorld = make_mat4(modelToWorld);
			objectUniformsUBO.modelView = /*modelView;*/modelView_Camera;
			objectUniformsUBO.modelViewProjection = mvp;
			objectUniformsUBO.normalMatrix = normalMat;
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ObjectUniformsUBO), &objectUniformsUBO);

			// draw this node's meshes
			for (u32 m = 0; m < node.numMeshes; ++m) {
				u32 ds = meshScene.meshIndices[node.meshIndexOffset + m];
				DrawSet& drawSet = drawSets[ds];
				Material_GL& mat = materials[drawSet.materialIndex];

				glUniform3fv(ambientLoc, 1, &mat.ambientColor[0]);
				glUniform3fv(diffuseLoc, 1, &mat.diffuseColor[0]);
				glUniform3fv(specularLoc, 1, &mat.specularColor[0]);
				glUniform1f(shininessLoc, mat.shininess);

				// renderer should do this as part of the render key sort/render, not the mesh
				// TEMP, assuming one texture
				if (mat.numTextures > 0) {
					// should NOT use this method to get the resource, it serializes to the worker thread
					// this part of the render is a time-critical section, should have the resourcePtr directly by now
					// store resourcePtr's within the model containing this mesh, render from the model file
					auto texPtr = g_resourceLoader.lock()->getResource(mat.textures[0].textureResourceHandle, CacheType::Cache_Materials);
					if (texPtr) {
						auto& tex = texPtr->getResource<Texture2D_GL>();
						tex.bind(SamplerBinding_Diffuse1);
						//glUniform1i(diffuseMapLoc, 4);

						// TEMP, these layout indexes should be in a header, and the gl call should be made after render key changes
						u32 getSurfaceColorFromTexture = 0;
						glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &getSurfaceColorFromTexture);
					}
				}
				else {
					u32 getSurfaceColorFromMaterial = 1;
					glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &getSurfaceColorFromMaterial);
				}
				
				// TEMP
				drawMesh(ds);
			}

			// push children to traverse
			for (u32 c = 0; c < node.numChildren; ++c) {
				u32 childNodeIndex = meshScene.childIndices[node.childIndexOffset + c];

				assert(childNodeIndex >= 0 && childNodeIndex < meshScene.numNodes && "child node index out of range");
				assert(childNodeIndex > nodeIndex && "child node is not lower in the tree");

				bfsQueue.push({ childNodeIndex, modelToWorld });
			}

			bfsQueue.pop();
		}

		ASSERT_GL_ERROR;
	}


	void Mesh_GL::initVAO(int drawSetIndex) const
	{
		assert(drawSetIndex >= 0 && drawSetIndex < (int)numDrawSets && "drawSetIndex out of range");

		auto& drawSet = drawSets[drawSetIndex];

		glGenVertexArrays(1, &drawSet.glVAO);
		glBindVertexArray(drawSets[drawSetIndex].glVAO);

		vertexBuffer.bind();
		indexBuffer.bind();

		if (drawSet.vertexFlags & Vertex_Positions) {
			glEnableVertexAttribArray(VertexLayout_Position);
			glVertexAttribPointer(
				VertexLayout_Position,
				3,                         // size
				GL_FLOAT,                  // type
				GL_FALSE,                  // normalize fixed-point?
				drawSet.vertexSize,        // stride
				(void*)drawSet.vertexBaseOffset // array buffer offset
				);
		}

		if (drawSet.vertexFlags & Vertex_Normals) {
			glEnableVertexAttribArray(VertexLayout_Normal);
			glVertexAttribPointer(
				VertexLayout_Normal,
				3, GL_FLOAT, GL_FALSE,
				drawSet.vertexSize,
				(void*)(drawSet.vertexBaseOffset + drawSet.normalOffset)
				);
		}

		if (drawSet.vertexFlags & Vertex_TangentsAndBitangents) {
			glEnableVertexAttribArray(VertexLayout_Tangent);
			glEnableVertexAttribArray(VertexLayout_Bitangent);
			glVertexAttribPointer(
				VertexLayout_Tangent,
				3, GL_FLOAT, GL_FALSE,
				drawSet.vertexSize,
				(void*)(drawSet.vertexBaseOffset + drawSet.tangentOffset)
				);
			glVertexAttribPointer(
				VertexLayout_Bitangent,
				3, GL_FLOAT, GL_FALSE,
				drawSet.vertexSize,
				(void*)(drawSet.vertexBaseOffset + drawSet.bitangentOffset)
				);
		}

		if (drawSet.vertexFlags & Vertex_TextureCoords) {
			unsigned int offset = drawSet.vertexBaseOffset + drawSet.texCoordsOffset;

			for (int c = 0; c < drawSet.numTexCoordChannels; ++c) {
				glEnableVertexAttribArray(VertexLayout_TextureCoords + c);
				glVertexAttribPointer(
					VertexLayout_TextureCoords + c,
					drawSet.numTexCoordComponents[c],
					GL_FLOAT, GL_FALSE,
					drawSet.vertexSize,
					(void*)offset
					);

				offset += (sizeof(r32) * drawSet.numTexCoordComponents[c]);
			}
		}

		if (drawSet.vertexFlags & Vertex_Colors) {
			unsigned int offset = drawSet.vertexBaseOffset + drawSet.colorsOffset;

			for (int c = 0; c < drawSet.numColorChannels; ++c) {
				glEnableVertexAttribArray(VertexLayout_Colors + c);
				glVertexAttribPointer(
					VertexLayout_Colors + c,
					4, GL_FLOAT, GL_FALSE,
					drawSet.vertexSize,
					(void*)offset
					);

				offset += (sizeof(r32) * 4);
			}
		}

		ASSERT_GL_ERROR;
	}


	void Mesh_GL::unbind(u32 drawSetIndex) const
	{
		assert(drawSetIndex >= 0 && drawSetIndex < (u32)numDrawSets && "drawSetIndex out of range");

		glBindVertexArray(0); // break the existing vertex array object binding

		/*auto& drawSet = drawSets[drawSetIndex];

		if (drawSet.vertexFlags & Vertex_Positions) {
		glDisableVertexAttribArray(VertexLayout_Position);
		}

		if (drawSet.vertexFlags & Vertex_Normals) {
		glDisableVertexAttribArray(VertexLayout_Normal);
		}

		if (drawSet.vertexFlags & Vertex_TangentsAndBitangents) {
		glDisableVertexAttribArray(VertexLayout_Tangent);
		glDisableVertexAttribArray(VertexLayout_Bitangent);
		}

		if (drawSet.vertexFlags & Vertex_TextureCoords) {
		for (int c = 0; c < drawSet.numTexCoordChannels; ++c) {
		glDisableVertexAttribArray(VertexLayout_TextureCoords + c);
		}
		}

		if (drawSet.vertexFlags & Vertex_Colors) {
		for (int c = 0; c < drawSet.numColorChannels; ++c) {
		glDisableVertexAttribArray(VertexLayout_Colors + c);
		}
		}*/
	}


	// Animation Functions

	u32 Mesh_GL::getAnimationTrackIndexByName(const char* name) const
	{
		for (u32 a = 0; a < animations.numAnimationTracks; ++a) {
			if (strncmp(animations.trackNames[a].name,
						name, MAX_ANIMATION_NAME_SIZE) == 0)
			{
				return a;
			}
		}
		return UINT32_MAX;
	}


	// Serialization Functions

	#define MESH_SERIALIZATION_CURRENT_VERSION	3

	/**
	 * Mesh_GL binary file header, contains all properties needed for serialization
	 */
	struct Mesh_GL_Header {
		u8		key[3];						// always {'g','m','d'}
		u8		version;					// file version
		u32		bufferSize;

		u32		numDrawSets;
		u32		numMaterials;
		u32		drawSetsOffset;
		u32		materialsOffset;
		u32		meshSceneOffset;
		
		u32		sceneNumNodes;
		u32		sceneNumChildIndices;
		u32		sceneNumMeshIndices;
		u32		sceneChildIndicesOffset;
		u32		sceneMeshIndicesOffset;
		u32		sceneMetaDataOffset;
		
		u32		animationsSize;
		u32		animationsOffset;
		u32		numAnimationTracks;
		u32		nodeAnimationsOffset;
		u32		positionKeysOffset;
		u32		rotationKeysOffset;
		u32		scalingKeysOffset;
		u32		animationTrackNamesOffset;

		u32		vertexBufferSize;
		u32		vertexBufferOffset;
		
		u32		indexBufferSize;
		u32		indexBufferOffset;
		u8		indexBufferFlags;

		u8		_padding[3];			// pad for 8-byte alignment of following struct in buffer
	};
	static_assert_aligned_size(Mesh_GL_Header, 8); // Mesh_GL_Header size should be multiple of 8 for alignment of mesh buffer
	

	void Mesh_GL::serialize(std::ostream& out)
	{
		u32 headerSize = sizeof(Mesh_GL_Header);
		u32 drawSetsSize = numDrawSets * sizeof(DrawSet);
		u32 materialsSize = numMaterials * sizeof(Material_GL);
		u32 sceneNodesSize = meshScene.numNodes * sizeof(MeshSceneNode);
		u32 sceneChildIndicesSize = meshScene.numChildIndices * sizeof(u32);
		u32 sceneMeshIndicesSize = meshScene.numMeshIndices * sizeof(u32);
		u32 sceneMetaDataSize = meshScene.numNodes * sizeof(MeshSceneNodeMetaData);

		// get the total size of the model buffer
		u32 bufferSize = drawSetsSize + materialsSize + sceneNodesSize +
			sceneChildIndicesSize + sceneMeshIndicesSize +
			sceneMetaDataSize + animationsSize +
			(u32)vertexBuffer.getSize() +
			(u32)indexBuffer.getSize();
			
		// build the header containing sizes and offsets
		Mesh_GL_Header header = {}; // zero-init the header
		header.key[0] = 'g'; header.key[1] = 'm'; header.key[2] = 'd';
		header.version					= MESH_SERIALIZATION_CURRENT_VERSION;
		header.bufferSize				= bufferSize;
		
		header.numDrawSets				= numDrawSets;
		header.numMaterials				= numMaterials;
		header.drawSetsOffset			= headerSize + 0;
		header.materialsOffset			= header.drawSetsOffset + drawSetsSize;
		header.meshSceneOffset			= header.materialsOffset + materialsSize;
		
		header.sceneNumNodes			= meshScene.numNodes;
		header.sceneNumChildIndices		= meshScene.numChildIndices;
		header.sceneNumMeshIndices		= meshScene.numMeshIndices;
		header.sceneChildIndicesOffset	= header.meshSceneOffset + sceneNodesSize;
		header.sceneMeshIndicesOffset	= header.sceneChildIndicesOffset + sceneChildIndicesSize;
		header.sceneMetaDataOffset		= header.sceneMeshIndicesOffset + sceneMeshIndicesSize;
		
		header.animationsSize			= animationsSize;
		header.animationsOffset			= header.sceneMetaDataOffset + sceneMetaDataSize;
		header.numAnimationTracks		= animations.numAnimationTracks;
		header.nodeAnimationsOffset		= animations.nodeAnimationsOffset;
		header.positionKeysOffset		= animations.positionKeysOffset;
		header.rotationKeysOffset		= animations.rotationKeysOffset;
		header.scalingKeysOffset		= animations.scalingKeysOffset;
		header.animationTrackNamesOffset = animations.trackNamesOffset;

		header.vertexBufferSize			= (u32)vertexBuffer.getSize();
		header.vertexBufferOffset		= header.animationsOffset + animationsSize;
		
		header.indexBufferSize			= (u32)indexBuffer.getSize();
		header.indexBufferOffset		= header.vertexBufferOffset + header.vertexBufferSize;
		header.indexBufferFlags			= (u8)indexBuffer.getFlags();

		// write header
		out.write((const char*)(&header), headerSize);

		// write data buffers
		out.write((const char*)(drawSets), drawSetsSize);
		out.write((const char*)(materials), materialsSize);
		out.write((const char*)(meshScene.sceneNodes), sceneNodesSize);
		out.write((const char*)(meshScene.childIndices), sceneChildIndicesSize);
		out.write((const char*)(meshScene.meshIndices), sceneMeshIndicesSize);
		out.write((const char*)(meshScene.sceneNodeMetaData), sceneMetaDataSize);
		out.write((const char*)(animations.animations), animationsSize);

		// source the vertex data from either the modelData buffer, or the VertexBuffer_GL's internal buffer,
		// it could be one or the other depending on whether the model was deserialized or imported
		unsigned char* vertexData = nullptr;
		if (vertexBufferOffset != 0) {
			vertexData = modelData.get() + vertexBufferOffset;
		}
		else {
			vertexData = vertexBuffer.data();
		}
		assert(vertexData != nullptr && "vertex buffer data not available");
		out.write((const char*)(vertexData), header.vertexBufferSize);
		
		unsigned char* indexData = nullptr;
		if (indexBufferOffset != 0) {
			indexData = modelData.get() + indexBufferOffset;
		}
		else {
			indexData = indexBuffer.data();
		}
		assert(indexData != nullptr && "index buffer data not available");
		out.write((const char*)(indexData), header.indexBufferSize);
	}


	void Mesh_GL::deserialize(std::istream& in)
	{
		// read the header
		Mesh_GL_Header header = {};
		size_t headerSize = sizeof(Mesh_GL_Header);
		in.read((char*)(&header), headerSize);

		// total size of the model data buffer, we are matching how the resource loader works
		size_t totalSize = headerSize + header.bufferSize;
		modelData = std::make_unique<unsigned char[]>(totalSize);

		// copy the header into the modelData buffer (this is how it would be using the resource loader)
		_memcpy_s(modelData.get(), headerSize, &header, headerSize);

		// read the model data buffer
		in.read((char*)(modelData.get() + headerSize), header.bufferSize);

		loadFromInternalMemory();
	}


	void Mesh_GL::loadFromInternalMemory()
	{
		// the header exists at the beginning of the modelData buffer
		Mesh_GL_Header& header = *(Mesh_GL_Header*)modelData.get();
		u32 totalSize = sizeof(Mesh_GL_Header) + header.bufferSize;

		// do some sanity checks
		if (header.key[0] != 'g' || header.key[1] != 'm' || header.key[2] != 'd' ||
			header.version != MESH_SERIALIZATION_CURRENT_VERSION)
		{
			assert(false && "Unrecognized file format");
		}

		if (header.drawSetsOffset != sizeof(Mesh_GL_Header) ||
			header.materialsOffset != header.drawSetsOffset + (header.numDrawSets * sizeof(DrawSet)) ||
			header.meshSceneOffset != header.materialsOffset + (header.numMaterials * sizeof(Material_GL)) ||
			header.sceneChildIndicesOffset != header.meshSceneOffset + (header.sceneNumNodes * sizeof(MeshSceneNode)) ||
			header.sceneMeshIndicesOffset != header.sceneChildIndicesOffset + (header.sceneNumChildIndices * sizeof(u32)) ||
			header.sceneMetaDataOffset != header.sceneMeshIndicesOffset + (header.sceneNumMeshIndices * sizeof(u32)) ||
			header.animationsOffset != header.sceneMetaDataOffset + (header.sceneNumNodes * sizeof(MeshSceneNodeMetaData)) ||
			header.vertexBufferOffset != header.animationsOffset + header.animationsSize ||
			header.indexBufferOffset != header.vertexBufferOffset + header.vertexBufferSize ||
			totalSize != header.indexBufferOffset + header.indexBufferSize)
		{
			assert(false && "File format deserialization error");
		}

		// copy properties from header to member vars
		sizeBytes						= totalSize;
		numDrawSets						= header.numDrawSets;
		numMaterials					= header.numMaterials;
		drawSetsOffset					= header.drawSetsOffset;
		materialsOffset					= header.materialsOffset;
		meshSceneOffset					= header.meshSceneOffset;
		animationsSize					= header.animationsSize;
		animationsOffset				= header.animationsOffset;
		vertexBufferOffset				= header.vertexBufferOffset;
		indexBufferOffset				= header.indexBufferOffset;
		
		meshScene.numNodes				= header.sceneNumNodes;
		meshScene.numChildIndices		= header.sceneNumChildIndices;
		meshScene.numMeshIndices		= header.sceneNumMeshIndices;
		meshScene.childIndicesOffset	= header.sceneChildIndicesOffset;
		meshScene.meshIndicesOffset		= header.sceneMeshIndicesOffset;
		meshScene.meshMetaDataOffset	= header.sceneMetaDataOffset;
		
		animations.numAnimationTracks	= header.numAnimationTracks;
		animations.nodeAnimationsOffset = header.nodeAnimationsOffset;
		animations.positionKeysOffset	= header.positionKeysOffset;
		animations.rotationKeysOffset	= header.rotationKeysOffset;
		animations.scalingKeysOffset	= header.scalingKeysOffset;
		animations.trackNamesOffset		= header.animationTrackNamesOffset;

		// set size and flags into the buffers, we later call loadFromMemory and use getters to read these back
		vertexBuffer.set(nullptr, header.vertexBufferSize);
		indexBuffer.set(nullptr,  header.indexBufferSize, (IndexBuffer_GL::IndexBufferFlags)header.indexBufferFlags);

		// fix up internal pointers based on offsets in header
		drawSets					= (DrawSet*)modelData.get() + drawSetsOffset;
		materials					= (Material_GL*)modelData.get() + materialsOffset;
		meshScene.sceneNodes		= (MeshSceneNode*)modelData.get() + meshSceneOffset;
		meshScene.childIndices		= (u32*)modelData.get() + meshScene.childIndicesOffset;
		meshScene.meshIndices		= (u32*)modelData.get() + meshScene.meshIndicesOffset;
		meshScene.sceneNodeMetaData	= (MeshSceneNodeMetaData*)modelData.get() + meshScene.meshMetaDataOffset;

		animations.animations		= (AnimationTrack*)modelData.get() + animationsOffset;
		animations.nodeAnimations	= (NodeAnimation*)modelData.get() + animationsOffset + animations.nodeAnimationsOffset;
		animations.positionKeys		= (PositionKeyFrame*)modelData.get() + animationsOffset + animations.positionKeysOffset;
		animations.rotationKeys		= (RotationKeyFrame*)modelData.get() + animationsOffset + animations.rotationKeysOffset;
		animations.scalingKeys		= (ScalingKeyFrame*)modelData.get() + animationsOffset + animations.scalingKeysOffset;
		animations.trackNames		= (AnimationTrackMetaData*)modelData.get() + animationsOffset + animations.trackNamesOffset;
	}

	
	void Mesh_GL::createBuffersFromInternalMemory()
	{
		// This function is called by the deserialization / resource loading routines, not by
		// the assimp import. The size/flags of the buffers are set in loadFromInternalMemory.

		vertexBuffer.loadFromMemory(modelData.get() + vertexBufferOffset,
										vertexBuffer.getSize());

		indexBuffer.loadFromMemory(modelData.get() + indexBufferOffset,
										indexBuffer.getSize(),
										IndexBuffer_GL::getSizeOfElement(indexBuffer.getFlags()));

		initVAOs();
	}


	// Constructors / Destructor

	Mesh_GL::Mesh_GL(ByteBuffer data, size_t size) :
		modelData(std::move(data)),
		sizeBytes{ (u32)size) }
	{
		loadFromInternalMemory();
	}


	Mesh_GL::Mesh_GL(size_t sizeBytes, uint16_t numDrawSets, uint16_t numMaterials,
						u32 drawSetsOffset, u32 materialsOffset, u32 meshSceneOffset,
						u32 animationsSize, u32 animationsOffset,
						ByteBuffer modelData,
						MeshSceneGraph&& meshScene,
						MeshAnimations&& meshAnimations,
						VertexBuffer_GL&& vb, IndexBuffer_GL&& ib) :
		sizeBytes{ (u32)sizeBytes },
		numDrawSets{ numDrawSets },
		numMaterials{ numMaterials },
		drawSetsOffset{ drawSetsOffset },
		materialsOffset{ materialsOffset },
		meshSceneOffset{ meshSceneOffset },
		animationsSize{ animationsSize },
		animationsOffset{ animationsOffset },
		vertexBufferOffset{ 0 },
		indexBufferOffset{ 0 },
		modelData(std::move(modelData)),
		meshScene(std::forward<MeshSceneGraph>(meshScene)),
		animations(std::forward<MeshAnimations>(meshAnimations)),
		vertexBuffer(std::forward<VertexBuffer_GL>(vb)),
		indexBuffer(std::forward<IndexBuffer_GL>(ib))
	{
		// fix up the internal pointers into modelData
		drawSets = (DrawSet*)modelData.get() + drawSetsOffset;
		materials = (Material_GL*)modelData.get() + materialsOffset;

		initVAOs();
	}


	Mesh_GL::Mesh_GL(Mesh_GL&& other) :
		numDrawSets{ other.numDrawSets },
		numMaterials{ other.numMaterials },
		drawSetsOffset{ other.drawSetsOffset },
		materialsOffset{ other.materialsOffset },
		meshSceneOffset{ other.meshSceneOffset },
		animationsSize{ other.animationsSize },
		animationsOffset{ other.animationsOffset },
		vertexBufferOffset{ other.vertexBufferOffset },
		indexBufferOffset{ other.indexBufferOffset },
		drawSets{ other.drawSets },
		materials{ other.materials },
		meshScene(std::move(other.meshScene)),
		animations(std::move(other.animations)),
		vertexBuffer(std::move(other.vertexBuffer)),
		indexBuffer(std::move(other.indexBuffer)),
		sizeBytes{ other.sizeBytes },
		modelData(std::move(other.modelData))
	{
		other.numDrawSets = 0;
		other.numMaterials = 0;
		other.drawSetsOffset = 0;
		other.materialsOffset = 0;
		other.meshSceneOffset = 0;
		other.animationsSize = 0;
		other.animationsOffset = 0;
		other.vertexBufferOffset = 0;
		other.indexBufferOffset = 0;
		other.drawSets = nullptr;
		other.materials = nullptr;
		other.sizeBytes = 0;
	}


	Mesh_GL::~Mesh_GL() {
		// delete the VAO objects
		for (u32 ds = 0; ds < numDrawSets; ++ds) {
			glDeleteVertexArrays(1, &drawSets[ds].glVAO);
		}
	}


	void skinning() {
		//Parameters

		const mat4 * poseMatrix = nullptr;
		u8 const * vertexData = nullptr;
		size_t posStride = 0;
		size_t blendIndexStride = 0;
		size_t blendWeightStride = 0;
		size_t numVertices = 0;
		size_t bytesPerVertex = 0;
		int numWeightsPerVertex = 0;

		vec3 max(-_FLT_MAX);
		vec3 min(_FLT_MIN);

		for (size_t i = 0; i < numVertices; ++i) {
			const vec3 *vPos = (const vec3*)vertexData + posStride;
			const u8 *blendIndex = (const u8*)vertexData + blendIndexStride;
			const r32 *blendWeight = (const r32*)vertexData + blendWeightStride;

			for (int j = 0; j < numWeightsPerVertex; ++j) {
				const vec4 translatedPos = poseMatrix[blendIndex[j]] * vec4(*vPos, 1.0f);

				max.x = max(max.x, translatedPos.x);
				max.y = max(max.y, translatedPos.y);
				max.z = max(max.z, translatedPos.z);
				min.x = min(min.x, translatedPos.x);
				min.y = min(min.y, translatedPos.y);
				min.z = min(min.z, translatedPos.z);
			}
			vertexData += bytesPerVertex;
		}
	}


	void Model_GL::render(
		ComponentId modelInstanceId,
		Scene& scene,
		u8 viewport,
		Engine& engine)
	{
		auto& entityMgr = *scene.entityManager;

		auto& instanceStore = entityMgr.getComponentStore<ModelInstance>();
		auto& instance = instanceStore.getComponentRecord(modelInstanceId);
		auto& sceneNode = entityMgr.getComponent<SceneNode>(instance.component.sceneNodeId);

		u32 currentNodeIndex = UINT32_MAX;
		dvec4 currentWorldPosition;
		dquat currentWorldOrientation;

		// TODO: should not be modifying the renderEntries vectors, need to make copies, or use in-place copies made for renderSystem
		for (int re = 0; re < renderEntries.keys.size(); ++re) {
			//renderEntries.keys[re].key.allKeys.fullscreenLayer;
			//renderEntries.keys[re].key.allKeys.sceneLayer;
			//renderEntries.keys[re].key.opaqueKey.frontToBackDepth;
			//renderEntries.keys[re].key.translucentKey.backToFrontDepth;
			
			auto& entry = renderEntries.entries[re];
			entry.entityId = instance.entityId;
			
			if (currentNodeIndex != entry.nodeIndex) {
				currentWorldPosition = dvec4(sceneNode.positionWorld, 1.0) + entry.positionWorld;
				currentWorldOrientation = normalize(sceneNode.orientationWorld + entry.orientationWorld);
				currentNodeIndex = entry.nodeIndex;
			}
			entry.positionWorld = currentWorldPosition;
			entry.orientationWorld = currentWorldOrientation;
		}

		engine.renderSystem->addRenderEntries(viewport, renderEntries.keys, renderEntries.entries);
	}


	void Model_GL::draw(
		ComponentId modelInstanceId,
		u32 drawSetIndex)
	{
		mesh.drawMesh(drawSetIndex);
	}


	void Model_GL::initRenderEntries()
	{
		renderEntries.keys.reserve(mesh.meshScene.numMeshIndices);
		renderEntries.entries.reserve(mesh.meshScene.numMeshIndices);

		struct BFSQueueItem {
			u32		nodeIndex;
			u32		parentNodeIndex;
			dmat4	modelTransform;
		};

		vector_queue<BFSQueueItem> bfsQueue;
		bfsQueue.reserve(mesh.meshScene.numNodes);

		bfsQueue.push({ 0, 0, dmat4() }); // push root node to start traversal

		while (!bfsQueue.empty()) {
			auto& thisItem = bfsQueue.front();

			u32 nodeIndex = thisItem.nodeIndex;
			assert(nodeIndex >= 0 && nodeIndex < mesh.meshScene.numNodes && "node index out of range");

			const auto& node = mesh.meshScene.sceneNodes[thisItem.nodeIndex];

			dmat4 localTransform = thisItem.modelTransform * dmat4(node.transform);
			dvec4 translation = localTransform[3];
			dquat orientation = normalize(quat_cast(localTransform));
			dvec3 scaling{ length(column(localTransform, 0)), length(column(localTransform, 1)), length(column(localTransform, 2)) };
			// TODO: keep an eye on calcs above, assumes there is no skew, use function below if bugs appear
			//decompose(localTransform, scaling, orientation, translation, dvec3(), dvec4());

			// add this node's meshes
			for (u32 m = 0; m < node.numMeshes; ++m) {
				u32 ds = mesh.meshScene.meshIndices[node.meshIndexOffset + m];
				DrawSet& drawSet = mesh.drawSets[ds];
				Material_GL& mat = mesh.materials[drawSet.materialIndex];

				RenderQueueKey key{};
				key.allKeys.sceneLayer = SceneLayer_SceneGeometry;

				if (mat.shaderKey.isTranslucent == 1 && mat.shaderKey.usesAlphaBlend == 1) {
					key.allKeys.sceneLayer = SceneLayer_Translucent;
					key.translucentKey.translucencyType = TranslucencyType_AlphaBlend;
				}
				else if (mat.shaderKey.isTranslucent == 1 && mat.shaderKey.usesAlphaTest == 1) {
					key.allKeys.sceneLayer = SceneLayer_Translucent;
					key.translucentKey.translucencyType = TranslucencyType_AlphaTest;
				}
				key.allKeys.fullscreenLayer = FullscreenLayer_Scene;

				// add key and entry to returned list
				renderEntries.keys.push_back(RenderQueue::KeyType{
					key,
					(u32)renderEntries.entries.size(),
					0
				});

				renderEntries.entries.emplace_back(RenderEntry{
					NullId_T,
					ds,
					nodeIndex,
					thisItem.parentNodeIndex,
					0, // currently unused padding
					translation,
					orientation,
					scaling,
					std::bind(&Model_GL::draw, this, _1, _2)
				});
			}

			// push children to traverse
			for (u32 c = 0; c < node.numChildren; ++c) {
				u32 childNodeIndex = mesh.meshScene.childIndices[node.childIndexOffset + c];

				assert(childNodeIndex >= 0 && childNodeIndex < mesh.meshScene.numNodes && "child node index out of range");
				assert(childNodeIndex > nodeIndex && "child node is not lower in the tree");

				bfsQueue.push({ childNodeIndex, nodeIndex, localTransform });
			}

			bfsQueue.pop();
		}

		assert(renderEntries.keys.capacity() == mesh.meshScene.numMeshIndices && "total draw calls should be equal to total mesh indices");
	}


	// TODO: finish this function, store handles to resources, load shader resource
	//	(but don't cause opengl calls directly... may have to do shader compile in yet another function)
	void Model_GL::loadMaterialResources(const std::wstring& filePath)
	{
		// load shaders and textures
		for (u32 m = 0; m < mesh.numMaterials; ++m) {
			auto& mat = mesh.materials[m];
			for (u32 t = 0; t < mat.numTextures; ++t) {
				auto& tex = mat.textures[t];
				if (tex.textureType != MaterialTexture_None) {
					// convert from ascii to wide character set
					string aName(tex.name);
					wstring wName;
					wName.assign(aName.begin(), aName.end());

					// prefix texture path with the path to the model being loaded
					wName = filePath.substr(0, filePath.find_last_of(L'/')) + L'/' + wName;
					logger.verbose(Logger::Category_Render, "trying to load %s", aName.assign(wName.begin(), wName.end()).c_str());

					// diffuse maps are in sRGB space, all others are not
					bool sRGB = (tex.textureType == MaterialTexture_Diffuse || tex.textureType == MaterialTexture_Diffuse_Occlusion ||
									tex.textureType == MaterialTexture_Diffuse_Opacity || tex.textureType == MaterialTexture_Diffuse_OpacityMask ||
									tex.textureType == MaterialTexture_Diffuse_Height || tex.textureType == MaterialTexture_Diffuse_Specular);

					// TEMP, right now loading is blocking, called on opengl thread. Once tasks are used, switch to calling from the loading thread.
					auto resHandle = render::loadTexture2D(wName, resource::CacheType::Cache_Materials, sRGB);

					// TEMP, blocking call, need to make this async, use task system
					// BUT, the continuation must update this handle, assuming "this" pointer is captured by reference,
					// the material may move in memory, since the resource system is free to move it.
					// Potential bug, use the resource ids to look up by handle to get its current memory location from the task
					// Also, can this model move in memory while tasks are queued holding a memory location back? Don't allow that to happen.
					tex.textureResourceHandle = resHandle.handle();
					auto texPtr = render::g_resourceLoader.lock()->getResource(tex.textureResourceHandle, resource::CacheType::Cache_Materials);
					auto& tex = texPtr->getResource<Texture2D_GL>();
					
					// TODO: where should I call the following??? Can't call here since this is called on loading thread
					//pTex->bind(GL_TEXTURE0);
					//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				}
			}
		}
	}

}