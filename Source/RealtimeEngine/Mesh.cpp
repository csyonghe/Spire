#include "Mesh.h"
#include "CoreLib/LibIO.h"

using namespace CoreLib::Basic;
using namespace CoreLib::IO;

namespace RealtimeEngine
{
	int Mesh::GetVertexSize()
	{
		int rs = 0;
		for (auto & attrib : Attributes)
			rs += attrib.GetSize();
		return rs;
	}
	void Mesh::LoadFromFile(const CoreLib::Basic::String & fileName)
	{
		auto reader = BinaryReader(new FileStream(fileName));
		reader.Read(&Bounds, 1);
		int attributes = reader.ReadInt32();
		for (int i = 0; i < attributes; i++)
		{
			VertexAttribute attr;
			attr.Name = reader.ReadString();
			attr.Components = reader.ReadChar();
			attr.Normalized = (reader.ReadChar()!=0);
			attr.Type = (VertexDataType)reader.ReadInt16();
			Attributes.Add(attr);
		}
		int vertSize = reader.ReadInt32();
		VertexData.SetSize(vertSize);
		reader.Read(VertexData.Buffer(), vertSize);
	}

	void Mesh::SaveToFile(const CoreLib::Basic::String & fileName)
	{
		auto writer = BinaryWriter(new FileStream(fileName, FileMode::Create));
		writer.Write(&Bounds, 1);

		writer.Write(Attributes.Count());
		for (auto & attr : Attributes)
		{
			writer.Write(attr.Name);
			writer.Write((char)attr.Components);
			writer.Write((char)attr.Normalized);
			writer.Write((short)attr.Type);
		}
		writer.Write(VertexData.Count());
		writer.Write(VertexData.Buffer(), VertexData.Count());
	}

}

