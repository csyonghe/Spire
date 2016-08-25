#include "CoreLib/Basic.h"
#include "CoreLib/Parser.h"
#include "CoreLib/LibIO.h"
#include "CoreLib/VectorMath.h"

using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace CoreLib::Text;
using namespace VectorMath;

String ExtractField(const String & src, const String & fieldName)
{
	int idx = src.IndexOf(fieldName);
	if (idx != -1)
	{
		int endIdx = src.IndexOf(L' ', idx);
		int endIdx2 = Math::Min(src.IndexOf(L'\n', idx), src.IndexOf(L'\r', idx));

		if (endIdx == -1 || endIdx2 != -1 && endIdx2 < endIdx)
			endIdx = endIdx2;
		return src.SubString(idx + fieldName.Length(), endIdx - idx - fieldName.Length());
	}
	return L"";
}

Vec3 ParseRotation(String src)
{
	Vec3 rs;
	Parser p(src);
	p.Read(L"(");
	p.Read(L"Pitch");
	p.Read(L"=");
	rs.x = (float)p.ReadDouble();
	p.Read(L",");
	p.Read(L"Yaw");
	p.Read(L"=");
	rs.y = (float)p.ReadDouble();
	p.Read(L",");
	p.Read(L"Roll");
	p.Read(L"=");
	rs.z = (float)p.ReadDouble();
	return rs;
}

Vec3 ParseTranslation(String src)
{
	Vec3 rs;
	Parser p(src);
	p.Read(L"(");
	p.Read(L"X");
	p.Read(L"=");
	rs.x = (float)p.ReadDouble();
	p.Read(L",");
	p.Read(L"Y");
	p.Read(L"=");
	rs.y = (float)p.ReadDouble();
	p.Read(L",");
	p.Read(L"Z");
	p.Read(L"=");
	rs.z = (float)p.ReadDouble();
	return rs;
}

String IndentString(String src)
{
	StringBuilder  sb;
	int indent = 0;
	bool beginTrim = true;
	for (int c = 0; c < src.Length(); c++)
	{
		auto ch = src[c];
		if (ch == L'\n')
		{
			sb << L"\n";

			beginTrim = true;
		}
		else
		{
			if (beginTrim)
			{
				while (c < src.Length() - 1 && (src[c] == L'\t' || src[c] == L'\n' || src[c] == L'\r' || src[c] == L' '))
				{
					c++;
					ch = src[c];
				}
				for (int i = 0; i < indent - 1; i++)
					sb << L'\t';
				if (ch != '}' && indent > 0)
					sb << L'\t';
				beginTrim = false;
			}

			if (ch == L'{')
				indent++;
			else if (ch == L'}')
				indent--;
			if (indent < 0)
				indent = 0;

			sb << ch;
		}
	}
	return sb.ProduceString();
}

int wmain(int argc, const wchar_t ** argv)
{
	if (argc <= 1)
		return 0;
	String fileName = argv[1];
	String src = File::ReadAllText(fileName);
	Parser parser(src);
	StringBuilder sb;
	while (!parser.IsEnd())
	{
		if (parser.ReadToken().Str == L"Begin" &&
			parser.ReadToken().Str == L"Actor" && 
			parser.ReadToken().Str == L"Class" &&
			parser.ReadToken().Str == L"=")
		{
			if (parser.ReadToken().Str == L"StaticMeshActor")
			{
				auto beginPos = parser.NextToken().Position;
				while (!(parser.NextToken().Str == L"End" && parser.NextToken(1).Str == L"Actor"))
				{
					parser.ReadToken();
				}
				auto endToken = parser.ReadToken();
				auto endPos = endToken.Position;
				auto actorStr = src.SubString(beginPos, endPos);
				auto name = ExtractField(actorStr, L"Name=");
				auto mesh = ExtractField(actorStr, L"StaticMesh=");
				auto location = ExtractField(actorStr, L"RelativeLocation=");
				auto rotation = ExtractField(actorStr, L"RelativeRotation=");
				auto scale = ExtractField(actorStr, L"RelativeScale=");
				auto material = ExtractField(actorStr, L"OverrideMaterials(0)=");
				sb << L"StaticMesh\n{\n";
				sb << L"name \"" << name << L"\"\n";
				sb << L"mesh \"" << mesh.SubString(mesh.IndexOf(L'.') + 1, mesh.Length() - mesh.IndexOf(L'.') - 3) << L".mesh\"\n";
				Matrix4 transform;
				Matrix4::CreateIdentityMatrix(transform);
				if (rotation.Length())
				{
					Matrix4 rot;
					auto r = ParseRotation(rotation);
					Matrix4::Rotation(rot, r.y, r.x, r.z);
					Matrix4::Multiply(transform, rot, transform);
				}
				if (scale.Length())
				{
					Matrix4 matS;
					auto s = ParseTranslation(scale);
					Matrix4::Scale(matS, s.x, s.y, s.z);
					Matrix4::Multiply(transform, matS, transform);
				}
				if (location.Length())
				{
					Matrix4 matTrans;
					auto s = ParseTranslation(location);
					Matrix4::Translation(matTrans, s.x, s.y, s.z);
					Matrix4::Multiply(transform, matTrans, transform);
				}
				sb << L"transform [";
				for (int i = 0; i < 16; i++)
					sb << transform.values[i] << L" ";
				sb << L"]\n";
				if (material.Length())
				{
					sb << L"material \"" << material.SubString(material.IndexOf(L'.') + 1, material.Length() - material.IndexOf(L'.') - 3) << L".material\"\n";
				}
				sb << L"}\n";
			}
		}
	}
	File::WriteAllText(Path::ReplaceExt(fileName, L"level"), IndentString(sb.ProduceString()));
    return 0;
}

