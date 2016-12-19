#ifndef BAKER_SL_COMPILED_PROGRAM_H
#define BAKER_SL_COMPILED_PROGRAM_H

#include "../CoreLib/Basic.h"
#include "Diagnostics.h"
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

		class ILModuleParameterSet;

		class ILModuleParameterInstance : public ILOperand
		{
		public:
			ILModuleParameterSet * Module = nullptr;
			int BufferOffset = -1;
			List<int> BindingPoints; // for legacy API, usually one item. Samplers may have multiple binding points in OpenGL.
			virtual String ToString()
			{
				return "moduleParam<" + Name + ">";
			}
		};

		class ILModuleParameterSet
		{
		public:
			int BufferSize = 0;
			String BindingName;
			int DescriptorSetId = -1;
			EnumerableDictionary<String, RefPtr<ILModuleParameterInstance>> Parameters;
		};

		class ILShader
		{
		public:
			CodePosition Position;
			String Name;
			EnumerableDictionary<String, RefPtr<ILModuleParameterSet>> ModuleParamSets;
			EnumerableDictionary<String, RefPtr<ILWorld>> Worlds;
			EnumerableDictionary<String, RefPtr<ILStage>> Stages;
		};

		class ILParameter
		{
		public:
			RefPtr<ILType> Type;
			ParameterQualifier Qualifier;
			ILParameter() = default;
			ILParameter(RefPtr<ILType> type, ParameterQualifier qualifier = ParameterQualifier::In)
				: Type(type), Qualifier(qualifier)
			{}
		};

		class ILFunction
		{
		public:
			EnumerableDictionary<String, ILParameter> Parameters;
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
					return WorldName + ":" + AlternateName;
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

		class ShaderMetaData
		{
		public:
			CoreLib::String ShaderName;
			CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<ILModuleParameterSet>> ParameterSets; // bindingName->DescSet
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
		public:
			DiagnosticSink sink;
			String ScheduleFile;
			RefPtr<ILProgram> Program;
			List<ShaderChoice> Choices;
			EnumerableDictionary<String, CompiledShaderSource> CompiledSource; // shader -> stage -> code
			void PrintDiagnostics()
			{
				for (int i = 0; i < sink.diagnostics.Count(); i++)
				{
					fprintf(stderr, "%S(%d): %s %d: %S\n",
                        sink.diagnostics[i].Position.FileName.ToWString(),
                        sink.diagnostics[i].Position.Line,
                        getSeverityName(sink.diagnostics[i].severity),
						sink.diagnostics[i].ErrorID,
                        sink.diagnostics[i].Message.ToWString());
				}
			}
			CompileResult()
			{}
			DiagnosticSink * GetErrorWriter()
			{
				return &sink;
			}
            int GetErrorCount()
            {
                return sink.GetErrorCount();
            }
		};

	}
}

#endif