#include "Naming.h"

namespace Spire
{
	namespace Compiler
	{
		using namespace CoreLib;

		String EscapeDoubleUnderscore(String str)
		{
			StringBuilder sb;
			bool isUnderScore = false;
			for (auto ch : str)
			{
				if (ch == L'_')
				{
					if (isUnderScore)
						sb << L"I_";
					else
						sb << L"_";
					isUnderScore = true;
				}
				else
				{
					isUnderScore = false;
					sb << ch;
				}
			}
			if (isUnderScore)
				sb << L"I";
			return sb.ProduceString();
		}

	}
}