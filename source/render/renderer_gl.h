#ifndef _RENDERER_GL_H
#define _RENDERER_GL_H

#include "utility/common.h"
#include "scene/scene_api.h"
#include "material_gl.h"
#include "buffers_gl.h"
#include "shader_gl.h"
#include "texture_gl.h"
#include "mesh_gl.h"


namespace render
{
	/**
	 * Global fonts loaded at startup, in memory for the lifetime of the render system.
	 * Pass these values to getFontId() function to get a font handle for the vector renderer.
	 */
	enum FontFace : u8 {
		Sans = 0,
		SansBold,
		SansItalic,
		SansBoldItalic,
		Exo,
		ExoBold,
		ExoItalic,
		ExoBoldItalic
	};

	/**
	 * Layer of geometry for the scene render passes, specified in render queue key. Value is
	 * ignored for ScenePostPass and FinalPostPass layers.
	 */
	enum RenderQueueSceneLayer : u8 {
		SceneLayer_SceneGeometry = 0,	// scene geometry for the g-buffer pass
		SceneLayer_LightVolumeGeometry,	// light volume geometry for deferred renderer
		SceneLayer_Skybox,				// skybox is a special case handled in the deferred renderer
		SceneLayer_Translucent,			// translucent geometry is rendered after framebuffer composition
		SceneLayer_VectorGeometry		// uses the vector renderer to overlay into the scene
	};

	/**
	 * Fullscreen layer determines order of composition. Each layer is effectively its own
	 * scene with unique geometry. The ScenePostPass and FinalPostPass layers are special cases
	 * where custom fullscreen filters can be added to the main scene, and final framebuffer.
	 */
	enum RenderQueueFullscreenLayer : u8 {
		FullscreenLayer_Scene = 0,
		FullscreenLayer_ScenePostPass,
		FullscreenLayer_HUD,
		FullscreenLayer_UI,
		FullscreenLayer_Debug,
		FullscreenLayer_FinalPostPass
	};

	enum RenderQueueTranslucencyType : u8 {
		TranslucencyType_AlphaTest = 0,	// alpha test only no blend
		TranslucencyType_AlphaBlend		// using typical src_alpha, one_minus_src_alpha blend
	};


	// Variables
	
	extern int g_fonts[FontFaceCount];

	/**
	 * Helper to get a font face id
	 */
	inline int getFontId(FontFace face) { return g_fonts[face]; }


	// Types

	/**
	 * @struct RenderQueueKey
	 * @var	frontToBackDepth	depth value converted to 32 bit integer, used for opaque objects
	 * @var	backToFrontDepth	inverse depth used for translucent objects
	 * @var	material			internal material feature bits, determines shader
	 * @var	instanced			0=no, 1=yes mesh is instanced
	 * @var	translucencyType	one of the RenderQueueTranslucencyType values
	 * @var	sceneLayer			one of the RenderQueueSceneLayer values
	 * @var	fullscreenLayer		one of the RenderQueueFullscreenLayer values
	 * @var	value				unioned with the vars above, used for sorting
	 */
	struct RenderQueueKey {
		union {
			struct AllKeys {			// AllKeys set for both opaque and translucent
				u32	_padding;
				u16	_padding1;

				u16	instanced        : 1;
				u16	sceneLayer       : 4;
				u16	fullscreenLayer  : 4;
				u16	_reserved        : 7;
			};

			struct OpaqueKey {			// OpaqueKey 48 bits split between material and depth
				u32	frontToBackDepth;
				u16	material;

				u16	_padding2;
			};

			struct TranslucentKey {		// TranslucentKey 46 bits for depth
				u64	translucencyType : 2;
				u64	backToFrontDepth : 46;

				u64	_padding3 : 16;
			};

			AllKeys			allKeys;
			OpaqueKey		opaqueKey;
			TranslucentKey	translucentKey;

			u64		value;
		};
	};


	struct RenderEntry {
		/**
		 * @param EntityId	entity id
		 * @param u32		drawset/submesh index to render
		 */
		typedef void DrawCallback(EntityId, u32);

		EntityId	entityId;
		u32			drawsetIndex;
		u32			nodeIndex;
		u32			parentNodeIndex;

		u32			_padding;

		dvec4		positionWorld;
		dquat		orientationWorld;
		dvec3		scale;
		
		DrawCallback*	drawCallback;
		// render flags?
	};


	struct RenderQueue
	{
		struct KeyType {
			RenderQueueKey	key;
			u32				entryIndex;
			u32				_padding;
		};
		typedef std::vector<KeyType>		KeyList;
		typedef std::vector<RenderEntry>	EntryList;

		explicit RenderQueue() {
			keys.reserve(RESERVE_RENDER_QUEUE);
			filteredKeys.reserve(RESERVE_RENDER_QUEUE);
			entries.reserve(RESERVE_RENDER_QUEUE);
		}

		~RenderQueue();

		void addRenderEntry(RenderQueueKey sortKey, RenderEntry&& entry);

		/**
		 * Sorts the keys, will be the traversal order for rendering
		 */
		void sortRenderQueue();

		/**
		 * Clear the keys and entries lists each frame
		 */
		void clearRenderEntries() {
			keys.clear();
			entries.clear();
		}

		KeyList		keys;			// set of keys to be sorted for rendering order
		KeyList		filteredKeys;	// filtered set of keys used internally by render system
		EntryList	entries;		// list of render entries for a frame
	};


	/**
	 * Maximum number of render viewports. Multiple viewports can be used for multi-screen
	 * rendering where each screen contains a separate camera view.
	 */
	#define MAX_VIEWPORTS	16

	struct ViewParameters {
		dmat4	viewMat;
		mat4	projMat;
		mat4	viewProjMat;
		r32		nearClipPlane;
		r32		farClipPlane;
		r32		frustumDistance;		// = farClipPlane - nearClipPlane
		r32		inverseFrustumDistance;	// = 1.0f / frustumDistance
	};


	struct Viewport {
		u32					left;
		u32					top;
		u32					width;
		u32					height;
		ViewParameters		params;
		RenderQueue			renderQueue;
		bool				display;
		std::bitset<RenderQueueFullscreenLayerCount> fullscreenLayers; // bits indicating which fullscreen layers are rendered in the viewport
		//RenderTarget ?? optional fbo, for rendering viewport to texture
	};


	struct DeferredRenderer_GL
	{
		RenderTarget_GL		gbuffer;					// g-buffer for deferred rendering
		RenderTarget_GL		colorBuffer;				// color buffer for FXAA

		ShaderId			mrtProgram;				// multiple render target geometry pass, renders the g-buffer
		ShaderId			skyboxProgram;			// skybox render from cubemap texture
		ShaderId			fullScreenQuadProgram;	// fullscreen quad program for deferred lighting and post-processing
		ShaderId			ssaoProgram;				// post-process screen space ambient occlusion shader
		ShaderId			fxaaProgram;				// post-process FXAA shader

		ShaderId			normalsTexture;			// random normal noise texture for ssao
		
		// temp, will be part of render queue in its own layer
		ShaderId			skyboxTexture;			// optional skybox cubemap passed in from external source


		explicit DeferredRenderer_GL() :
			gbuffer(RenderTarget_GL::TypeGBuffer),
			colorBuffer(RenderTarget_GL::TypeColor)
		{}
		~DeferredRenderer_GL();

		/**
		 * Initialize the renderer
		 */
		void init(int viewportWidth, int viewportHeight);

		/**
		 * Renders a viewport's render queue keys.
		 * @var viewport	the viewport to render
		 * @var keys		list of keys to render, usually set to viewport.filteredKeys (after
		 *					filtering) or viewport.keys for all, but any KeyList can be passed
		 */
		void renderGBuffer(Viewport& viewport, const RenderQueue::KeyList& keys);
		void renderLightVolumes(Viewport& viewport, const RenderQueue::KeyList& keys);
		void renderSkybox(Viewport& viewport, const RenderQueue::KeyList& keys);
		void renderPostProcess(Viewport& viewport, const RenderQueue::KeyList& keys);

		// temp, will be part of render queue in its own layer
		void setSkyboxTexture(const ResourcePtr& textureCubeMap) {
			skyboxTexture = textureCubeMap;
		}
	};


	struct VectorRenderer_GL
	{
		NVGcontext*		nvg;
		RenderTarget_GL	colorBuffer;	// color buffer for rendering in linear space

		ResourcePtr		testNoiseTexture;


		explicit VectorRenderer_GL() {}
		~VectorRenderer_GL();

		/**
		 * Initialize the renderer
		 */
		void init(int viewportWidth, int viewportHeight);

		/**
		 * Renders a viewport's render queue keys.
		 * @var viewport	the viewport to render
		 * @var keys		list of keys to render, usually set to viewport.filteredKeys (after
		 *					filtering) or viewport.keys for all, but any KeyList can be passed
		 */
		void renderViewport(Viewport& viewport, const RenderQueue::KeyList& keys);

		static void loadGlobalFonts(VectorRenderer_GL& inst);

	};


	/**
	 * need the systems to
	 *	- get full render states 1 and 2 from entity store, each holds copy of components
	 *		- if object in state 1 is missing from state 2, or vice versa, don't render it
	 *		- for each component in both states, interpolate between state 1 and state 2
	 *			- update position to interpolated value in quadtree culling system
	 *		- for each viewport, active camera's frustum sent to culling system
	 *			- frustum culled against quad tree objects, sends back list of rendered objects
	 *			- render called on each object, submits RenderEntry to render system
	 *
	 *	- update on fixed timesteps
	 *		- scene graph transforms updated, with dirty flag optimization
	 *		- when new scene info is ready, queue an event to tell render thread to swap states
	 */
	struct RenderSystem
	{
		Viewport			viewports[MAX_VIEWPORTS];
		u32					uboHandles[UBOTypeCount];
		DeferredRenderer_GL	deferredRenderer;
		VectorRenderer_GL	vectorRenderer;


		//typedef std::vector<Viewport>	ViewportList;

		explicit RenderSystem() {}

		~RenderSystem();

		void init(int viewportWidth, int viewportHeight);

		/**
		 * Separate call to initialize fonts since they are global state. Must be called after
		 * init, uses the vector renderer instance.
		 */
		void loadGlobalFonts() {
			VectorRenderer_GL::loadGlobalFonts(vectorRenderer);
		}

		/**
		 * Gets the OpenGL handle for a UBO. Used by the shader program loader to bind UBOs
		 * to uniform blocks after linking.
		 */
		u32 getUBOHandle(UBOType uboType) const {
			return uboHandles[uboType];
		}

		/**
		 * Sets the camera matrices and clip planes. Needs to be set for each viewport rendered
		 * and any time the view parameters change, usually once per frame.
		 */
		void setViewParameters(u8 viewport, ViewParameters&& viewParams)
		{
			assert(viewport < MAX_VIEWPORTS && "viewport out of range");

			viewports[viewport].params = std::forward<ViewParameters>(viewParams);
			viewports[viewport].display = true;
		}

		/**
		 * Adds render entries into the viewport's render queue.
		 */
		void addRenderEntries(u8 viewport, const std::vector<RenderQueue::KeyType>& keys, const std::vector<RenderEntry>& entries)
		{
			assert(viewport < MAX_VIEWPORTS && "viewport out of range");
			assert(keys.size() == entries.size() && "keys and entries sizes must match");

			RenderQueue& queue = viewports[viewport].renderQueue;

			queue.keys.reserve(queue.keys.size() + keys.size());
			queue.entries.reserve(queue.entries.size() + entries.size());

			int startingSize = (int)(queue.keys.size());
			int size = (int)(keys.size());

			for (int k = 0; k < size; ++k) {
				queue.keys.push_back(keys[k]);
				queue.keys.back().entryIndex += startingSize;
			}

			for (int e = 0; e < size; ++e) {
				queue.entries.push_back(entries[e]);
			}
		}

		void renderFrame(r32 interpolation);

		// temp, will be part of render queue in its own layer
		void setSkyboxTexture(const ResourcePtr& textureCubeMap) {
			deferredRenderer.setSkyboxTexture(textureCubeMap);
		}
	};

}


#endif