#include "DynamicVariable.h"

namespace GameEngine
{
	DynamicVariable DynamicVariable::Parse(CoreLib::Text::Parser & parser)
	{
		DynamicVariable var;
		if (parser.LookAhead(L"vec2"))
		{
			parser.ReadToken();
			var.VarType = DynamicVariableType::Vec2;
			parser.Read(L"[");
			var.Vec2Value.x = (float)parser.ReadDouble();
			var.Vec2Value.y = (float)parser.ReadDouble();
			parser.Read(L"]");
		}
		else if (parser.LookAhead(L"float"))
		{
			parser.ReadToken();
			var.VarType = DynamicVariableType::Float;
			parser.Read(L"[");
			var.FloatValue = (float)parser.ReadDouble();
			parser.Read(L"]");
		}
		else if (parser.LookAhead(L"vec3"))
		{
			parser.ReadToken();
			var.VarType = DynamicVariableType::Vec3;
			parser.Read(L"[");
			var.Vec3Value.x = (float)parser.ReadDouble();
			var.Vec3Value.y = (float)parser.ReadDouble();
			var.Vec3Value.z = (float)parser.ReadDouble();

			parser.Read(L"]");
		}
		else if (parser.LookAhead(L"vec4"))
		{
			parser.ReadToken();
			var.VarType = DynamicVariableType::Vec4;
			parser.Read(L"[");
			var.Vec4Value.x = (float)parser.ReadDouble();
			var.Vec4Value.y = (float)parser.ReadDouble();
			var.Vec4Value.z = (float)parser.ReadDouble();
			var.Vec4Value.w = (float)parser.ReadDouble();
			parser.Read(L"]");
		}
		else if (parser.LookAhead(L"int"))
		{
			parser.ReadToken();
			var.VarType = DynamicVariableType::Int;
			parser.Read(L"[");
			var.FloatValue = (float)parser.ReadInt();
			parser.Read(L"]");
		}
		else if (parser.LookAhead(L"texture"))
		{
			parser.ReadToken();
			var.VarType = DynamicVariableType::Texture;
			parser.Read(L"[");
			var.StringValue = parser.ReadStringLiteral();
			parser.Read(L"]");
		}
		return var;
	}

}

