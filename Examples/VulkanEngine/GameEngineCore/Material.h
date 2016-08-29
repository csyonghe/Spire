#ifndef GAME_ENGINE_MATERIAL_H
#define GAME_ENGINE_MATERIAL_H

#include "CoreLib/Basic.h"
#include "DynamicVariable.h"
#include "CoreLib/Parser.h"

namespace GameEngine
{
	class Level;

	class Material
	{
	public:
		CoreLib::String ShaderFile;
		bool ParameterDirty = true;
		CoreLib::EnumerableDictionary<CoreLib::String, DynamicVariable> Variables;
		void Parse(CoreLib::Text::Parser & parser);
		void LoadFromFile(const CoreLib::String & fullFileName);
	};
}

#endif