#include "pcheader.h"
#include "Renderer2D.h"

#include "Texture.h"

namespace OverEngine
{
	Renderer2D::Statistics Renderer2D::s_Statistics;

	struct Vertex
	{
		Vector4 a_Position = Vector4(0.0f);
		Color a_Color = Color(0.0f);

		float a_TexSlot                   = -1.0f;
		float a_TexFilter                 = 0.0f;
		float a_TexAlphaClippingThreshold = 0.0f;
		Vector2 a_TexWrapping             = Vector2(0.0f);
		Color a_TexBorderColor            = Color(0.0f);
		Rect a_TexRect                    = Rect(0.0f);
		Vector2 a_TexSize                 = Vector2(0.0f);
		Vector2 a_TexCoord                = Vector2(0.0f);
		Rect a_TexCoordRange              = Vector4(0.0f);
	};

	using DrawQuadVertices = std::array<Vertex, 4>;
	using DrawQuadIndices = std::array<uint32_t, 6>;

	struct Renderer2DData
	{
		Ref<VertexArray> vertexArray = nullptr;
		Ref<VertexBuffer> vertexBuffer = nullptr;
		Ref<IndexBuffer> indexBuffer = nullptr;

		Vector<DrawQuadVertices> Vertices;
		Vector<DrawQuadIndices> Indices;
		Vector<float> QuadsZIndices;

		uint32_t QuadCapacity;
		uint32_t FlushingQuadCount;

		uint32_t OpaqueInsertIndex = 0;

		Ref<Shader> BatchRenderer2DShader = nullptr;

		UnorderedMap<uint32_t, Ref<GAPI::Texture2D>> TextureBindList;

		Mat4x4 ViewProjectionMatrix;

		static constexpr float QuadVertices[3 * 4] = {
			-0.5f, -0.5f, 0.0f,
			 0.5f, -0.5f, 0.0f,
			 0.5f,  0.5f, 0.0f,
			-0.5f,  0.5f, 0.0f
		};

		static constexpr uint32_t QuadIndices[6] = { 0, 1, 2, 2, 3, 0 };

		static constexpr int ShaderSampler2Ds[32] = {
			0, 1, 2, 3, 4, 5, 6, 7,
			8, 9, 10, 11, 12, 13, 14, 15,
			16, 17, 18, 19, 20, 21, 22, 23,
			24, 25, 26, 27, 28, 29, 30, 31
		};
	};

	static Renderer2DData* s_Data;

	static void InsertVertices(bool transparent, const float& z, const DrawQuadVertices& vertices)
	{
		if (transparent)
		{
			for (auto idx = s_Data->Vertices.begin() + s_Data->OpaqueInsertIndex; idx < s_Data->Vertices.end(); idx++)
			{
				auto i = idx - s_Data->Vertices.begin();
				if (z < s_Data->QuadsZIndices[i])
				{
					s_Data->Vertices.emplace(idx, vertices);
					s_Data->QuadsZIndices.emplace(s_Data->QuadsZIndices.begin() + i, z);
					return;
				}
			}

			s_Data->Vertices.emplace_back(vertices);
			s_Data->QuadsZIndices.emplace_back(z);
			return;
		}

		// Opaque
		s_Data->Vertices.emplace(s_Data->Vertices.begin(), vertices);
		s_Data->QuadsZIndices.emplace(s_Data->QuadsZIndices.begin(), z);
		s_Data->OpaqueInsertIndex++;
	}

	static void GenIndices(uint32_t quadCount, uint32_t indexCount)
	{
		// Allocate storage on GPU memory
		s_Data->indexBuffer->AllocateStorage(indexCount);

		s_Data->Indices.reserve(quadCount);
		for (uint32_t i = (uint32_t)s_Data->Indices.size(); i < quadCount; i++)
		{
			// Generate indices
			DrawQuadIndices indices;
			for (uint32_t j = 0; j < 6; j++)
				indices[j] = Renderer2DData::QuadIndices[j] + 4 * i;
			s_Data->Indices.push_back(indices);
		}

		// Upload data to GPU
		s_Data->indexBuffer->BufferSubData((uint32_t*)s_Data->Indices.data(), indexCount);
	}

	void Renderer2D::Init(uint32_t initQuadCapacity)
	{
		s_Data = new Renderer2DData();

		s_Data->vertexArray = VertexArray::Create();
		s_Data->QuadCapacity = initQuadCapacity;

		s_Data->vertexBuffer = VertexBuffer::Create();
		s_Data->vertexBuffer->AllocateStorage(initQuadCapacity * 4 * sizeof(Vertex));
		s_Data->vertexBuffer->SetLayout({
			{ ShaderDataType::Float4, "a_Position" },
			{ ShaderDataType::Float4, "a_Color" },

			{ ShaderDataType::Float , "a_TexSlot" },
			{ ShaderDataType::Float , "a_TexFilter" },
			{ ShaderDataType::Float , "a_TexAlphaClippingThreshold" },
			{ ShaderDataType::Float2, "a_TexWrapping" },
			{ ShaderDataType::Float4, "a_TexBorderColor" },
			{ ShaderDataType::Float4, "a_TexRect" },
			{ ShaderDataType::Float2, "a_TexSize" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Float4, "a_TexCoordRange" },
		});
		s_Data->vertexArray->AddVertexBuffer(s_Data->vertexBuffer);

		s_Data->indexBuffer = IndexBuffer::Create();
		s_Data->indexBuffer->AllocateStorage(initQuadCapacity * 6);
		s_Data->vertexArray->SetIndexBuffer(s_Data->indexBuffer);

		s_Data->Vertices.reserve(initQuadCapacity);
		GenIndices(initQuadCapacity, 6 * initQuadCapacity);
		s_Data->QuadsZIndices.reserve(initQuadCapacity);

		s_Data->BatchRenderer2DShader = Shader::Create("assets/shaders/BatchRenderer2D.glsl");
		//s_Data->BatchRenderer2DShader = Shader::Create("BatchRenderer2D", BatchRenderer2DVertexShaderSrc, BatchRenderer2DFragmentShaderSrc);
		s_Data->BatchRenderer2DShader->Bind();
		s_Data->BatchRenderer2DShader->UploadUniformIntArray("u_Slots", Renderer2DData::ShaderSampler2Ds, 32);

		s_Statistics.Reset();
	}

	void Renderer2D::Shutdown()
	{
		delete s_Data;
	}

	Ref<Shader>& Renderer2D::GetShader()
	{
		return s_Data->BatchRenderer2DShader;
	}

	void Renderer2D::Reset()
	{
		s_Data->Vertices.clear();
		s_Data->QuadsZIndices.clear();
		s_Data->OpaqueInsertIndex = 0;
		s_Data->FlushingQuadCount = 0;
		s_Data->TextureBindList.clear();

		s_Statistics.Reset();
	}

	void Renderer2D::BeginScene(const Mat4x4& viewProjectionMatrix)
	{
		Reset();
		s_Data->ViewProjectionMatrix = viewProjectionMatrix;
	}

	void Renderer2D::BeginScene(const Mat4x4& viewMatrix, const Camera& camera)
	{
		Reset();
		s_Data->ViewProjectionMatrix = camera.GetProjection() * viewMatrix;
	}

	void Renderer2D::BeginScene(const Mat4x4& viewMatrix, const Mat4x4& projectionMatrix)
	{
		Reset();
		s_Data->ViewProjectionMatrix = projectionMatrix * viewMatrix;
	}

	void Renderer2D::EndScene()
	{
		Flush();
	}

	void Renderer2D::Flush()
	{
		if (s_Data->FlushingQuadCount == 0) // Nothing to draw
			return;

		uint32_t indexCount = s_Statistics.GetIndexCount();

		// Grow GPU Buffers
		if (s_Data->QuadCapacity < s_Data->FlushingQuadCount)
		{
			s_Data->QuadCapacity = s_Data->FlushingQuadCount;
			s_Data->vertexBuffer->AllocateStorage(s_Data->QuadCapacity * 4 * sizeof(Vertex));
			GenIndices(s_Data->QuadCapacity, indexCount);
		}

		// Upload Data
		s_Data->vertexBuffer->BufferSubData((float*)s_Data->Vertices.data(), s_Statistics.GetVertexCount() * sizeof(Vertex));

		// Bind Textures
		for (auto& t : s_Data->TextureBindList)
			t.second->Bind(t.first);

		// Bind VertexArray
		s_Data->vertexArray->Bind();

		// Bind Shader
		s_Data->BatchRenderer2DShader->Bind();

		// DrawCall
		RenderCommand::DrawIndexed(s_Data->vertexArray, indexCount);
		s_Statistics.DrawCalls++;
	}

	void Renderer2D::FlushAndReset()
	{
		Flush();

		s_Data->Vertices.clear();
		s_Data->QuadsZIndices.clear();
		s_Data->OpaqueInsertIndex = 0;
		s_Data->FlushingQuadCount = 0;
		s_Data->TextureBindList.clear();
	}

	/////////////////////////////////////////////////////////
	// FlatColor Quad ///////////////////////////////////////
	/////////////////////////////////////////////////////////

	void Renderer2D::DrawQuad(const Vector2& position, float rotation, const Vector2& size, const Color& color, float alphaClippingThreshold)
	{
		DrawQuad(Vector3(position, 0.0f), rotation, size, color, alphaClippingThreshold);
	}

	void Renderer2D::DrawQuad(const Vector3& position, float rotation, const Vector2& size, const Color& color, float alphaClippingThreshold)
	{
		Mat4x4 transform =
			glm::translate(Mat4x4(1.0f), position) *
			glm::rotate(Mat4x4(1.0f), rotation, Vector3(0, 0, 1)) *
			glm::scale(Mat4x4(1.0f), Vector3(size, 1.0f));

		DrawQuad(transform, color, alphaClippingThreshold);
	}

	void Renderer2D::DrawQuad(const Mat4x4& transform, const Color& color, float alphaClippingThreshold)
	{
		if (color.a <= alphaClippingThreshold)
			return;

		bool transparent = color.a < 1.0f;

		DrawQuadVertices vertices;

		for (int i = 0; i < 4; i++)
		{
			Vertex& vertex = vertices[i];

			vertex.a_Position.x = Renderer2DData::QuadVertices[3 * i];
			vertex.a_Position.y = Renderer2DData::QuadVertices[1 + 3 * i];
			vertex.a_Position.z = Renderer2DData::QuadVertices[2 + 3 * i];
			vertex.a_Position.w = 1.0f;
			vertex.a_Position = s_Data->ViewProjectionMatrix * transform * vertex.a_Position;

			vertex.a_Color = color;
		}

		InsertVertices(transparent, transform[3].z, vertices);

		s_Statistics.QuadCount++;
		s_Data->FlushingQuadCount++;
	}

	/////////////////////////////////////////////////////////
	// Textured Quad ////////////////////////////////////////
	/////////////////////////////////////////////////////////

	void Renderer2D::DrawQuad(const Vector2& position, float rotation, const Vector2& size, Ref<Texture2D> texture, const TexturedQuadExtraData& extraData)
	{
		DrawQuad(Vector3(position, 0.0f), rotation, size, texture, extraData);
	}

	void Renderer2D::DrawQuad(const Vector3& position, float rotation, const Vector2& size, Ref<Texture2D> texture, const TexturedQuadExtraData& extraData)
	{
		Mat4x4 transform =
			glm::translate(Mat4x4(1.0f), position) *
			glm::rotate(Mat4x4(1.0f), rotation, Vector3(0, 0, 1)) *
			glm::scale(Mat4x4(1.0f), Vector3(size, 1.0f));

		DrawQuad(transform, texture, extraData);
	}

	void Renderer2D::DrawQuad(const Mat4x4& transform, Ref<Texture2D> texture, const TexturedQuadExtraData& extraData)
	{
		if (!texture || texture->GetType() == TextureType::Placeholder)
			return;

		Ref<GAPI::Texture2D> textureToBind = texture->GetGPUTexture();

		int textureSlot = -1;
		for (const auto& t : s_Data->TextureBindList)
		{
			if (*t.second == *textureToBind)
			{
				textureSlot = t.first;
				break;
			}
		}

		if (textureSlot == -1)
		{
			uint32_t slot = (uint32_t)s_Data->TextureBindList.size();
			if (slot + 1 > RenderCommand::GetMaxTextureSlotCount())
			{
				Flush();
				Reset();
				slot = 0;
			}
			s_Data->TextureBindList[slot] = textureToBind;
			textureSlot = slot;
		}

		if (extraData.AlphaClipThreshold >= 1.0f || extraData.Tint.a <= extraData.AlphaClipThreshold)
			return;

		bool transparent = extraData.Tint.a < 1.0f || texture->GetFormat() == TextureFormat::RGBA;

		DrawQuadVertices vertices;

		for (int i = 0; i < 4; i++)
		{
			Vertex& vertex = vertices[i];

			vertex.a_Position.x = Renderer2DData::QuadVertices[    3 * i];
			vertex.a_Position.y = Renderer2DData::QuadVertices[1 + 3 * i];
			vertex.a_Position.z = Renderer2DData::QuadVertices[2 + 3 * i];
			vertex.a_Position.w = 1.0f;
			vertex.a_Position = s_Data->ViewProjectionMatrix * transform * vertex.a_Position;

			vertex.a_Color = extraData.Tint;

			// a_TexSlot
			vertex.a_TexSlot = (float)textureSlot;

			// a_TexFilter
			if (extraData.Filtering != TextureFiltering::None)
				vertex.a_TexFilter = (float)extraData.Filtering;
			else
				vertex.a_TexFilter = (float)texture->GetFiltering();

			vertex.a_TexAlphaClippingThreshold = extraData.AlphaClipThreshold;

			// a_TexSWrapping & a_TexTWrapping
			if (extraData.Wrapping.x != TextureWrapping::None)
				vertex.a_TexWrapping.x = (float)extraData.Wrapping.x;
			else
				vertex.a_TexWrapping.x = (float)texture->GetXWrapping();

			if (extraData.Wrapping.y != TextureWrapping::None)
				vertex.a_TexWrapping.y = (float)extraData.Wrapping.y;
			else
				vertex.a_TexWrapping.y = (float)texture->GetYWrapping();

			// a_TexBorderColor
			if (extraData.TextureBorderColor.first)
				vertex.a_TexBorderColor = extraData.TextureBorderColor.second;
			else
				vertex.a_TexBorderColor = texture->GetBorderColor();

			// a_TexRect
			vertex.a_TexRect = texture->GetRect();

			// a_TexSize
			vertex.a_TexSize = { texture->GetWidth() ,texture->GetHeight() };

			// a_TexCoord
			{
				float xTexCoord = Renderer2DData::QuadVertices[    3 * i] > 0.0f ? extraData.Tiling.x + extraData.Offset.x : extraData.Offset.x;
				float yTexCoord = Renderer2DData::QuadVertices[1 + 3 * i] > 0.0f ? extraData.Tiling.y + extraData.Offset.y : extraData.Offset.y;
				vertex.a_TexCoord.x = extraData.Flip.x ? extraData.Tiling.x - xTexCoord : xTexCoord;
				vertex.a_TexCoord.y = extraData.Flip.y ? extraData.Tiling.y - yTexCoord : yTexCoord;

				vertex.a_TexCoordRange.x = extraData.Flip.x ? extraData.Tiling.x - extraData.Offset.x : extraData.Offset.x; // Min X
				vertex.a_TexCoordRange.y = extraData.Flip.y ? extraData.Tiling.y - extraData.Offset.y : extraData.Offset.y; // Min Y

				float maxXTexCoord = extraData.Tiling.x + extraData.Offset.x;
				float maxYTexCoord = extraData.Tiling.y + extraData.Offset.y;

				vertex.a_TexCoordRange.z = extraData.Flip.x ? extraData.Tiling.x - maxXTexCoord : maxXTexCoord; // Max X
				vertex.a_TexCoordRange.w = extraData.Flip.y ? extraData.Tiling.y - maxYTexCoord : maxYTexCoord; // Max Y
			}
		}

		InsertVertices(transparent, transform[3].z, vertices);

		s_Statistics.QuadCount++;
		s_Data->FlushingQuadCount++;
	}
}
