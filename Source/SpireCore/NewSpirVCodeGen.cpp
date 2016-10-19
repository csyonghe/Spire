#include "CodeGenBackend.h"
#include "../CoreLib/Parser.h"
#include "IL.h"
#include "Syntax.h"
#include <vector>
#include <fstream>
#include "../CoreLib/TextIO.h"
#include "../CoreLib/LibIO.h"

using namespace CoreLib::Basic;

namespace Spire
{
	namespace Compiler
	{
		void PrintILShader(ILShader * shader)
		{
			printf("%S\n", shader->Name.Buffer());
			printf("%S\n", shader->Position.ToString().Buffer());
			printf("\n---\n\n");

			for (auto& stage : shader->Stages)
			{
				printf("Stage: %S\n", stage.Key.Buffer());
				auto& stageIL = stage.Value;

				int maxAttrNameLength = 0;
				int maxAttrValueLength = 0;
				for (auto& attr : stageIL->Attributes)
				{
					if (attr.Value.Name.Length() > maxAttrNameLength)
						maxAttrNameLength = attr.Value.Name.Length();

					if (attr.Value.Value.Length() > maxAttrValueLength)
						maxAttrValueLength = attr.Value.Value.Length();
				}

				for (auto& attr : stageIL->Attributes)
				{
					printf("\t%-*S = %-*S (%S)\n",
						maxAttrNameLength, attr.Value.Name.Buffer(),
						maxAttrValueLength, attr.Value.Value.Buffer(),
						attr.Value.Position.ToString().Buffer());
				}

				printf("\n");
			}

			printf("\n---\n\n");

			for (auto& world : shader->Worlds)
			{
				printf("World: %S\n", world.Key.Buffer());
				//auto& worldIL = world.Value;
			}
		}

		class SpirVCodeGen : public CodeGenBackend
		{
		public:
			virtual CompiledShaderSource GenerateShader(CompileResult & /*result*/, SymbolTable * /*symbols*/, ILShader * shader, ErrorWriter * /*err*/) override
			{
				PrintILShader(shader);
				system("pause");
				return CompiledShaderSource();
			}
		};

		CodeGenBackend * CreateSpirVCodeGen()
		{
			return new SpirVCodeGen();
		}
	}
}