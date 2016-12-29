// CLikeCodeGen.h
#ifndef SPIRE_C_LIKE_CODE_GEN_H
#define SPIRE_C_LIKE_CODE_GEN_H

//
// This file implements the shared logic for code generation in C-like
// languages, such as GLSL and HLSL.
//

#include "CodeGenBackend.h"
#include "../CoreLib/Tokenizer.h"
#include "Syntax.h"
#include "Naming.h"

namespace Spire
{
	namespace Compiler
	{
		using namespace CoreLib::Basic;

		ILRecordType * ExtractRecordType(ILType * type);
		String AddWorldNameSuffix(String name, String suffix);

		class CLikeCodeGen;

		class ExternComponentCodeGenInfo
		{
		public:
			enum class DataStructureType
			{
				StandardInput, Patch
			};
			enum class SystemVarType
			{
				None, TessCoord, InvocationId, ThreadId, FragCoord, PatchVertexCount, PrimitiveId
			};
			DataStructureType DataStructure = DataStructureType::StandardInput;
			RefPtr<ILType> Type;
			SystemVarType SystemVar = SystemVarType::None;
			bool IsArray = false;
			int ArrayLength = 0;
			int Binding = -1;
		};

		class CodeGenContext
		{
		public:
			CLikeCodeGen * codeGen;
			HashSet<String> GeneratedDefinitions;
			Dictionary<String, String> SubstituteNames;
			Dictionary<ILOperand*, String> VarName;
			CompileResult * Result = nullptr;
			HashSet<String> UsedVarNames;
			int BufferAllocator = 0;
			StringBuilder Body, Header, GlobalHeader;
			List<ILType*> Arguments;
			String ReturnVarName;
			HashSet<ExternComponentCodeGenInfo::SystemVarType> UsedSystemInputs;

			String GenerateCodeName(String name, String prefix)
			{
				StringBuilder nameBuilder;
				int startPos = 0;
				if (name.StartsWith("_sys_"))
					startPos = name.IndexOf('_', 5) + 1;
				nameBuilder << prefix;
				for (int i = startPos; i < name.Length(); i++)
				{
					if ((name[i] >= 'a' && name[i] <= 'z') || 
						(name[i] >= 'A' && name[i] <= 'Z') ||
						name[i] == '_' || 
						(name[i] >= '0' && name[i] <= '9'))
					{
						nameBuilder << name[i];
					}
					else
						nameBuilder << '_';
				}
				auto rs = nameBuilder.ToString();
				int i = 0;
				while (UsedVarNames.Contains(rs))
				{
					i++;
					rs = nameBuilder.ToString() + String(i);
				}
				UsedVarNames.Add(rs);

				return rs;
			}


			String DefineVariable(ILOperand * op);
		};

		class OutputStrategy : public Object
		{
		protected:
			CLikeCodeGen * codeGen = nullptr;
			ILWorld * world = nullptr;
		public:
			OutputStrategy(CLikeCodeGen * pCodeGen, ILWorld * pWorld)
			{
				codeGen = pCodeGen;
				world = pWorld;
			}

			virtual void DeclareOutput(CodeGenContext & ctx, ILStage * stage) = 0;
			virtual void ProcessExportInstruction(CodeGenContext & ctx, ExportInstruction * instr) = 0;
		};

		class CLikeCodeGen : public CodeGenBackend
		{
		protected:
			//ILWorld * currentWorld = nullptr;
			//ILRecordType * currentRecordType = nullptr;
			//bool exportWriteToPackedBuffer = false;
			RefPtr<OutputStrategy> outputStrategy;
			Dictionary<String, ExternComponentCodeGenInfo> extCompInfo;
			HashSet<String> intrinsicTextureFunctions;
			ImportInstruction * currentImportInstr = nullptr;
			bool useBindlessTexture = false;
			DiagnosticSink * errWriter;

			virtual OutputStrategy * CreateStandardOutputStrategy(ILWorld * world, String layoutPrefix) = 0;
			virtual OutputStrategy * CreatePackedBufferOutputStrategy(ILWorld * world) = 0;
			virtual OutputStrategy * CreateArrayOutputStrategy(ILWorld * world, bool pIsPatch, int pArraySize, String arrayIndex) = 0;

			// Hooks for declaring an input record based on the storage mode used (uniform, SSBO, etc.)
			virtual void DeclareStandardInputRecord(CodeGenContext & sb, const ILObjectDefinition & input, bool isVertexShader) = 0;
			virtual void DeclarePatchInputRecord(CodeGenContext & sb, const ILObjectDefinition & input, bool isVertexShader) = 0;

			// Hooks for generating per-stage kernels
			virtual StageSource GenerateSingleWorldShader(ILProgram * program, ILShader * shader, ILStage * stage) = 0;
			virtual StageSource GenerateHullShader(ILProgram * program, ILShader * shader, ILStage * stage) = 0;

			virtual void PrintParameterReference(StringBuilder& sb, ILModuleParameterInstance * param) = 0;

			// Print a reference to some entity that is input to a kernel
			virtual void PrintStandardInputReference(StringBuilder& sb, ILRecordType* recType, String inputName, String componentName) = 0;
			virtual void PrintStandardArrayInputReference(StringBuilder& sb, ILRecordType* recType, String inputName, String componentName) = 0;
			virtual void PrintPatchInputReference(StringBuilder& sb, ILRecordType* recType, String inputName, String componentName) = 0;
			virtual void PrintDefaultInputReference(StringBuilder& sb, ILRecordType* recType, String inputName, String componentName) = 0;
			virtual void PrintSystemVarReference(CodeGenContext & ctx, StringBuilder& sb, String inputName, ExternComponentCodeGenInfo::SystemVarType systemVar) = 0;

			//
			virtual void PrintTypeName(StringBuilder& sb, ILType* type) = 0;
			virtual void PrintCallInstrExprForTarget(CodeGenContext & ctx, CallInstruction * instr, String const& name);
			virtual void PrintMatrixMulInstrExpr(CodeGenContext & ctx, ILOperand* op0, ILOperand* op1);
			virtual void PrintRasterPositionOutputWrite(CodeGenContext & ctx, ILOperand * operand) = 0;
			virtual void PrintTextureCall(CodeGenContext & ctx, CallInstruction * instr) = 0;
			virtual void PrintProjectInstrExpr(CodeGenContext & ctx, ProjectInstruction * instr) = 0;

			// Helpers for printing call instructions
			void PrintDefaultCallInstrArgs(CodeGenContext & ctx, CallInstruction * instr);
			void PrintDefaultCallInstrExpr(CodeGenContext & ctx, CallInstruction * instr, String const& name);

		public:
            DiagnosticSink* getSink() { return this->errWriter; }
			void PrintType(StringBuilder & sbCode, ILType* type);

			void PrintDef(StringBuilder & sbCode, ILType* type, const String & name);

			String GetFuncOriginalName(const String & name);

			virtual void PrintOp(CodeGenContext & ctx, ILOperand * op, bool forceExpression = false);
			void PrintBinaryInstrExpr(CodeGenContext & ctx, BinaryInstruction * instr);
			void PrintBinaryInstr(CodeGenContext & ctx, BinaryInstruction * instr);
			void PrintUnaryInstrExpr(CodeGenContext & ctx, UnaryInstruction * instr);
			void PrintUnaryInstr(CodeGenContext & ctx, UnaryInstruction * instr);
			void PrintAllocVarInstrExpr(CodeGenContext & ctx, AllocVarInstruction * instr);
			void PrintAllocVarInstr(CodeGenContext & ctx, AllocVarInstruction * instr);
			void PrintFetchArgInstrExpr(CodeGenContext & ctx, FetchArgInstruction * instr);
			void PrintFetchArgInstr(CodeGenContext & ctx, FetchArgInstruction * instr);
			void PrintSelectInstrExpr(CodeGenContext & ctx, SelectInstruction * instr);
			void PrintSelectInstr(CodeGenContext & ctx, SelectInstruction * instr);
			void PrintCallInstrExpr(CodeGenContext & ctx, CallInstruction * instr);
			void PrintCallInstr(CodeGenContext & ctx, CallInstruction * instr);
			void PrintCastF2IInstrExpr(CodeGenContext & ctx, Float2IntInstruction * instr);
			void PrintCastF2IInstr(CodeGenContext & ctx, Float2IntInstruction * instr);
			void PrintCastI2FInstrExpr(CodeGenContext & ctx, Int2FloatInstruction * instr);
			void PrintCastI2FInstr(CodeGenContext & ctx, Int2FloatInstruction * instr);
			bool AppearAsExpression(ILInstruction & instr, bool force);
			void PrintExportInstr(CodeGenContext &ctx, ExportInstruction * exportInstr);
			void PrintUpdateInstr(CodeGenContext & ctx, MemberUpdateInstruction * instr);
			void PrintSwizzleInstrExpr(CodeGenContext & ctx, SwizzleInstruction * swizzle);
			void PrintImportInstr(CodeGenContext & ctx, ImportInstruction * importInstr);
			void PrintImportInstrExpr(CodeGenContext & ctx, ImportInstruction * importInstr);
			void PrintInstrExpr(CodeGenContext & ctx, ILInstruction & instr);
			void PrintInstr(CodeGenContext & ctx, ILInstruction & instr);
			void PrintLoadInputInstrExpr(CodeGenContext & ctx, LoadInputInstruction * instr);
			void GenerateCode(CodeGenContext & context, CFGNode * code);

		public:
			CLikeCodeGen();
			virtual void GenerateShaderMetaData(ShaderMetaData & result, ILProgram* program, ILShader * shader, DiagnosticSink * err);
			virtual CompiledShaderSource GenerateShader(CompileResult & result, SymbolTable *, ILShader * shader, DiagnosticSink * err) override;
			void GenerateStructs(StringBuilder & sb, ILProgram * program);
			void GenerateReferencedFunctions(StringBuilder & sb, ILProgram * program, ArrayView<ILWorld*> worlds);
			ExternComponentCodeGenInfo ExtractExternComponentInfo(const ILObjectDefinition & input);
			void PrintInputReference(CodeGenContext & ctx, StringBuilder & sb, String input);
			void DeclareInput(CodeGenContext & sb, const ILObjectDefinition & input, bool isVertexShader);

			void GenerateVertexShaderEpilog(CodeGenContext & ctx, ILWorld * world, ILStage * stage);

			StageSource GenerateVertexFragmentDomainShader(ILProgram * program, ILShader * shader, ILStage * stage);
			StageSource GenerateComputeShader(ILProgram * program, ILShader * shader, ILStage * stage);
			void GenerateFunctionDeclaration(StringBuilder & sbCode, ILFunction * function);
			String GenerateFunction(ILFunction * function);
		};
	}
}

#endif // SPIRE_C_LIKE_CODE_GEN_H
