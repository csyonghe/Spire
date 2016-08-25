#include "DeviceMarshalling.h"
#include "DeviceResourcePool.h"

namespace DemoEngine
{
	using namespace GL;

	void DeviceMesh::Draw(PrimitiveType ptype)
	{
		auto size = ConfigVertexArray();
		if (IndexBuffer)
			hw->DrawInstances(DataType::Int, ptype, 1, IndexCount);
		else
			hw->DrawArray(ptype, 0, BufferSize / size);
	}

	String UniformValue::ToString()
	{
		StringBuilder sb;
		switch (Type)
		{
		case UniformType::Texture2D:
		case UniformType::TextureCube:
			return String((long long)TextureHandle.Handle);
		case UniformType::ConstantFloat:
			return String(FloatValues[0]);
		case UniformType::ConstantFloat2:
			sb << FloatValues[0] << L"," << FloatValues[1];
			break;
		case UniformType::ConstantFloat3:
			sb << FloatValues[0] << L"," << FloatValues[1] << FloatValues[2];
			break;
		case UniformType::ConstantFloat4:
			sb << FloatValues[0] << L"," << FloatValues[1] << FloatValues[2] << FloatValues[3];
			break;
		case UniformType::ConstantMatrix3:
			for (int i = 0; i < 9; i++)
				sb << FloatValues[i] << L",";
			break;
		case UniformType::ConstantMatrix4:
			for (int i = 0; i < 16; i++)
				sb << FloatValues[i] << L",";
			break;
		case UniformType::ConstantInt:
			return String(IntValues[0]);
		case UniformType::ConstantInt2:
			sb << IntValues[0] << L"," << IntValues[1];
			break;
		case UniformType::ConstantInt3:
			sb << IntValues[0] << L"," << IntValues[1] << L"," << IntValues[2];
			break;
		case UniformType::ConstantInt4:
			sb << IntValues[0] << L"," << IntValues[1] << L"," << IntValues[2] << L"," << IntValues[3];
			break;
		}
		return sb.ProduceString();
	}

	void DeviceMesh::Draw(GL::CommandBuffer & cmdBuffer)
	{
		auto meshPtr = MeshBuffer.GetGpuAddress();
		auto vformat = CreateVertexFormat(Attributes);
		for (int i = 0; i < vformat.Attributes.Count(); i++)
		{
			cmdBuffer.AttributeAddress(i, meshPtr + vformat.Attributes[i].StartOffset);
		}
		if (IndexBuffer)
		{
			auto elementAddr = IndexBuffer->GetGpuAddress();
			cmdBuffer.ElementAddress(elementAddr, sizeof(int));
			cmdBuffer.DrawElements(0, 0, IndexCount);
		}
		else
			cmdBuffer.DrawArrays(0, BufferSize / vformat.Size);
	}
	int DeviceMesh::ConfigVertexArray()
	{
		auto meshPtr = MeshBuffer.GetGpuAddress();
		auto vformat = CreateVertexFormat(Attributes);
		hw->BindVertexArray(VAO);
		for (int i = 0; i < vformat.Attributes.Count(); i++)
		{
			int id = i;
			if (vformat.Attributes[i].Binding != -1)
				id = vformat.Attributes[i].Binding;
			glEnableVertexAttribArray(id);
			glVertexAttribFormatNV(id, GetDataTypeComponenets(vformat.Attributes[i].Type),
				GL::TranslateDataTypeToInputType(vformat.Attributes[i].Type), vformat.Attributes[i].Normalized,
				vformat.Size);
		}
		glEnableClientState(GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV);
		if (IndexBuffer)
		{
			auto indexPtr = IndexBuffer->GetGpuAddress();
			glEnableClientState(GL_ELEMENT_ARRAY_UNIFIED_NV);
			hw->BindBufferAddr(GL::BufferType::ElementBuffer, 0, indexPtr, IndexCount * sizeof(int));
		}
		for (int i = 0; i < vformat.Attributes.Count(); i++)
			hw->BindBufferAddr(GL::BufferType::ArrayBuffer, i, meshPtr + vformat.Attributes[i].StartOffset, BufferSize);
		return vformat.Size;
	}
}