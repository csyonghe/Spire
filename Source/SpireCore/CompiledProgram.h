#ifndef BAKER_SL_COMPILED_PROGRAM_H
#define BAKER_SL_COMPILED_PROGRAM_H

#include "../CoreLib/Basic.h"
#include "CompileError.h"
#include "IL.h"
#include "Syntax.h"

namespace Spire
{
	namespace Compiler
	{
		class ConstantPoolImpl;

		class ConstantPool
		{
		private:
			ConstantPoolImpl * impl;
		public:
			ILConstOperand * CreateConstant(ILConstOperand * c);
			ILConstOperand * CreateConstantIntVec(int val0, int val1);
			ILConstOperand * CreateConstantIntVec(int val0, int val1, int val2);
			ILConstOperand * CreateConstantIntVec(int val0, int val1, int val3, int val4);
			ILConstOperand * CreateConstant(int val, int vectorSize = 0);
			ILConstOperand * CreateConstant(float val, int vectorSize = 0);
			ILConstOperand * CreateConstant(float val, float val1);
			ILConstOperand * CreateConstant(float val, float val1, float val2);
			ILConstOperand * CreateConstant(float val, float val1, float val2, float val3);
			ILOperand * CreateDefaultValue(ILType * type);
			ILUndefinedOperand * GetUndefinedOperand();
			ConstantPool();
			~ConstantPool();
		};

		enum class InterfaceQualifier
		{
			Input, Output
		};

		

		class CompiledGlobalVar
		{
		public:
			String Name;
			String InputSourceWorld;
			String OrderingStr;
			ImportOperatorDefSyntaxNode ImportOperator;
			InterfaceQualifier Qualifier;
			RefPtr<ILType> Type;
			int OutputIndex = -1, InputIndex = -1;
			bool IsBuiltin = false;
			EnumerableDictionary<String, String> LayoutAttribs;
		};

		class ComponentDefinition
		{
		public:
			String Name;
			String OrderingStr;
			int Offset = 0;
			RefPtr<ILType> Type;
			EnumerableDictionary<String, String> LayoutAttribs;
		};

		class InterfaceBlock : public Object
		{
		public:
			String Name;
			String SourceWorld;
			int Size = 0;
			EnumerableDictionary<String, String> Attributes;
			EnumerableHashSet<String> UserWorlds;
			EnumerableDictionary<String, ComponentDefinition> Entries;
		};

		class InputInterface
		{
		public:
			InterfaceBlock * Block;
			ImportOperatorDefSyntaxNode ImportOperator;
		};

		class CompiledShader;

		class CompiledComponent
		{
		public:
			ILOperand * CodeOperand;
			EnumerableDictionary<String, String> Attributes;
		};

		class CompiledWorld
		{
		public:
			String TargetMachine;
			String ShaderName, WorldName;
			Token ExportOperator;
			EnumerableDictionary<String, String> BackendParameters;
			EnumerableDictionary<String, InputInterface> WorldInputs;
			InterfaceBlock * WorldOutput;

			bool IsAbstract = false;
			CodePosition WorldDefPosition;
			EnumerableDictionary<String, String> Attributes;
			EnumerableHashSet<String> ReferencedFunctions;
			EnumerableDictionary<String, CompiledComponent> LocalComponents;
			EnumerableDictionary<String, ImportInstruction*> ImportInstructions;
			RefPtr<CFGNode> Code = new CFGNode();
			CompiledShader * Shader;
		};

		class InterfaceMetaData
		{
		public:
			CoreLib::Basic::String Name;
			Spire::Compiler::ILBaseType Type;
			EnumerableDictionary<String, String> Attributes;

			int GetHashCode()
			{
				return Name.GetHashCode();
			}
			bool operator == (const InterfaceMetaData & other)
			{
				return Name == other.Name;
			}
		};

		class WorldMetaData
		{
		public:
			CoreLib::Basic::String Name;
			CoreLib::Basic::String TargetName;
			CoreLib::Basic::String OutputBlock;
			CoreLib::Basic::List<CoreLib::Basic::String> InputBlocks;
			CoreLib::Basic::List<CoreLib::Basic::String> Components;
		};

		class InterfaceBlockEntry : public InterfaceMetaData
		{
		public:
			int Offset = 0, Size = 0;
		};
		class InterfaceBlockMetaData
		{
		public:
			String Name;
			int Size = 0;
			EnumerableHashSet<InterfaceBlockEntry> Entries;
			EnumerableDictionary<String, String> Attributes;
			EnumerableHashSet<String> UserWorlds;
		};
		class ShaderMetaData
		{
		public:
			CoreLib::String ShaderName;
			CoreLib::EnumerableDictionary<CoreLib::String, WorldMetaData> Worlds;
			EnumerableDictionary<String, InterfaceBlockMetaData> InterfaceBlocks;
		};

		class CompiledShader
		{
		public:
			ShaderMetaData MetaData;
			EnumerableDictionary<String, RefPtr<InterfaceBlock>> InterfaceBlocks;
			EnumerableDictionary<String, RefPtr<CompiledWorld>> Worlds;
		};
		class CompiledFunction
		{
		public:
			EnumerableDictionary<String, RefPtr<ILType>> Parameters;
			RefPtr<ILType> ReturnType;
			RefPtr<CFGNode> Code;
			String Name;
		};
		class CompiledProgram
		{
		public:
			RefPtr<ConstantPool> ConstantPool = new Compiler::ConstantPool();
			List<RefPtr<CompiledShader>> Shaders;
			List<RefPtr<CompiledFunction>> Functions;
		};

		class ShaderChoiceValue
		{
		public:
			String WorldName, AlternateName;
			ShaderChoiceValue() = default;
			ShaderChoiceValue(String world, String alt)
			{
				WorldName = world;
				AlternateName = alt;
			}
			static ShaderChoiceValue Parse(String str);
			String ToString()
			{
				if (AlternateName.Length() == 0)
					return WorldName;
				else
					return WorldName + L":" + AlternateName;
			}
			bool operator == (const ShaderChoiceValue & val)
			{
				return WorldName == val.WorldName && AlternateName == val.AlternateName;
			}
			bool operator != (const ShaderChoiceValue & val)
			{
				return WorldName != val.WorldName || AlternateName != val.AlternateName;
			}
			int GetHashCode()
			{
				return WorldName.GetHashCode() ^ AlternateName.GetHashCode();
			}
		};

		class ShaderChoice
		{
		public:
			String ChoiceName;
			String DefaultValue;
			List<ShaderChoiceValue> Options;
		};

		class CompiledShaderSource
		{
		private:
			void PrintAdditionalCode(StringBuilder & sb, String userCode);
		public:
			String GlobalHeader;
			EnumerableDictionary<String, String> InputDeclarations; // indexed by world
			String OutputDeclarations;
			String GlobalDefinitions;
			String LocalDeclarations;
			String MainCode;
			CoreLib::Basic::EnumerableDictionary<CoreLib::String, CoreLib::String> ComponentAccessNames;
			String GetAllCodeGLSL(String additionalHeader, String additionalGlobalDeclaration, String preambleCode, String epilogCode);
			String GetAllCodeGLSL()
			{
				return GetAllCodeGLSL(L"", L"", L"", L"");
			}
			void ParseFromGLSL(String code);
		};

		void IndentString(StringBuilder & sb, String src);

		class CompileResult
		{
		private:
			ErrorWriter errWriter;
		public:
			bool Success;
			List<CompileError> ErrorList, WarningList;
			String ScheduleFile;
			RefPtr<CompiledProgram> Program;
			List<ShaderChoice> Choices;
			EnumerableDictionary<String, EnumerableDictionary<String, CompiledShaderSource>> CompiledSource; // file -> world -> code
			void PrintError(bool printWarning = false)
			{
				for (int i = 0; i < ErrorList.Count(); i++)
				{
					printf("%s(%d): error %d: %s\n", ErrorList[i].Position.FileName.ToMultiByteString(), ErrorList[i].Position.Line,
						ErrorList[i].ErrorID, ErrorList[i].Message.ToMultiByteString());
				}
				if (printWarning)
					for (int i = 0; i < WarningList.Count(); i++)
					{
						printf("%s(%d): warning %d: %s\n", WarningList[i].Position.FileName.ToMultiByteString(),
							WarningList[i].Position.Line, WarningList[i].ErrorID, WarningList[i].Message.ToMultiByteString());
					}
			}
			CompileResult()
				: errWriter(ErrorList, WarningList)
			{}
			ErrorWriter * GetErrorWriter()
			{
				return &errWriter;
			}
		};

	}
}

#endif