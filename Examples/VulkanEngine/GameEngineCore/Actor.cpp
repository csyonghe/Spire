#include "Actor.h"
#include "Engine.h"

namespace GameEngine
{
	using namespace VectorMath;

	Vec3 Actor::ParseVec3(CoreLib::Text::Parser & parser)
	{
		Vec3 rs;
		parser.Read(L"[");
		rs.x = (float)parser.ReadDouble();
		rs.y = (float)parser.ReadDouble();
		rs.z = (float)parser.ReadDouble();
		parser.Read(L"]");
		return rs;
	}
	Vec4 Actor::ParseVec4(CoreLib::Text::Parser & parser)
	{
		Vec4 rs;
		parser.Read(L"[");
		rs.x = (float)parser.ReadDouble();
		rs.y = (float)parser.ReadDouble();
		rs.z = (float)parser.ReadDouble();
		rs.w = (float)parser.ReadDouble();
		parser.Read(L"]");
		return rs;
	}
	Matrix4 Actor::ParseMatrix4(CoreLib::Text::Parser & parser)
	{
		Matrix4 rs;
		parser.Read(L"[");
		for (int i = 0; i < 16; i++)
			rs.values[i] = (float)parser.ReadDouble();
		parser.Read(L"]");
		return rs;
	}

	void Actor::Serialize(CoreLib::StringBuilder & sb, const VectorMath::Vec3 & v)
	{
		sb << L"[";
		for (int i = 0; i < 3; i++)
			sb << v[i] << L" ";
		sb << L"]";
	}

	void Actor::Serialize(CoreLib::StringBuilder & sb, const VectorMath::Vec4 & v)
	{
		sb << L"[";
		for (int i = 0; i < 4; i++)
			sb << v[i] << L" ";
		sb << L"]";
	}

	void Actor::Serialize(CoreLib::StringBuilder & sb, const VectorMath::Matrix4 & v)
	{
		sb << L"[";
		for (int i = 0; i < 16; i++)
			sb << v.values[i] << L" ";
		sb << L"]";
	}

	bool Actor::ParseField(Level * level, CoreLib::Text::Parser & parser, bool &)
	{
		if (parser.LookAhead(L"name"))
		{
			parser.ReadToken();
			Name = parser.ReadStringLiteral();
			return true;
		}
		else if (parser.LookAhead(L"bounds"))
		{
			parser.ReadToken();
			Bounds.Min = ParseVec3(parser);
			Bounds.Max = ParseVec3(parser);
			return true;
		}
		else if (parser.LookAhead(L"transform"))
		{
			parser.ReadToken();
			LocalTransform = ParseMatrix4(parser);
			return true;
		}
		else if (parser.LookAhead(L"component"))
		{
			parser.ReadToken();
			SubComponents.Add(Engine::Instance()->ParseActor(level, parser));
			return true;
		}
		return false;
	}
	void Actor::SerializeFields(CoreLib::StringBuilder & sb)
	{
		sb << L"name \"" << Name << L"\"\n";
		sb << L"transform ";
		Serialize(sb, LocalTransform);
		sb << L"\n";
		for (auto & comp : SubComponents)
		{
			sb << L"component ";
			comp->SerializeToText(sb);
		}
	}
	void Actor::Parse(Level * level, CoreLib::Text::Parser & parser, bool & isInvalid)
	{
		parser.ReadToken(); // skip class name
		parser.Read(L"{");
		while (!parser.IsEnd() && !parser.LookAhead(L"}"))
		{
			if (!ParseField(level, parser, isInvalid))
				parser.ReadToken();
		}
		parser.Read(L"}");
	}
	void Actor::SerializeToText(CoreLib::StringBuilder & sb)
	{
		sb << GetTypeName() << L"\n{\n";
		SerializeFields(sb);
		sb << L"}\n";
	}
	Actor::~Actor()
	{
		OnUnload();
	}
}


