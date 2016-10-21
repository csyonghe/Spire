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
			List<CompileError> & errors;
			String fileName;
			HashSet<String> typeNames;
			HashSet<String> classNames;
			void FillPosition(SyntaxNode * node)
			{
				int id = Math::Min(pos, tokens.Count() - 1);
				if (id >= 0)
				{
					node->Position = tokens[id].Position;
				}
				else
				{
					node->Position = CodePosition(0, 0, fileName);
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
			Parser(List<Token> & _tokens, List<CompileError> & _errors, String _fileName)
				: pos(0), tokens(_tokens), errors(_errors), fileName(_fileName)
			{
				typeNames.Add(L"int");
				typeNames.Add(L"uint");
				typeNames.Add(L"bool");
				typeNames.Add(L"float");
				typeNames.Add(L"half");
				typeNames.Add(L"void");
				typeNames.Add(L"ivec2");
				typeNames.Add(L"ivec3");
				typeNames.Add(L"ivec4");
				typeNames.Add(L"uvec2");
				typeNames.Add(L"uvec3");
				typeNames.Add(L"uvec4");
				typeNames.Add(L"vec2");
				typeNames.Add(L"vec3");
				typeNames.Add(L"vec4");
				typeNames.Add(L"mat3");
				typeNames.Add(L"mat4");
				typeNames.Add(L"mat4x4");
				typeNames.Add(L"mat3x3");
				typeNames.Add(L"int2");
				typeNames.Add(L"int3");
				typeNames.Add(L"int4");
				typeNames.Add(L"uint2");
				typeNames.Add(L"uint3");
				typeNames.Add(L"uint4");
				typeNames.Add(L"float2");
				typeNames.Add(L"float3");
				typeNames.Add(L"float4");
				typeNames.Add(L"half2");
				typeNames.Add(L"half3");
				typeNames.Add(L"half4");
				typeNames.Add(L"float3x3");
				typeNames.Add(L"float4x4");
				typeNames.Add(L"half3x3");
				typeNames.Add(L"half4x4");
				typeNames.Add(L"Texture2D");
				typeNames.Add(L"sampler2D");
				typeNames.Add(L"sampler2DShadow");
				typeNames.Add(L"samplerCube");
				typeNames.Add(L"samplerCubeShadow");
				typeNames.Add(L"Texture");
				typeNames.Add(L"Uniform");
				typeNames.Add(L"ArrayBuffer");
				typeNames.Add(L"PackedBuffer");
				typeNames.Add(L"StorageBuffer");
				typeNames.Add(L"Patch");
			}
			RefPtr<ProgramSyntaxNode> Parse();
		private:
			Token & ReadToken();
			Token & ReadToken(TokenType type);
			Token & ReadToken(const wchar_t * string);
			bool LookAheadToken(TokenType type, int offset = 0);
			bool LookAheadToken(const wchar_t * string, int offset = 0);
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