#include "Mesh.h"
#include "CoreLib/LibIO.h"

using namespace CoreLib::Basic;
using namespace CoreLib::IO;

namespace GameEngine
{
	void Mesh::LoadFromFile(const CoreLib::Basic::String & fileName)
	{
		RefPtr<FileStream> stream = new FileStream(fileName);
		LoadFromStream(stream.Ptr());
		stream->Close();
	}

	void Mesh::LoadFromStream(Stream * stream)
	{
		auto reader = BinaryReader(stream);
		int typeId = reader.ReadInt32();
		vertexFormat = MeshVertexFormat(typeId);
		vertCount = reader.ReadInt32();
		int indexCount = reader.ReadInt32();
		reader.Read(&Bounds, 1);
		AllocVertexBuffer(vertCount);
		Indices.SetSize(indexCount);
		reader.Read((char*)GetVertexBuffer(), vertCount * vertexFormat.GetVertexSize());
		reader.Read(Indices.Buffer(), indexCount);
		reader.ReleaseStream();
	}

	void Mesh::SaveToStream(Stream * stream)
	{
		auto writer = BinaryWriter(stream);
		writer.Write(GetVertexTypeId());
		writer.Write(vertCount);
		writer.Write(Indices.Count());
		writer.Write(&Bounds, 1);
		writer.Write((char*)GetVertexBuffer(), vertCount * GetVertexSize());
		writer.Write(Indices.Buffer(), Indices.Count());
		writer.ReleaseStream();
	}

	void Mesh::SaveToFile(const CoreLib::String & fileName)
	{
		RefPtr<FileStream> stream = new FileStream(fileName, FileMode::Create);
		SaveToStream(stream.Ptr());
		stream->Close();
	}

	union MeshVertexFormatTypeIdConverter
	{
		struct Fields
		{
			unsigned int hasSkinning : 1;
			unsigned int hasTangent : 1;
			unsigned int numUVs : 4;
			unsigned int numColors : 4;
		} f;
		int typeId;
	};

	MeshVertexFormat::MeshVertexFormat(int typeId)
	{
		MeshVertexFormatTypeIdConverter convertor;
		convertor.typeId = typeId;
		hasSkinning = convertor.f.hasSkinning;
		hasTangent = convertor.f.hasTangent;
		numUVs = (int)convertor.f.numUVs;
		numColors = (int)convertor.f.numColors;
		vertSize = CalcVertexSize();
	}

	String MeshVertexFormat::GetShaderDefinition()
	{
		if (shaderDef.Length() == 0)
		{
			StringBuilder sb;
			sb << L"module MeshVertex\n{\n";
			sb << L"public @rootVert vec3 vertPos;\n";
			for (int i = 0; i < numUVs; i++)
				sb << L"public @rootVert vec2 vertUV" << i << L";\n";
			for (int i = numUVs; i < 8; i++)
				sb << L"public inline vec2 vertUV" << i << L" = vec2(0.0);\n";
			if (hasTangent)
			{
				sb << LR"(
				@rootVert uint tangentFrame;
				vec4 tangentFrameQuaternion
				{
					vec4 result;
					float inv255 = 2.0 / 255.0;
					result.x = float(tangentFrame & 255) * inv255 - 1.0;
					result.y = float((tangentFrame >> 8) & 255) * inv255 - 1.0;
					result.z = float((tangentFrame >> 16) & 255) * inv255 - 1.0;
					result.w = float((tangentFrame >> 24) & 255) * inv255 - 1.0;
					return result;
				}
				public @vs vec3 vertNormal
				{
					return normalize(QuaternionRotate(tangentFrameQuaternion, vec3(0.0, 1.0, 0.0)));
				}
				public @vs vec3 vertTangent
				{
					return normalize(QuaternionRotate(tangentFrameQuaternion, vec3(1.0, 0.0, 0.0)));
				}
				public vec3 vertBinormal = cross(vertTangent, vertNormal);
				)";
			}
			else
			{
				sb << LR"(
				public @vs vec3 vertNormal = vec3(0.0, 1.0, 0.0);
				public @vs vec3 vertTangent = vec3(1.0, 0.0, 0.0);
				public vec3 vertBinormal = vec3(0.0, 0.0, 1.0);
				)";
			}
			for (int i = 0; i < numColors; i++)
				sb << L"public @rootVert vec4 vertColor" << i << L";\n";
			for (int i = numColors; i < 8; i++)
				sb << L"public inline vec4 vertColor" << i << L" = vec4(0.0);\n";
			if (hasSkinning)
			{
				sb << "public @rootVert uint boneIds;\n";
				sb << "public @rootVert uint boneWeights;\n";
			}
			else
			{
				sb << "public inline uint boneIds = 255;\n";
				sb << "public inline uint boneWeights = 0;\n";
			}
			sb << L"}\n";
			shaderDef = sb.ProduceString();
		}
		return shaderDef;
	}

	int MeshVertexFormat::GetTypeId()
	{
		MeshVertexFormatTypeIdConverter convertor;
		convertor.typeId = 0;
		convertor.f.hasSkinning = hasSkinning;
		convertor.f.hasTangent = hasTangent;
		convertor.f.numUVs = numUVs;
		convertor.f.numColors = numColors;
		return convertor.typeId;
	}

}

