#if 0
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

		enum class ExecutionModel
		{
			Invalid = 777,
			Vertex = 0,
			TessellationControl = 1,
			TessellationEvaluation = 2,
			Geometry = 3,
			Fragment = 4,
			GLCompute = 5,
			Kernel = 6
		};

		String ExecutionModelToString(ExecutionModel em)
		{
			switch (em)
			{
			case ExecutionModel::Invalid:
				return "invalid";
			case ExecutionModel::Vertex:
				return "Vertex";
			case ExecutionModel::TessellationControl:
				return "TessellationContro";
			case ExecutionModel::TessellationEvaluation:
				return "TessellationEvaluation";
			case ExecutionModel::Geometry:
				return "Geometry";
			case ExecutionModel::Fragment:
				return "Fragment";
			case ExecutionModel::GLCompute:
				return "GLCompute";
			case ExecutionModel::Kernel:
				return "Kerne";
			default:
				throw NotImplementedException("unknown ExecutionMode");
			}
		}

		enum class ExecutionMode
		{
			Invalid = 777,
			Invocations = 0,
			PixelCenterInteger = 6,
			OriginUpperLeft = 7,
			OriginLowerLeft = 8,
			EarlyFragmentTests = 9,
			DepthReplacing = 12,
			DepthGreater = 14,
			DepthLess = 15,
			DepthUnchanged = 16,
			LocalSize = 17
		};

		String ExecutionModeToString(ExecutionMode em)
		{
			switch (em)
			{
			case ExecutionMode::Invalid:
				return "invalid";
			case ExecutionMode::Invocations:
				return "Invocations";
			case ExecutionMode::PixelCenterInteger:
				return "PixelCenterInteger";
			case ExecutionMode::OriginUpperLeft:
				return "OriginUpperLeft";
			case ExecutionMode::OriginLowerLeft:
				return "OriginLowerLeft";
			case ExecutionMode::EarlyFragmentTests:
				return "EarlyFragmentTests";
			case ExecutionMode::DepthReplacing:
				return "DepthReplacing";
			case ExecutionMode::DepthGreater:
				return "DepthGreater";
			case ExecutionMode::DepthLess:
				return "DepthLess";
			case ExecutionMode::DepthUnchanged:
				return "DepthUnchanged";
			case ExecutionMode::LocalSize:
				return "LocalSize";
			default:
				throw NotImplementedException("unknown ExecutionMode");
			}
		}

		enum class StorageClass
		{
			Invalid = 777,
			UniformConstant = 0,
			Input = 1,
			Uniform = 2,
			Output = 3,
			Workgroup = 4,
			CrossWorkGroup = 5,
			Private = 6,
			Function = 7,
			Generic = 8,
			PushConstant = 9,
			AtomicCounter = 10,
			Image = 11
		};

		String StorageClassToString(StorageClass store)
		{
			switch (store)
			{
			case StorageClass::UniformConstant:
				return "UniformConstant";
			case StorageClass::Input:
				return "Input";
			case StorageClass::Uniform:
				return "Uniform";
			case StorageClass::Output:
				return "Output";
			case StorageClass::Workgroup:
				return "Workgroup";
			case StorageClass::CrossWorkGroup:
				return "CrossWorkGroup";
			case StorageClass::Private:
				return "Private";
			case StorageClass::Function:
				return "Function";
			case StorageClass::Generic:
				return "Generic";
			case StorageClass::PushConstant:
				return "PushConstant";
			case StorageClass::AtomicCounter:
				return "AtomicCounter";
			case StorageClass::Image:
				return "Image";
			default:
				throw NotImplementedException("Unknown StorageClass: ");
			}
		}

		enum class MemoryAccess
		{
			None = 0, //0x0
			Volatile = 1, //0x1
			Aligned = 2, //0x2
			Nontemporal = 4 //0x4
		};

		String MemoryAccessToString(MemoryAccess ma)
		{
			switch (ma)
			{
			case MemoryAccess::None:
				return "None";
			case MemoryAccess::Volatile:
				return "Volatile";
			case MemoryAccess::Aligned:
				return "Aligned";
			case MemoryAccess::Nontemporal:
				return "Nontempora";
			default:
				throw NotImplementedException("Unknown MemoryAccess");
			}
		}

		enum class Decoration
		{
			Invalid = 777,
			Block = 2,
			BufferBlock = 3,
			RowMajor = 4,
			ColMajor = 5,
			ArrayStride = 6,
			MatrixStride = 7,
			BuiltIn = 11,
			Flat = 14,
			Constant = 22,
			Location = 30,
			Component = 31,
			Index = 32,
			Binding = 33,
			DescriptorSet = 34,
			Offset = 35
		};

		String DecorationToString(Decoration d)
		{
			switch (d)
			{
			case Decoration::Invalid:
				return "invalid";
			case Decoration::Block:
				return "Block";
			case Decoration::BufferBlock:
				return "BufferBlock";
			case Decoration::RowMajor:
				return "RowMajor";
			case Decoration::ColMajor:
				return "ColMajor";
			case Decoration::ArrayStride:
				return "ArrayStride";
			case Decoration::MatrixStride:
				return "MatrixStride";
			case Decoration::BuiltIn:
				return "BuiltIn";
			case Decoration::Flat:
				return "Flat";
			case Decoration::Constant:
				return "Constant";
			case Decoration::Location:
				return "Location";
			case Decoration::Component:
				return "Component";
			case Decoration::Index:
				return "Index";
			case Decoration::Binding:
				return "Binding";
			case Decoration::DescriptorSet:
				return "DescriptorSet";
			case Decoration::Offset:
				return "Offset";
			default:
				throw NotImplementedException("unknown Decoration");
			}
		}

		enum class BuiltIn
		{
			Invalid = 777,
			Position = 0,
			PointSize = 1,
			ClipDistance = 3,
			CullDistance = 4,
			FragDepth = 22,
			WorkgroupSize = 25,
			GlobalInvocationId = 28
		};

		String BuiltinToString(BuiltIn b)
		{
			switch (b)
			{
			case BuiltIn::Invalid:
				return "invalid";
			case BuiltIn::Position:
				return "Position";
			case BuiltIn::PointSize:
				return "PointSize";
			case BuiltIn::ClipDistance:
				return "ClipDistance";
			case BuiltIn::CullDistance:
				return "CullDistance";
			case BuiltIn::FragDepth:
				return "FragDepth";
			case BuiltIn::WorkgroupSize:
				return "WorkgroupSize";
			case BuiltIn::GlobalInvocationId:
				return "GlobalInvocationId";
			default:
				throw NotImplementedException("unknown Builtin");
			}
		}

		enum class Dim
		{
			e1D = 0,
			e2D = 1,
			e3D = 2,
			eCube = 3,
			eRect = 4,
			eBuffer = 5,
			eSubpassData = 6
		};

		String DimToString(Dim b)
		{
			switch (b)
			{
			case Dim::e1D:
				return "1D";
			case Dim::e2D:
				return "2D";
			case Dim::e3D:
				return "3D";
			case Dim::eCube:
				return "Cube";
			case Dim::eRect:
				return "Rect";
			case Dim::eBuffer:
				return "Buffer";
			case Dim::eSubpassData:
				return "SubpassData";
			default:
				throw NotImplementedException("unknown Builtin");
			}
		}

		enum class ImageOperands
		{
			None = 0,
			Bias = 0x1,
			Lod = 0x2,
			Grad = 0x4,
			ConstOffset = 0x8,
			Offset = 0x10,
			ConstOffsets = 0x20,
			Sample = 0x40,
			MinLod = 0x80
		};

		String ImageOperandsToString(ImageOperands io)
		{
			switch (io)
			{
			case ImageOperands::None:
				return "None";
			case ImageOperands::Bias:
				return "Bias";
			case ImageOperands::Lod:
				return "Lod";
			case ImageOperands::Grad:
				return "Grad";
			case ImageOperands::ConstOffset:
				return "ConstOffset";
			case ImageOperands::Offset:
				return "Offset";
			case ImageOperands::ConstOffsets:
				return "ConstOffsets";
			case ImageOperands::Sample:
				return "Sample";
			case ImageOperands::MinLod:
				return "MinLod";
			default:
				throw NotImplementedException("unknown Image Operands");
			}
		}

		Dictionary<String, int> GenGLSLstd450InstructionSet()
			//https://www.khronos.org/registry/spir-v/specs/1.0/GLSL.std.450.html
		{
			Dictionary<String, int> ret;

			ret["abs"] = 4;	//fabs, actually :(
			ret["sign"] = 6;	//fsign, actually :(
			ret["floor"] = 8;
			ret["cei"] = 9;
			ret["fract"] = 10;
			ret["sin"] = 13;
			ret["cos"] = 14;
			ret["tan"] = 15;
			ret["asin"] = 16;
			ret["acos"] = 17;
			ret["atan"] = 18;
			ret["atan2"] = 25;
			ret["pow"] = 26;
			ret["exp"] = 27;
			ret["log"] = 28;
			ret["exp2"] = 29;
			ret["log2"] = 30;

			ret["sqrt"] = 31;

			ret["min"] = 37;
			ret["max"] = 40;
			ret["clamp"] = 43;

			ret["mix"] = 46;

			ret["step"] = 48;
			ret["smoothstep"] = 49;

			ret["length"] = 66;

			ret["cross"] = 68;
			ret["normalize"] = 69;

			ret["reflect"] = 71;
			ret["refract"] = 72;

			return ret;
		}

		String GetFuncOriginalName(const String & name)
		{
			String originalName;
			int splitPos = name.IndexOf('@');
			if (splitPos == 0)
				return name;
			if (splitPos != -1)
				originalName = name.SubString(0, splitPos);
			else
				originalName = name;
			return originalName;
		}

		String SpirVFloatToString(float v)
		{
			String rs(v, "%.12e");
			if (!rs.Contains('.') && !rs.Contains('e') && !rs.Contains('E'))
				rs = rs + ".0";
			return rs;
		};

		String SpirVUIntToString(unsigned int i)
		{
			String s;
			if (i >> 31)
				s = s + "1";
			s = s + int(i & 0x7fffffff);
			return s;
		}

		RefPtr<ILType> GetTypeFromString(String s)
		{
			CoreLib::Text::Parser parser(s);
			return TypeFromString(parser);
		}

		//UniformOrBuffer - 0: none; 1: uniform; 2: buffer 
		int GetBaseAlignment(ILType* Type, int UniformOrBuffer)
		{
			auto RoundUpTo = [](int x, int r)
			{
				if (x%r)
					x += r - x%r;
				return x;
			};
			if (auto basicType = dynamic_cast<ILBasicType*>(Type))
			{
				return Type->GetAlignment();
			}
			else if (auto arrayType = dynamic_cast<ILArrayType*>(Type))
			{
				int elementAlignment = GetBaseAlignment(arrayType->BaseType.Ptr(), UniformOrBuffer);
				if (UniformOrBuffer == 1)
					elementAlignment = RoundUpTo(elementAlignment, 16);
				return elementAlignment;
			}
			else if (auto structType = dynamic_cast<ILStructType*>(Type))
			{
				int maxAlignment = -1;
				for (auto &member : structType->Members)
				{
					int memberAlignment = GetBaseAlignment(member.Type.Ptr(), UniformOrBuffer);
					maxAlignment = std::max(maxAlignment, memberAlignment);
				}
				if (UniformOrBuffer == 1)
					maxAlignment = RoundUpTo(maxAlignment, 16);
				return maxAlignment;
			}
			return -1;
		}

		int GetSize(ILType* Type, int UniformOrBuffer)
		{
			auto RoundUpTo = [](int x, int r)
			{
				if (x%r)
					x += r - x%r;
				return x;
			};

			if (auto basicType = dynamic_cast<ILBasicType*>(Type))
			{
				return Type->GetSize();
			}
			else if (auto arrayType = dynamic_cast<ILArrayType*>(Type))
			{
				return GetSize(arrayType->BaseType.Ptr(), UniformOrBuffer) * arrayType->ArrayLength;
			}
			else if (auto structType = dynamic_cast<ILStructType*>(Type))
			{
				int rs = 0;
				for (auto &member : structType->Members)
				{
					int memberAlignment = GetBaseAlignment(member.Type.Ptr(), UniformOrBuffer);
					rs = RoundUpTo(rs, memberAlignment);
					rs += GetSize(member.Type.Ptr(), UniformOrBuffer);
				}
				return rs;
			}
			return 0;
		}

		enum class IDClass
		{
			None,
			TypeofValue,
			TypeofPointer,
			Pointer,
			Value,
			Function
		};

		class IDInfo
		{
		private:
			bool available;
			IDClass idClass;
			int ID;
			String variableName; // only available for Class:Pointer
			String typeName;
			int typeID;
			int baseTypeID;
			RefPtr<ILType> typeIL;
			StorageClass store;
			CompiledFunction * func = nullptr;
			ILOperand *op;
		public:
			IDInfo()
				:available(false)
			{
			}

			//
			static IDInfo CreateIDInfoForTypeofValue(int ID, RefPtr<ILType> typeIL, int UniformOrBuffer = 0)
			{
				IDInfo ret;
				ret.available = true;
				ret.idClass = IDClass::TypeofValue;
				ret.ID = ID;
				ret.typeName = "";
				if (typeIL) 
				{
					ret.typeName = typeIL->ToString();
					if (UniformOrBuffer)
						ret.typeName = ret.typeName + "#" + UniformOrBuffer;
				}
				ret.typeID = ID;
				ret.typeIL = typeIL;
				return ret;
			}

			static IDInfo CreateIDInfoForValue(int ID, RefPtr<ILType> typeIL, ILOperand *op, int typeID)
			{
				IDInfo ret;
				ret.available = true;
				ret.idClass = IDClass::Value;
				ret.ID = ID;
				if (op)
					ret.variableName = op->Name;
				else
					ret.variableName = "";
				ret.op = op;
				ret.typeName = typeIL->ToString();
				ret.typeID = typeID;
				ret.typeIL = typeIL;
				return ret;
			}

			static IDInfo CreateIDInfoForPointer(int ID, ILOperand *op, int typeID, RefPtr<ILType> basetypeIL, int basetypeID, StorageClass store)
			{
				IDInfo ret;
				ret.available = true;
				ret.idClass = IDClass::Pointer;
				ret.ID = ID;
				ret.op = op;
				if (op)
					ret.variableName = op->Name;
				else
					ret.variableName = "";
				ret.typeName = basetypeIL->ToString();
				ret.typeID = typeID;
				ret.baseTypeID = basetypeID;
				ret.typeIL = basetypeIL;
				ret.store = store;
				return ret;
			}

			static IDInfo CreateIDInfoForTypeofPointer(int ID, RefPtr<ILType> baseTypeIL, int baseTypeID, StorageClass store)
			{
				IDInfo ret;
				ret.available = true;
				ret.idClass = IDClass::TypeofPointer;
				ret.ID = ID;
				ret.typeName = baseTypeIL->ToString();
				ret.typeID = ID;
				ret.baseTypeID = baseTypeID;
				ret.typeIL = baseTypeIL;
				ret.store = store;
				return ret;
			}

			static IDInfo CreateIDInfoForFunction(int ID, CompiledFunction * func)
			{
				IDInfo ret;
				ret.available = true;
				ret.idClass = IDClass::Function;
				ret.ID = ID;
				ret.func = func;
				return ret;
			}

			bool IsAvailable()
			{
				return available;
			}
			int GetID()
			{
				if (!available) return -1;
				return ID;
			}
			IDClass GetClass()
			{
				if (!available) return IDClass::None;
				return idClass;
			}
			bool IsTypeofValue()
			{
				if (!available) return false;
				return idClass == IDClass::TypeofValue;
			}
			bool IsValue()
			{
				if (!available) return false;
				return idClass == IDClass::Value;
			}
			bool IsPointer()
			{
				if (!available) return false;
				return idClass == IDClass::Pointer;
			}
			bool IsTypeofPointer()
			{
				if (!available) return false;
				return idClass == IDClass::TypeofPointer;
			}
			bool IsFunction()
			{
				if (!available) return false;
				return idClass == IDClass::Function;
			}
			String GetVariableName()
			{
				if (!available) return "";
				return variableName;
			}
			String GetTypeName()
			{
				if (!available) return "";
				return typeName;
			}
			int GetTypeID()
			{
				if (!available) return -1;
				return typeID;
			}
			int GetBaseTypeID()
			{
				if (!available) return -1;
				return baseTypeID;
			}
			RefPtr<ILType> GetILType()
			{
				if (!available) return nullptr;
				return typeIL;
			}
			StorageClass GetStorageClass()
			{
				if (!available) return StorageClass::Invalid;
				return store;
			}
			CompiledFunction * GetFunc()
			{
				if (!available || idClass != IDClass::Function)
					return nullptr;
				return func;
			}
			ILOperand* GetOp()
			{
				if (!available)
					return nullptr;
				return op;
			}
		};

		class SpirVCodeBuilder
		{
			List<unsigned int> streamHeader;
			List<unsigned int> streamDebug;
			List<unsigned int> streamAnnotation;
			List<unsigned int> streamTypeDefinition;
			List<unsigned int> streamFunctionHeader;
			List<unsigned int> streamFunctionVariable;
			List<unsigned int> streamFunctionBody;
			List<unsigned int> streamProcessedFunctions;
			StringBuilder sbTextHeader;
			//OpCapability, OpExtension, OpExtInstImport, OpMemoryModel, OpEntryPoint, OpExecutionMode
			StringBuilder sbDebug;
			//OpName, OpMemberName
			StringBuilder sbTextAnnotation;
			//OpDecorate, OpMemberDecorate, OpGroupDecorate, OpGroupMemberDecorate, OpDecoration Group
			StringBuilder sbTextTypeDefinition;
			//OpTypeXXXX, OpConstant, global variable declarations(all OpVariable instructions whose storage class is not Function)
			StringBuilder sbTextFunctionDefinitions;
			StringBuilder sbTextFunctionHeader;
			StringBuilder sbTextFunctionVariable;
			StringBuilder sbTextFunctionBody;

		public:
			void Clear()
			{
				streamHeader.Clear();
				streamDebug.Clear();
				streamAnnotation.Clear();
				streamTypeDefinition.Clear();
				streamFunctionHeader.Clear();
				streamFunctionVariable.Clear();
				streamFunctionBody.Clear();
				streamProcessedFunctions.Clear();
				
				sbTextHeader.Clear();
				sbDebug.Clear();
				sbTextAnnotation.Clear();
				sbTextTypeDefinition.Clear();
				sbTextFunctionDefinitions.Clear();
				sbTextFunctionHeader.Clear();
				sbTextFunctionVariable.Clear();
				sbTextFunctionBody.Clear();
			}
			void Initiate()
			{
				Clear();

				streamHeader.Add(0x07230203); // magic number
				streamHeader.Add(0x00010000); // version 
				streamHeader.Add(0x00080001); // register number
				streamHeader.Add(0x0000ffff); // ID bound
				streamHeader.Add(0x00000000); // reserved
			}
			void ProduceFunction()
			{
				//---------------- for binary code ----------------
				streamProcessedFunctions.AddRange(streamFunctionHeader);
				streamFunctionHeader.Clear();

				streamProcessedFunctions.AddRange(streamFunctionVariable);
				streamFunctionVariable.Clear();

				streamProcessedFunctions.AddRange(streamFunctionBody);
				streamFunctionBody.Clear();

				//---------------- for text code ----------------
				sbTextFunctionDefinitions
					<< sbTextFunctionHeader.ToString()
					<< sbTextFunctionVariable.ToString()
					<< sbTextFunctionBody.ToString();
				sbTextFunctionHeader.Clear();
				sbTextFunctionVariable.Clear();
				sbTextFunctionBody.Clear();
			}
			List<unsigned int> ProduceWordStream(int IDBound)
			{
				streamHeader[3] = IDBound + 5;
				List<unsigned int> ret;
				ret.AddRange(streamHeader);
				ret.AddRange(streamDebug);
				ret.AddRange(streamAnnotation);
				ret.AddRange(streamTypeDefinition);
				ret.AddRange(streamProcessedFunctions);
				return ret;
			}
			String ProduceTextCode()
			{
				String ret;
				ret = ret + sbTextHeader.ToString();
				ret = ret + sbDebug.ToString();
				ret = ret + sbTextAnnotation.ToString();
				ret = ret + sbTextTypeDefinition.ToString();
				ret = ret + sbTextFunctionDefinitions.ToString();
				return ret;
			}
			void ProgramHeader()
			{
				sbTextHeader << LR"(OpCapability Shader)" << EndLine;
				sbTextHeader << LR"(%1 = OpExtInstImport "GLSL.std.450")" << EndLine;
				sbTextHeader << LR"(OpMemoryModel Logical GLSL450)" << EndLine;

				streamHeader.Add(17 + (2 << 16));	//wordCount and opCode
				streamHeader.Add(1);	//Shader

										//hardcoded
				streamHeader.Add(0x0006000B);
				streamHeader.Add(0x00000001);
				streamHeader.Add(0x4c534c47);
				streamHeader.Add(0x6474732E);
				streamHeader.Add(0x3035342E);
				streamHeader.Add(0);

				streamHeader.Add(14 + (3 << 16));
				streamHeader.Add(0);
				streamHeader.Add(1);
			}
			void OpFunction(const int funcID, const int returnTypeID, const int functionTypeID)
			{

				sbTextFunctionHeader << LR"(%)" << funcID << LR"( = OpFunction )";
				sbTextFunctionHeader << LR"(%)" << returnTypeID;
				sbTextFunctionHeader << LR"( None)";
				sbTextFunctionHeader << LR"( %)" << functionTypeID;
				sbTextFunctionHeader << EndLine;

				streamFunctionHeader.Add(54 + (5 << 16));
				streamFunctionHeader.Add(returnTypeID);
				streamFunctionHeader.Add(funcID);
				streamFunctionHeader.Add(0);	// function control - 0
				streamFunctionHeader.Add(functionTypeID);
			}
			void OpFunctionParameter(const int paramID, const int typeID)
			{
				sbTextFunctionHeader << LR"(%)" << paramID << LR"( = OpFunctionParameter %)" << typeID << EndLine;

				streamFunctionHeader.Add(55 + (3 << 16));
				streamFunctionHeader.Add(typeID);
				streamFunctionHeader.Add(paramID);
			}
			void OpTypeFunction(const int functionTypeID, const int returnTypeID, const List<int> &argIDList)
			{
				sbTextTypeDefinition << LR"(%)" << functionTypeID << LR"( = OpTypeFunction %)" << returnTypeID;
				for (auto & arg : argIDList)
					sbTextTypeDefinition << LR"( %)" << arg;
				sbTextTypeDefinition << EndLine;

				streamTypeDefinition.Add(33 + ((3 + argIDList.Count()) << 16));
				streamTypeDefinition.Add(functionTypeID);
				streamTypeDefinition.Add(returnTypeID);
				for (auto & arg : argIDList)
					streamTypeDefinition.Add(arg);
			}
			void OpLabel_AtFunctionHeader(const int label)
			{
				sbTextFunctionHeader << LR"(%)" << label << LR"( = OpLabel)" << EndLine;

				streamFunctionHeader.Add(248 + (2 << 16));
				streamFunctionHeader.Add(label);
			}
			void OpLabel_AtFunctionBody(const int label)
			{
				sbTextFunctionBody << LR"(%)" << label << LR"( = OpLabel)" << EndLine;

				streamFunctionBody.Add(248 + (2 << 16));
				streamFunctionBody.Add(label);
			}
			void OpBranch(const int ID)
			{
				sbTextFunctionBody << LR"(OpBranch %)" << ID << EndLine;

				streamFunctionBody.Add(249 + (2 << 16));
				streamFunctionBody.Add(ID);
			}
			void OpBranchConditional(const int cond, const int tb, const int fb)
			{
				sbTextFunctionBody << LR"(OpBranchConditional %)" << cond << LR"( %)" << tb << LR"( %)" << fb << EndLine;

				streamFunctionBody.Add(250 + (4 << 16));
				streamFunctionBody.Add(cond);
				streamFunctionBody.Add(tb);
				streamFunctionBody.Add(fb);
			}
			void OpLoopMerge(const int merge, const int cont)
			{
				sbTextFunctionBody << LR"(OpLoopMerge %)" << merge << LR"( %)" << cont << LR"( None)" << EndLine;

				streamFunctionBody.Add(246 + (4 << 16));
				streamFunctionBody.Add(merge);
				streamFunctionBody.Add(cont);
				streamFunctionBody.Add(0);	//loop control: none
			}
			void OpSelectionMerge(const int merge)
			{
				sbTextFunctionBody << LR"(OpSelectionMerge %)" << merge << LR"( None)" << EndLine;

				streamFunctionBody.Add(247 + (3 << 16));
				streamFunctionBody.Add(merge);
				streamFunctionBody.Add(0);	//selection control: none
			}
			void OpPhi(const int ID, const int typeID, const List<int> branches)
			{
				//<branches>: (variable1, parent branch1), (variable2, parent branch2), ...
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpPhi %)" << typeID;
				for (const auto & x : branches)
					sbTextFunctionBody << LR"( %)" << x;
				sbTextFunctionBody << EndLine;

				streamFunctionBody.Add(245 + ((3 + branches.Count()) << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				for (const auto & x : branches)
					streamFunctionBody.Add(x);
			}
			void OpFunctionCall(const int ID, const int typeID, const int funcID, const List<int> &args)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpFunctionCall %)" << typeID << LR"( %)" << funcID;
				for (auto & arg : args)
					sbTextFunctionBody << LR"( %)" << arg;
				sbTextFunctionBody << EndLine;

				streamFunctionBody.Add(57 + ((4 + args.Count()) << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(funcID);
				for (auto & arg : args)
					streamFunctionBody.Add(arg);
			}
			void OpKill()
			{
				sbTextFunctionBody << LR"(OpKill)" << EndLine;
				
				streamFunctionBody.Add(252 + (1<<16));
			}
			void OpReturn()
			{
				sbTextFunctionBody << LR"(OpReturn)" << EndLine;

				streamFunctionBody.Add(253 + (1 << 16));
			}
			void OpReturnValue(const int ID)
			{
				sbTextFunctionBody << LR"(OpReturnValue %)" << ID << EndLine;

				streamFunctionBody.Add(254 + (2 << 16));
				streamFunctionBody.Add(ID);
			}
			void OpFunctionEnd()
			{
				sbTextFunctionBody << LR"(OpFunctionEnd)" << EndLine;

				streamFunctionBody.Add(56 + (1 << 16));
			}
			int EncodeString(List<unsigned int> & stream, String S)
			{
				auto encoder = CoreLib::IO::Encoding::UTF8;
				List<char> bytes;
				encoder->GetBytes(bytes, S);
				int padding = (4 - (bytes.Count() & 3)) & 3;
				for (int i = 0; i < padding; i++)
					bytes.Add(0);
				int oldSize = stream.Count();
				stream.SetSize(oldSize + (bytes.Count() >> 2));
				memcpy(stream.Buffer() + oldSize, bytes.Buffer(), bytes.Count());
				if (padding==0)
					stream.Add(0);
				return stream.Count() - oldSize;
			}
			void OpEntryPoint(const ExecutionModel currentExecutionModel, const int entryID, const List<int> &interfaceIDs)
			{
				sbTextHeader << LR"(OpEntryPoint )";
				sbTextHeader << ExecutionModelToString(currentExecutionModel) << LR"( )";
				sbTextHeader << LR"(%)" << entryID << LR"( "main" )";
				for (auto & id : interfaceIDs)
					sbTextHeader << LR"( %)" << id;
				sbTextHeader << EndLine;

				int len_i = streamHeader.Count();
				streamHeader.Add(0);
				streamHeader.Add((int)currentExecutionModel);
				streamHeader.Add(entryID);
				int NameLen = EncodeString(streamHeader, "main");
				for (auto & id : interfaceIDs)
					streamHeader.Add(id);
				streamHeader[len_i] = (15) + ((1 + 1 + NameLen + 1 + interfaceIDs.Count()) << 16);
			}
			void OpExecutionMode(const int entryID, const ExecutionMode mode, const int op1, const int op2, const int op3)
			{
				bool LocalSize = false;
				if (mode == ExecutionMode::LocalSize)
					LocalSize = true;

				sbTextHeader << LR"(OpExecutionMode %)" << entryID << LR"( )" << ExecutionModeToString(mode);
				if (LocalSize)
					sbTextHeader << op1 << op2 << op3;
				sbTextHeader << EndLine;

				int len = 3;
				if (LocalSize)
					len += 3;
				streamHeader.Add(16 + (len << 16));
				streamHeader.Add(entryID);
				streamHeader.Add((int)mode);
				if (LocalSize) {
					streamHeader.Add(op1);
					streamHeader.Add(op2);
					streamHeader.Add(op3);
				}
			}
			int Decorate(const Decoration deco, int op1 = 0)
			{
				int len = 0;
				sbTextAnnotation << " " << DecorationToString(deco);
				streamAnnotation.Add((int)deco);
				len++;
				if (deco == Decoration::Location ||
					deco == Decoration::Offset ||
					deco == Decoration::MatrixStride ||
					deco == Decoration::ArrayStride ||
					deco == Decoration::DescriptorSet ||
					deco == Decoration::Binding)
				{
					sbTextAnnotation << LR"( )" << op1;
					streamAnnotation.Add(op1);
					len++;
				}
				if (deco == Decoration::BuiltIn)
				{
					BuiltIn builtin = static_cast<BuiltIn>(op1);
					sbTextAnnotation << LR"( )" << BuiltinToString(builtin);
					streamAnnotation.Add(op1);
					len++;
				}
				return len;
			}
			void OpDecorate(const int ID, const Decoration deco, int op1 = 0)
			{
				sbTextAnnotation << LR"(OpDecorate %)" << ID;

				int len_i = streamAnnotation.Count();
				streamAnnotation.Add(0);
				streamAnnotation.Add(ID);
				int deco_len = Decorate(deco, op1);
				streamAnnotation[len_i] = 71 + ((2 + deco_len) << 16);

				sbTextAnnotation << EndLine;
			}
			void OpMemberDecorate(const int ID, const int memberIndex, const Decoration deco, int op1 = 0)
			{
				sbTextAnnotation << LR"(OpMemberDecorate %)" << ID << LR"( )" << memberIndex;

				int len_i = streamAnnotation.Count();
				streamAnnotation.Add(0);
				streamAnnotation.Add(ID);
				streamAnnotation.Add(memberIndex);
				int deco_len = Decorate(deco, op1);
				streamAnnotation[len_i] = 72 + ((3 + deco_len) << 16);

				sbTextAnnotation << EndLine;
			}
			void OpSNegate(const int ID, const int typeID, const int valueID)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpSNegate %)" << typeID << LR"( %)" << valueID << EndLine;

				streamFunctionBody.Add(126 + (4 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(valueID);
			}
			void OpFNegate(const int ID, const int typeID, const int valueID)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpFNegate %)" << typeID << LR"( %)" << valueID << EndLine;

				streamFunctionBody.Add(127 + (4 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(valueID);
			}
			void OpFAdd(const int ID, const int typeID, const int op1, const int op2)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpFAdd %)" << typeID << LR"( %)" << op1 << LR"( %)" << op2 << EndLine;

				streamFunctionBody.Add(129 + (5 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(op1);
				streamFunctionBody.Add(op2);
			}
			void OpFMul(const int ID, const int typeID, const int op1, const int op2)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpFMul %)" << typeID << LR"( %)" << op1 << LR"( %)" << op2 << EndLine;

				streamFunctionBody.Add(133 + (5 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(op1);
				streamFunctionBody.Add(op2);
			}
			void OpINotEqual(const int ID, const int typeID, const int id0, const int id1)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpINotEqual %)" << typeID << LR"( %)" << id0 << LR"( %)" << id1 << EndLine;

				streamFunctionBody.Add(171 + (5 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(id0);
				streamFunctionBody.Add(id1);
			}
			void OpNot(const int ID, const int typeID, const int valueID)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpNot %)" << typeID << LR"( %)" << valueID << EndLine;

				streamFunctionBody.Add(200 + (4 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(valueID);
			}
			void OpLogicalNot(const int ID, const int typeID, const int valueID)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpLogicalNot %)" << typeID << LR"( %)" << valueID << EndLine;

				streamFunctionBody.Add(168 + (4 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(valueID);
			}
			void OpBinaryInstr(const int ID, String opStr, const int typeID, const int ID0, const int ID1)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = )";
				sbTextFunctionBody << opStr;
				sbTextFunctionBody << LR"( %)" << typeID << LR"( %)" << ID0 << LR"( %)" << ID1 << EndLine;

				int opCode = -1;
				String opStr_prefix = opStr.SubString(0, 2);
				opStr = opStr.SubString(2, opStr.Length() - 2);
				if (opStr == "FMu")
					opCode = 133;
				else if (opStr == "IMu")
					opCode = 132;
				else if (opStr == "FAdd")
					opCode = 129;
				else if (opStr == "IAdd")
					opCode = 128;
				else if (opStr == "UDiv")
					opCode = 134;
				else if (opStr == "SDiv")
					opCode = 135;
				else if (opStr == "FDiv")
					opCode = 136;
				else if (opStr == "FSub")
					opCode = 131;
				else if (opStr == "ISub")
					opCode = 130;
				else if (opStr == "UMod")
					opCode = 137;
				else if (opStr == "SMod")
					opCode = 139;
				else if (opStr == "FMod")
					opCode = 141;
				else if (opStr == "ShiftLeftLogica")
					opCode = 196;
				else if (opStr == "ShiftRightArithmetic")
					opCode = 195;
				else if (opStr == "ShiftRightLogica")
					opCode = 194;
				else if (opStr == "BitwiseXor")
					opCode = 198;
				else if (opStr == "BitwiseAnd")
					opCode = 199;
				else if (opStr == "BitwiseOr")
					opCode = 197;
				else if (opStr == "LogicalAnd")
					opCode = 167;
				else if (opStr == "LogicalOr")
					opCode = 166;
				else if (opStr == "INotEqua")
					opCode = 171;
				else if (opStr == "FOrdNotEqua")
					opCode = 182;
				else if (opStr == "IEqua")
					opCode = 170;
				else if (opStr == "FOrdEqua")
					opCode = 180;
				else if (opStr == "SGreaterThanEqua")
					opCode = 175;
				else if (opStr == "FOrdGreaterThanEqua")
					opCode = 190;
				else if (opStr == "SGreaterThan")
					opCode = 173;
				else if (opStr == "FOrdGreaterThan")
					opCode = 186;
				else if (opStr == "SLessThanEqua")
					opCode = 179;
				else if (opStr == "FOrdLessThanEqua")
					opCode = 188;
				else if (opStr == "SLessThan")
					opCode = 177;
				else if (opStr == "FOrdLessThan")
					opCode = 184;
				else if (opStr == "UGreaterThan")
					opCode = 172;
				else if (opStr == "UGreaterThanEqua")
					opCode = 174;
				else if (opStr == "ULessThan")
					opCode = 176;
				else if (opStr == "ULessThanEqua")
					opCode = 178;
				if (opCode == -1)
					throw InvalidOperationException("unrecognized op string in CodeGenerator::OpBinaryInstr(): " + opStr);

				streamFunctionBody.Add(opCode + (5 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(ID0);
				streamFunctionBody.Add(ID1);
			}
			void OpMatrixTimesScalar(const int ID, const int typeID, const int ID0, const int ID1)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpMatrixTimesScalar %)" << typeID << LR"( %)" << ID0 << LR"( %)" << ID1 << EndLine;

				streamFunctionBody.Add(143 + (5 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(ID0);
				streamFunctionBody.Add(ID1);
			}
			void OpVectorTimesMatrix(const int ID, const int typeID, const int ID0, const int ID1)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpVectorTimesMatrix %)" << typeID << LR"( %)" << ID0 << LR"( %)" << ID1 << EndLine;

				streamFunctionBody.Add(144 + (5 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(ID0);
				streamFunctionBody.Add(ID1);
			}
			void OpMatrixTimesVector(const int ID, const int typeID, const int ID0, const int ID1)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpMatrixTimesVector %)" << typeID << LR"( %)" << ID0 << LR"( %)" << ID1 << EndLine;

				streamFunctionBody.Add(145 + (5 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(ID0);
				streamFunctionBody.Add(ID1);
			}
			void OpMatrixTimesMatrix(const int ID, const int typeID, const int ID0, const int ID1)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpMatrixTimesMatrix %)" << typeID << LR"( %)" << ID0 << LR"( %)" << ID1 << EndLine;

				streamFunctionBody.Add(146 + (5 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(ID0);
				streamFunctionBody.Add(ID1);
			}
			void OpConstantBool(const int typeID, const int ID, const bool b)
			{
				if (b)
				{
					sbTextTypeDefinition << LR"(%)" << ID << LR"( = OpConstantTrue%)" << EndLine;

					streamTypeDefinition.Add(41 + (3 << 16));
					streamTypeDefinition.Add(typeID);
					streamTypeDefinition.Add(ID);
				}
				else
				{
					sbTextTypeDefinition << LR"(%)" << ID << LR"( = OpConstantFalse%)" << EndLine;

					streamTypeDefinition.Add(42 + (3 << 16));
					streamTypeDefinition.Add(typeID);
					streamTypeDefinition.Add(ID);
				}
			}
			void OpConstantFloat(const int ID, const int typeID, float f)
			{
				sbTextTypeDefinition << LR"(%)" << ID << LR"( = OpConstant %)" << typeID << LR"( )" << SpirVFloatToString(f) << EndLine;

				streamTypeDefinition.Add(43 + (4 << 16));
				streamTypeDefinition.Add(typeID);
				streamTypeDefinition.Add(ID);
				streamTypeDefinition.Add(*reinterpret_cast<unsigned int*>(&f));
				//printf("%.8f -> %x\n", f, streamTypeDefinition[streamTypeDefinition.Count() - 1]);
			}
			void OpConstantInt(const int ID, const int typeID, int i)
			{
				sbTextTypeDefinition << LR"(%)" << ID << LR"( = OpConstant %)" << typeID << LR"( )" << i << EndLine;

				streamTypeDefinition.Add(43 + (4 << 16));
				streamTypeDefinition.Add(typeID);
				streamTypeDefinition.Add(ID);
				streamTypeDefinition.Add(*reinterpret_cast<unsigned int*>(&i));
			}
			void OpConstantUInt(const int ID, const int typeID, const unsigned int i)
			{
				sbTextTypeDefinition << LR"(%)" << ID << LR"( = OpConstant %)" << typeID << LR"( )";
				sbTextTypeDefinition << SpirVUIntToString(i);
				sbTextTypeDefinition << EndLine;

				streamTypeDefinition.Add(43 + (4 << 16));
				streamTypeDefinition.Add(typeID);
				streamTypeDefinition.Add(ID);
				streamTypeDefinition.Add(i);
			}
			void OpConstantComposite(const int ID, const int typeID, const List<int> &args)
			{
				sbTextTypeDefinition << LR"(%)" << ID << LR"( = OpConstantComposite %)" << typeID;
				for (auto & id : args)
					sbTextTypeDefinition << LR"( %)" << id;
				sbTextTypeDefinition << EndLine;

				streamTypeDefinition.Add(44 + ((3 + args.Count()) << 16));
				streamTypeDefinition.Add(typeID);
				streamTypeDefinition.Add(ID);
				for (auto & id : args)
					streamTypeDefinition.Add(id);
			}
			void OpCompositeConstruct(const int ID, const int typeID, const List<int> &args)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpCompositeConstruct %)" << typeID;
				for (auto & id : args)
					sbTextFunctionBody << LR"( %)" << id;
				sbTextFunctionBody << EndLine;

				streamFunctionBody.Add(80 + ((3 + args.Count()) << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				for (auto & id : args)
					streamFunctionBody.Add(id);
			}
			void OpCompositeExtract(const int ID, const int baseTypeID, const int compositeID, const int index)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpCompositeExtract %)" << baseTypeID << LR"( %)" << compositeID
					<< LR"( )" << index << EndLine;

				streamFunctionBody.Add(81 + (5 << 16));
				streamFunctionBody.Add(baseTypeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(compositeID);
				streamFunctionBody.Add(index);
			}
			void OpCompositeInsert(const int ID, const int typeID, const int objectID, const int compositeID, const int index)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpCompositeInsert %)"
					<< typeID << LR"( %)" << objectID << LR"( %)" << compositeID << LR"( )" << index << EndLine;

				streamFunctionBody.Add(82 + (6 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(objectID);
				streamFunctionBody.Add(compositeID);
				streamFunctionBody.Add(index);
			}
			void OpExtInst(const int ID, const int typeID, const int instrNumber, const List<int> &Arguments)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpExtInst %)" << typeID << LR"( %1 )";
				sbTextFunctionBody << instrNumber;
				for (auto & arg : Arguments)
					sbTextFunctionBody << LR"( %)" << arg;
				sbTextFunctionBody << EndLine;

				streamFunctionBody.Add(12 + ((5 + Arguments.Count()) << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(1);	//instruction set **<ID>**
				streamFunctionBody.Add(instrNumber);
				for (auto & arg : Arguments)
					streamFunctionBody.Add(arg);
			}
			void OpStore(const int op0, const int op1)
			{
				sbTextFunctionBody << LR"(OpStore %)" << op0 << LR"( %)" << op1 << EndLine;

				streamFunctionBody.Add(62 + (3 << 16));
				streamFunctionBody.Add(op0);
				streamFunctionBody.Add(op1);
			}
			void OpLoad(const int ID, const int typeID, const int variableID, const MemoryAccess ma)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpLoad %)" << typeID << LR"( %)"
					<< variableID << LR"( )" << MemoryAccessToString(ma) << EndLine;

				if (ma != MemoryAccess::None)
					throw NotImplementedException("not support memory access in CodeGenerator::OpLoad(): " + MemoryAccessToString(ma));

				streamFunctionBody.Add(61 + (4 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(variableID);
			}
			void OpVariable(const int ID, const int typeID, StorageClass store)
			{
				StringBuilder instrBuilder;
				instrBuilder << LR"(%)" << ID << LR"( = OpVariable %)" << typeID << LR"( )" << StorageClassToString(store) << EndLine;
				if (store == StorageClass::Function)
					sbTextFunctionVariable << instrBuilder.ProduceString() << EndLine;
				else
					sbTextTypeDefinition << instrBuilder.ProduceString() << EndLine;

				if (store == StorageClass::Function)
				{
					streamFunctionVariable.Add(59 + (4 << 16));
					streamFunctionVariable.Add(typeID);
					streamFunctionVariable.Add(ID);
					streamFunctionVariable.Add((int)store);
				}
				else
				{
					streamTypeDefinition.Add(59 + (4 << 16));
					streamTypeDefinition.Add(typeID);
					streamTypeDefinition.Add(ID);
					streamTypeDefinition.Add((int)store);
				}
			}
			void OpAccessChain(const int ID, const int typeID, const int structID, const int indexID)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpAccessChain %)" << typeID
					<< LR"( %)" << structID << LR"( %)" << indexID << EndLine;

				streamFunctionBody.Add(65 + (5 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(structID);
				streamFunctionBody.Add(indexID);
			}
			void OpImageSampleImplicitLod(
				const int ID, 
				const int typeID, 
				const int textureID, 
				const int coordinateID,
				const int Bias = -1)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpImageSampleImplicitLod %)"
					<< typeID << LR"( %)" << textureID << LR"( %)" << coordinateID;
				if (Bias != -1)
					sbTextFunctionBody << LR"( Bias %)" << Bias;
				sbTextFunctionBody << EndLine;

				int len = 5;
				int IO = 0;
				if (Bias != -1)
				{
					len++;
					IO |= (int)ImageOperands::Bias;
				}
				if (IO)
					len++;
				streamFunctionBody.Add(87 + (len << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(textureID);
				streamFunctionBody.Add(coordinateID);
				if (IO)
					streamFunctionBody.Add(IO);
				if (Bias != -1)
					streamFunctionBody.Add(Bias);
			}
			void OpImageSampleExplicitLod(
				const int ID, 
				const int typeID, 
				const int textureID, 
				const int coordinateID, 
				const int LodID,
				const int Bias = -1,
				const int GradX = -1, 
				const int GradY = -1)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpImageSampleExplicitLod %)"
					<< typeID << LR"( %)" << textureID << LR"( %)" << coordinateID;
				if (Bias != -1)
					sbTextFunctionBody << LR"( Bias %)" << Bias;
				if (GradX != -1)
					sbTextFunctionBody << LR"( Grad %)" << GradX << LR"( %)" << GradY;
				else
					sbTextFunctionBody << LR"( Lod %)" << LodID;
				sbTextFunctionBody << EndLine;

				int IO = 0;
				int len = 5;
				if (Bias != -1)
				{
					IO |= (int)ImageOperands::Bias;
					len++;
				}
				if (GradX != -1) {
					IO |= (int)ImageOperands::Grad;
					len += 2;
				}
				else
				{
					IO |= (int)ImageOperands::Lod;
					len++;
				}
				if (IO)
					len++;
				streamFunctionBody.Add(88 + (len << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(textureID);
				streamFunctionBody.Add(coordinateID);
				if (IO)
					streamFunctionBody.Add(IO);
				if (Bias != -1)
					streamFunctionBody.Add(Bias);
				if (GradX != -1)
				{
					streamFunctionBody.Add(GradX);
					streamFunctionBody.Add(GradY);
				}
				else
					streamFunctionBody.Add(LodID);
			}
			void OpImageSampleDrefImplicitLod(
				const int ID,
				const int typeID,
				const int textureID,
				const int coordinateID,
				const int DrefID)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpImageSampleDrefImplicitLod %)"
					<< typeID << LR"( %)" << textureID << LR"( %)" << coordinateID << LR"( %)" << DrefID;
				sbTextFunctionBody << EndLine;

				int len = 6;
				streamFunctionBody.Add(89 + (len << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(textureID);
				streamFunctionBody.Add(coordinateID);
				streamFunctionBody.Add(DrefID);
			}
			void OpImageSampleDrefExplicitLod(
				const int ID,
				const int typeID,
				const int textureID,
				const int coordinateID, 
				const int DrefID,
				const int LodID)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpImageSampleDrefExplicitLod %)"
					<< typeID << LR"( %)" << textureID << LR"( %)" << coordinateID << LR"( %)" << DrefID;
				sbTextFunctionBody << LR"( Lod %)" << LodID;
				sbTextFunctionBody << EndLine;

				int len = 8;
				streamFunctionBody.Add(90 + (len << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(textureID);
				streamFunctionBody.Add(coordinateID);
				streamFunctionBody.Add(DrefID);
				streamFunctionBody.Add((int)ImageOperands::Lod);
				streamFunctionBody.Add(LodID);
			}
			void OpImageSampleProjDrefImplicitLod(
				const int ID,
				const int typeID,
				const int textureID,
				const int coordinateID,
				const int DrefID) 
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpImageSampleProjDrefImplicitLod %)"
					<< typeID << LR"( %)" << textureID << LR"( %)" << coordinateID << LR"( %)" << DrefID;
				sbTextFunctionBody << EndLine;

				int len = 6;
				streamFunctionBody.Add(93 + (len << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(textureID);
				streamFunctionBody.Add(coordinateID);
				streamFunctionBody.Add(DrefID);
			}
			void OpImageSampleProjDrefExplicitLod(
				const int ID,
				const int typeID,
				const int textureID,
				const int coordinateID,
				const int DrefID,
				const int LodID)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpImageSampleProjDrefExplicitLod %)"
					<< typeID << LR"( %)" << textureID << LR"( %)" << coordinateID << LR"( %)" << DrefID;
				sbTextFunctionBody << LR"( Lod %)" << LodID;
				sbTextFunctionBody << EndLine;

				int len = 8;
				streamFunctionBody.Add(94 + (len << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(textureID);
				streamFunctionBody.Add(coordinateID);
				streamFunctionBody.Add(DrefID);
				streamFunctionBody.Add((int)ImageOperands::Lod);
				streamFunctionBody.Add(LodID);
			}
			void OpConvertSToF(const int ID, const int typeID, const int operandID)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpConvertSToF %)" << typeID << LR"( %)" << operandID << EndLine;

				streamFunctionBody.Add(111 + (4 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(operandID);
			}
			void OpConvertFToS(const int ID, const int typeID, const int operandID)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpConvertFToS %)" << typeID << LR"( %)" << operandID << EndLine;

				streamFunctionBody.Add(110 + (4 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(operandID);
			}
			void OpConvertFToU(const int ID, const int typeID, const int operandID)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpConvertFToU %)" << typeID << LR"( %)" << operandID << EndLine;

				streamFunctionBody.Add(109 + (4 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(operandID);
			}
			void OpConvertUToF(const int ID, const int typeID, const int operandID)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpConvertUToF %)" << typeID << LR"( %)" << operandID << EndLine;

				streamFunctionBody.Add(112 + (4 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(operandID);
			}
			void OpBitCast(const int ID, const int typeID, const int operandID)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpBitcast %)" << typeID << LR"( %)" << operandID << EndLine;

				streamFunctionBody.Add(124 + (4 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(operandID);
			}
			void OpTypeVoid(const int ID)
			{
				sbTextTypeDefinition << LR"(%)" << ID << LR"( = OpTypeVoid)" << EndLine;

				streamTypeDefinition.Add(19 + (2 << 16));
				streamTypeDefinition.Add(ID);
			}
			void OpTypeBool(const int ID)
			{
				sbTextTypeDefinition << LR"(%)" << ID << LR"( = OpTypeBool)" << EndLine;

				streamTypeDefinition.Add(20 + (2 << 16));
				streamTypeDefinition.Add(ID);
			}
			void OpTypeInt(const int ID, const int width, const int signedness)
			{
				sbTextTypeDefinition << LR"(%)" << ID << LR"( = OpTypeInt )" << width << LR"( )" << signedness << EndLine;

				streamTypeDefinition.Add(21 + (4 << 16));
				streamTypeDefinition.Add(ID);
				streamTypeDefinition.Add(width);
				streamTypeDefinition.Add(signedness);
			}
			void OpTypeFloat(const int ID, const int width)
			{
				sbTextTypeDefinition << LR"(%)" << ID << LR"( = OpTypeFloat )" << width << EndLine;

				streamTypeDefinition.Add(22 + (3 << 16));
				streamTypeDefinition.Add(ID);
				streamTypeDefinition.Add(width);
			}
			void OpTypeVector(const int ID, const int eleTypeID, const int vecLen)
			{
				sbTextTypeDefinition << LR"(%)" << ID << LR"( = OpTypeVector %)" << eleTypeID << LR"( )" << vecLen << EndLine;

				streamTypeDefinition.Add(23 + (4 << 16));
				streamTypeDefinition.Add(ID);
				streamTypeDefinition.Add(eleTypeID);
				streamTypeDefinition.Add(vecLen);
			}
			void OpTypeMatrix(const int ID, const int colTypeID, const int Dim)
			{
				sbTextTypeDefinition << LR"(%)" << ID << LR"( = OpTypeMatrix %)" << colTypeID << LR"( )" << Dim << EndLine;

				streamTypeDefinition.Add(24 + (4 << 16));
				streamTypeDefinition.Add(ID);
				streamTypeDefinition.Add(colTypeID);
				streamTypeDefinition.Add(Dim);
			}
			void OpTypeImage(const int ID, const int sampledTypeID, const Dim d, const int depth)
			{
				sbTextTypeDefinition << LR"(%)" << ID << LR"( = OpTypeImage %)" << sampledTypeID
					<< LR"( )" << DimToString(d) << LR"( )" << depth << LR"( 0 0 1 Unknown)" << EndLine;

				streamTypeDefinition.Add(25 + (9 << 16));
				streamTypeDefinition.Add(ID);
				streamTypeDefinition.Add(sampledTypeID);
				streamTypeDefinition.Add((int)d);			//Dim
				streamTypeDefinition.Add(depth);			//depth
				streamTypeDefinition.Add(0);				//arrayed
				streamTypeDefinition.Add(0);				//MS
				streamTypeDefinition.Add(1);				//sampled: will be used with sampler
				streamTypeDefinition.Add(0);				//image format: Unknown
			}
			void OpTypeSampledImage(const int ID, const int imageTypeID)
			{
				sbTextTypeDefinition << LR"(%)" << ID << LR"( = OpTypeSampledImage %)" << imageTypeID << EndLine;

				streamTypeDefinition.Add(27 + (3 << 16));
				streamTypeDefinition.Add(ID);
				streamTypeDefinition.Add(imageTypeID);
			}
			void OpTypePointer(const int ID, const StorageClass store, const int baseTypeID)
			{
				sbTextTypeDefinition << LR"(%)" << ID << LR"( = OpTypePointer )" << StorageClassToString(store)
					<< LR"( %)" << baseTypeID << EndLine;

				streamTypeDefinition.Add(32 + (4 << 16));
				streamTypeDefinition.Add(ID);
				streamTypeDefinition.Add((int)store);
				streamTypeDefinition.Add(baseTypeID);
			}
			void OpTypeArray(const int ID, const int elementTypeID, const int lengthID)
			{
				sbTextTypeDefinition << LR"(%)" << ID << LR"( = OpTypeArray %)" << elementTypeID << LR"( %)" << lengthID << EndLine;

				streamTypeDefinition.Add(28 + (4 << 16));
				streamTypeDefinition.Add(ID);
				streamTypeDefinition.Add(elementTypeID);
				streamTypeDefinition.Add(lengthID);
			}
			void OpTypeRuntimeArray(const int ID, const int elementTypeID)
			{
				sbTextTypeDefinition << LR"(%)" << ID << LR"( = OpTypeRuntimeArray %)" << elementTypeID << EndLine;

				streamTypeDefinition.Add(29 + (3 << 16));
				streamTypeDefinition.Add(ID);
				streamTypeDefinition.Add(elementTypeID);
			}
			void OpTypeStruct(const int ID, const List<int> & memberIDList)
			{
				sbTextTypeDefinition << LR"(%)" << ID << LR"( = OpTypeStruct)";
				for (auto & member : memberIDList)
					sbTextTypeDefinition << LR"( %)" << member;
				sbTextTypeDefinition << EndLine;

				streamTypeDefinition.Add(30 + ((2 + memberIDList.Count()) << 16));
				streamTypeDefinition.Add(ID);
				for (auto & member : memberIDList)
					streamTypeDefinition.Add(member);
			}

			void OpDot(const int ID, const int typeID, const int ID0, const int ID1)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpDot %)" << typeID << LR"( %)" << ID0 << LR"( %)" << ID1 << EndLine;

				streamFunctionBody.Add(148 + (5 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(ID0);
				streamFunctionBody.Add(ID1);
			}

			void OpTranspose(const int ID, const int typeID, const int Op0)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpTranspose %)" << typeID << LR"( %)" << Op0 << EndLine;

				streamFunctionBody.Add(84 + (4 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(Op0);
			}

			void OpDPdx(const int ID, const int typeID, const int Op0)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpDPdx %)" << typeID << LR"( %)" << Op0 << EndLine;

				streamFunctionBody.Add(207 + (4 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(Op0);
			}

			void OpDPdy(const int ID, const int typeID, const int Op0)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpDPdy %)" << typeID << LR"( %)" << Op0 << EndLine;

				streamFunctionBody.Add(208 + (4 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(Op0);
			}

			void OpFwidth(const int ID, const int typeID, const int Op0)
			{
				sbTextFunctionBody << LR"(%)" << ID << LR"( = OpFwidth %)" << typeID << LR"( %)" << Op0 << EndLine;

				streamFunctionBody.Add(209 + (4 << 16));
				streamFunctionBody.Add(typeID);
				streamFunctionBody.Add(ID);
				streamFunctionBody.Add(Op0);
			}

			void OpName(int ID, String Name)
			{
				sbDebug << LR"(OpName %)" << ID << LR"( ")" << Name << LR"(")" << EndLine;

				int len_i = streamDebug.Count();
				streamDebug.Add(0);
				streamDebug.Add(ID);
				int len = EncodeString(streamDebug, Name);
				streamDebug[len_i] = 5 + ((2 + len) << 16);
			}

			void OpMemberName(int ID, int index, String Name)
			{
				sbDebug << LR"(OpMemberName %)" << ID << LR"( )" << index << LR"( ")" << Name << LR"(")" << EndLine;

				int len_i = streamDebug.Count();
				streamDebug.Add(0);
				streamDebug.Add(ID);
				streamDebug.Add(index);
				int len = EncodeString(streamDebug, Name);
				streamDebug[len_i] = 6 + ((3 + len) << 16);
			}
		};

		class SpirVCodeGenContext
		{
		public:
			CompileResult * Result = nullptr;

			int CurrentID = 0;
			int MainFunctionID = 0;
			int MainFunctionReturnTypeID = 0;
			int MainFunctionTypeID = 0;
			int ReturnID = -1;
			Dictionary<String, int> TypeNameToID;					// 'int' - 1, 'vec4' - 2
			Dictionary<String, int> TypeStorageToTypePointerID;		// 'uint Function' - 5, 'mat3 Uniform' - 6
			Dictionary<String, int> FunctionNameToFunctionTypeID;
			Dictionary<String, int> FunctionNameToFunctionID;
			Dictionary<ILOperand*, int> ParameterNameToID;

			List<Dictionary<ILOperand*, int>> StackVariableNameToStorageID;
			List<Dictionary<ILOperand*, int>> StackVariableNameToValueID;
			Dictionary<String, int> InterfaceNameToID;

			Dictionary<int, int> Dictionary_ConstantIntToID;
			Dictionary<unsigned int, int> Dictionary_ConstantUIntToID;
			Dictionary<int, int> Dictionary_ConstantBoolToID;

			Dictionary<int, IDInfo> IDInfos;

			List<int> StackMergeBlock;
			List<int> StackContinueBlock;

			SpirVCodeBuilder CodeGen;

			void Clear()
			{
				Result = nullptr;
				CurrentID = 0;
				MainFunctionID = 0;
				MainFunctionReturnTypeID = 0;
				MainFunctionTypeID = 0;
				ReturnID = -1;
				StackVariableNameToStorageID.Clear();
				StackVariableNameToValueID.Clear();
				TypeNameToID.Clear();
				TypeStorageToTypePointerID.Clear();
				FunctionNameToFunctionTypeID.Clear();
				FunctionNameToFunctionID.Clear();
				ParameterNameToID.Clear();
				Dictionary_ConstantIntToID.Clear();
				Dictionary_ConstantUIntToID.Clear();
				Dictionary_ConstantBoolToID.Clear();
				IDInfos.Clear();
				StackMergeBlock.Clear();
				StackContinueBlock.Clear();
				CodeGen.Clear();
				InterfaceNameToID.Clear();
			}

			void ClearBuffer()
			{
				StackVariableNameToStorageID.Clear();
				StackVariableNameToValueID.Clear();
				ParameterNameToID.Clear();
				ReturnID = -1;
			}

			void PushScope()
			{
				StackVariableNameToStorageID.Add(Dictionary<ILOperand*, int>());
				StackVariableNameToValueID.Add(Dictionary<ILOperand*, int>());
			}

			void PopScope()
			{
				StackVariableNameToStorageID.RemoveAt(StackVariableNameToStorageID.Count() - 1);
				StackVariableNameToValueID.RemoveAt(StackVariableNameToValueID.Count() - 1); 
			}

			void UpdateVariable(ILOperand* op, int id)
			{
				if (op == nullptr)
					return;
				StackVariableNameToStorageID.Last()[op] = id;
			}

			int FindVariableID(ILOperand* op)
			{
				auto it = StackVariableNameToStorageID.end();
				while (it != StackVariableNameToStorageID.begin())
				{
					it--;
					if (it->ContainsKey(op))
						return (*it)[op];
				}
				return -1;
			}

			// ***NOTICE***: 
			//   There is no relation between VariableNameToStorageID and ValueIDToVariableNames.

			void UpdateValue(ILOperand *op, int id)
			{
				if (op == nullptr)
					return;
				StackVariableNameToValueID.Last()[op] = id;
			}

			int FindValueID(ILOperand *op)
			{
				auto it = StackVariableNameToValueID.end();
				while (it != StackVariableNameToValueID.begin())
				{
					it--;
					if (it->ContainsKey(op))
						return (*it)[op];
				}
				return -1;
			}

			void InvalidateValue(ILOperand *op)
			{
				if (StackVariableNameToValueID.Last().ContainsKey(op))
					StackVariableNameToValueID.Last()[op] = -1;
			}

			int DefineBasicType(RefPtr<ILType> Type)
			{
				String typeName = Type->ToString();
				if (TypeNameToID.ContainsKey(typeName))
					return TypeNameToID[typeName];
				TypeNameToID[typeName] = -1; //marked as visited

				if (typeName == "int" || typeName.StartsWith("ivec"))
				{
					DefineBasicType(new ILBasicType(ILBaseType::Int));

					if (typeName == "int")
					{
						++CurrentID;
						CodeGen.OpTypeInt(CurrentID, 32, 1);
						TypeNameToID[typeName] = CurrentID;
					}

					if (typeName.StartsWith("ivec"))
					{
						++CurrentID;
						CodeGen.OpTypeVector(CurrentID, TypeNameToID["int"](), StringToInt(typeName[4]));
						TypeNameToID[typeName] = CurrentID;
					}
				}

				if (typeName == "uint" || typeName.StartsWith("uvec"))
				{
					DefineBasicType(new ILBasicType(ILBaseType::UInt));

					if (typeName == "uint") {
						++CurrentID;
						CodeGen.OpTypeInt(CurrentID, 32, 0);
						TypeNameToID[typeName] = CurrentID;
					}

					if (typeName.StartsWith("uvec"))
					{
						++CurrentID;
						CodeGen.OpTypeVector(CurrentID, TypeNameToID["uint"](), StringToInt(typeName[4]));
						TypeNameToID[typeName] = CurrentID;
					}
				}

				if (typeName == "float" || typeName.StartsWith("vec") || typeName.StartsWith("mat"))
				{
					DefineBasicType(new ILBasicType(ILBaseType::Float));

					if (typeName == "float")
					{
						++CurrentID;
						CodeGen.OpTypeFloat(CurrentID, 32);
						TypeNameToID[typeName] = CurrentID;
					}

					if (typeName.StartsWith("vec"))
					{
						++CurrentID;
						CodeGen.OpTypeVector(CurrentID, TypeNameToID["float"](), StringToInt(typeName[3]));
						TypeNameToID[typeName] = CurrentID;
					}

					if (typeName == "mat3")
					{
						DefineBasicType(new ILBasicType(ILBaseType::Float3));
						++CurrentID;
						CodeGen.OpTypeMatrix(CurrentID, TypeNameToID["vec3"](), 3);
						TypeNameToID[typeName] = CurrentID;
					}

					if (typeName == "mat4")
					{
						DefineBasicType(new ILBasicType(ILBaseType::Float4));
						++CurrentID;
						CodeGen.OpTypeMatrix(CurrentID, TypeNameToID["vec4"](), 4);
						TypeNameToID[typeName] = CurrentID;
					}
				}

				if (typeName == "sampler2D")
				{
					//according to vulkan specification
					//	Resource Descriptors, Descriptor Types, Sampled Image
					DefineBasicType(new ILBasicType(ILBaseType::Float));

					++CurrentID;
					CodeGen.OpTypeImage(CurrentID, TypeNameToID["float"](), Dim::e2D, 0);
					int tmp = CurrentID;

					++CurrentID;
					CodeGen.OpTypeSampledImage(CurrentID, tmp);
					TypeNameToID[typeName] = CurrentID;
				}

				if (typeName == "samplerCube")
				{
					DefineBasicType(new ILBasicType(ILBaseType::Float));

					++CurrentID;
					CodeGen.OpTypeImage(CurrentID, TypeNameToID["float"](), Dim::eCube, 0);
					int tmp = CurrentID;

					++CurrentID;
					CodeGen.OpTypeSampledImage(CurrentID, tmp);
					TypeNameToID[typeName] = CurrentID;
				}

				if (typeName == "samplerCubeShadow")
				{
					DefineBasicType(new ILBasicType(ILBaseType::Float));

					++CurrentID;
					CodeGen.OpTypeImage(CurrentID, TypeNameToID["float"](), Dim::eCube, 1);
					int tmp = CurrentID;

					++CurrentID;
					CodeGen.OpTypeSampledImage(CurrentID, tmp);
					TypeNameToID[typeName] = CurrentID;
				}

				if (typeName == "sampler2DShadow")
				{
					DefineBasicType(new ILBasicType(ILBaseType::Float));

					++CurrentID;
					CodeGen.OpTypeImage(CurrentID, TypeNameToID["float"](), Dim::e2D, 1);
					int tmp = CurrentID;

					++CurrentID;
					CodeGen.OpTypeSampledImage(CurrentID, tmp);
					TypeNameToID[typeName] = CurrentID;
				}

				if (typeName == "bool")
				{
					++CurrentID;
					CodeGen.OpTypeBool(CurrentID);
					TypeNameToID[typeName] = CurrentID;
				}

				if (TypeNameToID[typeName] == -1)
				{
					throw InvalidProgramException("fail to generate type definition for: " + typeName);
				}

				int id = TypeNameToID[typeName];
				IDInfos[id] = IDInfo::CreateIDInfoForTypeofValue(id, Type);

				return id;
			}

			//UniformOrBuffer - 0: none; 1: uniform; 2: buffer 
			int DefineType(RefPtr<ILType> Type, int UniformOrBuffer = 0)
			{
				if (!Type)
				{
					if (TypeNameToID.ContainsKey("void"))
						return TypeNameToID["void"];
					++CurrentID;
					//TypeDefinition << LR"(%)" <<  << LR"( = OpTypeVoid)" << EndLine;
					CodeGen.OpTypeVoid(CurrentID);
					TypeNameToID["void"] = CurrentID;
					IDInfos[CurrentID] = IDInfo::CreateIDInfoForTypeofValue(CurrentID, nullptr);
					return CurrentID;
				}
				int RetID = -1;
				if (auto ArrayType = dynamic_cast<ILArrayType*>(Type.Ptr()))
				{
					String IndexName = Type->ToString();
					if (UniformOrBuffer != 0)
						IndexName = IndexName + "#" + UniformOrBuffer;
					if (TypeNameToID.ContainsKey(IndexName))
						return TypeNameToID[IndexName];

					if (ArrayType->ArrayLength != 0)
					{
						//normal constant-length array
						int lengthID = AddInstrConstantInt(ArrayType->ArrayLength);

						int elementTypeID = DefineType(ArrayType->BaseType, UniformOrBuffer);
						++CurrentID;
						CodeGen.OpTypeArray(CurrentID, elementTypeID, lengthID);
						TypeNameToID[IndexName] = CurrentID;
						IDInfos[CurrentID] = IDInfo::CreateIDInfoForTypeofValue(CurrentID, Type, UniformOrBuffer);
						RetID = CurrentID;
					}
					else
					{
						//dynamic array
						int elementTypeID = DefineType(ArrayType->BaseType, UniformOrBuffer);
						++CurrentID;
						CodeGen.OpTypeRuntimeArray(CurrentID, elementTypeID);
						TypeNameToID[IndexName] = CurrentID;
						IDInfos[CurrentID] = IDInfo::CreateIDInfoForTypeofValue(CurrentID, Type, UniformOrBuffer);
						RetID = CurrentID;
					}

					if (UniformOrBuffer != 0)
					{
						int Stride = GetSize(ArrayType->BaseType.Ptr(), UniformOrBuffer);
						CodeGen.OpDecorate(RetID, Decoration::ArrayStride, Stride);
					}
				}
				if (auto StructType = dynamic_cast<ILStructType*>(Type.Ptr()))
				{
					String IndexName = Type->ToString();
					if (UniformOrBuffer != 0)
						IndexName = IndexName + "#" + UniformOrBuffer;
					if (TypeNameToID.ContainsKey(IndexName))
						return TypeNameToID[IndexName];

					List<int> memberIDList;
					for (auto & member : StructType->Members)
						memberIDList.Add(DefineType(member.Type, UniformOrBuffer));
					++CurrentID;
					CodeGen.OpTypeStruct(CurrentID, memberIDList);
					TypeNameToID[IndexName] = CurrentID;
					IDInfos[CurrentID] = IDInfo::CreateIDInfoForTypeofValue(CurrentID, Type, UniformOrBuffer);
					RetID = CurrentID;

					//generate decoration for struct layout
					if (UniformOrBuffer != 0)
					{
						int Offset = 0;
						int Index = 0;
						for (auto & MemberTypeID : memberIDList)
						{
							RefPtr<ILType> MemberType = IDInfos[MemberTypeID]().GetILType();

							int BaseAlignment = GetBaseAlignment(MemberType.Ptr(), UniformOrBuffer);

							//round up to baseAlignment
							if (Offset % BaseAlignment)
								Offset += BaseAlignment - Offset % BaseAlignment;

							AddInstrMemberDecorate(RetID, Index, Decoration::Offset, Offset);

							if (MemberType->IsFloatMatrix())
							{
								AddInstrMemberDecorate(RetID, Index, Decoration::ColMajor);
								AddInstrMemberDecorate(RetID, Index, Decoration::MatrixStride, 16);
							}

							Offset += GetSize(MemberType.Ptr(), UniformOrBuffer);
							Index++;
						}
					}

					//generate debug information
					CodeGen.OpName(RetID, Type->ToString());
					int index = 0;
					for (auto &member : StructType->Members)
					{
						CodeGen.OpMemberName(RetID, index, member.FieldName);
						index++;
					}
				}
				if (auto BasicType = dynamic_cast<ILBasicType*>(Type.Ptr()))
				{
					if (TypeNameToID.ContainsKey(Type->ToString()))
						return TypeNameToID[Type->ToString()];
					RetID = DefineBasicType(Type);
				}
				return RetID;
			}

			int DefineTypePointer(RefPtr<ILType> Type, StorageClass store, int UniformOrBuffer = 0)
			{
				String PointerName = Type->ToString() + "$" + StorageClassToString(store) + "#" + UniformOrBuffer;
				if (TypeStorageToTypePointerID.ContainsKey(PointerName))
					return TypeStorageToTypePointerID[PointerName]();

				int basetypeID = DefineType(Type, UniformOrBuffer);
				++CurrentID;
				CodeGen.OpTypePointer(CurrentID, store, basetypeID);
				TypeStorageToTypePointerID[PointerName] = CurrentID;
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForTypeofPointer(CurrentID, Type, basetypeID, store);
				return CurrentID;
			}

			int AddInstrTypeFunction(CompiledFunction * func, const List<RefPtr<ILType>> & argTypes, RefPtr<ILType> returnType)
			{
				int returnTypeID = DefineType(returnType);
				for (auto & arg : argTypes)
					DefineTypePointer(arg, StorageClass::Function);
				int functionTypeID = ++CurrentID;
				List<int> argIDList;
				for (auto & arg : argTypes)
					argIDList.Add(DefineTypePointer(arg, StorageClass::Function));
				CodeGen.OpTypeFunction(functionTypeID, returnTypeID, argIDList);
				IDInfos[functionTypeID] = IDInfo::CreateIDInfoForFunction(functionTypeID, func);
				if (func)
					FunctionNameToFunctionTypeID[func->Name] = functionTypeID;
				else
					FunctionNameToFunctionTypeID["main"] = functionTypeID;
				return functionTypeID;
			}

			int AddInstrConstantBool(int value)
			{
				if (Dictionary_ConstantBoolToID.ContainsKey(value != 0))
					return Dictionary_ConstantBoolToID[value != 0].GetValue();

				auto Type = GetTypeFromString("bool");
				int typeID = DefineType(Type);
				++CurrentID;
				CodeGen.OpConstantBool(typeID, CurrentID, value != 0);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(CurrentID, Type, 0, typeID);
				Dictionary_ConstantBoolToID[value != 0] = CurrentID;
				return CurrentID;
			}

			int AddInstrConstantFloat(float f)
			{
				auto Type = GetTypeFromString("float");
				int typeID = DefineType(Type);
				++CurrentID;
				CodeGen.OpConstantFloat(CurrentID, typeID, f);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(CurrentID, Type, 0, typeID);
				return CurrentID;
			}

			int AddInstrConstantInt(int i)
			{
				auto Type = GetTypeFromString("int");
				if (Dictionary_ConstantIntToID.ContainsKey(i))
					return Dictionary_ConstantIntToID[i].GetValue();

				int typeID = DefineType(Type);
				++CurrentID;
				CodeGen.OpConstantInt(CurrentID, typeID, i);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(CurrentID, Type, 0, typeID);
				Dictionary_ConstantIntToID[i] = CurrentID;
				return CurrentID;
			}

			int AddInstrConstantUInt(unsigned int i)
			{
				auto Type = GetTypeFromString("uint");
				if (Dictionary_ConstantUIntToID.ContainsKey(i))
					return Dictionary_ConstantUIntToID[i].GetValue();

				int typeID = DefineType(Type);
				++CurrentID;
				CodeGen.OpConstantUInt(CurrentID, typeID, i);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(CurrentID, Type, 0, typeID);
				Dictionary_ConstantUIntToID[i] = CurrentID;
				return CurrentID;
			}

			int AddInstrConstantCompositeFloat(float *f, int len)
			{
				RefPtr<ILType> Type;
				if (len == 2)
					Type = GetTypeFromString("vec2");
				else if (len == 3)
					Type = GetTypeFromString("vec3");
				else if (len == 4)
					Type = GetTypeFromString("vec4");
				else
					throw InvalidOperationException("Invalid type in AddInstrConstantCompositeFloat(): vec"+len);
				int typeID = DefineType(Type);

				List<int> elementIDs;
				for (int i = 0; i < len; i++)
					elementIDs.Add(AddInstrConstantFloat(f[i]));

				++CurrentID;
				CodeGen.OpConstantComposite(CurrentID, typeID, elementIDs);

				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(CurrentID, Type, 0, typeID);
				return CurrentID;
			}

			int AddInstrConstantCompositeInt(int *v, int len)
			{
				RefPtr<ILType> Type;
				if (len == 2)
					Type = GetTypeFromString("ivec2");
				else if (len == 3)
					Type = GetTypeFromString("ivec3");
				else if (len == 4)
					Type = GetTypeFromString("ivec4");
				else
					throw InvalidOperationException("Invalid type in AddInstrConstantCompositeInt(): ivec" + len);
				int typeID = DefineType(Type);

				List<int> elementIDs;
				for (int i = 0; i < len; i++)
					elementIDs.Add(AddInstrConstantInt(v[i]));

				++CurrentID;
				CodeGen.OpConstantComposite(CurrentID, typeID, elementIDs);

				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(CurrentID, Type, 0, typeID);
				return CurrentID;
			}

			int AddInstrConstantCompositeUInt(unsigned int *v, int len)
			{
				RefPtr<ILType> Type;
				if (len == 2)
					Type = GetTypeFromString("uvec2");
				else if (len == 3)
					Type = GetTypeFromString("uvec3");
				else if (len == 4)
					Type = GetTypeFromString("uvec4");
				else
					throw InvalidOperationException("Invalid type in AddInstrConstantCompositeUInt(): uvec" + len);
				int typeID = DefineType(Type);

				List<int> elementIDs;
				for (int i = 0; i < len; i++)
					elementIDs.Add(AddInstrConstantUInt(*(v + i)));

				++CurrentID;
				CodeGen.OpConstantComposite(CurrentID, typeID, elementIDs);

				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(CurrentID, Type, 0, typeID);
				return CurrentID;
			}

			int AddInstrConstantMatrix(float *f, int n)
			{
				RefPtr<ILType> Type;
				if (n == 3)
					Type = GetTypeFromString("mat3");
				else if (n == 4)
					Type = GetTypeFromString("mat4");
				else
					throw InvalidOperationException("Invalid type in AddInstrConstantMatrix(): mat" + n);
				int typeID = DefineType(Type);

				List<int> vectorIDs;
				for (int i = 0; i < n; i++)
					vectorIDs.Add(AddInstrConstantCompositeFloat(f + (i * n), n));

				++CurrentID;
				CodeGen.OpConstantComposite(CurrentID, typeID, vectorIDs);

				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(CurrentID, Type, 0, typeID);
				return CurrentID;
			}

			int AddInstrCompositeConstruct(ILOperand* op, RefPtr<ILType> Type, List<int> Arguments)
				// return ID of this instruction
			{
				int typeID = DefineType(Type);
				++CurrentID;
				CodeGen.OpCompositeConstruct(CurrentID, typeID, Arguments);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(CurrentID, Type, op, typeID);
				UpdateValue(op, CurrentID);
				return CurrentID;
			}

			int AddInstrCompositeExtract(int ID, RefPtr<ILType> baseType, int index)
			{
				int baseTypeID = DefineType(baseType);
				++CurrentID;
				CodeGen.OpCompositeExtract(CurrentID, baseTypeID, ID, index);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(CurrentID, baseType, 0, baseTypeID);
				return CurrentID;
			}

			int AddInstrCompositeInsert(RefPtr<ILType> Type, int ID, int index, int op)
			{
				//ID[index] = op
				int typeID = DefineType(Type);
				++CurrentID;
				CodeGen.OpCompositeInsert(CurrentID, typeID, op, ID, index);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(CurrentID, Type, 0, typeID);
				return CurrentID;
			}

			int AddInstrExtInst(ILOperand* op, RefPtr<ILType> Type, int instrNumber, List<int> Arguments)
			{
				int typeID = DefineType(Type);
				++CurrentID;
				CodeGen.OpExtInst(CurrentID, typeID, instrNumber, Arguments);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(CurrentID, Type, op, typeID);
				UpdateValue(op, CurrentID);
				return CurrentID;
			}

			void AddInstrStore(ILOperand *op, int op0, int op1) {
				CodeGen.OpStore(op0, op1);
				UpdateValue(op, op1);
				return;
			}

			int AddInstrVariableDeclaration(ILOperand *op, RefPtr<ILType> typeIL, StorageClass store, String DebugName = "", int UniformOrBuffer = 0)
			{
				int typeID = DefineTypePointer(typeIL, store, UniformOrBuffer);
				++CurrentID;
				CodeGen.OpVariable(CurrentID, typeID, store);
				UpdateVariable(op, CurrentID);
				IDInfos[CurrentID] =
					IDInfo::CreateIDInfoForPointer(CurrentID, op, typeID, typeIL, IDInfos[typeID]().GetBaseTypeID(), store);
				//Debug Information
				CodeGen.OpName(CurrentID, DebugName!=""? DebugName : (op?op->Name:""));
				return CurrentID;
			}

			int AddInstrAccessChain_VectorMember(ILOperand *op, int ID, int indexID, int index)
			{
				String variableName = "";
				if (index == -1 && indexID == -1)
					throw InvalidOperationException("indexID=-1 && index=-1 in AddInstrAccessChain_VectorMember()");

				if (indexID == -1) {
					//indexID == -1 && index != -1
					indexID = AddInstrConstantInt(index);
				}

				if (index != -1)
				{
					variableName = IDInfos[ID]().GetVariableName() + "[" + index + "]";
				}

				RefPtr<ILType> TypeIL = IDInfos[ID]().GetILType();
				RefPtr<ILType> memberTypeIL = nullptr;
				if (TypeIL->IsFloatMatrix())
				{
					if (TypeIL->ToString() == "mat3")
						memberTypeIL = GetTypeFromString("vec3");
					else if (TypeIL->ToString() == "mat4")
						memberTypeIL = GetTypeFromString("vec4");
				}
				else if (TypeIL->IsFloatVector())
				{
					memberTypeIL = GetTypeFromString("float");
				}
				else if (TypeIL->IsIntVector())
				{
					memberTypeIL = GetTypeFromString("int");
				}
				else if (TypeIL->IsUIntVector())
				{
					memberTypeIL = GetTypeFromString("uint");
				}
				else
					throw InvalidOperationException("invalid operand type for access chain: " + TypeIL->ToString());

				int memberTypeID = DefineTypePointer(memberTypeIL, IDInfos[ID]().GetStorageClass());

				++CurrentID;
				/*FunctionBody << LR"(%)" << CurrentID << LR"( = OpAccessChain %)" << memberTypeID
				<< LR"( %)" << ID << LR"( %)" << indexID << EndLine;*/
				CodeGen.OpAccessChain(CurrentID, memberTypeID, ID, indexID);
				IDInfos[CurrentID] =
					IDInfo::CreateIDInfoForPointer(
						CurrentID,
						op,
						memberTypeID,
						memberTypeIL,
						IDInfos[memberTypeID]().GetBaseTypeID(),
						IDInfos[ID]().GetStorageClass()
					);
				UpdateVariable(op, CurrentID);
				return CurrentID;
			}

			int AddInstrAccessChain_StructMember(ILOperand *op, int ID, int indexID, int index)
			{
				if (indexID == -1)
					indexID = AddInstrConstantInt(index);
				RefPtr<ILType> Type = IDInfos[ID]().GetILType();
				ILStructType * structType = dynamic_cast<ILStructType*>(Type.Ptr());
				RefPtr<ILType> memberBasetypeIL = structType->Members[index].Type;
				int memberTypeID = DefineTypePointer(memberBasetypeIL, IDInfos[ID]().GetStorageClass());

				++CurrentID;
				/*
				FunctionBody << LR"(%)" << CurrentID << LR"( = OpAccessChain %)"
				<< memberTypeID
				<< LR"( %)" << ID << LR"( %)" << indexID << EndLine;
				*/
				CodeGen.OpAccessChain(CurrentID, memberTypeID, ID, indexID);

				UpdateVariable(op, CurrentID);
				IDInfos[CurrentID] =
					IDInfo::CreateIDInfoForPointer(
						CurrentID,
						op,
						memberTypeID,
						memberBasetypeIL,
						IDInfos[memberTypeID]().GetBaseTypeID(),
						IDInfos[ID]().GetStorageClass()
					);
				return CurrentID;
			}

			int AddInstrAccessChain_StructMember(ILOperand *op, String memberName)
			{
				int structID = FindVariableID(op);
				ILStructType* structIL = dynamic_cast<ILStructType*>(IDInfos[structID]().GetILType().Ptr());
				if (!structIL)
					throw InvalidProgramException("can not convert to ILStruct in AddInstrAccessChain_StructMember()");
				int index = structIL->Members.FindFirst([&](ILStructType::ILStructField member)
				{
					return member.FieldName == memberName;
				});
				int indexID = AddInstrConstantInt(index);
				return AddInstrAccessChain_StructMember(op, structID, indexID, index);
			}

			int AddInstrAccessChain_StructMember(ILOperand *op, int structID, String memberName)
			{
				ILStructType* structIL = dynamic_cast<ILStructType*>(IDInfos[structID]().GetILType().Ptr());
				if (!structIL)
					throw InvalidProgramException("can not convert to ILStruct in AddInstrAccessChain_StructMember()");
				int index = structIL->Members.FindFirst([&](ILStructType::ILStructField member)
				{
					return member.FieldName == memberName;
				});
				int indexID = AddInstrConstantInt(index);
				return AddInstrAccessChain_StructMember(op, structID, indexID, index);
			}

			int AddInstrAccessChain_ArrayMember(ILOperand *op, RefPtr<ILType> Type, int ID, int indexID)
			{
				if (!Type)
					throw InvalidProgramException("empty type in AddInstrAccessChain_ArrayMember()");
				auto arrayType = dynamic_cast<ILArrayType*>(Type.Ptr());
				int baseTypeID = DefineTypePointer(arrayType->BaseType, IDInfos[ID]().GetStorageClass()); //it's a pointer

				++CurrentID;
				CodeGen.OpAccessChain(CurrentID, baseTypeID, ID, indexID);
				IDInfos[CurrentID] =
					IDInfo::CreateIDInfoForPointer(
						CurrentID,
						op,
						baseTypeID,
						arrayType->BaseType,
						IDInfos[baseTypeID]().GetBaseTypeID(),
						IDInfos[ID]().GetStorageClass()
					);
				UpdateVariable(op, CurrentID);

				return CurrentID;
			}

			int AddInstrLoad(int variableID, MemoryAccess ma)
			{
				++CurrentID;
				RefPtr<ILType> Type = IDInfos[variableID]().GetILType();
				int TypeID = IDInfos[variableID]().GetBaseTypeID();
				CodeGen.OpLoad(CurrentID, TypeID, variableID, ma);
				IDInfos[CurrentID]
					= IDInfo::CreateIDInfoForValue(
						CurrentID,
						Type,
						0,
						TypeID);
				return CurrentID;
			}

			int AddInstrLoad(ILOperand *op, ILOperand *targetOp, MemoryAccess ma)
			{
				int targetID = FindVariableID(targetOp);
				if (targetID == -1)
					return -1;
				int typeID = IDInfos[targetID]().GetBaseTypeID();
				++CurrentID;
				CodeGen.OpLoad(CurrentID, typeID, targetID, ma);
				IDInfos[CurrentID]
					= IDInfo::CreateIDInfoForValue(
						CurrentID,
						IDInfos[targetID]().GetILType(),
						op,
						typeID);
				UpdateValue(op, CurrentID);
				return CurrentID;
			}

			int AddInstrINotEqual(int id0, int id1)
			{
				RefPtr<ILType> typeIL = GetTypeFromString("bool");
				int typeID = DefineType(typeIL);
				++CurrentID;
				CodeGen.OpINotEqual(CurrentID, typeID, id0, id1);
				IDInfos[CurrentID]
					= IDInfo::CreateIDInfoForValue(
						CurrentID,
						typeIL,
						0,
						typeID
					);
				return CurrentID;
			}

			int AddInstrTexture(
				ILOperand *op, 
				int textureID, 
				int coordinateID, 
				ExecutionModel currentExecutionModel,
				int Bias = -1,
				int GradX = -1,
				int GradY = -1)
			{
				RefPtr<ILType> typeIL = GetTypeFromString("vec4");
				int typeID = DefineType(typeIL);

				++CurrentID;
				if (currentExecutionModel == ExecutionModel::Fragment && GradX == -1)
				{
					//implicit LOD
					CodeGen.OpImageSampleImplicitLod(CurrentID, typeID, textureID, coordinateID, Bias);
				}
				else
				{
					//explicit LOD
					int zeroID = AddInstrConstantInt(0);
					CodeGen.OpImageSampleExplicitLod(CurrentID, typeID, textureID, coordinateID, zeroID, Bias, GradX, GradY);
				}
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					typeIL,
					op,
					typeID
				);
				UpdateValue(op, CurrentID);

				return CurrentID;
			}

			int AddInstrTextureShadow(
				ILOperand *op, 
				int textureID, 
				int coordinateID, 
				ExecutionModel currentExecutionModel) 
			{
				RefPtr<ILType> typeIL = GetTypeFromString("vec4");
				int typeID = DefineType(typeIL);

				int veclen = IDInfos[coordinateID]().GetILType()->GetVectorSize();
				int DrefID = AddInstrCompositeExtract(coordinateID, GetTypeFromString("float"), veclen-1);

				++CurrentID;
				if (currentExecutionModel == ExecutionModel::Fragment)
				{
					//implicit LOD
					CodeGen.OpImageSampleDrefImplicitLod(CurrentID, typeID, textureID, coordinateID, DrefID);
				}
				else
				{
					//explicit LOD
					int zeroID = AddInstrConstantInt(0);
					CodeGen.OpImageSampleDrefExplicitLod(CurrentID, typeID, textureID, coordinateID, DrefID, zeroID);
				}
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					typeIL,
					op,
					typeID
				);
				UpdateValue(op, CurrentID);

				return CurrentID;
			}

			int AddInstrTexture2DShadowProj(
				ILOperand *op,
				int textureID,
				int coordinateID,	////coordinateID: u, v, depth, q
				ExecutionModel currentExecutionModel)
			{
				RefPtr<ILType> typeIL = GetTypeFromString("vec4");
				int typeID = DefineType(typeIL);

				int DrefID = AddInstrCompositeExtract(coordinateID, GetTypeFromString("float"), 2);
				int qID = AddInstrCompositeExtract(coordinateID, GetTypeFromString("float"), 3);
				int NewCoordinateID = AddInstrCompositeInsert(IDInfos[coordinateID]().GetILType(), coordinateID, 2, qID);

				++CurrentID;
				if (currentExecutionModel == ExecutionModel::Fragment)
				{
					//implicit LOD
					CodeGen.OpImageSampleProjDrefImplicitLod(CurrentID, typeID, textureID, NewCoordinateID, DrefID);
				}
				else
				{
					//explicit LOD
					int zeroID = AddInstrConstantInt(0);
					CodeGen.OpImageSampleProjDrefExplicitLod(CurrentID, typeID, textureID, NewCoordinateID, DrefID, zeroID);
				}
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					typeIL,
					op,
					typeID
				);
				UpdateValue(op, CurrentID);

				return CurrentID;
			}

			int AddInstrConvertSToF(int destTypeID, int operandID)
			{
				++CurrentID;
				CodeGen.OpConvertSToF(CurrentID, destTypeID, operandID);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					IDInfos[destTypeID]().GetILType(),
					0,
					destTypeID
				);
				return CurrentID;
			}

			int AddInstrConvertFToS(int destTypeID, int operandID)
			{
				++CurrentID;
				CodeGen.OpConvertFToS(CurrentID, destTypeID, operandID);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					IDInfos[destTypeID]().GetILType(),
					0,
					destTypeID
				);
				return CurrentID;
			}

			int AddInstrConvertUToF(int destTypeID, int operandID)
			{
				++CurrentID;
				CodeGen.OpConvertUToF(CurrentID, destTypeID, operandID);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					IDInfos[destTypeID]().GetILType(),
					0,
					destTypeID
				);
				return CurrentID;
			}

			int AddInstrConvertFToU(int destTypeID, int operandID)
			{
				++CurrentID;
				CodeGen.OpConvertFToU(CurrentID, destTypeID, operandID);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					IDInfos[destTypeID]().GetILType(),
					0,
					destTypeID
				);
				return CurrentID;
			}

			int AddInstrConvertSToU(int destTypeID, int operandID)
			{
				++CurrentID;
				CodeGen.OpBitCast(CurrentID, destTypeID, operandID);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					IDInfos[destTypeID]().GetILType(),
					0,
					destTypeID
				);
				return CurrentID;
			}

			int AddInstrConvertUToS(int destTypeID, int operandID) 
			{
				++CurrentID;
				CodeGen.OpBitCast(CurrentID, destTypeID, operandID);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					IDInfos[destTypeID]().GetILType(),
					0,
					destTypeID
				);
				return CurrentID;
			}

			int AddInstrBitcast(int destTypeID, int operandID)
			{
				++CurrentID;
				CodeGen.OpBitCast(CurrentID, destTypeID, operandID);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					IDInfos[destTypeID]().GetILType(),
					0,
					destTypeID
				);
				return CurrentID;
			}

			int AddInstrFunctionCall(ILOperand *op, int typeID, int funcID, List<int> &args)
			{
				++CurrentID;
				CodeGen.OpFunctionCall(CurrentID, typeID, funcID, args);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					IDInfos[typeID]().GetILType(),
					op,
					typeID
				);
				UpdateValue(op, CurrentID);
				return CurrentID;
			}

			int AddInstrFnegate(ILOperand *op, int typeID, int valueID)
			{
				++CurrentID;
				CodeGen.OpFNegate(CurrentID, typeID, valueID);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					IDInfos[typeID]().GetILType(),
					op, 
					typeID
				);
				UpdateValue(op, CurrentID);
				return CurrentID;
			}

			int AddInstrSnegate(ILOperand *op, int typeID, int valueID)
			{
				++CurrentID;
				CodeGen.OpSNegate(CurrentID, typeID, valueID);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					IDInfos[typeID]().GetILType(),
					op,
					typeID
				);
				UpdateValue(op, CurrentID);
				return CurrentID;
			}

			int AddInstrNot(ILOperand *op, int typeID, int valueID)
			{
				++CurrentID;
				CodeGen.OpNot(CurrentID, typeID, valueID);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					IDInfos[typeID]().GetILType(),
					op,
					typeID
				);
				UpdateValue(op, CurrentID);
				return CurrentID;
			}

			int AddInstrLogicalNot(ILOperand *op, int typeID, int valueID)
			{
				++CurrentID;
				CodeGen.OpLogicalNot(CurrentID, typeID, valueID);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					IDInfos[typeID]().GetILType(),
					op,
					typeID
				);
				UpdateValue(op, CurrentID);
				return CurrentID;
			}

			int AddInstrMatrixTimesScalar(ILOperand *op, int ID0, int ID1)
			{
				++CurrentID;
				CodeGen.OpMatrixTimesScalar(CurrentID, IDInfos[ID0]().GetTypeID(), ID0, ID1);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					IDInfos[ID0]().GetILType(),
					op,
					IDInfos[ID0]().GetTypeID()
				);
				UpdateValue(op, CurrentID);
				return CurrentID;
			}

			int AddInstrVectorTimesMatrix(ILOperand *op, int ID0, int ID1)
			{
				++CurrentID;
				CodeGen.OpVectorTimesMatrix(CurrentID, IDInfos[ID0]().GetTypeID(), ID0, ID1);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					IDInfos[ID0]().GetILType(),
					op,
					IDInfos[ID0]().GetTypeID()
				);
				UpdateValue(op, CurrentID);
				return CurrentID;
			}

			int AddInstrMatrixTimesVector(ILOperand *op, int ID0, int ID1)
			{
				++CurrentID;
				CodeGen.OpMatrixTimesVector(CurrentID, IDInfos[ID1]().GetTypeID(), ID0, ID1);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					IDInfos[ID1]().GetILType(),
					op,
					IDInfos[ID1]().GetTypeID()
				);
				UpdateValue(op, CurrentID);
				return CurrentID;
			}

			int AddInstrMatrixTimesMatrix(ILOperand *op, int ID0, int ID1)
			{
				++CurrentID;
				CodeGen.OpMatrixTimesMatrix(CurrentID, IDInfos[ID0]().GetTypeID(), ID0, ID1);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					IDInfos[ID0]().GetILType(),
					op,
					IDInfos[ID0]().GetTypeID()
				);
				UpdateValue(op, CurrentID);
				return CurrentID;
			}

			int AddInstrBinaryInstr(ILOperand *op, RefPtr<ILType> instrType, const String &opStr, int ID0, int ID1)
			{
				int instrTypeID = DefineType(instrType);
				CurrentID++;
				CodeGen.OpBinaryInstr(CurrentID, opStr, instrTypeID, ID0, ID1);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(CurrentID, instrType, op, instrTypeID);
				UpdateValue(op, CurrentID);
				return CurrentID;
			}

			int AddInstrMulAdd(int operandID, float mul, float add)
			{
				//return 0.5*op+0.5
				int typeID = IDInfos[operandID]().GetTypeID();  //shoudl be float 
				RefPtr<ILType> typeIL = IDInfos[operandID]().GetILType();

				int mul_ID = AddInstrConstantFloat(mul);
				mul_ID = ConvertBasicType(mul_ID, IDInfos[mul_ID]().GetILType(), IDInfos[operandID]().GetILType());

				int add_ID = AddInstrConstantFloat(add);
				add_ID = ConvertBasicType(add_ID, IDInfos[add_ID]().GetILType(), IDInfos[operandID]().GetILType());

				++CurrentID;
				//ctx.FunctionBody << LR"(%)" << ctx.CurrentID << LR"( = OpFMul %)" << typeID << LR"( %)" << operandID << LR"( %)" << mul_ID << EndLine;
				CodeGen.OpFMul(CurrentID, typeID, operandID, mul_ID);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(CurrentID, typeIL, 0, typeID);
				++CurrentID;
				//ctx.FunctionBody << LR"(%)" << ctx.CurrentID << LR"( = OpFAdd %)" << typeID << LR"( %)" << ctx.CurrentID-1 << LR"( %)" << add_ID << EndLine;
				CodeGen.OpFAdd(CurrentID, typeID, CurrentID - 1, add_ID);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(CurrentID, typeIL, 0, typeID);
				return CurrentID;
			}

			int ConvertBasicType(int operandID, RefPtr<ILType> srcType, RefPtr<ILType> dstType)
			{
				String srcStr = srcType->ToString();
				String dstStr = dstType->ToString();
				if (srcStr == dstStr)
					return operandID;

				if (dstType->IsBool())
				{
					if (srcType->IsInt())
						return AddInstrINotEqual(operandID, AddInstrConstantInt(0));
					if (srcType->IsUInt())
						return AddInstrINotEqual(operandID, AddInstrConstantUInt(0));
					throw NotImplementedException("only convert int to bool in ConvertBasicType(): " + srcType->ToString());
				}

				//from column vector to column vector
				bool srcIsColumnVector = (srcType->IsFloat() || srcType->IsFloatVector() || srcType->IsIntegral()) && !srcType->IsFloatMatrix();
				bool dstIsColumnVector = (dstType->IsFloat() || dstType->IsFloatVector() || dstType->IsIntegral()) && !dstType->IsFloatMatrix();
				if (srcIsColumnVector && dstIsColumnVector && srcType->GetVectorSize() != dstType->GetVectorSize())
				{
					//make src ID have length equal to destType

					RefPtr<ILType> elementType;
					if (srcType->IsInt() || srcType->IsUInt())
						elementType = new ILBasicType(ILBaseType::Int);
					else if (srcType->IsUInt() || srcType->IsUIntVector())
						elementType = new ILBasicType(ILBaseType::UInt);
					else
						elementType = new ILBasicType(ILBaseType::Float);
					List<int> arguments;
					int extraID = -1;

					if (srcType->GetVectorSize() == 1)
					{
						for (int i = 0; i < dstType->GetVectorSize(); i++)
						{
							arguments.Add(operandID);
						}
					}
					else {
						for (int i = 0; i < dstType->GetVectorSize(); i++)
						{
							if (i >= srcType->GetVectorSize())
							{
								if (i == srcType->GetVectorSize())
								{
									if (srcType->IsInt() || srcType->IsIntVector())
										extraID = AddInstrConstantInt(0);
									else if (srcType->IsUInt() || srcType->IsUIntVector())
										extraID = AddInstrConstantUInt(0);
									else
										extraID = AddInstrConstantFloat(0.0);
								}
								arguments.Add(extraID);
							}
							else
							{
								if (srcType->GetVectorSize() == 1)
									arguments.Add(operandID);
								else
									arguments.Add(AddInstrCompositeExtract(operandID, elementType, i));
							}
						}
					}

					int BaseType = (dynamic_cast<ILBasicType*>(srcType.Ptr()))->Type;
					int newBaseType = BaseType & ~15; // int or float
					newBaseType += dstType->GetVectorSize() - 1;
					RefPtr<ILType> newSrcType = new ILBasicType(ILBaseType(newBaseType));

					operandID = AddInstrCompositeConstruct(0, newSrcType, arguments);
					srcType = newSrcType;
				}

				srcStr = srcType->ToString();
				dstStr = dstType->ToString();
				if (srcStr == dstStr)
					return operandID;

				//from scalar to matrix
				if ((srcStr == "float" || srcStr == "int" || srcStr == "uint") && dstType->IsFloatMatrix())
				{
					throw NotImplementedException("scalar to matrix conversion is not supported yet.");
				}

				//from matrix to matrix
				if (srcType->IsFloatMatrix() && dstType->IsFloatMatrix())
				{
					throw NotImplementedException("matrix to matrix conversion is not supported yet.");
				}

				if (srcType->GetVectorSize() != dstType->GetVectorSize())
				{
					throw NotImplementedException("can not convert " + srcType->ToString() + " to " + dstType->ToString());
				}

				//component-wise conversion

				bool srcFloat = srcType->IsFloat() || srcType->IsFloatVector();
				bool srcInt = srcType->IsInt() || srcType->IsIntVector();
				bool srcUint = srcType->IsUInt() || srcType->IsUIntVector();
				bool dstFloat = dstType->IsFloat() || dstType->IsFloatVector();
				bool dstInt = dstType->IsInt() || dstType->IsIntVector();
				bool dstUint = dstType->IsUInt() || dstType->IsUIntVector();

				int destTypeID = DefineType(dstType);
				if (srcInt && dstFloat)
					return AddInstrConvertSToF(destTypeID, operandID);
				else if (srcFloat && dstInt)
					return AddInstrConvertFToS(destTypeID, operandID);
				else if (srcUint && dstFloat)
					return AddInstrConvertUToF(destTypeID, operandID);
				else if (srcFloat && dstUint)
					return AddInstrConvertFToU(destTypeID, operandID);
				else if (srcInt && dstUint)
					return AddInstrConvertSToU(destTypeID, operandID);
				else if (srcUint && dstInt)
					return AddInstrConvertUToS(destTypeID, operandID);
				else
					throw NotImplementedException("can not convert " + srcType->ToString() + " to " + dstType->ToString());
			}

			void AddInstrSelectionMerge(int MergeLabel) {
				CodeGen.OpSelectionMerge(MergeLabel);
			}

			void AddInstrBranchConditional(int ID, int TrueLabel, int FalseLabel)
			{
				CodeGen.OpBranchConditional(ID, TrueLabel, FalseLabel);
			}

			void AddInstrLabel_AtFunctionBody(int Label) {
				CodeGen.OpLabel_AtFunctionBody(Label);
			}

			void AddInstrLabel_AtFunctionHeader(int Label)
			{
				CodeGen.OpLabel_AtFunctionHeader(Label);
			}

			void AddInstrBranch(int Target)
			{
				CodeGen.OpBranch(Target);;
			}

			int AddInstrPhi(ILOperand *op, int ID1, int Label1, int ID2, int Label2)
			{
				List<int> branches;
				branches.Add(ID1); branches.Add(Label1);
				branches.Add(ID2); branches.Add(Label2);

				++CurrentID;
				CodeGen.OpPhi(CurrentID, IDInfos[ID1]().GetTypeID(), branches);

				IDInfos[CurrentID] =
					IDInfo::CreateIDInfoForValue(
						CurrentID,
						IDInfos[ID1]().GetILType(),
						op,
						IDInfos[ID1]().GetTypeID()
					);
				UpdateValue(op, CurrentID);
				return CurrentID;
			}

			void AddInstrLoopMerge(int MergeLabel, int ContinueLabel)
			{
				CodeGen.OpLoopMerge(MergeLabel, ContinueLabel);
			}

			void AddInstrReturnValue(int operandID) 
			{
				CodeGen.OpReturnValue(operandID);
			}

			void AddInstrKill() 
			{
				CodeGen.OpKill();
			}

			void AddInstrDecorate(int ID, Decoration deco, int ID1 = 0)
			{
				CodeGen.OpDecorate(ID, deco, ID1);
			}

			void AddInstrMemberDecorate(int ID, int index, Decoration deco, int ID1 = 0)
			{
				CodeGen.OpMemberDecorate(ID, index, deco, ID1);
			}

			void AddInstrFunction(int funcID, int returnTypeID, int funcTypeID, String funcName) 
			{
				CodeGen.OpName(funcID, funcName);
				CodeGen.OpFunction(funcID, returnTypeID, funcTypeID);
			}

			void AddInstrFunctionParameter(ILOperand *op, int typeID, String DebugName)
			{
				int paramID = ++CurrentID;
				CodeGen.OpName(paramID, DebugName);
				CodeGen.OpFunctionParameter(paramID, typeID);
				IDInfos[paramID] = IDInfo::CreateIDInfoForPointer(
					paramID,
					op,
					typeID,
					IDInfos[typeID]().GetILType(),
					IDInfos[typeID]().GetBaseTypeID(),
					StorageClass::Function
				);
				ParameterNameToID[op] = paramID;
				UpdateVariable(op, paramID);
			}

			void AddInstrReturn()
			{
				CodeGen.OpReturn();
			}

			void AddInstrFunctionEnd()
			{
				CodeGen.OpFunctionEnd();
			}

			void AddInstrEntryPoint(ExecutionModel EM, int entryID, const List<int>& interfaceIDs)
			{
				CodeGen.OpEntryPoint(EM, entryID, interfaceIDs);
			}

			void AddInstrExecutionMode(int ID, ExecutionMode mode, int op1 = -1, int op2 = -1, int op3 = -1) 
			{
				CodeGen.OpExecutionMode(ID, mode, op1, op2, op3);
			}

			int AddInstrDot(ILOperand *op, RefPtr<ILType> typeIL, int ID0, int ID1)
			{
				int typeID = DefineType(typeIL);
				++CurrentID;
				CodeGen.OpDot(CurrentID, typeID, ID0, ID1);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					typeIL,
					op,
					typeID
				);
				UpdateValue(op, CurrentID);
				return CurrentID;
			}

			int AddInstrTranspose(ILOperand *op, RefPtr<ILType> typeIL, int ID)
			{
				int typeID = DefineType(typeIL);
				++CurrentID;
				CodeGen.OpTranspose(CurrentID, typeID, ID);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					typeIL,
					op,
					typeID
				);
				UpdateValue(op, CurrentID);
				return CurrentID;
			}

			int AddInstrDFdx(ILOperand *op, RefPtr<ILType> typeIL, int ID)
			{
				int typeID = DefineType(typeIL);
				++CurrentID;
				CodeGen.OpDPdx(CurrentID, typeID, ID);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					typeIL,
					op,
					typeID
				);
				UpdateValue(op, CurrentID);
				return CurrentID;
			}

			int AddInstrDFdy(ILOperand *op, RefPtr<ILType> typeIL, int ID)
			{
				int typeID = DefineType(typeIL);
				++CurrentID;
				CodeGen.OpDPdy(CurrentID, typeID, ID);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					typeIL,
					op,
					typeID
				);
				UpdateValue(op, CurrentID);
				return CurrentID;
			}

			int AddInstrFwidth(ILOperand *op, RefPtr<ILType> typeIL, int ID)
			{
				int typeID = DefineType(typeIL);
				++CurrentID;
				CodeGen.OpFwidth(CurrentID, typeID, ID);
				IDInfos[CurrentID] = IDInfo::CreateIDInfoForValue(
					CurrentID,
					typeIL,
					op,
					typeID
				);
				UpdateValue(op, CurrentID);
				return CurrentID;
			}

			void ProduceFunction()
			{
				CodeGen.ProduceFunction();
			}

			List<unsigned int> ProduceWordStream()
			{
				return CodeGen.ProduceWordStream(CurrentID);
			}

			String ProduceTextCode() 
			{
				return CodeGen.ProduceTextCode();
			}
		};



		class SpirvModule
		{
		private:
			SpirVCodeGenContext ctx;
			ExecutionModel currentExecutionModel;
			List<int> interfaceIDs;
			bool DepthReplacing;
			bool LocalSize;
			bool BufferImportOrExport;
			CompiledWorld * currentWorld;

			int GetOperandValue(ILOperand * op)
			{
				int id = -1;
				if (auto c = dynamic_cast<ILConstOperand*>(op))
				{
					auto type = c->Type.Ptr();
					if (type->IsFloat())
					{
						id = ctx.AddInstrConstantFloat(c->FloatValues[0]);
					}
					else if (type->IsInt())
					{
						id = ctx.AddInstrConstantInt(c->IntValues[0]);
					}
					else if (type->IsUInt())
					{
						id = ctx.AddInstrConstantUInt(*((unsigned int*)(&c->IntValues[0])));
					}
					else if (auto baseType = dynamic_cast<ILBasicType*>(type))
					{
						if (baseType->Type == ILBaseType::Float2)
						{
							id = ctx.AddInstrConstantCompositeFloat(c->FloatValues, 2);
						}
						else if (baseType->Type == ILBaseType::Float3)
						{
							id = ctx.AddInstrConstantCompositeFloat(c->FloatValues, 3);
						}
						else if (baseType->Type == ILBaseType::Float4)
						{
							id = ctx.AddInstrConstantCompositeFloat(c->FloatValues, 4);
						}
						else if (baseType->Type == ILBaseType::Float3x3)
						{
							id = ctx.AddInstrConstantMatrix(c->FloatValues, 3);
						}
						else if (baseType->Type == ILBaseType::Float4x4)
						{
							id = ctx.AddInstrConstantMatrix(c->FloatValues, 4);
						}
						else if (baseType->Type == ILBaseType::Int2)
						{
							id = ctx.AddInstrConstantCompositeInt(c->IntValues, 2);
						}
						else if (baseType->Type == ILBaseType::Int3)
						{
							id = ctx.AddInstrConstantCompositeInt(c->IntValues, 3);
						}
						else if (baseType->Type == ILBaseType::Int4)
						{
							id = ctx.AddInstrConstantCompositeInt(c->IntValues, 4);
						}
						else if (baseType->Type == ILBaseType::UInt2)
						{
							id = ctx.AddInstrConstantCompositeUInt((unsigned int*)c->IntValues, 2);
						}
						else if (baseType->Type == ILBaseType::UInt3)
						{
							id = ctx.AddInstrConstantCompositeUInt((unsigned int*)c->IntValues, 3);
						}
						else if (baseType->Type == ILBaseType::UInt4)
						{
							id = ctx.AddInstrConstantCompositeUInt((unsigned int*)c->IntValues, 4);
						}
						else if (baseType->Type == ILBaseType::Bool)
						{
							id = ctx.AddInstrConstantBool(c->IntValues[0]);
						}
					}
					else
						throw InvalidOperationException("Illegal constant.");
				}
				else if (auto instr = dynamic_cast<ILInstruction*>(op))
				{
					id = ctx.FindValueID(op);
					if (id == -1)
					{
						//need to load it from storage
						id = ctx.AddInstrLoad(op, op, MemoryAccess::None);
					}
				}
				else
					throw InvalidOperationException("Unsupported operand type.");

				return id;
			}

			int GetOperandPointer(ILOperand * op)
			{
				int id = -1;
				//RefPtr<ILType> result_type;
				if (auto c = dynamic_cast<ILConstOperand*>(op))
				{
					int valueID = GetOperandValue(op);
					id = ctx.AddInstrVariableDeclaration(op, op->Type, StorageClass::Function);
					ctx.AddInstrStore(op, id, valueID);
				}
				else if (auto instr = dynamic_cast<ILInstruction*>(op))
				{
					id = ctx.FindVariableID(op);
					if (id == -1)
					{
						int valueID = ctx.FindValueID(op);
						if (valueID == -1)
							throw InvalidOperationException("can not find variable ID in Get OperandPointer(): " + op->ToString());
						id = ctx.AddInstrVariableDeclaration(op, instr->Type, StorageClass::Function);
						ctx.AddInstrStore(op, id, valueID);
					}
				}
				else
					throw InvalidOperationException("Unsupported operand type.");

				return id;
			}

			void PrintAllocVarInstr(AllocVarInstruction * instr, StorageClass store)
			{
				if (dynamic_cast<ILConstOperand*>(instr->Size.Ptr()))
				{
					ctx.AddInstrVariableDeclaration((ILOperand*)instr, instr->Type, store);
				}
				else
					throw InvalidProgramException("size operand of allocVar instr is not an intermediate.");
			}

			Dictionary<String, int> GLSLstd450InstructionSet = GenGLSLstd450InstructionSet();

			void PrintCallInstr(CallInstruction * instr)
				// return ID of this instruction
			{
				String callName = GetFuncOriginalName(instr->Function);

				//------------------------- texture instructions -------------------------
				if (callName == "texture")
				{
					if (instr->Arguments[0]->Type->IsNonShadowTexture())
					{
						if (instr->Arguments[0]->Type->ToString() == "sampler2D")
						{
							//*** no bias!!!
							//__intrinsic vec4 texture(sampler2D tex, vec2 coord);
							ctx.AddInstrTexture(
								(ILOperand*)instr,
								GetOperandValue(instr->Arguments[0].Ptr()),
								GetOperandValue(instr->Arguments[1].Ptr()),
								currentExecutionModel
							);
							return;
						}
						else if (instr->Arguments[0]->Type->ToString() == "samplerCube")
						{
							if (instr->Arguments.Count() == 2)
							{
								//__intrinsic vec4 texture(samplerCube tex, vec3 coord);
								ctx.AddInstrTexture(
									(ILOperand*)instr,
									GetOperandValue(instr->Arguments[0].Ptr()),
									GetOperandValue(instr->Arguments[1].Ptr()),
									currentExecutionModel
								);
								return;
							}
							else
							{
								//__intrinsic vec4 texture(samplerCube tex, vec3 coord, float bias);
								ctx.AddInstrTexture(
									(ILOperand*)instr,
									GetOperandValue(instr->Arguments[0].Ptr()),
									GetOperandValue(instr->Arguments[1].Ptr()),
									currentExecutionModel,
									GetOperandValue(instr->Arguments[2].Ptr())
								);
								return;
							}
						}
					}
					else
					{
						//instr->Arguments[0]->Type->IsShadowTexture

						//__intrinsic float texture(sampler2DShadow tex, vec3 coord);
						//__intrinsic float texture(samplerCubeShadow tex, vec4 coord);
						ctx.AddInstrTextureShadow(
							(ILOperand*)instr,
							GetOperandValue(instr->Arguments[0].Ptr()),
							GetOperandValue(instr->Arguments[1].Ptr()),
							currentExecutionModel
						);
						return;
					}
				}

				if (callName == "textureGrad")
				{
					//__intrinsic vec4 textureGrad(sampler2D tex, vec2 coord, vec2 dPdx, vec2 dPdy);
					//__intrinsic vec4 textureGrad(samplerCube tex, vec3 coord, vec3 dPdx, vec3 dPdy);
					ctx.AddInstrTexture(
						(ILOperand*)instr,
						GetOperandValue(instr->Arguments[0].Ptr()),
						GetOperandValue(instr->Arguments[1].Ptr()),
						currentExecutionModel,
						-1,	//Bias
						GetOperandValue(instr->Arguments[2].Ptr()),
						GetOperandValue(instr->Arguments[3].Ptr())
					);
					return;
				}

				if (callName == "textureProj")
				{
					if (instr->Arguments[0]->Type->ToString() == "sampler2DShadow")
					{
						//__intrinsic float textureProj(sampler2DShadow tex, vec4 coord);
						ctx.AddInstrTexture2DShadowProj(
							(ILOperand*)instr,
							GetOperandValue(instr->Arguments[0].Ptr()),
							GetOperandValue(instr->Arguments[1].Ptr()),
							currentExecutionModel
						);
						return;
					}
				}

				//------------------------- Dot Instruction ------------------------------
				if (callName == "dot"
					&& instr->Arguments.Count() == 2
					&& instr->Arguments[0]->Type->ToString() == instr->Arguments[1]->Type->ToString()
					&& instr->Arguments[0]->Type->IsFloatVector()
					&& !instr->Arguments[0]->Type->IsFloatMatrix())
				{
					ctx.AddInstrDot(
						(ILOperand*)instr,
						instr->Type,
						GetOperandValue(instr->Arguments[0].Ptr()),
						GetOperandValue(instr->Arguments[1].Ptr())
					);
					return;
				}

				//------------------------- Transpose Instruction ------------------------------
				if (callName == "transpose" && instr->Arguments.Count() == 1 && instr->Arguments[0]->Type->IsFloatMatrix())
				{
					ctx.AddInstrTranspose((ILOperand*)instr, instr->Type, GetOperandValue(instr->Arguments[0].Ptr()));
					return;
				}

				//------------------------- Derivative Instruction -----------------------------
				if (callName == "dFdx")
				{
					ctx.AddInstrDFdx((ILOperand*)instr, instr->Type, GetOperandValue(instr->Arguments[0].Ptr()));
					return;
				}
				else if (callName == "dFdy")
				{
					ctx.AddInstrDFdy((ILOperand*)instr, instr->Type, GetOperandValue(instr->Arguments[0].Ptr()));
					return;
				}
				else if (callName == "fwidth")
				{
					ctx.AddInstrFwidth((ILOperand*)instr, instr->Type, GetOperandValue(instr->Arguments[0].Ptr()));
					return;
				}

				//------------------------- user-defined  instructions -------------------------
				int funcID;
				if (ctx.FunctionNameToFunctionID.TryGetValue(instr->Function, funcID))
				{
					RefPtr<ILType> returnType = ctx.IDInfos[
						ctx.FunctionNameToFunctionTypeID[instr->Function]()
					]().GetFunc()->ReturnType;
						int typeID = ctx.DefineType(returnType);
						List<int> args;
						for (auto & arg : instr->Arguments) {
							int valueID = GetOperandValue(arg.Ptr());
							int paramID = ctx.AddInstrVariableDeclaration(0, arg->Type, StorageClass::Function, "param");
							// the name of the parameter must be empty; or may conflict with non-param variables
							ctx.AddInstrStore(0, paramID, valueID);
							args.Add(paramID);
						}
						ctx.AddInstrFunctionCall((ILOperand*)instr, typeID, funcID, args);
						return;
				}

				//------------------------- ext-import  instructions -----------------------------
				List<int> Arguments;

				for (auto & arg : instr->Arguments) {
					int valueID = GetOperandValue(arg.Ptr());
					if (callName == "mix") {
						//the mix instruction in spirv only accept mix(vec_, vec_, vec_);
						//however, front end of SPIRE can accept mix(vec_, vec_, float);
						valueID = ctx.ConvertBasicType(valueID, arg->Type, instr->Type);
					}
					Arguments.Add(valueID);
				}

				if (GLSLstd450InstructionSet.ContainsKey(callName))
				{
					ctx.AddInstrExtInst((ILOperand*)instr, instr->Type, GLSLstd450InstructionSet[callName](), Arguments);
					return;
				}


				//------------------------- built-in constructors -----------------------------

				RefPtr<ILType> dstType = GetTypeFromString(callName);
				if (dstType == nullptr)
					throw InvalidOperationException("can not call: " + callName);
				RefPtr<ILBasicType> dstBasicType = dstType;

				if (instr->Arguments.Count() > 1)
				{
					//composite
					for (auto & ID : Arguments)
					{
						auto argBasicType = dynamic_cast<ILBasicType*>((ctx.IDInfos[ID]().GetILType()).Ptr());
						if (argBasicType)
							if (argBasicType->IsIntegral() != dstType->IsIntegral())
							{
								RefPtr<ILType> argDstType = new ILBasicType(ILBaseType((dstBasicType->Type & ~15) + (argBasicType->Type & 15)));
								ID = ctx.ConvertBasicType(ID, ctx.IDInfos[ID]().GetILType(), argDstType);
							}
					}

					if (instr->Type->IsFloatMatrix() &&
						(Arguments.Count() == 0 || (Arguments.Count() > 0 && ctx.IDInfos[Arguments[0]]().GetILType()->IsScalar())))
					{
						//need to arrange scalars into vectors
						int n = (int)sqrt((float)instr->Type->GetVectorSize() + 1e-6);
						int diff = n * n - Arguments.Count();
						if (diff > 0)
							for (int i = 0; i < diff; i++)
								Arguments.Add(ctx.AddInstrConstantFloat(0.0));
						List<int> newArguments;
						RefPtr<ILType> vectorType = new ILBasicType(ILBaseType(ILBaseType::Float + n - 1));
						for (int i = 0; i < n; i++)
						{
							List<int> subArguments;
							for (int j = 0; j < n; j++)
								subArguments.Add(Arguments[i*n + j]);
							newArguments.Add(ctx.AddInstrCompositeConstruct(0, vectorType, subArguments));
						}
						Arguments = newArguments;
					}

					ctx.AddInstrCompositeConstruct((ILOperand*)instr, instr->Type, Arguments);
					return;
				}
				else
				{
					//need conversion
					int ID = Arguments[0];
					ID = ctx.ConvertBasicType(ID, ctx.IDInfos[ID]().GetILType(), dstType);
					ctx.UpdateValue((ILOperand*)instr, ID);
					return;
				}

			}

			void PrintUnaryInstr(UnaryInstruction * instr)
			{
				auto op0 = instr->Operand.Ptr();
				if (instr->Is<LoadInstruction>())
				{
					ctx.AddInstrLoad((ILOperand*)instr, op0, MemoryAccess::None);
					return;
				}

				if (instr->Is<NotInstruction>())
					instr->Type = GetTypeFromString("bool");

				int op0ValueID = GetOperandValue(op0);
				op0ValueID = ctx.ConvertBasicType(op0ValueID, ctx.IDInfos[op0ValueID]().GetILType(), instr->Type);
				RefPtr<ILType> op0ILType = ctx.IDInfos[op0ValueID]().GetILType();

				if (instr->Is<Float2IntInstruction>() || instr->Is<Int2FloatInstruction>() || instr->Is<CopyInstruction>())
				{
					ctx.UpdateValue((ILOperand*)instr, op0ValueID);
					return;
				}

				int instrTypeID = ctx.DefineType(instr->Type);

				if (instr->Is<NegInstruction>())
				{
					if (op0ILType->IsFloat() || op0ILType->IsFloatVector())
						ctx.AddInstrFnegate((ILOperand*)instr, instrTypeID, op0ValueID);
					else if (op0ILType->IsInt() || op0ILType->IsIntVector())
						ctx.AddInstrSnegate((ILOperand*)instr, instrTypeID, op0ValueID);
					else if (op0ILType->IsUInt() || op0ILType->IsUIntVector())
						throw InvalidOperationException("trying to negate a uint in PrintUnaryInstruction(): " + instr->ToString());
				}
				else if (instr->Is<BitNotInstruction>())
					ctx.AddInstrNot((ILOperand*)instr, instrTypeID, op0ValueID);
				else if (instr->Is<NotInstruction>())
					ctx.AddInstrLogicalNot((ILOperand*)instr, instrTypeID, op0ValueID);
				else
					throw InvalidProgramException("unsupported unary instruction.");
			}

			void PrintBinaryInstr(BinaryInstruction * instr)
			{
				auto op0 = instr->Operands[0].Ptr();
				auto op1 = instr->Operands[1].Ptr();

				//-------------------------------------Store Instruction------------------------------------------
				if (instr->Is<StoreInstruction>())
				{
					if (auto structType = dynamic_cast<ILStructType*>(op0->Type.Ptr()))
					{
						int op0ID = ctx.FindVariableID(op0);
						int op1ID = ctx.FindVariableID(op1);
						int index = 0;
						for (int i = 0; i < structType->Members.Count(); i++)
						{
							int indexID = ctx.AddInstrConstantInt(index);
							int dest = ctx.AddInstrAccessChain_StructMember(0, op0ID, indexID, index);
							int pSrc = ctx.AddInstrAccessChain_StructMember(0, op1ID, indexID, index);
							int vSrc = ctx.AddInstrLoad(pSrc, MemoryAccess::None);
							ctx.AddInstrStore(0, dest, vSrc);
							index++;
						}
						return;
					}

					int op0ID = ctx.FindVariableID(op0); // should be a pointer 
					int op1ID = GetOperandValue(op1);
					op1ID = ctx.ConvertBasicType(op1ID, ctx.IDInfos[op1ID]().GetILType(), ctx.IDInfos[op0ID]().GetILType());
					ctx.AddInstrStore(instr, op0ID, op1ID); //TO FIX
					return;
				}

				//----------------------------------Member Load Instruction---------------------------------------
				if (instr->Is<MemberLoadInstruction>())
				{
					int fatherID = GetOperandPointer(op0);
					if (op0->Type->IsVector())
					{
						if (auto c = dynamic_cast<ILConstOperand*>(op1))
						{
							//if op1 is constant, take that as index of vector 
							int memberID = ctx.AddInstrAccessChain_VectorMember((ILOperand*)instr, fatherID, -1, c->IntValues[0]);
							int retID = ctx.AddInstrLoad(memberID, MemoryAccess::None);
							ctx.UpdateValue((ILOperand*)instr, retID);
							return;
						}
						else
						{
							//if op1 is not constant, compute it
							int memberID = ctx.AddInstrAccessChain_VectorMember((ILOperand*)instr, fatherID, GetOperandValue(op1), -1);
							int retID = ctx.AddInstrLoad(memberID, MemoryAccess::None);
							ctx.UpdateValue((ILOperand*)instr, retID);
							return;
						}
					}
					else if (auto structType = dynamic_cast<ILStructType*>(op0->Type.Ptr()))
					{
						if (auto c = dynamic_cast<ILConstOperand*>(op1))
						{
							//index of struct must be constant
							int indexID = GetOperandValue(c);
							int memberID = ctx.AddInstrAccessChain_StructMember((ILOperand*)instr, fatherID, indexID, c->IntValues[0]);
							int retID = ctx.AddInstrLoad(memberID, MemoryAccess::None);
							ctx.UpdateValue((ILOperand*)instr, retID);
							return;
						}
						else
							throw InvalidOperationException("wrong: " + instr->ToString());
					}
					else if (auto arrayType = dynamic_cast<ILArrayType*>(op0->Type.Ptr()))
					{
						int memberID = ctx.AddInstrAccessChain_ArrayMember((ILOperand*)instr, op0->Type, fatherID, GetOperandValue(op1));
						int retID = ctx.AddInstrLoad(memberID, MemoryAccess::None);
						ctx.UpdateValue((ILOperand*)instr, retID);
						return;
					}
					else
						throw InvalidOperationException("wrong op0 type for MemberLoadInstruction(): " + op0->Type->ToString());
				}

				int ID0 = GetOperandValue(op0);
				int ID1 = GetOperandValue(op1);
				RefPtr<ILType> ID0Type = ctx.IDInfos[ID0]().GetILType();
				RefPtr<ILType> ID1Type = ctx.IDInfos[ID1]().GetILType();

				//----------------------------------Vec/Mat Multiplication---------------------------------------
				if (instr->Is<MulInstruction>())
				{
					//scalar X matrix or matrix X scalar
					if (ID0Type->IsScalar() && ID1Type->IsFloatMatrix() ||
						ID1Type->IsScalar() && ID0Type->IsFloatMatrix())
					{
						if (ID1Type->IsFloatMatrix())
							Swap(ID0, ID1);
						//now ID0 is matrix, ID1 is scalar, 
						ID1 = ctx.ConvertBasicType(ID1, ctx.IDInfos[ID1]().GetILType(), GetTypeFromString("float"));
						ctx.AddInstrMatrixTimesScalar((ILOperand*)instr, ID0, ID1);
						return;
					}

					//vector X matrix
					if (ID0Type->IsFloatVector() && !ID0Type->IsFloatMatrix() && ID1Type->IsFloatMatrix())
					{
						ctx.AddInstrVectorTimesMatrix((ILOperand*)instr, ID0, ID1);
						return;
					}

					//matrix X vector
					if (ID1Type->IsFloatVector() && !ID1Type->IsFloatMatrix() && ID0Type->IsFloatMatrix())
					{
						ctx.AddInstrMatrixTimesVector((ILOperand*)instr, ID0, ID1);
						return;
					}

					//matrix X matrix
					if (ID0Type->IsFloatMatrix() && ID1Type->IsFloatMatrix())
					{
						ctx.AddInstrMatrixTimesMatrix((ILOperand*)instr, ID0, ID1);
						return;
					}
				}

				//---------------------------------Boolean-Related Instruction---------------------------------------
				bool ResultIsLogical =
					instr->Is<OrInstruction>() || instr->Is<AndInstruction>() ||
					instr->Is<CmpeqlInstruction>() || instr->Is<CmpgeInstruction>() || instr->Is<CmpgtInstruction>() ||
					instr->Is<CmpleInstruction>() || instr->Is<CmpltInstruction>() || instr->Is<CmpneqInstruction>();

				if (dynamic_cast<ILBasicType*>(instr->Type.Ptr())->Type == 0)
				{
					instr->Type = ID0Type;
				}

				if (ResultIsLogical)
				{
					RefPtr<ILType> OperandType = nullptr;
					if (instr->Is<OrInstruction>() || instr->Is<AndInstruction>())
					{
						OperandType = GetTypeFromString("bool");
					}
					else
					{
						if (ID0Type->IsFloat() || ID1Type->IsFloat())
							OperandType = GetTypeFromString("float");
						else if (ID0Type->IsUInt() || ID1Type->IsUInt())
							OperandType = GetTypeFromString("uint");
						else if (ID0Type->IsInt() || ID1Type->IsInt())
							OperandType = GetTypeFromString("int");
					}

					ID0 = ctx.ConvertBasicType(ID0, ID0Type, OperandType);
					ID1 = ctx.ConvertBasicType(ID1, ID1Type, OperandType);

					instr->Type = GetTypeFromString("bool");
					ID0Type = ctx.IDInfos[ID0]().GetILType();
					ID1Type = ctx.IDInfos[ID1]().GetILType();
				}
				else
				{
					ID0 = ctx.ConvertBasicType(ID0, ID0Type, instr->Type);
					ID1 = ctx.ConvertBasicType(ID1, ID1Type, instr->Type);

					ID0Type = ctx.IDInfos[ID0]().GetILType();
					ID1Type = ctx.IDInfos[ID1]().GetILType();
				}

				//--------------------------------------Get Binary Operator String---------------------------------------
				String opStr;
				bool needPrefix = false;
				bool Signed = false;
				if (instr->Is<MulInstruction>())
				{
					opStr = "Mu";
					needPrefix = true;
				}
				else if (instr->Is<AddInstruction>())
				{
					opStr = "Add";
					needPrefix = true;
				}
				else if (instr->Is<DivInstruction>())
				{
					opStr = "Div";
					needPrefix = true;
					Signed = true;
				}
				else if (instr->Is<SubInstruction>())
				{
					opStr = "Sub";
					needPrefix = true;
				}
				else if (instr->Is<ModInstruction>())
				{
					opStr = "Mod";
					needPrefix = true;
					Signed = true;
				}
				else if (instr->Is<ShlInstruction>())
				{
					opStr = "ShiftLeftLogica";
				}
				else if (instr->Is<ShrInstruction>())
				{
					if (ID0Type->IsUInt() || ID0Type->IsUIntVector())
						opStr = "ShiftRightLogica";
					else
						opStr = "ShiftRightArithmetic";
				}
				else if (instr->Is<BitXorInstruction>())
				{
					opStr = "BitwiseXor";
				}
				else if (instr->Is<BitAndInstruction>())
				{
					opStr = "BitwiseAnd";
				}
				else if (instr->Is<BitOrInstruction>())
				{
					opStr = "BitwiseOr";
				}
				else if (instr->Is<AndInstruction>())
				{
					opStr = "LogicalAnd";
				}
				else if (instr->Is<OrInstruction>())
				{
					opStr = "LogicalOr";
				}
				else if (instr->Is<CmpneqInstruction>())
				{
					if (ID0Type->IsIntegral())
						opStr = "INotEqua";
					else
						opStr = "FOrdNotEqua";
				}
				else if (instr->Is<CmpeqlInstruction>())
				{
					if (ID0Type->IsIntegral())
						opStr = "IEqua";
					else
						opStr = "FOrdEqua";
				}
				else if (instr->Is<CmpgeInstruction>())
				{
					if (ID0Type->IsIntegral())
					{
						if (ID0Type->IsUInt() || ID0Type->IsUIntVector())
							opStr = "UGreaterThanEqua";
						else
							opStr = "SGreaterThanEqua";
					}
					else
						opStr = "FOrdGreaterThanEqua";
				}
				else if (instr->Is<CmpgtInstruction>())
				{
					if (ID0Type->IsIntegral())
					{
						if (ID0Type->IsUInt() || ID0Type->IsUIntVector())
							opStr = "UGreaterThan";
						else
							opStr = "SGreaterThan";
					}
					else
						opStr = "FOrdGreaterThan";
				}
				else if (instr->Is<CmpleInstruction>())
				{
					if (ID0Type->IsIntegral())
					{
						if (ID0Type->IsUInt() || ID0Type->IsUIntVector())
							opStr = "ULessThanEqua";
						else
							opStr = "SLessThanEqua";
					}
					else
						opStr = "FOrdLessThanEqua";
				}
				else if (instr->Is<CmpltInstruction>())
				{
					if (ID0Type->IsIntegral())
					{
						if (ID0Type->IsUInt() || ID0Type->IsUIntVector())
							opStr = "ULessThan";
						else
							opStr = "SLessThan";
					}
					else
						opStr = "FOrdLessThan";
				}
				else
					throw InvalidProgramException("unsupported binary instruction: " + instr->ToString());

				//---------------------------------------Generate Instrction---------------------------------------

				String finalOpStr = LR"(Op)";
				if (needPrefix)
				{
					if (ID0Type->IsFloat() || ID0Type->IsFloatVector())
						finalOpStr = finalOpStr + LR"(F)";
					else if (ID0Type->IsIntegral())
					{
						if (Signed)
							finalOpStr = finalOpStr + LR"(S)";
						else
							finalOpStr = finalOpStr + LR"(I)";
					}
				}
				finalOpStr = finalOpStr + opStr;

				ctx.AddInstrBinaryInstr((ILOperand*)instr, instr->Type, finalOpStr, ID0, ID1);
			}

			void PrintUpdateInstr(MemberUpdateInstruction * instr)
			{
				int variableID = ctx.AddInstrVariableDeclaration((ILOperand*)instr, instr->Operands[0]->Type, StorageClass::Function);
				ctx.AddInstrStore((ILOperand*)instr, variableID, GetOperandValue(instr->Operands[0].Ptr()));

				auto typeIL = ctx.IDInfos[variableID]().GetILType().Ptr();
				int memberID = -1;
				int indexID = GetOperandValue(instr->Operands[1].Ptr());

				if (indexID == -1)
					throw InvalidOperationException("bad index in PrintUpdateInstr(): " + instr->Operands[1]->ToString());

				if (auto structType = dynamic_cast<ILStructType*>(typeIL))
				{
					auto c = dynamic_cast<ILConstOperand*>(instr->Operands[1].Ptr());
					if (c)
						memberID = ctx.AddInstrAccessChain_StructMember(0, variableID, indexID, c->IntValues[0]);
					else
						throw InvalidOperationException("index of struct must be const in PrintUpdateInstr(): " + instr->Operands[1]->ToString());
				}
				else if (auto arrayType = dynamic_cast<ILArrayType*>(typeIL))
				{
					memberID = ctx.AddInstrAccessChain_ArrayMember(0, typeIL, variableID, indexID);
				}
				else if (auto vecType = dynamic_cast<ILBasicType*>(typeIL))
				{
					if (!typeIL->IsVector())
						throw InvalidOperationException("unable to update members of type: " + typeIL->ToString());
					memberID = ctx.AddInstrAccessChain_VectorMember(0, variableID, indexID, -1);
				}
				else
					throw InvalidOperationException("not supported type in PrintUpdateInstr(): " + typeIL->ToString());

				ctx.AddInstrStore(
					0,
					memberID,
					GetOperandValue(instr->Operands[2].Ptr())
				);
				ctx.InvalidateValue((ILOperand*)instr);
			}

			void PrintSelectInstr(SelectInstruction * instr)
			{
				int ID0 = GetOperandValue(instr->Operands[0].Ptr());
				ID0 = ctx.ConvertBasicType(
					ID0,
					ctx.IDInfos[ID0]().GetILType(),
					GetTypeFromString("bool"));

				int TrueLabel = ++ctx.CurrentID;
				int FalseLabel = ++ctx.CurrentID;
				int MergeLabel = ++ctx.CurrentID;

				ctx.AddInstrSelectionMerge(MergeLabel);
				ctx.AddInstrBranchConditional(ID0, TrueLabel, FalseLabel);

				ctx.AddInstrLabel_AtFunctionBody(TrueLabel);
				int ID1 = GetOperandValue(instr->Operands[1].Ptr());
				ID1 = ctx.ConvertBasicType(ID1, ctx.IDInfos[ID1]().GetILType(), instr->Type);
				ctx.AddInstrBranch(MergeLabel);

				ctx.AddInstrLabel_AtFunctionBody(FalseLabel);
				int ID2 = GetOperandValue(instr->Operands[2].Ptr());
				ID2 = ctx.ConvertBasicType(ID2, ctx.IDInfos[ID2]().GetILType(), instr->Type);
				ctx.AddInstrBranch(MergeLabel);

				ctx.AddInstrLabel_AtFunctionBody(MergeLabel);

				ctx.AddInstrPhi((ILOperand*)instr, ID1, TrueLabel, ID2, FalseLabel);
			}

			void PrintFetchArgInstr(FetchArgInstruction * instr)
			{
				if (instr->ArgId == 0)
				{
					ctx.ReturnID = ctx.AddInstrVariableDeclaration((ILOperand*)instr, instr->Type, StorageClass::Function);
				}
			}

			void PrintExportInstr(ExportInstruction * instr)
			{
				String exportOpName = instr->ExportOperator;

				if (exportOpName == "fragmentExport")
				{
					CompiledComponent ccomp;
					bool isNormal = false;
					bool isDepthOutput = false;
					if (currentWorld->LocalComponents.TryGetValue(instr->ComponentName, ccomp))
					{
						if (ccomp.Attributes.ContainsKey("Norma"))
							isNormal = true;
						if (ccomp.Attributes.ContainsKey("DepthOutput"))
							isDepthOutput = true;
					}
					String exportName;
					if (isDepthOutput)
						exportName = "gl_FragDepth";
					else
						exportName = instr->ComponentName;

					int exportID = ctx.InterfaceNameToID[exportName];
					if (exportID == -1)
						throw InvalidOperationException("can not find component for export instruction for fragmentExport in PrintExportInstr(): " + exportName);

					int operandID = GetOperandValue(instr->Operand.Ptr());
					if (isNormal)
						operandID = ctx.AddInstrMulAdd(operandID, 0.5, 0.5);

					ctx.AddInstrStore((ILOperand*)instr, exportID, operandID);
				}

				else if (exportOpName == "standardExport")
				{
					int storeID = ctx.AddInstrAccessChain_StructMember(0, ctx.InterfaceNameToID["blk" + currentWorld->WorldOutput->Name], instr->ComponentName);
					ctx.AddInstrStore((ILOperand*)instr, storeID, GetOperandValue(instr->Operand.Ptr()));
				}

				else if (exportOpName == "bufferExport")
				{
					auto & comp = currentWorld->WorldOutput->Entries[instr->ComponentName]();

					auto UIntType = GetTypeFromString("uint");
					auto FloatType = GetTypeFromString("float");
					int GIIDx = ctx.AddInstrLoad(
						ctx.AddInstrAccessChain_VectorMember(0, ctx.InterfaceNameToID["gl_GlobalInvocationID"], -1, 0),
						MemoryAccess::None
					);	//GlobalInvocationID.x
					int baseIndex = ctx.AddInstrBinaryInstr(
						0,
						UIntType,
						"OpIMu",
						GIIDx,
						ctx.AddInstrConstantUInt(currentWorld->WorldOutput->Size / 4)
					);
					baseIndex = ctx.AddInstrBinaryInstr(
						0,
						UIntType,
						"OpIAdd",
						baseIndex,
						ctx.AddInstrConstantUInt(comp.Offset / 4)
					);

					RefPtr<ILType> compElementType =
						(!comp.Type->IsIntegral())
						? FloatType
						: ((comp.Type->IsUInt() || comp.Type->IsUIntVector()) ? UIntType : GetTypeFromString("int"));

					for (int i = 0; i < comp.Type->GetVectorSize(); i++)
					{
						int index = ctx.AddInstrBinaryInstr(
							0,
							UIntType,
							"OpIAdd",
							baseIndex,
							ctx.AddInstrConstantUInt(i)
						);

						int blockID = ctx.InterfaceNameToID["blk" + currentWorld->WorldOutput->Name];
						int arrayID = ctx.AddInstrAccessChain_StructMember(0, blockID, -1, 0);
						int storeID = ctx.AddInstrAccessChain_ArrayMember(
							0,
							ctx.IDInfos[arrayID]().GetILType(),
							arrayID,
							index
						);

						int valueID = -1;
						if (instr->Operand->Type->GetVectorSize() > 1)
							valueID = ctx.AddInstrLoad(
								ctx.AddInstrAccessChain_VectorMember(0, GetOperandPointer(instr->Operand.Ptr()), -1, i),
								MemoryAccess::None
							);
						else
							valueID = ctx.AddInstrLoad(
								GetOperandPointer(instr->Operand.Ptr()),
								MemoryAccess::None
							);

						valueID = ctx.ConvertBasicType(valueID, ctx.IDInfos[valueID]().GetILType(), compElementType);

						if (ctx.IDInfos[valueID]().GetILType()->IsIntegral())
							valueID = ctx.AddInstrBitcast(ctx.DefineType(FloatType), valueID);

						ctx.AddInstrStore(0, storeID, valueID);
					}
				}

				else
					throw InvalidOperationException("not valid export operator in PrintExportInstr(): " + exportOpName);
			}

			void PrintImportInstr(ImportInstruction * instr)
			{
				auto block = instr->SourceWorld->WorldOutput;

				if (instr->ImportOperator->Name.Content == "standardImport")
				{
					ctx.AddInstrAccessChain_StructMember(instr, ctx.InterfaceNameToID["blk" + block->Name], instr->ComponentName);
				}

				else if (instr->ImportOperator->Name.Content == "vertexImport")
				{
					int componentID = ctx.InterfaceNameToID[instr->ComponentName];
					if (componentID == -1)
						throw InvalidOperationException("can not find import component for vertexImport in PrintImportInstr(): " + instr->ComponentName);
					ctx.UpdateVariable(instr, componentID);
				}

				else if (instr->ImportOperator->Name.Content == "uniformImport")
				{
					if (instr->Type->IsTexture())
					{
						int pointerID = ctx.InterfaceNameToID[instr->ComponentName];
						if (pointerID == -1)
							throw InvalidOperationException("can not find import component for uniformImport in PrintImportInstr(): " + instr->ComponentName);
						ctx.UpdateVariable(instr, pointerID);
					}
					else
					{
						ctx.AddInstrAccessChain_StructMember(instr, ctx.InterfaceNameToID["blk" + block->Name], instr->ComponentName);
					}
				}

				else if (instr->ImportOperator->Name.Content == "textureImport")
				{
					int textureStorageID = ctx.InterfaceNameToID[instr->ComponentName];
					int textureValueID = ctx.AddInstrLoad(textureStorageID, MemoryAccess::None);

					int operandID = -1;
					operandID = ctx.AddInstrTexture(
						0,
						textureValueID,
						GetOperandValue(instr->Arguments[0].Ptr()),
						currentExecutionModel
					);

					operandID = ctx.ConvertBasicType(
						operandID,
						ctx.IDInfos[operandID]().GetILType(),
						instr->Type);
					CompiledComponent ccomp;
					if (instr->SourceWorld->LocalComponents.TryGetValue(instr->ComponentName, ccomp))
					{
						if (ccomp.Attributes.ContainsKey("Norma"))
							operandID = ctx.AddInstrMulAdd(operandID, 2.0, -1.0);
					}

					int storeID = ctx.AddInstrVariableDeclaration((ILOperand*)instr, instr->Type, StorageClass::Function);
					ctx.AddInstrStore((ILOperand*)instr, storeID, operandID);
				}

				else if (instr->ImportOperator->Name.Content == "bufferImport")
				{
					//instr->Name[][] = 
					//	*(int*/float*) 
					//	( block->Name + block->Entries[instr->ComponentName].GetValue().Offset / 4 + i + gl_GlobalInvocationID.x * block->Size / 4 )

					auto UIntType = GetTypeFromString("uint");
					auto FloatType = GetTypeFromString("float");
					int GIIDx = ctx.AddInstrLoad(
						ctx.AddInstrAccessChain_VectorMember(0, ctx.InterfaceNameToID["gl_GlobalInvocationID"], -1, 0),
						MemoryAccess::None
					);	//GlobalInvocationID.x
					int baseIndex = ctx.AddInstrBinaryInstr(
						0,
						UIntType,
						"OpIMu",
						GIIDx,
						ctx.AddInstrConstantUInt(block->Size / 4)
					);
					baseIndex = ctx.AddInstrBinaryInstr(
						0,
						UIntType,
						"OpIAdd",
						baseIndex,
						ctx.AddInstrConstantUInt(block->Entries[instr->ComponentName]().Offset / 4)
					);

					RefPtr<ILType> instrElementType =
						(!instr->Type->IsIntegral())
						? FloatType
						: ((instr->Type->IsUInt() || instr->Type->IsUIntVector()) ? UIntType : GetTypeFromString("int"));

					int vecSize = instr->Type->GetVectorSize();
					int srcIDs[16];
					for (int i = 0; i < vecSize; i++)
					{
						int index = ctx.AddInstrBinaryInstr(
							0,
							UIntType,
							"OpIAdd",
							baseIndex,
							ctx.AddInstrConstantUInt(i)
						);
						int blockID = ctx.InterfaceNameToID["blk" + block->Name];
						int arrayID = ctx.AddInstrAccessChain_StructMember(0, blockID, -1, 0);
						int srcID = ctx.AddInstrLoad(
							ctx.AddInstrAccessChain_ArrayMember(0, ctx.IDInfos[arrayID]().GetILType(), arrayID, index),
							MemoryAccess::None
						);
						srcIDs[i] = ctx.AddInstrBitcast(ctx.DefineType(instrElementType), srcID);
					}

					int valueID = -1;

					if (instr->Type->IsFloatMatrix())
					{
						int n = 3;
						auto colType = GetTypeFromString("vec3");
						if (instr->Type->GetVectorSize() == 16)
						{
							n = 4;
							colType = GetTypeFromString("vec4");
						}
						int colIDs[4];
						for (int i = 0; i < n; i++)
						{
							List<int> args;
							for (int j = 0; j < n; j++)
								args.Add(srcIDs[i*n + j]);
							colIDs[i] = ctx.AddInstrCompositeConstruct(0, colType, args);
						}
						List<int> args;
						for (int i = 0; i < n; i++)
							args.Add(colIDs[i]);
						valueID = ctx.AddInstrCompositeConstruct(0, instr->Type, args);
					}
					else if (instr->Type->IsVector())
					{
						int n = 3;
						if (instr->Type->GetVectorSize() == 16)
							n = 4;
						List<int> args;
						for (int i = 0; i < n; i++)
							args.Add(srcIDs[i]);
						valueID = ctx.AddInstrCompositeConstruct(0, instr->Type, args);
					}
					else //scalar
					{
						valueID = srcIDs[0];
					}

					int storeID = ctx.AddInstrVariableDeclaration(instr, instr->Type, StorageClass::Function);
					ctx.AddInstrStore(instr, storeID, valueID);
				}

				else
					throw NotImplementedException("import in PrintImportInstr(): " + instr->ImportOperator->Name.Content);
			}

			void PrintInstr(ILInstruction & instr)
			{
				if (auto binInstr = instr.As<BinaryInstruction>())
					PrintBinaryInstr(binInstr);
				else if (auto allocVar = instr.As<AllocVarInstruction>())
					PrintAllocVarInstr(allocVar, StorageClass::Function);
				else if (auto call = instr.As<CallInstruction>())
					PrintCallInstr(call);
				else if (auto exportInstr = instr.As<ExportInstruction>())
				{
					PrintExportInstr(exportInstr);
					//throw InvalidOperationException("export instruction not supported");

				}
				else if (auto import = instr.As<ImportInstruction>())
				{
					PrintImportInstr(import);
					//throw InvalidOperationException("import instruction not supported");
				}
				else if (auto update = instr.As<MemberUpdateInstruction>())
					PrintUpdateInstr(update);
				else if (auto unaryInstr = instr.As<UnaryInstruction>())
					PrintUnaryInstr(unaryInstr);
				else if (auto select = instr.As<SelectInstruction>())
					PrintSelectInstr(select);
				else if (auto fetchArg = instr.As<FetchArgInstruction>()) //for function: return instruction
					PrintFetchArgInstr(fetchArg);
				else
					throw NotImplementedException("unsupported instruction in PrintInstr()" + instr.ToString());
			}

			void PrintIf(IfInstruction * instr)
			{
				int operandID = GetOperandValue(instr->Operand.Ptr());
				operandID = ctx.ConvertBasicType(
					operandID,
					ctx.IDInfos[operandID]().GetILType(),
					GetTypeFromString("bool"));

				int TrueLabel = ++ctx.CurrentID;
				int FalseLabel = ++ctx.CurrentID;
				int MergeLabel = ++ctx.CurrentID;

				ctx.AddInstrSelectionMerge(MergeLabel);
				ctx.AddInstrBranchConditional(operandID, TrueLabel, FalseLabel);

				ctx.PushScope();
				ctx.AddInstrLabel_AtFunctionBody(TrueLabel);
				GenerateCode(instr->TrueCode.Ptr(), TrueLabel);
				ctx.AddInstrBranch(MergeLabel);
				ctx.PopScope();

				ctx.PushScope();
				ctx.AddInstrLabel_AtFunctionBody(FalseLabel);
				if (instr->FalseCode)
					GenerateCode(instr->FalseCode.Ptr(), FalseLabel);
				ctx.AddInstrBranch(MergeLabel);
				ctx.PopScope();

				ctx.AddInstrLabel_AtFunctionBody(MergeLabel);
			}

			void PrintFor(ForInstruction * instr)
			{
				int HeaderBlockLabel = ++ctx.CurrentID;
				int ConditionBlockLabel = ++ctx.CurrentID;
				int BodyBlockLabel = ++ctx.CurrentID;
				int UpdateBlockLabel = ++ctx.CurrentID;
				int MergeBlockLabel = ++ctx.CurrentID;

				ctx.AddInstrBranch(HeaderBlockLabel);

				ctx.AddInstrLabel_AtFunctionBody(HeaderBlockLabel);
				ctx.AddInstrLoopMerge(MergeBlockLabel, UpdateBlockLabel);
				ctx.AddInstrBranch(ConditionBlockLabel);

				// condition block
				ctx.PushScope();
				ctx.AddInstrLabel_AtFunctionBody(ConditionBlockLabel);
				GenerateCode(instr->ConditionCode.Ptr(), ConditionBlockLabel);
				int conditionID = GetOperandValue(instr->ConditionCode->GetLastInstruction());
				conditionID = ctx.ConvertBasicType(
					conditionID,
					ctx.IDInfos[conditionID]().GetILType(),
					GetTypeFromString("bool"));
				ctx.AddInstrBranchConditional(conditionID, BodyBlockLabel, MergeBlockLabel);
				ctx.PopScope();

				// body block
				ctx.PushScope();
				ctx.AddInstrLabel_AtFunctionBody(BodyBlockLabel);
				ctx.StackMergeBlock.Add(MergeBlockLabel);
				ctx.StackContinueBlock.Add(UpdateBlockLabel);
				GenerateCode(instr->BodyCode.Ptr(), BodyBlockLabel);
				ctx.StackMergeBlock.RemoveAt(ctx.StackMergeBlock.Count() - 1);
				ctx.StackContinueBlock.RemoveAt(ctx.StackContinueBlock.Count() - 1);
				ctx.AddInstrBranch(UpdateBlockLabel);
				ctx.PopScope();

				// update block
				ctx.PushScope();
				ctx.AddInstrLabel_AtFunctionBody(UpdateBlockLabel);
				GenerateCode(instr->SideEffectCode.Ptr(), UpdateBlockLabel);
				ctx.AddInstrBranch(HeaderBlockLabel);
				ctx.PopScope();

				// merge block
				ctx.AddInstrLabel_AtFunctionBody(MergeBlockLabel);
			}

			void PrintWhileDo(WhileInstruction * instr)
			{
				int HeaderBlockLabel = ++ctx.CurrentID;
				int ConditionBlockLabel = ++ctx.CurrentID;
				int BodyBlockLabel = ++ctx.CurrentID;
				int UpdateBlockLabel = ++ctx.CurrentID;
				int MergeBlockLabel = ++ctx.CurrentID;

				ctx.AddInstrBranch(HeaderBlockLabel);

				// header block
				ctx.AddInstrLabel_AtFunctionBody(HeaderBlockLabel);
				ctx.AddInstrLoopMerge(MergeBlockLabel, UpdateBlockLabel);
				ctx.AddInstrBranch(ConditionBlockLabel);

				// condition block
				ctx.PushScope();
				ctx.AddInstrLabel_AtFunctionBody(ConditionBlockLabel);
				GenerateCode(instr->ConditionCode.Ptr(), ConditionBlockLabel, true);
				int conditionID = GetOperandValue(instr->ConditionCode->GetLastInstruction()->As<ReturnInstruction>()->Operand.Ptr());
				conditionID = ctx.ConvertBasicType(
					conditionID,
					ctx.IDInfos[conditionID]().GetILType(),
					GetTypeFromString("bool")
				);
				ctx.AddInstrBranchConditional(conditionID, BodyBlockLabel, MergeBlockLabel);
				ctx.PopScope();

				// body block
				ctx.PushScope();
				ctx.AddInstrLabel_AtFunctionBody(BodyBlockLabel);
				ctx.StackMergeBlock.Add(MergeBlockLabel);
				ctx.StackContinueBlock.Add(UpdateBlockLabel);
				GenerateCode(instr->BodyCode.Ptr(), BodyBlockLabel);
				ctx.StackMergeBlock.RemoveAt(ctx.StackMergeBlock.Count() - 1);
				ctx.StackContinueBlock.RemoveAt(ctx.StackContinueBlock.Count() - 1);
				ctx.AddInstrBranch(UpdateBlockLabel);
				ctx.PopScope();

				// update block (empty)
				ctx.AddInstrLabel_AtFunctionBody(UpdateBlockLabel);
				ctx.AddInstrBranch(HeaderBlockLabel);

				// merge block
				ctx.AddInstrLabel_AtFunctionBody(MergeBlockLabel);
			}

			void PrintDoWhile(DoInstruction * instr)
			{
				int HeaderBlockLabel = ++ctx.CurrentID;
				int BodyBlockLabel = ++ctx.CurrentID;
				int ConditionBlockLabel = ++ctx.CurrentID;
				int MergeBlockLabel = ++ctx.CurrentID;

				//ctx.FunctionBody << LR"(OpBranch %)" << HeaderBlockLabel << EndLine;
				ctx.AddInstrBranch(HeaderBlockLabel);

				// header block
				ctx.AddInstrLabel_AtFunctionBody(HeaderBlockLabel);
				ctx.AddInstrLoopMerge(MergeBlockLabel, ConditionBlockLabel);
				ctx.AddInstrBranch(BodyBlockLabel);

				// body block
				ctx.PushScope();
				ctx.AddInstrLabel_AtFunctionBody(BodyBlockLabel);
				ctx.StackMergeBlock.Add(MergeBlockLabel);
				ctx.StackContinueBlock.Add(ConditionBlockLabel);
				GenerateCode(instr->BodyCode.Ptr(), BodyBlockLabel);
				ctx.StackMergeBlock.RemoveAt(ctx.StackMergeBlock.Count() - 1);
				ctx.StackContinueBlock.RemoveAt(ctx.StackContinueBlock.Count() - 1);
				ctx.AddInstrBranch(ConditionBlockLabel);
				ctx.PopScope();

				// condition block
				ctx.PushScope();
				ctx.AddInstrLabel_AtFunctionBody(ConditionBlockLabel);
				GenerateCode(instr->ConditionCode.Ptr(), ConditionBlockLabel, true);
				int conditionID = GetOperandValue(instr->ConditionCode->GetLastInstruction()->As<ReturnInstruction>()->Operand.Ptr());
				conditionID = ctx.ConvertBasicType(
					conditionID,
					ctx.IDInfos[conditionID]().GetILType(),
					GetTypeFromString("bool")
				);
				ctx.AddInstrBranchConditional(conditionID, HeaderBlockLabel, MergeBlockLabel);
				ctx.PopScope();

				// merge block
				ctx.AddInstrLabel_AtFunctionBody(MergeBlockLabel);
			}

			void GenerateCode(CFGNode * code, int givenLabel = -1, bool LoopReturn = false)
			{
				if (givenLabel == -1)
				{
					++ctx.CurrentID; //label ID
					ctx.AddInstrLabel_AtFunctionBody(ctx.CurrentID);
				}

				List<int> usedID;
				for (auto & instr : *code)
				{

					if (auto ifInstr = instr.As<IfInstruction>())
					{
						PrintIf(ifInstr);
					}
					else if (auto forInstr = instr.As<ForInstruction>())
					{
						PrintFor(forInstr);
					}
					else if (auto doInstr = instr.As<DoInstruction>())
					{
						PrintDoWhile(doInstr);
					}
					else if (auto whileInstr = instr.As<WhileInstruction>())
					{
						PrintWhileDo(whileInstr);
					}
					else if (auto ret = instr.As<ReturnInstruction>())
					{
						if (!LoopReturn)
						{
							if (ret->Operand)
								ctx.AddInstrReturnValue(GetOperandValue(ret->Operand.Ptr()));
							else
								ctx.AddInstrReturn();
							ctx.AddInstrLabel_AtFunctionBody(++ctx.CurrentID);
						}
					}
					else if (instr.Is<BreakInstruction>())
					{
						ctx.AddInstrBranch(ctx.StackMergeBlock.Last());
						ctx.AddInstrLabel_AtFunctionBody(++ctx.CurrentID);
					}
					else if (instr.Is<ContinueInstruction>())
					{
						ctx.AddInstrBranch(ctx.StackContinueBlock.Last());
						ctx.AddInstrLabel_AtFunctionBody(++ctx.CurrentID);
					}
					else if (instr.Is<DiscardInstruction>())
					{
						ctx.AddInstrKill();
						ctx.AddInstrLabel_AtFunctionBody(++ctx.CurrentID);
					}
					else
					{
						PrintInstr(instr);
					}
				}
			}

		public:

			bool BufferPreProcessed;

			void Initiate(ExecutionModel model, CompiledWorld * shaderWorld)
			{
				ctx.Clear();
				ctx.CodeGen.Initiate();
				ctx.CurrentID = 1; // reserved for extinst
				ctx.CodeGen.ProgramHeader();
				ctx.PushScope();
				interfaceIDs.Clear();
				DepthReplacing = false;
				LocalSize = false;
				BufferImportOrExport = false;
				BufferPreProcessed = false;
				currentWorld = shaderWorld;
				currentExecutionModel = model;
			}

			int GenerateFunctionDeclaration(CompiledFunction *func)
				//return the ID of the Function Type
			{
				List<RefPtr<ILType>> args;
				for (auto & instr : *func->Code)
					if (auto arg = instr.As<FetchArgInstruction>())
						if (arg->ArgId != 0)
						{
							args.Add(arg->Type);
						}
				int funcTypeID = ctx.AddInstrTypeFunction(func, args, func->ReturnType);
				ctx.FunctionNameToFunctionID[func->Name] = ++ctx.CurrentID;
				return funcTypeID;
			}

			int GenerateFunctionDefinition(CompiledFunction *func, ILOperand *vertexOutput = nullptr)
				//return the ID of the Function
			{
				ctx.ClearBuffer();

				ctx.PushScope();

				int funcID = ctx.FunctionNameToFunctionID[func->Name]();
				int funcTypeID = ctx.FunctionNameToFunctionTypeID[func->Name]();

				ctx.AddInstrFunction(funcID, ctx.DefineType(func->ReturnType), funcTypeID, GetFuncOriginalName(func->Name));

				for (auto & instr : *func->Code)
					if (auto arg = instr.As<FetchArgInstruction>())
						if (arg->ArgId != 0)
						{
							if (!ctx.ParameterNameToID.ContainsKey((ILOperand*)&instr))
							{
								ctx.DefineType(arg->Type);
								int typeID = ctx.DefineTypePointer(arg->Type, StorageClass::Function);
								ctx.AddInstrFunctionParameter((ILOperand*)&instr, typeID, arg->Name);
							}
						}

				++ctx.CurrentID;
				ctx.AddInstrLabel_AtFunctionHeader(ctx.CurrentID);

				func->Code->NameAllInstructions();
				GenerateCode(func->Code.Ptr(), ctx.CurrentID);

				if (vertexOutput)
				{
					int valueID = ctx.AddInstrLoad(nullptr, vertexOutput, MemoryAccess::None);
					int gl_PositionID = ctx.AddInstrAccessChain_StructMember(
						0,
						ctx.InterfaceNameToID["variable_gl_PerVertex"],
						"gl_Position"
					);
					ctx.AddInstrStore(0, gl_PositionID, valueID);
				}

				ctx.AddInstrReturn();
				ctx.AddInstrFunctionEnd();

				ctx.ProduceFunction();

				ctx.PopScope();

				return funcID;
			}

			void Decorate(int ID, Decoration deco, int op1 = -1)
			{
				ctx.AddInstrDecorate(ID, deco, op1);
			}

			void MemberDecorate(int ID, int index, Decoration deco, int op1 = -1)
			{
				ctx.AddInstrMemberDecorate(ID, index, deco, op1);
			}

			void SetDepthReplacing()
			{
				DepthReplacing = true;
			}

			void SetLocalSize(unsigned int x)
			{
				//layout(local_size_x = <x> ) in;
				LocalSize = true;
				unsigned int workgroup[3] = { x, 1, 1 };
				ctx.AddInstrDecorate(
					ctx.AddInstrConstantCompositeUInt(workgroup, 3),
					Decoration::BuiltIn,
					(int)BuiltIn::WorkgroupSize
				);
			}

			void GenerateEntryPoint(int mainFunctionID)
			{
				ctx.AddInstrEntryPoint(currentExecutionModel, mainFunctionID, interfaceIDs);
				if (currentExecutionModel == ExecutionModel::Fragment)
				{
					ctx.AddInstrExecutionMode(mainFunctionID, ExecutionMode::OriginUpperLeft);
				}
				if (DepthReplacing)
				{
					ctx.AddInstrExecutionMode(mainFunctionID, ExecutionMode::DepthReplacing);
				}
				if (LocalSize)
				{
					ctx.AddInstrExecutionMode(mainFunctionID, ExecutionMode::LocalSize, 256, 1, 1);
				}
			}

			int GenerateGlGlobalInvocationID()
				//return the ID of the gl_GlobalInvocationID variable
			{
				int GlobalInvocationID = ctx.AddInstrVariableDeclaration(0, GetTypeFromString("uvec3"), StorageClass::Input, "gl_GlobalInvocationID", 0);
				ctx.AddInstrDecorate(GlobalInvocationID, Decoration::BuiltIn, (int)BuiltIn::GlobalInvocationId);
				ctx.InterfaceNameToID["gl_GlobalInvocationID"] = GlobalInvocationID;
				return GlobalInvocationID;
			}

			int GenerateInterfaceForSingleVariable(
				String varName,
				RefPtr<ILType> varType,
				StorageClass storageClass,
				int location = -1,
				int bindIndex = -1)
				//return the ID of the variable
			{
				int entID = ctx.AddInstrVariableDeclaration(0, varType, storageClass, varName, 0);
				ctx.InterfaceNameToID[varName] = entID;
				if (location != -1)
					ctx.AddInstrDecorate(entID, Decoration::Location, location);
				if (bindIndex != -1)
				{
					ctx.AddInstrDecorate(entID, Decoration::DescriptorSet, 0);
					ctx.AddInstrDecorate(entID, Decoration::Binding, bindIndex);
				}
				if (storageClass == StorageClass::Input || storageClass == StorageClass::Output)
					interfaceIDs.Add(entID);
				return entID;
			}

			std::pair<int,int> GenerateInterfaceForStructVariable(
				String structTypeName,
				String structVariableName,
				List<RefPtr<ILType>> memberTypes,
				List<String> memberNames,
				int UniformOrBuffer,
				StorageClass storageClass,
				int location = -1,
				int bindindex = -1)
				//first  - the ID of the struct type
				//second - the ID of the variable 
			{
				RefPtr<ILStructType> structIL = new ILStructType();
				structIL->TypeName = structTypeName;
				int n = memberTypes.Count();
				for (int i = 0; i < n; i++)
				{
					ILStructType::ILStructField field;
					field.Type = memberTypes[i];
					field.FieldName = memberNames[i];
					structIL->Members.Add(field);
				}

				int structTypeID = ctx.DefineType(structIL, UniformOrBuffer);
				int structVariableID = ctx.AddInstrVariableDeclaration(0, structIL, storageClass, structVariableName, UniformOrBuffer);
				ctx.InterfaceNameToID[structVariableName] = structVariableID;

				if (UniformOrBuffer == 1)
				{
					ctx.AddInstrDecorate(structVariableID, Decoration::DescriptorSet, 0);
					ctx.AddInstrDecorate(structTypeID, Decoration::Block);
				}
				else if (UniformOrBuffer == 2)
				{
					ctx.AddInstrDecorate(structVariableID, Decoration::DescriptorSet, 0);
					ctx.AddInstrDecorate(structTypeID, Decoration::BufferBlock);
				}

				if (location != -1)
				{
					ctx.AddInstrDecorate(structVariableID, Decoration::Location, location);
				}

				if (bindindex != -1)
				{
					ctx.AddInstrDecorate(structVariableID, Decoration::Binding, bindindex);
				}

				if (storageClass == StorageClass::Input || storageClass == StorageClass::Output)
				{
					for (int i = 0; i < n; i++)
						if (memberTypes[i]->IsIntegral())
							ctx.AddInstrMemberDecorate(structTypeID, i, Decoration::Flat);
					interfaceIDs.Add(structVariableID);
				}

				return std::make_pair(structTypeID, structVariableID);
			}

			CompiledShaderSource GenerateShaderWorld()
			{
				CompiledShaderSource rs;

				auto binaryForm = ctx.ProduceWordStream();
				rs.BinaryCode.SetSize(binaryForm.Count() * sizeof(unsigned int));
				memcpy(rs.BinaryCode.Buffer(), binaryForm.Buffer(), rs.BinaryCode.Count());

				rs.MainCode = ctx.ProduceTextCode();

				return rs;
			}
		};

		class SpirVCodeGen : public CodeGenBackend
		{
			String vertexOutputName;
			EnumerableDictionary<String, String> backendArguments;

		public:

			ExecutionModel currentExecutionModel = ExecutionModel::Invalid;

			void ProcessBufferImportOrExportInterfaces(SpirvModule & spvModule, String blkName, int index)
			{
				if (!spvModule.BufferPreProcessed)
				{
					spvModule.BufferPreProcessed = true;
					spvModule.SetLocalSize(256);
					spvModule.GenerateGlGlobalInvocationID();

					List<RefPtr<ILType>> memberTypes;
					List<String> memberNames;
					memberTypes.Add(GetTypeFromString("uint"));
					memberNames.Add("sys_thread_count");
					spvModule.GenerateInterfaceForStructVariable(
						"SystemBlock",
						"blkSystemBlock",
						memberTypes,
						memberNames,
						1,
						StorageClass::Uniform,
						-1,
						0
					);
				}

				List<RefPtr<ILType>> memberTypes;
				List<String> memberNames;
				memberTypes.Add(GetTypeFromString("float[0]"));
				memberNames.Add("a");
				spvModule.GenerateInterfaceForStructVariable(
					blkName,
					"blk" + blkName,
					memberTypes,
					memberNames,
					2,
					StorageClass::Uniform,
					-1,
					index
				);
			}

			virtual CompiledShaderSource GenerateShaderWorld(CompileResult & result, SymbolTable * /*symbols*/, CompiledWorld * shaderWorld, Dictionary<String, ImportOperatorHandler*>& opHandlers, Dictionary<String, ExportOperatorHandler*>& exportHandlers) override
			{
				
				currentExecutionModel = ExecutionModel::Invalid;
				if (shaderWorld->ExportOperator.Content == "fragmentExport")
					currentExecutionModel = ExecutionModel::Fragment;
				else if (shaderWorld->BackendParameters.ContainsKey("vertex"))
					currentExecutionModel = ExecutionModel::Vertex;
				else if (shaderWorld->ExportOperator.Content == "bufferExport")
					currentExecutionModel = ExecutionModel::GLCompute;

				CompiledFunction mainFunction;
				mainFunction.Name = "main";
				mainFunction.ReturnType = nullptr;
				mainFunction.Code = shaderWorld->Code;

				SpirvModule spvModule;
				spvModule.Initiate(currentExecutionModel, shaderWorld);
				spvModule.GenerateFunctionDeclaration(&mainFunction);

				for (auto funcName : shaderWorld->ReferencedFunctions)
					for(auto & func : result.Program->Functions)
						if (funcName == func->Name)
							spvModule.GenerateFunctionDeclaration(func.Ptr());

				for (auto funcName : shaderWorld->ReferencedFunctions)
					for (auto & func : result.Program->Functions)
						if (funcName == func->Name)
							spvModule.GenerateFunctionDefinition(func.Ptr());

				for (auto & inputBlock : shaderWorld->WorldInputs)
				{
					auto block = inputBlock.Value.Block;
					if (!block->UserWorlds.Contains(shaderWorld->WorldName))
						continue;
					String impOpName = inputBlock.Value.ImportOperator.Name.Content;

					if (impOpName == "standardImport")
					{
						List<RefPtr<ILType>> memberTypes;
						List<String> memberNames;
						for (auto & ent : block->Entries)
						{
							memberTypes.Add(ent.Value.Type);
							memberNames.Add(ent.Value.Name);
						}
						spvModule.GenerateInterfaceForStructVariable(
							block->Name,
							"blk"+block->Name,
							memberTypes,
							memberNames,
							0,
							StorageClass::Input,
							0
						);
					}

					else if (impOpName == "vertexImport")
					{
						int location = 0;
						for (auto & ent : block->Entries)
						{
							spvModule.GenerateInterfaceForSingleVariable(
								ent.Key,
								ent.Value.Type,
								StorageClass::Input,
								location
							);
							location++;
						}
					}

					else if (impOpName == "uniformImport")
					{
						int nonTextureCount = 0;
						for (auto & ent : block->Entries)
							if (!ent.Value.Type->IsTexture())
								nonTextureCount++;

						int TypeOfStruct = 1; //1: uniform buffer; 2: shader storage buffer; 0: not a buffer
						if (block->Attributes.ContainsKey("ShaderStorageBlock"))
							TypeOfStruct = 2;

						if (nonTextureCount)
						{
							List<RefPtr<ILType>> memberTypes;
							List<String> memberNames;
							for (auto & ent : block->Entries)
								if (!ent.Value.Type->IsTexture())
								{
									memberTypes.Add(ent.Value.Type);
									memberNames.Add(ent.Value.Name);
								}
							String strIndex;
							int index = -1;
							if (block->Attributes.TryGetValue("Index", strIndex))
								index = StringToInt(strIndex);
							spvModule.GenerateInterfaceForStructVariable(
								block->Name,
								"blk" + block->Name,
								memberTypes,
								memberNames,
								TypeOfStruct,
								StorageClass::Uniform,
								-1,
								index
							);
						}
						
						int bindPoint = 0;
						String bindingStart;
						if (backendArguments.TryGetValue("TextureBindingStart", bindingStart))
							bindPoint = StringToInt(bindingStart);

						for(auto & ent : block->Entries)
							if (ent.Value.Type->IsTexture())
							{
								spvModule.GenerateInterfaceForSingleVariable(
									ent.Key,
									ent.Value.Type,
									StorageClass::UniformConstant,
									-1,
									bindPoint
								);
								bindPoint++;
							}
					}

					else if (impOpName == "textureImport")
					{
						int bindPoint = 0;
						String strIndex;
						if (block->Attributes.TryGetValue("Index", strIndex))
							bindPoint = StringToInt(strIndex);
						for (auto & ent : block->Entries)
						{
							spvModule.GenerateInterfaceForSingleVariable(
								ent.Key,
								ent.Value.Type,
								StorageClass::UniformConstant,
								-1,
								bindPoint
							);
							bindPoint++;
						}
					}

					else if (impOpName == "bufferImport")
					{
						String strIdx;
						int index = 0;
						if (block->Attributes.TryGetValue("Index", strIdx))
							index = StringToInt(strIdx);
						ProcessBufferImportOrExportInterfaces(spvModule, block->Name, index);
					}

					else 
						throw NotImplementedException("not implemented input interface: " + impOpName);
				}

				if (shaderWorld->ExportOperator.Content == "fragmentExport")
				{
					int location = 0;
					for (auto & ent : shaderWorld->WorldOutput->Entries)
						if (!ent.Value.LayoutAttribs.ContainsKey("DepthOutput"))
						{
							spvModule.GenerateInterfaceForSingleVariable(
								ent.Key,
								ent.Value.Type,
								StorageClass::Output,
								location
							);
							location++;
						}
						else
						{
							int entID = spvModule.GenerateInterfaceForSingleVariable(
								"gl_FragDepth",
								ent.Value.Type,
								StorageClass::Output,
								location
							);
							spvModule.SetDepthReplacing();
							spvModule.Decorate(entID, Decoration::BuiltIn, (int)BuiltIn::FragDepth);
						}
				}

				else if (shaderWorld->ExportOperator.Content == "standardExport")
				{
					List<RefPtr<ILType>> memberTypes;
					List<String> memberNames;
					for (auto & ent : shaderWorld->WorldOutput->Entries)
					{
						memberTypes.Add(ent.Value.Type);
						memberNames.Add(ent.Value.Name);
					}
					spvModule.GenerateInterfaceForStructVariable(
						shaderWorld->WorldOutput->Name,
						"blk" + shaderWorld->WorldOutput->Name,
						memberTypes,
						memberNames,
						0,
						StorageClass::Output,
						0
					);
				}

				else if (shaderWorld->ExportOperator.Content == "bufferExport")
				{
					String strIdx;
					int index = 0;
					if (shaderWorld->WorldOutput->Attributes.TryGetValue("Index", strIdx))
						index = StringToInt(strIdx);
					ProcessBufferImportOrExportInterfaces(spvModule, shaderWorld->WorldOutput->Name, index);
				}

				else
					throw NotImplementedException("not implemented output interface: " + shaderWorld->ExportOperator.Content);

				if (vertexOutputName.Length())
				{
					CompiledComponent ccomp;
					if (shaderWorld->LocalComponents.TryGetValue(vertexOutputName, ccomp))
					{
						List<RefPtr<ILType>> memberTypes;
						List<String> memberNames;
						memberTypes.Add(GetTypeFromString("vec4"));
						memberNames.Add("gl_Position");
						memberTypes.Add(GetTypeFromString("float"));
						memberNames.Add("gl_PointSize");
						memberTypes.Add(GetTypeFromString("float[1]"));
						memberNames.Add("gl_ClipDistance");
						memberTypes.Add(GetTypeFromString("float[1]"));
						memberNames.Add("gl_CullDistance");

						int typeID = spvModule.GenerateInterfaceForStructVariable(
							"gl_PerVertex",
							"variable_gl_PerVertex",
							memberTypes,
							memberNames,
							0,
							StorageClass::Output,
							-1,
							-1
						).first;

						spvModule.Decorate(typeID, Decoration::Block);
						spvModule.MemberDecorate(typeID, 0, Decoration::BuiltIn, (int)BuiltIn::Position);
						spvModule.MemberDecorate(typeID, 1, Decoration::BuiltIn, (int)BuiltIn::PointSize);
						spvModule.MemberDecorate(typeID, 2, Decoration::BuiltIn, (int)BuiltIn::ClipDistance);
						spvModule.MemberDecorate(typeID, 3, Decoration::BuiltIn, (int)BuiltIn::CullDistance);
					}
				}

				ILOperand *vertexOutput = nullptr;
				if (vertexOutputName.Length())
				{
					CompiledComponent ccomp;
					if (shaderWorld->LocalComponents.TryGetValue(vertexOutputName, ccomp))
					{
						vertexOutput = ccomp.CodeOperand;
					}
					else
						throw InvalidOperationException("can not find vertexOutputName");
				}

				int mainFunctionID = spvModule.GenerateFunctionDefinition(&mainFunction, vertexOutput);

				spvModule.GenerateEntryPoint(mainFunctionID);

				return spvModule.GenerateShaderWorld();
				
				/*
				ctx.Clear();

				currentWorld = shaderWorld;
				CompiledShaderSource rs;
				ctx.Result = &result;

				currentExecutionModel = ExecutionModel::Invalid;
				if (currentWorld->ExportOperator.Content == "fragmentExport")
					currentExecutionModel = ExecutionModel::Fragment;
				else if (currentWorld->BackendParameters.ContainsKey("vertex"))
					currentExecutionModel = ExecutionModel::Vertex;
				else if (currentWorld->ExportOperator.Content == "bufferExport")
					currentExecutionModel = ExecutionModel::GLCompute;

				if (ExecutionModel::Invalid == currentExecutionModel)
					throw InvalidOperationException("invalid execution model for shader world: " + currentWorld->WorldName);

				ctx.CurrentID = 1;

				ctx.CodeGen.Initiate();
				ctx.CodeGen.ProgramHeader();

				//add all functions type definition
				for (auto funcName : shaderWorld->ReferencedFunctions)
				{
					for (auto &func : result.Program->Functions)
					{
						if (func->Name == funcName)
						{
							List<RefPtr<ILType>> args;
							for (auto & instr : *func->Code)
								if (auto arg = instr.As<FetchArgInstruction>())
									if (arg->ArgId != 0)
									{
										args.Add(arg->Type);
									}
							ctx.AddInstrTypeFunction(func.Ptr(), args, func->ReturnType);
							ctx.FunctionNameToFunctionID[func->Name] = ++ctx.CurrentID; 
							//this is reserverd for the result ID of the OpFunction instruction! 
						}
					}
				}

				//add all functions definition
				for (auto funcName : shaderWorld->ReferencedFunctions)
				{
					for (auto &func : result.Program->Functions)
					{
						if (func->Name == funcName)
						{

							ctx.ClearBuffer();

							ctx.PushScope();

							int funcID = ctx.FunctionNameToFunctionID[func->Name]();
							int funcTypeID = ctx.FunctionNameToFunctionTypeID[func->Name]();

							ctx.AddInstrFunction(funcID, ctx.DefineType(func->ReturnType), funcTypeID, GetFuncOriginalName(funcName));

							for (auto & instr : *func->Code)
								if (auto arg = instr.As<FetchArgInstruction>())
									if (arg->ArgId != 0)
									{
										if (!ctx.ParameterNameToID.ContainsKey((ILOperand*)&instr))
										{
											ctx.DefineType(arg->Type);
											int typeID = ctx.DefineTypePointer(arg->Type, StorageClass::Function);
											ctx.AddInstrFunctionParameter((ILOperand*)&instr, typeID, arg->Name);
										}
									}

							++ctx.CurrentID;
							ctx.AddInstrLabel_AtFunctionHeader(ctx.CurrentID);

							func->Code->NameAllInstructions();
							GenerateCode(func->Code.Ptr(), ctx.CurrentID);

							ctx.AddInstrReturn();
							ctx.AddInstrFunctionEnd();

							ctx.ProduceFunction();

							ctx.PopScope();
						}
					}
				}

				ctx.ClearBuffer();

				ctx.PushScope();

				List<int> interfaceIDs;
				bool DepthReplacing = false;
				bool LocalSize = false;
				bool BufferImportOrExport = false;
				ProcessInterfaces(interfaceIDs, shaderWorld, opHandlers, DepthReplacing, LocalSize, BufferImportOrExport);

				//for gl_Position
				if (vertexOutputName.Length())
				{
					CompiledComponent ccomp;
					if (currentWorld->LocalComponents.TryGetValue(vertexOutputName, ccomp))
					{
						RefPtr<ILStructType> structIL = new ILStructType();
						structIL->TypeName = "gl_PerVertex";

						ILStructType::ILStructField f1;
						f1.Type = GetTypeFromString("vec4");
						f1.FieldName = "gl_Position";
						structIL->Members.Add(f1);
						ILStructType::ILStructField f2;
						f2.Type = GetTypeFromString("float");
						f2.FieldName = "gl_PointSize";
						structIL->Members.Add(f2);
						ILStructType::ILStructField f3;
						f3.Type = GetTypeFromString("float[1]");
						f3.FieldName = "gl_ClipDistance";
						structIL->Members.Add(f3);
						ILStructType::ILStructField f4;
						f4.Type = GetTypeFromString("float[1]");
						f4.FieldName = "gl_CullDistance";
						structIL->Members.Add(f4);

						int structTypeID = ctx.DefineType(structIL);
						int structVariableID = ctx.AddInstrVariableDeclaration(0, structIL, StorageClass::Output, "gl_PerVertex");
						ctx.InterfaceNameToID["gl_PerVertex"] = structVariableID;

						ctx.AddInstrDecorate(structTypeID, Decoration::Block);
						ctx.AddInstrMemberDecorate(structTypeID, 0, Decoration::BuiltIn, (int)BuiltIn::Position);
						ctx.AddInstrMemberDecorate(structTypeID, 1, Decoration::BuiltIn, (int)BuiltIn::PointSize);
						ctx.AddInstrMemberDecorate(structTypeID, 2, Decoration::BuiltIn, (int)BuiltIn::ClipDistance);
						ctx.AddInstrMemberDecorate(structTypeID, 3, Decoration::BuiltIn, (int)BuiltIn::CullDistance);

						interfaceIDs.Add(structVariableID);
					}
					else
						throw InvalidOperationException("can not find vertexOutputName");
				}

				//add Main function type definition
				ctx.MainFunctionTypeID = ctx.AddInstrTypeFunction(nullptr, List<RefPtr<ILType>>(), nullptr);
				ctx.MainFunctionID = ++ctx.CurrentID;

				//Entry Point
				ctx.AddInstrEntryPoint(currentExecutionModel, ctx.MainFunctionID, interfaceIDs);

				//execution mode
				if (currentExecutionModel == ExecutionModel::Fragment)
				{
					//CodeGen.OpExecutionMode(ctx.MainFunctionID, ExecutionMode::OriginUpperLeft);
					ctx.AddInstrExecutionMode(ctx.MainFunctionID, ExecutionMode::OriginUpperLeft);
				}
				if (DepthReplacing)
				{
					//CodeGen.OpExecutionMode(ctx.MainFunctionID, ExecutionMode::DepthReplacing);
					ctx.AddInstrExecutionMode(ctx.MainFunctionID, ExecutionMode::DepthReplacing);
				}
				if (LocalSize)
				{
					ctx.AddInstrExecutionMode(ctx.MainFunctionID, ExecutionMode::LocalSize, 256, 1, 1);
				}

				//MainFunction
				ctx.AddInstrFunction(ctx.MainFunctionID, ctx.TypeNameToID["void"](), ctx.MainFunctionTypeID, "main");

				++ctx.CurrentID;
				ctx.AddInstrLabel_AtFunctionHeader(ctx.CurrentID);

				if (BufferImportOrExport)
				{
					int GIIDx = ctx.AddInstrLoad(
						ctx.AddInstrAccessChain_VectorMember(0, ctx.InterfaceNameToID["gl_GlobalInvocationID"], -1, 0),
						MemoryAccess::None
					);	//GlobalInvocationID.x
					int SysThreadCountID = ctx.AddInstrLoad(
						ctx.AddInstrAccessChain_StructMember(0, ctx.InterfaceNameToID["SystemBlock"], "sys_thread_count"),
						MemoryAccess::None
					);
					int conditionID = ctx.AddInstrBinaryInstr(
						0, GetTypeFromString("bool"), "OpUGreaterThanEqua", GIIDx, SysThreadCountID
					);
					int MergeLabel = ++ctx.CurrentID;
					int TrueLabel = ++ctx.CurrentID;
					ctx.AddInstrSelectionMerge(MergeLabel);
					ctx.AddInstrBranchConditional(conditionID, TrueLabel, MergeLabel);
					ctx.AddInstrLabel_AtFunctionBody(TrueLabel);
					ctx.AddInstrReturn();
					ctx.AddInstrLabel_AtFunctionBody(MergeLabel);
				}

				shaderWorld->Code->NameAllInstructions();
				GenerateCode(shaderWorld->Code.Ptr(), ctx.CurrentID);

				if (vertexOutputName.Length())
				{
					CompiledComponent ccomp;
					if (currentWorld->LocalComponents.TryGetValue(vertexOutputName, ccomp))
					{
						int valueID = ctx.AddInstrLoad(nullptr, ccomp.CodeOperand, MemoryAccess::None);
						int gl_PositionID = ctx.AddInstrAccessChain_StructMember(
							0,
							ctx.InterfaceNameToID["gl_PerVertex"],
							"gl_Position"
						);
						ctx.AddInstrStore(0, gl_PositionID, valueID);
					}
					else
						throw InvalidOperationException("can not find vertexOutputName");
				}

				//MainFunction End
				ctx.AddInstrReturn();
				ctx.AddInstrFunctionEnd();

				ctx.ProduceFunction();

				ctx.PopScope();

				auto binaryForm = ctx.ProduceWordStream();
				rs.BinaryCode.SetSize(binaryForm.Count() * sizeof(unsigned int));
				memcpy(rs.BinaryCode.Buffer(), binaryForm.Buffer(), rs.BinaryCode.Count());

				rs.MainCode = ctx.ProduceTextCode();

				rs.OutputDeclarations = "spirv";

				//print IL 
				{
					auto compiledProgram = result.Program.Ptr();
					StringBuilder sb;

					//function part
					sb << "function" << EndLine;
					sb << "{" << EndLine;
					for (auto &pfunc : compiledProgram->Functions) {
						sb << pfunc->ReturnType->ToString() << " " << pfunc->Name << "(";
						bool first = true;
						for (auto &name2param : pfunc->Parameters) {
							if (!first)
								sb << ", ";
							sb << name2param.Value->ToString() << " " << name2param.Key;
							first = false;
						}
						sb << ")" << EndLine;
						sb << "{" << EndLine;
						pfunc->Code->NameAllInstructions();
						sb << pfunc->Code->ToString() << EndLine;
						sb << "}" << EndLine;
					}
					sb << "}" << EndLine;

					//shader part
					for (auto &pshader : compiledProgram->Shaders) {
						sb << "Shader " << pshader->MetaData.ShaderName << EndLine;
						sb << "{" << EndLine;
						for (auto &pworld : pshader->Worlds) {
							sb << "World " << pworld.Key << EndLine;
							sb << "{" << EndLine;
							pworld.Value.Ptr()->Code->NameAllInstructions();
							sb << pworld.Value.Ptr()->Code->ToString() << EndLine;
							sb << "}" << EndLine;
						}
						sb << "}" << EndLine;
					}

					StringBuilder sb_indent;
					IndentString(sb_indent, sb.ToSt\`ring());
					CoreLib::IO::StreamWriter sw("IL-" + currentWorld->ShaderName + "-" + currentWorld->WorldOutput->Name + String(".out"));
					sw.Write(sb_indent.ToString());
				}

				currentWorld = nullptr;

				return rs;
				*/
			}

			virtual void SetParameters(const EnumerableDictionary<String, String>& arguments) override
			{
				backendArguments = arguments;
				if (!arguments.TryGetValue("vertex", vertexOutputName))
					vertexOutputName = "";
			}
		};

		CodeGenBackend * CreateSpirVCodeGen()
		{
			return new SpirVCodeGen();
		}
	}
}
#endif