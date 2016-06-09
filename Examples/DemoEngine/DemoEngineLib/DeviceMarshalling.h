#ifndef REALTIME_ENGINE_DEVICE_MARSHALLING_H
#define REALTIME_ENGINE_DEVICE_MARSHALLING_H

#include "CoreLib/Basic.h"
#include "CoreLib/Graphics.h"
#include "ModelResource.h"
#include "CoreLib/LibGL.h"

namespace RealtimeEngine
{
	enum class UniformType
	{
		Texture2D, TextureCube,
		ConstantFloat, ConstantFloat2, ConstantFloat3, ConstantFloat4, ConstantMatrix3, ConstantMatrix4,
		ConstantInt, ConstantInt2, ConstantInt3, ConstantInt4,
	};

	class UniformValue
	{
	public:
		UniformType Type;
		union
		{
			float FloatValues[16];
			int IntValues[16];
			GL::TextureHandle TextureHandle;
		};
		CoreLib::Basic::String TextureName;
		UniformValue()
		{
		}
		UniformValue(UniformType type, CoreLib::Basic::String value)
		{
			Type = type;
			TextureName = value;
		}
		UniformValue(UniformType type, float value)
		{
			Type = type;
			FloatValues[0] = value;

		}
		UniformValue(UniformType type, int value)
		{
			Type = type;
			IntValues[0] = value;

		}
		UniformValue(UniformType type, VectorMath::Vec2 value)
		{
			Type = type;
			FloatValues[0] = value.x;
			FloatValues[1] = value.y;
			FloatValues[2] = 0.0f;
			FloatValues[3] = 0.0f;
		}
		UniformValue(UniformType type, VectorMath::Vec4 value)
		{
			Type = type;
			FloatValues[0] = value.x;
			FloatValues[1] = value.y;
			FloatValues[2] = value.z;
			FloatValues[3] = value.w;
		}
		String ToString();
	};

	class SystemUniforms
	{
	public:
		CoreLib::Graphics::ViewFrustum Views[2];
		Vec3 LightDir, LightColor;
		float Time = 0.0f;
	};

	class BufferRange
	{
	public:
		uint64_t IndexStart = 0, IndexEnd = 0;
	};

	class DeviceMesh : public Object
	{
	private:
		GL::HardwareRenderer * hw;
	public:
		GL::VertexArray VAO;
		GL::BufferObject MeshBuffer;
		GL::BufferObject * IndexBuffer = nullptr;
		int BufferSize = 0;
		int IndexCount = 0;
		List<VertexAttribute> Attributes;
		CoreLib::Graphics::BBox Bounds;
		DeviceMesh(GL::HardwareRenderer * pHw)
		{
			hw = pHw;
			VAO = hw->CreateVertexArray();
			MeshBuffer = hw->CreateBuffer(GL::BufferUsage::ArrayBuffer);
		}
		~DeviceMesh()
		{
			hw->DestroyBuffer(MeshBuffer);
			hw->DestroyVertexArray(VAO);
		}
		int ConfigVertexArray();
		void Draw(GL::PrimitiveType ptype = GL::PrimitiveType::Triangles);
		void Draw(GL::CommandBuffer & cmdBuffer);
	};
}
#endif
