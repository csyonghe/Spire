#include "CodeGenBackend.h"
#include "../CoreLib/Parser.h"
#include "Syntax.h"

using namespace CoreLib::Basic;

namespace Spire
{
	namespace Compiler
	{
		class SpirVCodeGen : public CodeGenBackend
		{
			virtual CompiledShaderSource GenerateShaderWorld(CompileResult & /*result*/, SymbolTable * /*symbols*/, CompiledWorld * /*shader*/, 
				Dictionary<String, ImportOperatorHandler*>& /*opHandlers*/, Dictionary<String, ExportOperatorHandler*>& /*exportHandlers*/) override
			{
				return CompiledShaderSource();
			}
			virtual void SetParameters(const EnumerableDictionary<String, String>& /*arguments*/) override
			{
			}
		};

		CodeGenBackend * CreateSpirVCodeGen()
		{
			return new SpirVCodeGen();
		}
	}
}