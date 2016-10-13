#ifndef GAME_ENGINE_DYNAMIC_VAR_H
#define GAME_ENGINE_DYNAMIC_VAR_H

#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "CoreLib/Parser.h"

namespace GameEngine
{
	enum class DynamicVariableType
	{
		Float, Int, Vec2, Vec3, Vec4, Texture
	};

	class DynamicVariable
	{
	public:
		CoreLib::String Name;
		DynamicVariableType VarType;
		union
		{
			float FloatValue;
			int IntValue;
			VectorMath::Vec2 Vec2Value;
			VectorMath::Vec3 Vec3Value;
			VectorMath::Vec4 Vec4Value;
		};
		CoreLib::String StringValue;
		static DynamicVariable Parse(CoreLib::Text::Parser & parser);
	};
}
#endif