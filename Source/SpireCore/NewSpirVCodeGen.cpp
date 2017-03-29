#include "CodeGenBackend.h"
#include "../CoreLib/Tokenizer.h"
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
			printf("%S\n", shader->Name.ToWString());
			printf("%S\n", shader->Position.ToString().ToWString());
			printf("\n---\n\n");

			for (auto& stage : shader->Stages)
			{
				printf("Stage: %S\n", stage.Key.ToWString());
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
						maxAttrNameLength, attr.Value.Name.ToWString(),
						maxAttrValueLength, attr.Value.Value.ToWString(),
						attr.Value.Position.ToString().ToWString());
				}

				printf("\n");
			}

			printf("\n---\n\n");

			for (auto& world : shader->Worlds)
			{
				printf("World: %S\n", world.Key.ToWString());
				//auto& worldIL = world.Value;
			}
		}

		class SpirVCodeGen : public CodeGenBackend
		{
		public:
			virtual CompiledShaderSource GenerateShader(CompileResult & /*result*/, SymbolTable * /*symbols*/, ILShader * shader, DiagnosticSink * /*err*/) override
			{
				PrintILShader(shader);
				system("pause");
				return CompiledShaderSource();
			}

            LayoutRule GetDefaultLayoutRule() override
            {
                return LayoutRule::Std140;
            }

    };

		CodeGenBackend * CreateSpirVCodeGen()
		{
			return new SpirVCodeGen();
		}
	}
}