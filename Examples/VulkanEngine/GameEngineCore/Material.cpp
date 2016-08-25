#include "Material.h"
#include "CoreLib/LibIO.h"

namespace GameEngine
{
	void Material::Parse(CoreLib::Text::Parser & parser)
	{
		parser.Read(L"material");
		parser.Read(L"{");
		while (!parser.IsEnd() && !parser.LookAhead(L"}"))
		{
			if (parser.LookAhead(L"shader"))
			{
				parser.ReadToken();
				ShaderFile = parser.ReadStringLiteral();
			}
			else if (parser.LookAhead(L"var"))
			{
				parser.ReadToken();
				auto name = parser.ReadWord();
				parser.Read(L"=");
				auto val = DynamicVariable::Parse(parser);
				Variables[name] = val;
			}
		}
		parser.Read(L"}");
	}

	void Material::LoadFromFile(const CoreLib::String & fullFileName)
	{
		CoreLib::Text::Parser parser(CoreLib::IO::File::ReadAllText(fullFileName));
		Parse(parser);
	}
}

