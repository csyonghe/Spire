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
				if (ch == '_')
				{
					if (isUnderScore)
						sb << "I_";
					else
						sb << "_";
					isUnderScore = true;
				}
				else
				{
					isUnderScore = false;
					sb << ch;
				}
			}
			if (isUnderScore)
				sb << "I";
			return sb.ProduceString();
		}

	}
}