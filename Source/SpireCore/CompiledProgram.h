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
			ILConstOperand * CreateConstant(bool b);
			ILOperand * CreateDefaultValue(ILType * type);
			ILUndefinedOperand * GetUndefinedOperand();
			ConstantPool();
			~ConstantPool();
		};

		class ILShader;

		class ILWorld : public Object
		{
		public:
			String Name;
			CodePosition Position;
			RefPtr<ILRecordType> OutputType;
			List<ILObjectDefinition> Inputs;
			RefPtr<CFGNode> Code;
			EnumerableDictionary<String, ILOperand*> Components;
			bool IsAbstract = false;
			EnumerableDictionary<String, String> Attributes;
			EnumerableHashSet<String> ReferencedFunctions; // internal names of referenced functions
			ILShader * Shader = nullptr;
		};

		class StageAttribute
		{
		public:
			String Name;
			String Value;
			CodePosition Position;
		};

		class ILStage : public Object
		{
		public:
			CodePosition Position;
			String Name;
			String StageType;
			EnumerableDictionary<String, StageAttribute> Attributes;
		};

		class ILShader
		{
		public:
			CodePosition Position;
			String Name;
			EnumerableDictionary<String, RefPtr<ILWorld>> Worlds;
			EnumerableDictionary<String, RefPtr<ILStage>> Stages;
		};

		class ILFunction
		{
		public:
			EnumerableDictionary<String, RefPtr<ILType>> Parameters;
			RefPtr<ILType> ReturnType;
			RefPtr<CFGNode> Code;
			String Name;
		};

		class ILProgram
		{
		public:
			RefPtr<ConstantPool> ConstantPool = new Compiler::ConstantPool();
			List<RefPtr<ILShader>> Shaders;
			EnumerableDictionary<String, RefPtr<ILFunction>> Functions;
			List<RefPtr<ILStructType>> Structs;
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


		class InterfaceMetaData
		{
		public:
			CoreLib::Basic::String Name;
			RefPtr<Spire::Compiler::ILType> Type;
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

		class StageMetaData
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
			CoreLib::EnumerableDictionary<CoreLib::String, StageMetaData> Stages;
			EnumerableDictionary<String, InterfaceBlockMetaData> InterfaceBlocks;
		};

		class StageSource
		{
		public:
			String MainCode;
			List<unsigned char> BinaryCode;
		};

		class CompiledShaderSource
		{
		public:
			EnumerableDictionary<String, StageSource> Stages;
			ShaderMetaData MetaData;
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
			RefPtr<ILProgram> Program;
			List<ShaderChoice> Choices;
			EnumerableDictionary<String, CompiledShaderSource> CompiledSource; // shader -> stage -> code
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