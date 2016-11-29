#ifndef RASTER_RENDERER_PARSER_H
#define RASTER_RENDERER_PARSER_H

#include "Lexer.h"
#include "Syntax.h"

namespace Spire
{
	namespace Compiler
	{
		const int MaxExprLevel = 12;

		// TODO: implement two pass parsing for file reference and struct type recognition

		class Parser
		{
		private:
			int anonymousParamCounter = 0;
			int pos;
			List<RefPtr<Scope>> scopeStack;
			List<Token> & tokens;
            DiagnosticSink * sink;
			String fileName;
			HashSet<String> typeNames;
			HashSet<String> classNames;
			bool isInImportOperator = false;
			void FillPosition(SyntaxNode * node)
			{
				int id = Math::Min(pos, tokens.Count() - 1);
				if (id >= 0)
				{
					node->Position = tokens[id].Position;
				}
				else
				{
					node->Position = CodePosition(0, 0, 0, fileName);
				}
				node->Scope = scopeStack.Last();
			}
			void PushScope()
			{
				scopeStack.Add(new Scope());
				if (scopeStack.Count() > 1)
					scopeStack.Last()->Parent = scopeStack[scopeStack.Count() - 2].Ptr();
			}
			void PopScope()
			{
				scopeStack.Last() = 0;
				scopeStack.RemoveAt(scopeStack.Count() - 1);
			}
		public:
			Parser(List<Token> & _tokens, DiagnosticSink * sink, String _fileName)
				: pos(0), tokens(_tokens), sink(sink), fileName(_fileName)
			{
				typeNames.Add("int");
				typeNames.Add("uint");
				typeNames.Add("bool");
				typeNames.Add("float");
				typeNames.Add("half");
				typeNames.Add("void");
				typeNames.Add("ivec2");
				typeNames.Add("ivec3");
				typeNames.Add("ivec4");
				typeNames.Add("uvec2");
				typeNames.Add("uvec3");
				typeNames.Add("uvec4");
				typeNames.Add("vec2");
				typeNames.Add("vec3");
				typeNames.Add("vec4");
				typeNames.Add("mat3");
				typeNames.Add("mat4");
				typeNames.Add("mat4x4");
				typeNames.Add("mat3x3");
				typeNames.Add("int2");
				typeNames.Add("int3");
				typeNames.Add("int4");
				typeNames.Add("uint2");
				typeNames.Add("uint3");
				typeNames.Add("uint4");
				typeNames.Add("float2");
				typeNames.Add("float3");
				typeNames.Add("float4");
				typeNames.Add("half2");
				typeNames.Add("half3");
				typeNames.Add("half4");
				typeNames.Add("float3x3");
				typeNames.Add("float4x4");
				typeNames.Add("half3x3");
				typeNames.Add("half4x4");
				typeNames.Add("Texture2D");
				typeNames.Add("texture");
				typeNames.Add("Texture");
				typeNames.Add("sampler");
				typeNames.Add("SamplerState");
				typeNames.Add("sampler_state");
				typeNames.Add("Uniform");
				typeNames.Add("StructuredBuffer");
				typeNames.Add("RWStructuredBuffer");
				typeNames.Add("PackedBuffer");
				typeNames.Add("StorageBuffer");
				typeNames.Add("Patch");
			}
			RefPtr<ProgramSyntaxNode> Parse();
		private:
			Token & ReadToken();
			Token & ReadToken(CoreLib::Text::TokenType type);
			Token & ReadToken(const char * string);
			bool LookAheadToken(CoreLib::Text::TokenType type, int offset = 0);
			bool LookAheadToken(const char * string, int offset = 0);
			Token & ReadTypeKeyword();
			VariableModifier ReadVariableModifier();
			bool IsTypeKeyword();
			EnumerableDictionary<String, String>	ParseAttribute();
			RefPtr<ProgramSyntaxNode>				ParseProgram();
			RefPtr<ShaderSyntaxNode>				ParseShader();
			RefPtr<PipelineSyntaxNode>				ParsePipeline();
			RefPtr<StageSyntaxNode>					ParseStage();
			RefPtr<ComponentSyntaxNode>				ParseComponent();
			RefPtr<WorldSyntaxNode>					ParseWorld();
			RefPtr<RateSyntaxNode>					ParseRate();
			RefPtr<ImportSyntaxNode>				ParseImport();
			RefPtr<ImportStatementSyntaxNode>		ParseImportStatement();
			RefPtr<ImportOperatorDefSyntaxNode>		ParseImportOperator();
			RefPtr<FunctionSyntaxNode>				ParseFunction(bool parseBody = true);
			RefPtr<StructSyntaxNode>				ParseStruct();
			RefPtr<StatementSyntaxNode>				ParseStatement();
			RefPtr<BlockStatementSyntaxNode>		ParseBlockStatement();
			RefPtr<VarDeclrStatementSyntaxNode>		ParseVarDeclrStatement();
			RefPtr<IfStatementSyntaxNode>			ParseIfStatement();
			RefPtr<ForStatementSyntaxNode>			ParseForStatement();
			RefPtr<WhileStatementSyntaxNode>		ParseWhileStatement();
			RefPtr<DoWhileStatementSyntaxNode>		ParseDoWhileStatement();
			RefPtr<BreakStatementSyntaxNode>		ParseBreakStatement();
			RefPtr<ContinueStatementSyntaxNode>		ParseContinueStatement();
			RefPtr<ReturnStatementSyntaxNode>		ParseReturnStatement();
			RefPtr<ExpressionStatementSyntaxNode>	ParseExpressionStatement();
			RefPtr<ExpressionSyntaxNode>			ParseExpression(int level = 0);
			RefPtr<ExpressionSyntaxNode>			ParseLeafExpression();
			RefPtr<ParameterSyntaxNode>				ParseParameter();
			RefPtr<TypeSyntaxNode>					ParseType();

			Parser & operator = (const Parser &) = delete;
		};
		
	}
}

#endif