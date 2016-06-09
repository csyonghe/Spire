#ifndef REALTIME_ENGINE_MESH_H
#define REALTIME_ENGINE_MESH_H

#include "CoreLib/Basic.h"
#include "CoreLib/Graphics.h"

namespace RealtimeEngine
{
	enum class VertexDataType
	{
		Half, Float, Char, UChar, Short, UShort, Int, 
		UInt4_10_10_10_2
	};

	class VertexAttribute
	{
	public:
		CoreLib::Basic::String Name;
		VertexDataType Type;
		bool Normalized;
		int Components;
		VertexAttribute() = default;
		VertexAttribute(CoreLib::Basic::String name, VertexDataType type, int components, bool normalize)
		{
			Name = name;
			Type = type;
			Components = components;
			Normalized = normalize;
		}
		int GetSize()
		{
			switch (Type)
			{
			case VertexDataType::Char:
			case VertexDataType::UChar:
				return Components;
			case VertexDataType::Half:
			case VertexDataType::Short:
			case VertexDataType::UShort:
				return Components * 2;
			case VertexDataType::Float:
			case VertexDataType::Int:
				return Components * 4;
			case VertexDataType::UInt4_10_10_10_2:
				return 4;
			}
			throw CoreLib::Basic::InvalidProgramException();
		}
	};

	class Mesh
	{
	public:
		CoreLib::Graphics::BBox Bounds;
		CoreLib::Basic::List<VertexAttribute> Attributes;
		CoreLib::Basic::List<unsigned char> VertexData;
		int GetVertexSize();
		void LoadFromFile(const CoreLib::Basic::String & fileName);
		void SaveToFile(const CoreLib::Basic::String & fileName);
		 
	};
}

#endif
