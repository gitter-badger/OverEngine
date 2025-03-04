#pragma once

#include "RenderCommand.h"

#include "OverEngine/Renderer/Shader.h"
#include "OverEngine/Renderer/Camera.h"
#include "OverEngine/Renderer/Texture.h"

namespace OverEngine
{
	struct TexturedQuadExtraData
	{
		Color Tint = Color(1.0f);

		Vector2 Tiling = Vector2(1.0f);
		Vector2 Offset = Vector2(0.0f);
		Vec2T<bool> Flip{ false, false };

		Vec2T<TextureWrapping> Wrapping{ TextureWrapping::None, TextureWrapping::None };
		TextureFiltering Filtering = TextureFiltering::None;

		float AlphaClipThreshold = 0.0f;

		/**
		 * first : Is overriding TextureBorderColor?
		 * second : Override TextureBorderColor to what?
		 */
		std::pair<bool, Color> TextureBorderColor{ false, Color(1.0f) };
	};

	class Renderer2D
	{
	public:
		static void Init(uint32_t initQuadCapacity = 10);
		static void Shutdown();
		static Ref<Shader>& GetShader();

		static void Reset();

		static void BeginScene(const Mat4x4& viewProjectionMatrix);
		static void BeginScene(const Mat4x4& viewMatrix, const Camera& camera);
		static void BeginScene(const Mat4x4& viewMatrix, const Mat4x4& projectionMatrix);

		static void EndScene();
		static void Flush();
		static void FlushAndReset();

		inline static void DrawQuad(const Vector2& position, float rotation, const Vector2& size, const Color& color, float alphaClippingThreshold = 0.0f);
		static void DrawQuad(const Vector3& position, float rotation, const Vector2& size, const Color& color, float alphaClippingThreshold = 0.0f);
		static void DrawQuad(const Mat4x4& transform, const Color& color, float alphaClippingThreshold = 0.0f);

		inline static void DrawQuad(const Vector2& position, float rotation, const Vector2& size, Ref<Texture2D> texture, const TexturedQuadExtraData& extraData = TexturedQuadExtraData());
		static void DrawQuad(const Vector3& position, float rotation, const Vector2& size, Ref<Texture2D> texture, const TexturedQuadExtraData& extraData = TexturedQuadExtraData());
		static void DrawQuad(const Mat4x4& transform, Ref<Texture2D> texture, const TexturedQuadExtraData& extraData = TexturedQuadExtraData());

		struct Statistics
		{
			void Reset()
			{
				QuadCount = 0;
				DrawCalls = 0;
			}

			uint32_t GetIndexCount() { return 6 * QuadCount; }
			uint32_t GetVertexCount() { return 4 * QuadCount; }

			uint32_t QuadCount;
			uint32_t DrawCalls;
		};

		static Statistics& GetStatistics() { return s_Statistics; }
	private:
		static Statistics s_Statistics;
	};
}
