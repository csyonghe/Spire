#include "CompiledProgram.h"
#include "../CoreLib/Parser.h"
namespace Spire
{
	namespace Compiler
	{
		void CompiledShaderSource::PrintAdditionalCode(StringBuilder & sb, String userCode)
		{
			auto lines = CoreLib::Text::Parser::SplitString(userCode, L'\n');
			for (auto & line : lines)
			{
				CoreLib::Text::Parser parser(line);
				while (!parser.IsEnd())
				{
					auto word = parser.ReadToken().Str;
					if (word == L"$")
					{
						auto compName = parser.ReadToken().Str;
						String accessName;
						if (ComponentAccessNames.TryGetValue(compName, accessName))
						{
							sb << accessName;
						}
						else
							throw InvalidOperationException(L"cannot resolve symbol \'" + compName + L"\'.");
					}
					else
					{
						sb << word;
						if (word != L"#")
							sb << L" ";
					}
				}
				sb << EndLine;
			}
		}
		String CompiledShaderSource::GetAllCodeGLSL(String additionalHeader, String additionalGlobalDeclaration, String preambleCode, String epilogCode)
		{
			StringBuilder sb;
			sb << GlobalHeader << EndLine;
			sb << additionalHeader << EndLine;
			for (auto & compAccess : ComponentAccessNames)
			{
				sb << L"//$" << compAccess.Key << L"$" << compAccess.Value << EndLine;
			}
			for (auto &input : InputDeclarations)
			{
				sb << L"//! input from " << input.Key << EndLine;
				sb << input.Value;
			}
			sb << EndLine << L"//! output declarations" << EndLine << OutputDeclarations;
			sb << EndLine << L"//! global declarations" << EndLine;
			sb << GlobalDefinitions << EndLine;
			sb << additionalGlobalDeclaration;
			sb << EndLine << L"//! end declarations";
			sb << EndLine << L"void main()\n{";
			sb << EndLine << L"//! local declarations" << EndLine;
			sb << LocalDeclarations; 
			sb << EndLine << L"//! main code" << EndLine;
			PrintAdditionalCode(sb, preambleCode);
			sb << EndLine << MainCode << EndLine;
			PrintAdditionalCode(sb, epilogCode);
			sb << EndLine << L"//! end code" << EndLine;
			sb << L"}\n";
			StringBuilder sbIndent;
			IndentString(sbIndent, sb.ProduceString());
			return sbIndent.ProduceString();
		}

		void CompiledShaderSource::ParseFromGLSL(String code)
		{
			List<String> lines = CoreLib::Text::Parser::SplitString(code, L'\n');
			List<String> chunks;
			List<String> headers;
			StringBuilder currentBuilder;
			for (auto & line : lines)
			{
				auto trimLine = line.TrimStart();
				if (trimLine.StartsWith(L"//!"))
				{
					chunks.Add(currentBuilder.ToString());
					headers.Add(trimLine);
					currentBuilder.Clear();
				}
				else if (trimLine.StartsWith(L"//$"))
				{
					auto words = CoreLib::Text::Parser::SplitString(trimLine.SubString(3, trimLine.Length() - 3), L'$');
					if (words.Count() == 2)
						ComponentAccessNames[words[0]] = words[1];
				}
				else if (trimLine.Length() > 0)
					currentBuilder << trimLine << L'\n';
			}
			chunks.Add(currentBuilder.ToString());
			if (chunks.Count() == headers.Count() + 1 && chunks.Count() != 0)
			{
				GlobalHeader = chunks[0];
				for (int i = 0; i < headers.Count(); i++)
				{
					auto & header = headers[i];
					auto & chunk = chunks[i + 1];
					if (header.StartsWith(L"//! input from "))
					{
						String world = header.SubString(15, header.Length() - 15);
						InputDeclarations[world] = chunk;
					}
					else if (header.StartsWith(L"//! output declarations"))
						OutputDeclarations = chunk;
					else if (header.StartsWith(L"//! global declarations"))
						GlobalDefinitions = chunk;
					else if (header.StartsWith(L"//! local declarations"))
						LocalDeclarations = chunk;
					else if (header.StartsWith(L"//! main code"))
						MainCode = chunk;
				}
			}
		}

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