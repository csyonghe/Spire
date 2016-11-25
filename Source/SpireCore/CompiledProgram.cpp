#include "CompiledProgram.h"

namespace Spire
{
	namespace Compiler
	{
		void IndentString(StringBuilder & sb, String src)
		{
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
		}
		ShaderChoiceValue ShaderChoiceValue::Parse(String str)
		{
			ShaderChoiceValue result;
			int idx = str.IndexOf(L':');
			if (idx == -1)
				return ShaderChoiceValue(str, L"");
			return ShaderChoiceValue(str.SubString(0, idx), str.SubString(idx + 1, str.Length() - idx - 1));
		}
		
}
}