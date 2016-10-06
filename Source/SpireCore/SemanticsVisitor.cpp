#include "SyntaxVisitors.h"
#include "IL.h"

namespace Spire
{
	namespace Compiler
	{
		bool IsNumeric(BaseType t)
		{
			return t == BaseType::Int || t == BaseType::Float || t == BaseType::UInt;
		}

		String TranslateHLSLTypeNames(String name)
		{
			if (name == L"float2" || name == L"half2")
				return L"vec2";
			else if (name == L"float3" || name == L"half3")
				return L"vec3";
			else if (name == L"float4" || name == L"half4")
				return L"vec4";
			else if (name == L"half")
				return L"float";
			else if (name == L"int2")
				return L"ivec2";
			else if (name == L"int3")
				return L"ivec3";
			else if (name == L"int4")
				return L"ivec4";
			else if (name == L"uint2")
				return L"uvec2";
			else if (name == L"uint3")
				return L"uvec3";
			else if (name == L"uint4")
				return L"uvec4";
			else if (name == L"float3x3" || name == L"half3x3")
				return L"mat3";
			else if (name == L"float4x4" || name == L"half4x4")
				return L"mat4";
			else
				return name;
		}

		class SemanticsVisitor : public SyntaxVisitor
		{
			ProgramSyntaxNode * program = nullptr;
			FunctionSyntaxNode * function = nullptr;
			FunctionSymbol * currentFunc = nullptr;
			ShaderSymbol * currentShader = nullptr;
			PipelineSymbol * currentPipeline = nullptr;
			ImportOperatorDefSyntaxNode * currentImportOperator = nullptr;
			ShaderComponentSymbol * currentComp = nullptr;
			ComponentSyntaxNode * currentCompNode = nullptr;
			List<SyntaxNode *> loops;
			SymbolTable * symbolTable;
		public:
			SemanticsVisitor(SymbolTable * symbols, ErrorWriter * pErr)
				:SyntaxVisitor(pErr), symbolTable(symbols)
			{
			}
			// return true if world0 depends on world1 (there exists a series of import operators that converts world1 variables to world0)
			bool IsWorldDependent(PipelineSymbol * pipeline, String world0, String world1)
			{
				HashSet<String> depWorldsSet;
				List<String> depWorlds;
				depWorlds.Add(world0);
				for (int i = 0; i < depWorlds.Count(); i++)
				{
					auto & dep = pipeline->WorldDependency[world0].GetValue();
					if (dep.Contains(world1))
						return true;
					else
					{
						for (auto w : dep)
							if (depWorldsSet.Add(w))
								depWorlds.Add(w);
					}
				}
				return false;
			}
		public:
			// Translate Types
			RefPtr<ExpressionType> typeResult;
			RefPtr<ExpressionType> TranslateTypeNode(const RefPtr<TypeSyntaxNode> & node)
			{
				node->Accept(this);
				return typeResult;
			}
			RefPtr<TypeSyntaxNode> VisitBasicType(BasicTypeSyntaxNode * typeNode) override
			{
				RefPtr<BasicExpressionType> expType = new BasicExpressionType();
				if (typeNode->TypeName == L"int")
					expType->BaseType = BaseType::Int;
				else if (typeNode->TypeName == L"uint")
					expType->BaseType = BaseType::UInt;
				else if (typeNode->TypeName == L"float" || typeNode->TypeName == L"half")
					expType->BaseType = BaseType::Float;
				else if (typeNode->TypeName == L"ivec2" || typeNode->TypeName == L"int2")
					expType->BaseType = BaseType::Int2;
				else if (typeNode->TypeName == L"ivec3" || typeNode->TypeName == L"int3")
					expType->BaseType = BaseType::Int3;
				else if (typeNode->TypeName == L"ivec4" || typeNode->TypeName == L"int4")
					expType->BaseType = BaseType::Int4;
				else if (typeNode->TypeName == L"uvec2" || typeNode->TypeName == L"uint2")
					expType->BaseType = BaseType::UInt2;
				else if (typeNode->TypeName == L"uvec3" || typeNode->TypeName == L"uint3")
					expType->BaseType = BaseType::UInt3;
				else if (typeNode->TypeName == L"uvec4" || typeNode->TypeName == L"uint4")
					expType->BaseType = BaseType::UInt4;
				else if (typeNode->TypeName == L"vec2" || typeNode->TypeName == L"float2" || typeNode->TypeName == L"half2")
					expType->BaseType = BaseType::Float2;
				else if (typeNode->TypeName == L"vec3" || typeNode->TypeName == L"float3" || typeNode->TypeName == L"half3")
					expType->BaseType = BaseType::Float3;
				else if (typeNode->TypeName == L"vec4" || typeNode->TypeName == L"float4" || typeNode->TypeName == L"half4")
					expType->BaseType = BaseType::Float4;
				else if (typeNode->TypeName == L"mat3" || typeNode->TypeName == L"mat3x3" || typeNode->TypeName == L"float3x3" || typeNode->TypeName == L"half3x3")
					expType->BaseType = BaseType::Float3x3;
				else if (typeNode->TypeName == L"mat4" || typeNode->TypeName == L"mat4x4" || typeNode->TypeName == L"float4x4" || typeNode->TypeName == L"half4x4")
					expType->BaseType = BaseType::Float4x4;
				else if (typeNode->TypeName == L"sampler2D")
					expType->BaseType = BaseType::Texture2D;
				else if (typeNode->TypeName == L"samplerCube")
					expType->BaseType = BaseType::TextureCube;
				else if (typeNode->TypeName == L"sampler2DShadow")
					expType->BaseType = BaseType::TextureShadow;
				else if (typeNode->TypeName == L"samplerCubeShadow")
					expType->BaseType = BaseType::TextureCubeShadow;
				else if (typeNode->TypeName == L"void")
					expType->BaseType = BaseType::Void;
				else if (typeNode->TypeName == L"bool")
					expType->BaseType = BaseType::Bool;
				else
				{
					expType->BaseType = BaseType::Struct;
					RefPtr<StructSymbol> ssym;
					if (symbolTable->Structs.TryGetValue(typeNode->TypeName, ssym))
					{
						expType->Struct = ssym.Ptr();
					}
					else if (currentPipeline || currentShader)
					{
						PipelineSymbol * pipe = currentPipeline ? currentPipeline : currentShader->Pipeline;
						if (pipe)
						{
							if (pipe->Worlds.ContainsKey(typeNode->TypeName))
							{
								expType->BaseType = BaseType::Record;
								expType->RecordTypeName = typeNode->TypeName;
							}
							else
								Error(31040, L"undefined type name: '" + typeNode->TypeName + L"'.", typeNode);
						}
						else
							typeResult = ExpressionType::Error;
					}
					else
					{
						Error(31040, L"undefined type name: '" + typeNode->TypeName + L"'.", typeNode);
						typeResult = ExpressionType::Error;
						return typeNode;
					}
				}
				typeResult = expType;
				return typeNode;
			}
			RefPtr<TypeSyntaxNode> VisitArrayType(ArrayTypeSyntaxNode * typeNode) override
			{
				RefPtr<ArrayExpressionType> rs = new ArrayExpressionType();
				rs->ArrayLength = typeNode->ArrayLength;
				typeNode->BaseType->Accept(this);
				rs->BaseType = typeResult;
				typeResult = rs;
				return typeNode;
			}
			RefPtr<TypeSyntaxNode> VisitGenericType(GenericTypeSyntaxNode * typeNode) override
			{
				RefPtr<GenericExpressionType> rs = new GenericExpressionType();
				typeNode->BaseType->Accept(this);
				rs->BaseType = typeResult;
				rs->GenericTypeName = typeNode->GenericTypeName;
				typeResult = rs;
				return typeNode;
			}
		public:
			RefPtr<PipelineSyntaxNode> VisitPipeline(PipelineSyntaxNode * pipeline)
			{
				RefPtr<PipelineSymbol> psymbol = new PipelineSymbol();
				psymbol->SyntaxNode = pipeline;
				currentPipeline = psymbol.Ptr();
				symbolTable->Pipelines.Add(pipeline->Name.Content, psymbol);
				for (auto world : pipeline->Worlds)
				{
					WorldSymbol worldSym;
					worldSym.IsAbstract = world->IsAbstract;
					worldSym.SyntaxNode = world.Ptr();
					if (!psymbol->Worlds.ContainsKey(world->Name.Content))
					{
						psymbol->Worlds.Add(world->Name.Content, worldSym);
						psymbol->WorldDependency.Add(world->Name.Content, EnumerableHashSet<String>());
						psymbol->ReachableWorlds.Add(world->Name.Content, EnumerableHashSet<String>());
						psymbol->ImplicitWorldDependency.Add(world->Name.Content, EnumerableHashSet<String>());
						psymbol->ImplicitlyReachableWorlds.Add(world->Name.Content, EnumerableHashSet<String>());

					}
					else
					{
						Error(33001, L"world \'" + world->Name.Content + L"\' is already defined.", world.Ptr());
					}
				}
				for (auto comp : pipeline->AbstractComponents)
				{
					comp->Type = TranslateTypeNode(comp->TypeNode);
					if (comp->IsParam || comp->IsInput || (comp->Rate && comp->Rate->Worlds.Count() == 1
						&& psymbol->IsAbstractWorld(comp->Rate->Worlds.First().World.Content)))
						AddNewComponentSymbol(psymbol->Components, comp);
					else
						Error(33003, L"cannot define components in a pipeline.",
							comp.Ptr());
				}
				for (auto & op : pipeline->ImportOperators)
				{
					if (auto list = psymbol->ImportOperators.TryGetValue(op->Name.Content))
					{
						list->Add(op);
					}
					else
					{
						List<RefPtr<ImportOperatorDefSyntaxNode>> nlist;
						nlist.Add(op);
						psymbol->ImportOperators[op->Name.Content] = nlist;
						for (auto & param : op->Parameters)
							param->Type = TranslateTypeNode(param->TypeNode);
					}
				}
				// add initial world dependency edges
				for (auto op : pipeline->ImportOperators)
				{
					if (!psymbol->WorldDependency.ContainsKey(op->DestWorld.Content))
						Error(33004, L"undefined world name '" + op->DestWorld.Content + L"'.", op->DestWorld);
					else
					{
						if (psymbol->Worlds[op->DestWorld.Content].GetValue().IsAbstract)
							Error(33005, L"abstract world cannot appear as target as an import operator.", op->DestWorld);
						else if (!psymbol->WorldDependency.ContainsKey(op->SourceWorld.Content))
							Error(33006, L"undefined world name '" + op->SourceWorld.Content + L"'.", op->SourceWorld);
						else
						{
							if (IsWorldDependent(psymbol.Ptr(), op->SourceWorld.Content, op->DestWorld.Content))
							{
								Error(33007, L"import operator '" + op->Name.Content + L"' creates a circular dependency between world '" + op->SourceWorld.Content + L"' and '" + op->DestWorld.Content + L"'",
									op->Name);
							}
							else
							{
								psymbol->WorldDependency[op->DestWorld.Content].GetValue().Add(op->SourceWorld.Content);
								if (op->Parameters.Count() == 0)
									psymbol->ImplicitWorldDependency[op->DestWorld.Content].GetValue().Add(op->SourceWorld.Content);

							}
						}
					}
					
				}
				// propagate world dependency graph
				bool changed = true;
				while (changed)
				{
					changed = false;
					for (auto world : pipeline->Worlds)
					{
						EnumerableHashSet<String> & dependentWorlds = psymbol->WorldDependency[world->Name.Content].GetValue();
						List<String> loopRange;
						for (auto w : dependentWorlds)
							loopRange.Add(w);
						for (auto w : loopRange)
						{
							EnumerableHashSet<String> & ddw = psymbol->WorldDependency[w].GetValue();
							for (auto ww : ddw)
							{
								if (!dependentWorlds.Contains(ww))
								{
									dependentWorlds.Add(ww);
									changed = true;
								}
							}
						}
					}
				}
				changed = true;
				while (changed)
				{
					changed = false;
					for (auto world : pipeline->Worlds)
					{
						EnumerableHashSet<String> & dependentWorlds = psymbol->ImplicitWorldDependency[world->Name.Content].GetValue();
						List<String> loopRange;
						for (auto w : dependentWorlds)
							loopRange.Add(w);
						for (auto w : loopRange)
						{
							EnumerableHashSet<String> & ddw = psymbol->ImplicitWorldDependency[w].GetValue();
							for (auto ww : ddw)
							{
								if (!dependentWorlds.Contains(ww))
								{
									dependentWorlds.Add(ww);
									changed = true;
								}
							}
						}
					}
				}
				// fill in reachable worlds
				for (auto world : psymbol->Worlds)
				{
					if (auto depWorlds = psymbol->WorldDependency.TryGetValue(world.Key))
					{
						for (auto & dep : *depWorlds)
						{
							psymbol->ReachableWorlds[dep].GetValue().Add(world.Key);
						}
					}
					if (auto depWorlds = psymbol->ImplicitWorldDependency.TryGetValue(world.Key))
					{
						for (auto & dep : *depWorlds)
						{
							psymbol->ImplicitlyReachableWorlds[dep].GetValue().Add(world.Key);
						}
					}
				}

				for (auto & op : pipeline->ImportOperators)
				{
					currentImportOperator = op.Ptr();
					HashSet<String> paraNames;
					for (auto & para : op->Parameters)
					{
						if (paraNames.Contains(para->Name))
							Error(30002, L"parameter \'" + para->Name + L"\' already defined.", para.Ptr());
						else
							paraNames.Add(para->Name);
						VariableEntry varEntry;
						varEntry.Name = para->Name;
						para->Type = TranslateTypeNode(para->TypeNode);
						varEntry.Type.DataType = para->Type;
						op->Scope->Variables.AddIfNotExists(varEntry.Name, varEntry);
						if (varEntry.Type.DataType->Equals(ExpressionType::Void.Ptr()))
							Error(30016, L"'void' can not be parameter type.", para.Ptr());
					}
					op->Body->Accept(this);
					currentImportOperator = nullptr;
				}
				currentPipeline = nullptr;
				return pipeline;
			}

			virtual RefPtr<ImportSyntaxNode> VisitImport(ImportSyntaxNode * import) override
			{
				RefPtr<ShaderSymbol> refShader;
				symbolTable->Shaders.TryGetValue(import->ShaderName.Content, refShader);
				if (refShader)
				{
					// type check
					List<ShaderComponentSymbol*> paramList;
					for (auto & comp : refShader->Components)
						if (comp.Value->IsParam())
							paramList.Add(comp.Value.Ptr());
					int position = 0;
					bool namedArgumentAppeared = false;
					for (auto & arg : import->Arguments)
					{
						if (arg->ArgumentName.Content.Length())
							namedArgumentAppeared = true;
						else
						{
							if (namedArgumentAppeared)
							{
								Error(33030, L"positional argument cannot appear after a named argument.", arg->Expression.Ptr());
								break;
							}
							if (position >= paramList.Count())
							{
								Error(33031, L"too many arguments.", arg->Expression.Ptr());
								break;
							}
							arg->ArgumentName.Content = paramList[position]->Name;
							arg->ArgumentName.Position = arg->Position;
						}
						position++;
						arg->Accept(this);
						RefPtr<ShaderComponentSymbol> refComp;
						if (refShader->Components.TryGetValue(arg->ArgumentName.Content, refComp))
						{
							if (!refComp->Type->DataType->Equals(arg->Expression->Type.Ptr()))
							{
								Error(33027, L"argument type (" + arg->Expression->Type->ToString() + L") does not match parameter type (" + refComp->Type->DataType->ToString() + L")", arg->Expression.Ptr());
							}
							if (!refComp->IsParam())
								Error(33028, L"'" + arg->ArgumentName.Content + L"' is not a parameter of '" + import->ShaderName.Content + L"'.", arg->ArgumentName);
						}
						else
							Error(33028, L"'" + arg->ArgumentName.Content + L"' is not a parameter of '" + import->ShaderName.Content + L"'.", arg->ArgumentName);
					}
				}
				return import;
			}

			class ShaderImportVisitor : public SyntaxVisitor
			{
			private:
				SymbolTable * symbolTable = nullptr;
				ShaderSymbol * currentShader = nullptr;
				ShaderComponentSymbol * currentComp = nullptr;
			public:
				ShaderImportVisitor(ErrorWriter * writer, SymbolTable * symTable)
					: SyntaxVisitor(writer), symbolTable(symTable)
				{}
				virtual RefPtr<ShaderSyntaxNode> VisitShader(ShaderSyntaxNode * shader) override
				{
					currentShader = symbolTable->Shaders[shader->Name.Content].GetValue().Ptr();
					SyntaxVisitor::VisitShader(shader);
					currentShader = nullptr;
					return shader;
				}
				virtual RefPtr<ComponentSyntaxNode> VisitComponent(ComponentSyntaxNode * comp) override
				{
					RefPtr<ShaderComponentSymbol> compSym;
					currentShader->Components.TryGetValue(comp->Name.Content, compSym);
					currentComp = compSym.Ptr();
					SyntaxVisitor::VisitComponent(comp);
					if (!compSym->IsParam() && (compSym->Type->DataType->IsArray() || compSym->Type->DataType->IsStruct() ||
						compSym->Type->DataType->IsTexture()))
					{
						bool valid = true;
						bool isInStorageBuffer = true;
						if (comp->Rate)
						{
							for (auto & w : comp->Rate->Worlds)
							{
								auto world = currentShader->Pipeline->Worlds.TryGetValue(w.World.Content);
								if (world)
								{
									if (!world->IsAbstract)
										valid = false;
									isInStorageBuffer = isInStorageBuffer && world->SyntaxNode->LayoutAttributes.ContainsKey(L"ShaderStorageBlock");
								}
							}
						}
						else 
							valid = false;
						if (!valid)
						{
							Error(33035, L"\'" + compSym->Name + L"\': sampler, struct and array types only allowed in input worlds.", comp->Name);
						}
						else
						{
							auto basicType = compSym->Type->DataType->AsBasicType();

							if (!isInStorageBuffer && basicType && basicType->Struct != nullptr)
							{
								Error(33036, L"\'" + compSym->Name + L"\': struct must only be defined in a input world with [ShaderStorageBlock] attribute.", comp->Name);
							}
						}
					}
					currentComp = nullptr;
					return comp;
				}
				virtual RefPtr<ImportSyntaxNode> VisitImport(ImportSyntaxNode * import) override
				{
					RefPtr<ShaderSymbol> refShader;
					symbolTable->Shaders.TryGetValue(import->ShaderName.Content, refShader);
					if (!refShader)
						Error(33015, L"undefined identifier \'" + import->ShaderName.Content + L"\'.", import->ShaderName);
					currentShader->DependentShaders.Add(refShader.Ptr());
					if (!currentComp)
					{
						ShaderUsing su;
						su.Shader = refShader.Ptr();
						su.IsPublic = import->IsPublic;
						if (import->IsInplace)
						{
							currentShader->ShaderUsings.Add(su);
						}
						else
						{
							if (currentShader->ShaderObjects.ContainsKey(import->ObjectName.Content) ||
								currentShader->Components.ContainsKey(import->ObjectName.Content))
							{
								Error(33018, L"\'" + import->ShaderName.Content + L"\' is already defined.", import->ShaderName);
							}
							currentShader->ShaderObjects[import->ObjectName.Content] = su;
						}
					}
					if (currentComp)
						Error(33016, L"'using': importing not allowed in component definition.", import->ShaderName);
					return import;
				}
			};

			// pass 1: fill components in shader symbol table
			void VisitShaderPass1(ShaderSyntaxNode * shader)
			{
				HashSet<String> inheritanceSet;
				auto curShader = shader;
				inheritanceSet.Add(curShader->Name.Content);
				auto & shaderSymbol = symbolTable->Shaders[curShader->Name.Content].GetValue();
				this->currentShader = shaderSymbol.Ptr();
				if (shader->Pipeline.Content.Length() == 0) // implicit pipeline
				{
					if (program->Pipelines.Count() == 1)
					{
						shader->Pipeline = shader->Name; // get line and col from shader name
						shader->Pipeline.Content = program->Pipelines.First()->Name.Content;
					}
					else
					{
						// current compilation context has more than one pipeline defined,
						// in which case we do not allow implicit pipeline specification
						Error(33002, L"explicit pipeline specification required for shader '" +
							shader->Name.Content + L"' because multiple pipelines are defined in current context.", curShader->Name);
					}
				}
				
				auto pipelineName = shader->Pipeline.Content;
				auto pipeline = symbolTable->Pipelines.TryGetValue(pipelineName);
				if (pipeline)
					shaderSymbol->Pipeline = pipeline->Ptr();
				else
				{
					Error(33010, L"pipeline \'" + pipelineName + L"' is not defined.", shader->Pipeline);
					throw 0;
				}
				if (shader->IsModule)
					shaderSymbol->IsAbstract = true;
				// add components to symbol table
				for (auto & mbr : shader->Members)
				{
					if (auto comp = dynamic_cast<ComponentSyntaxNode*>(mbr.Ptr()))
					{
						comp->Type = TranslateTypeNode(comp->TypeNode);
						if (comp->IsParam)
						{
							shaderSymbol->IsAbstract = true;
							if (!shaderSymbol->SyntaxNode->IsModule)
							{
								Error(33009, L"parameters can only be defined in modules.", shaderSymbol->SyntaxNode);
							}
						}
						AddNewComponentSymbol(shaderSymbol->Components, comp);
					}
				}
				// add shader objects to symbol table
				ShaderImportVisitor importVisitor(err, symbolTable);
				shader->Accept(&importVisitor);

				for (auto & comp : shaderSymbol->Components)
				{
					for (auto & impl : comp.Value->Implementations)
					{
						bool inAbstractWorld = false;
						if (impl->SyntaxNode->Rate)
						{
							auto & userSpecifiedWorlds = impl->SyntaxNode->Rate->Worlds;
							for (auto & world : userSpecifiedWorlds)
							{
								if (!shaderSymbol->Pipeline->WorldDependency.ContainsKey(world.World.Content))
									Error(33012, L"\'" + world.World.Content + L"' is not a defined world in '" +
										pipelineName + L"'.", world.World);
								WorldSymbol worldSym;

								if (shaderSymbol->Pipeline->Worlds.TryGetValue(world.World.Content, worldSym))
								{
									if (worldSym.IsAbstract)
									{
										inAbstractWorld = true;
										if (userSpecifiedWorlds.Count() > 1)
										{
											Error(33013, L"abstract world cannot appear with other worlds.",
												world.World);
										}
									}
								}
							}
						}
						if (!inAbstractWorld && !impl->SyntaxNode->IsParam
							&& !impl->SyntaxNode->Expression && !impl->SyntaxNode->BlockStatement)
						{
							Error(33014, L"non-abstract component must have an implementation.",
								impl->SyntaxNode.Ptr());
						}
					}
				}
				this->currentShader = nullptr;
			}
			// pass 2: type checking component definitions
			void VisitShaderPass2(ShaderSyntaxNode * shaderNode)
			{
				RefPtr<ShaderSymbol> shaderSym;
				if (!symbolTable->Shaders.TryGetValue(shaderNode->Name.Content, shaderSym))
					return;
				this->currentShader = shaderSym.Ptr();
				for (auto & comp : shaderNode->Members)
				{
					comp->Accept(this);
				}
				this->currentShader = nullptr;
			}

			bool MatchType_RecordType(String recTypeName, ExpressionType * valueType)
			{
				if (valueType->IsGenericType(L"Uniform") || valueType->IsGenericType(L"Patch"))
				{
					valueType = valueType->AsGenericType()->BaseType.Ptr();
				}
				if (auto basicType = valueType->AsBasicType())
					return basicType->RecordTypeName == recTypeName;
				return false;
			}

			bool MatchType_ValueReceiver(ExpressionType * receiverType, ExpressionType * valueType)
			{
				if (receiverType->Equals(valueType))
					return true;
				if (receiverType->IsIntegral() && valueType->Equals(ExpressionType::Int.Ptr()))
					return true;
				if (receiverType->Equals(ExpressionType::Float.Ptr()) && valueType->IsIntegral())
					return true;
				if (receiverType->IsVectorType() && valueType->IsVectorType())
				{
					auto recieverBasicType = receiverType->AsBasicType();
					auto valueBasicType = valueType->AsBasicType();
					if (GetVectorBaseType(recieverBasicType->BaseType) == BaseType::Float &&
						GetVectorSize(recieverBasicType->BaseType) == GetVectorSize(valueBasicType->BaseType))
						return true;
					if (GetVectorBaseType(recieverBasicType->BaseType) == BaseType::UInt &&
						GetVectorBaseType(valueBasicType->BaseType) == BaseType::Int &&
						GetVectorSize(recieverBasicType->BaseType) == GetVectorSize(valueBasicType->BaseType))
						return true;
				}
				return false;
			}
			virtual RefPtr<ComponentSyntaxNode> VisitComponent(ComponentSyntaxNode * comp) override
			{
				this->currentCompNode = comp;
				RefPtr<ShaderComponentSymbol> compSym;
				currentShader->Components.TryGetValue(comp->Name.Content, compSym);
				this->currentComp = compSym.Ptr();
				if (comp->Expression)
				{
					comp->Expression = comp->Expression->Accept(this).As<ExpressionSyntaxNode>();
					if (!MatchType_ValueReceiver(compSym->Type->DataType.Ptr(), comp->Expression->Type.Ptr()) && 
						!comp->Expression->Type->Equals(ExpressionType::Error.Ptr()))
						Error(30019, L"type mismatch \'" + comp->Expression->Type->ToString() + L"\' and \'" +
							currentComp->Type->DataType->ToString() + L"\'", comp->Name);
				}
				if (comp->BlockStatement)
					comp->BlockStatement->Accept(this);
				this->currentComp = nullptr;
				this->currentCompNode = nullptr;
				return comp;
			}
			virtual RefPtr<StatementSyntaxNode> VisitImportStatement(ImportStatementSyntaxNode * importStmt) override
			{
				importStmt->Import->Accept(this);
				return importStmt;
			}
			void AddNewComponentSymbol(EnumerableDictionary<String, RefPtr<ShaderComponentSymbol>> & components, RefPtr<ComponentSyntaxNode> comp)
			{
				RefPtr<ShaderComponentSymbol> compSym;
				RefPtr<ShaderComponentImplSymbol> compImpl = new ShaderComponentImplSymbol();
				if (comp->Rate)
					for (auto w : comp->Rate->Worlds)
						compImpl->Worlds.Add(w.World.Content);
				compImpl->SyntaxNode = comp;
				if (compImpl->SyntaxNode->Rate)
				{
					for (auto & w : compImpl->SyntaxNode->Rate->Worlds)
						if (w.Pinned)
							compImpl->SrcPinnedWorlds.Add(w.World.Content);
				}
				if (compImpl->SyntaxNode->AlternateName.Type == TokenType::Identifier)
				{
					compImpl->AlternateName = compImpl->SyntaxNode->AlternateName.Content;
				}
				if (compImpl->SyntaxNode->IsOutput)
				{
					if (compImpl->SyntaxNode->Rate)
					{
						for (auto & w : compImpl->SyntaxNode->Rate->Worlds)
							compImpl->ExportWorlds.Add(w.World.Content);
					}
					else
					{
						Error(33019, L"component \'" + compImpl->SyntaxNode->Name.Content + L"\': definition marked as 'export' must have an explicitly specified world.",
							compImpl->SyntaxNode.Ptr());
					}
				}
				if (!components.TryGetValue(comp->Name.Content, compSym))
				{
					compSym = new ShaderComponentSymbol();
					compSym->Type = new Type();
					compSym->Name = comp->Name.Content;
					compSym->Type->DataType = comp->Type;
					components.Add(comp->Name.Content, compSym);
				}
				else
				{
					if (comp->IsParam)
						Error(33029, L"\'" + compImpl->SyntaxNode->Name.Content + L"\': requirement clash with previous definition.",
							compImpl->SyntaxNode.Ptr());
					symbolTable->CheckComponentImplementationConsistency(err, compSym.Ptr(), compImpl.Ptr());
				}
				compSym->Implementations.Add(compImpl);
			}
			virtual RefPtr<ProgramSyntaxNode> VisitProgram(ProgramSyntaxNode * programNode) override
			{
				HashSet<String> funcNames;
				this->program = programNode;
				this->function = nullptr;
				for (auto & s : program->Structs)
				{
					RefPtr<StructSymbol> ssym = new StructSymbol();
					ssym->Name = s->Name.Content;
					ssym->SyntaxNode = s;
					ssym->Type = new ILStructType();
					symbolTable->Structs.Add(s->Name.Content, ssym);
				}
				for (auto & s : program->Structs)
					VisitStruct(s.Ptr());
				for (auto & pipeline : program->Pipelines)
				{
					VisitPipeline(pipeline.Ptr());
				}
				for (auto & func : program->Functions)
				{
					VisitFunctionDeclaration(func.Ptr());
					if (funcNames.Contains(func->InternalName))
					{
						StringBuilder argList;
						argList << L"(";
						for (auto & param : func->Parameters)
						{
							argList << param->Type->ToString();
							if (param != func->Parameters.Last())
								argList << L", ";
						}
						argList << L")";
						Error(30001, L"function \'" + func->Name + argList.ProduceString() + L"\' redefinition.", func.Ptr());
					}
					else
						funcNames.Add(func->InternalName);
				}
				for (auto & func : program->Functions)
				{
					func->Accept(this);
				}
				// build initial symbol table for shaders
				for (auto & shader : program->Shaders)
				{
					RefPtr<ShaderSymbol> shaderSym = new ShaderSymbol();
					shaderSym->SyntaxNode = shader.Ptr();
					if (symbolTable->Shaders.ContainsKey(shader->Name.Content))
					{
						Error(33018, L"shader '" + shader->Name.Content + "' has already been defined.", shader->Name);
					}
					symbolTable->Shaders[shader->Name.Content] = shaderSym;
				}
				HashSet<ShaderSyntaxNode*> validShaders;
				for (auto & shader : program->Shaders)
				{
					int lastErrorCount = err->GetErrorCount();
					VisitShaderPass1(shader.Ptr());
					if (err->GetErrorCount() == lastErrorCount)
						validShaders.Add(shader.Ptr());
				}
				if (err->GetErrorCount() != 0)
					return programNode;
				// shader dependency is discovered in pass 1, we can now sort the shaders
				if (!symbolTable->SortShaders())
				{
					HashSet<ShaderSymbol*> sortedShaders;
					for (auto & shader : symbolTable->ShaderDependenceOrder)
						sortedShaders.Add(shader);
					for (auto & shader : symbolTable->Shaders)
						if (!sortedShaders.Contains(shader.Value.Ptr()))
						{
							Error(33011, L"shader '" + shader.Key + L"' involves circular reference.", shader.Value->SyntaxNode->Name);
						}
				}

				for (auto & shader : symbolTable->ShaderDependenceOrder)
				{
					if (!validShaders.Contains(shader->SyntaxNode))
						continue;
					int lastErrorCount = err->GetErrorCount();
					VisitShaderPass2(shader->SyntaxNode);
					if (err->GetErrorCount() != lastErrorCount)
						validShaders.Remove(shader->SyntaxNode);
				}
				// update symbol table with only valid shaders
				EnumerableDictionary<String, RefPtr<ShaderSymbol>> newShaderSymbols;
				for (auto & shader : symbolTable->Shaders)
				{
					if (validShaders.Contains(shader.Value->SyntaxNode))
						newShaderSymbols.AddIfNotExists(shader.Key, shader.Value);
				}
				symbolTable->Shaders = _Move(newShaderSymbols);
				return programNode;
			}

			virtual RefPtr<StructSyntaxNode> VisitStruct(StructSyntaxNode * structNode) override
			{
				RefPtr<StructSymbol> st;
				if (symbolTable->Structs.TryGetValue(structNode->Name.Content, st))
				{
					st->Type->TypeName = structNode->Name.Content;
					for (auto node : structNode->Fields)
					{
						node->Type = TranslateTypeNode(node->TypeNode);
						ILStructType::ILStructField f;
						f.FieldName = node->Name.Content;
						f.Type = TranslateExpressionType(node->Type.Ptr());
						st->Type->Members.Add(f);
					}
				}
				return structNode;
			}

			virtual RefPtr<FunctionSyntaxNode> VisitFunction(FunctionSyntaxNode *functionNode) override
			{
				if (!functionNode->IsExtern)
				{
					currentFunc = symbolTable->Functions.TryGetValue(functionNode->InternalName)->Ptr();
					this->function = functionNode;
					functionNode->Body->Accept(this);
					this->function = NULL;
					currentFunc = nullptr;
				}
				return functionNode;
			}

			void VisitFunctionDeclaration(FunctionSyntaxNode *functionNode)
			{
				this->function = functionNode;
				auto returnType = TranslateTypeNode(functionNode->ReturnTypeNode);
				functionNode->ReturnType = returnType;
				StringBuilder internalName;
				internalName << functionNode->Name;
				HashSet<String> paraNames;
				for (auto & para : functionNode->Parameters)
				{
					if (paraNames.Contains(para->Name))
						Error(30002, L"parameter \'" + para->Name + L"\' already defined.", para.Ptr());
					else
						paraNames.Add(para->Name);
					VariableEntry varEntry;
					varEntry.Name = para->Name;
					para->Type = TranslateTypeNode(para->TypeNode);
					varEntry.Type.DataType = para->Type;
					functionNode->Scope->Variables.AddIfNotExists(varEntry.Name, varEntry);
					if (varEntry.Type.DataType->Equals(ExpressionType::Void.Ptr()))
						Error(30016, L"'void' can not be parameter type.", para.Ptr());
					internalName << L"@" << varEntry.Type.DataType->ToString();
				}
				functionNode->InternalName = internalName.ProduceString();	
				RefPtr<FunctionSymbol> symbol = new FunctionSymbol();
				symbol->SyntaxNode = functionNode;
				symbolTable->Functions[functionNode->InternalName] = symbol;
				this->function = NULL;
			}
			
			virtual RefPtr<StatementSyntaxNode> VisitBlockStatement(BlockStatementSyntaxNode *stmt) override
			{
				for (auto & node : stmt->Statements)
				{
					node->Accept(this);
				}
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitBreakStatement(BreakStatementSyntaxNode *stmt) override
			{
				if (!loops.Count())
					Error(30003, L"'break' must appear inside loop constructs.", stmt);
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitContinueStatement(ContinueStatementSyntaxNode *stmt) override
			{
				if (!loops.Count())
					Error(30004, L"'continue' must appear inside loop constructs.", stmt);
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitDoWhileStatement(DoWhileStatementSyntaxNode *stmt) override
			{
				loops.Add(stmt);
				if (stmt->Predicate != NULL)
					stmt->Predicate = stmt->Predicate->Accept(this).As<ExpressionSyntaxNode>();
				if (!stmt->Predicate->Type->Equals(ExpressionType::Error.Ptr()) && 
					!stmt->Predicate->Type->Equals(ExpressionType::Int.Ptr()) &&
					!stmt->Predicate->Type->Equals(ExpressionType::Bool.Ptr()))
					Error(30005, L"'while': expression must evaluate to int.", stmt);
				stmt->Statement->Accept(this);

				loops.RemoveAt(loops.Count() - 1);
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitForStatement(ForStatementSyntaxNode *stmt) override
			{
				loops.Add(stmt);
				VariableEntry iterVar;
				if (stmt->TypeDef != nullptr)
				{
					stmt->IterationVariableType = TranslateTypeNode(stmt->TypeDef);
					VariableEntry varEntry;
					varEntry.IsComponent = false;
					varEntry.Name = stmt->IterationVariable.Content;
					varEntry.Type.DataType = stmt->IterationVariableType;
					stmt->Scope->Variables.AddIfNotExists(stmt->IterationVariable.Content, varEntry);
				}
				if (!stmt->Scope->FindVariable(stmt->IterationVariable.Content, iterVar))
					Error(30015, L"undefined identifier \'" + stmt->IterationVariable.Content + L"\'", stmt->IterationVariable);
				else
				{
					if (!iterVar.Type.DataType->Equals(ExpressionType::Float.Ptr()) && !iterVar.Type.DataType->Equals(ExpressionType::Int.Ptr()))
						Error(30035, L"iteration variable \'" + stmt->IterationVariable.Content + L"\' can only be a int or float", stmt->IterationVariable);
					stmt->InitialExpression = stmt->InitialExpression->Accept(this).As<ExpressionSyntaxNode>();
					if (stmt->InitialExpression->Type != iterVar.Type.DataType)
						Error(30019, L"type mismatch \'" + stmt->InitialExpression->Type->ToString() + L"\' and \'" +
							iterVar.Type.DataType->ToString() + L"\'", stmt->InitialExpression.Ptr());
					stmt->EndExpression = stmt->EndExpression->Accept(this).As<ExpressionSyntaxNode>();
					if (stmt->EndExpression->Type != iterVar.Type.DataType)
						Error(30019, L"type mismatch \'" + stmt->EndExpression->Type->ToString() + L"\' and \'" +
							iterVar.Type.DataType->ToString() + L"\'", stmt->EndExpression.Ptr());
					if (stmt->StepExpression != nullptr)
					{
						stmt->StepExpression = stmt->StepExpression->Accept(this).As<ExpressionSyntaxNode>();
						if (stmt->StepExpression->Type != iterVar.Type.DataType)
							Error(30019, L"type mismatch \'" + stmt->StepExpression->Type->ToString() + L"\' and \'" +
								iterVar.Type.DataType->ToString() + L"\'", stmt->StepExpression.Ptr());
					}
				}

				stmt->Statement->Accept(this);

				loops.RemoveAt(loops.Count() - 1);
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitIfStatement(IfStatementSyntaxNode *stmt) override
			{
				if (stmt->Predicate != NULL)
					stmt->Predicate = stmt->Predicate->Accept(this).As<ExpressionSyntaxNode>();
				if (!stmt->Predicate->Type->Equals(ExpressionType::Error.Ptr()) 
					&& (!stmt->Predicate->Type->Equals(ExpressionType::Int.Ptr()) && 
						!stmt->Predicate->Type->Equals(ExpressionType::Bool.Ptr())))
					Error(30006, L"'if': expression must evaluate to int.", stmt);

				if (stmt->PositiveStatement != NULL)
					stmt->PositiveStatement->Accept(this);
				
				if (stmt->NegativeStatement != NULL)
					stmt->NegativeStatement->Accept(this);
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitReturnStatement(ReturnStatementSyntaxNode *stmt) override
			{
				if (currentCompNode && currentCompNode->BlockStatement->Statements.Count() &&
					stmt != currentCompNode->BlockStatement->Statements.Last().Ptr())
				{
					Error(30026, L"'return' can only appear as the last statement in component definition.", stmt);
				}
				if (!stmt->Expression)
				{
					if (function && !function->ReturnType->Equals(ExpressionType::Void.Ptr()))
						Error(30006, L"'return' should have an expression.", stmt);
				}
				else
				{
					stmt->Expression = stmt->Expression->Accept(this).As<ExpressionSyntaxNode>();
					if (!stmt->Expression->Type->Equals(ExpressionType::Error.Ptr()))
					{
						if (function && !MatchType_ValueReceiver(function->ReturnType.Ptr(), stmt->Expression->Type.Ptr()))
							Error(30007, L"expression type '" + stmt->Expression->Type->ToString()
								+ L"' does not match function's return type '"
								+ function->ReturnType->ToString() + L"'", stmt);
						if (currentComp && !MatchType_ValueReceiver(currentComp->Type->DataType.Ptr(), stmt->Expression->Type.Ptr()))
						{
							Error(30007, L"expression type '" + stmt->Expression->Type->ToString()
								+ L"' does not match component's type '"
								+ currentComp->Type->DataType->ToString() + L"'", stmt);
						}
						if (currentImportOperator && !MatchType_RecordType(currentImportOperator->SourceWorld.Content, stmt->Expression->Type.Ptr()))
							Error(30007, L"expression type '" + stmt->Expression->Type->ToString() + L"' does not match import operator's type '" + currentImportOperator->SourceWorld.Content
								+ L"'.", stmt);
					}
				}
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitVarDeclrStatement(VarDeclrStatementSyntaxNode *stmt) override
			{
				stmt->Type = TranslateTypeNode(stmt->TypeNode);
				if (stmt->Type->IsTexture())
				{
					Error(30033, L"cannot declare a local variable of 'texture' type.", stmt);
				}
				if (stmt->Type->AsGenericType())
				{
					Error(30033, L"cannot declare a local variable of this type.", stmt);
				}
				for (auto & para : stmt->Variables)
				{
					VariableEntry varDeclr;
					varDeclr.Name = para->Name;
					if (stmt->Scope->Variables.ContainsKey(para->Name))
						Error(30008, L"variable " + para->Name + L" already defined.", para.Ptr());

					varDeclr.Type.DataType = stmt->Type;
					if (varDeclr.Type.DataType->Equals(ExpressionType::Void.Ptr()))
						Error(30009, L"invalid type 'void'.", stmt);
					if (varDeclr.Type.DataType->IsArray() && varDeclr.Type.DataType->AsArrayType()->ArrayLength <= 0)
						Error(30025, L"array size must be larger than zero.", stmt);

					stmt->Scope->Variables.AddIfNotExists(para->Name, varDeclr);
					if (para->Expression != NULL)
					{
						para->Expression = para->Expression->Accept(this).As<ExpressionSyntaxNode>();
						if (!MatchType_ValueReceiver(varDeclr.Type.DataType.Ptr(), para->Expression->Type.Ptr())
							&& !para->Expression->Type->Equals(ExpressionType::Error.Ptr()))
						{
							Error(30019, L"type mismatch \'" + para->Expression->Type->ToString() + L"\' and \'" +
								varDeclr.Type.DataType->ToString() + L"\'", para.Ptr());
						}
					}
				}
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitWhileStatement(WhileStatementSyntaxNode *stmt) override
			{
				loops.Add(stmt);
				stmt->Predicate = stmt->Predicate->Accept(this).As<ExpressionSyntaxNode>();
				if (!stmt->Predicate->Type->Equals(ExpressionType::Error.Ptr()) && 
					!stmt->Predicate->Type->Equals(ExpressionType::Int.Ptr()) &&
					!stmt->Predicate->Type->Equals(ExpressionType::Bool.Ptr()))
					Error(30010, L"'while': expression must evaluate to int.", stmt);

				stmt->Statement->Accept(this);
				loops.RemoveAt(loops.Count() - 1);
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitExpressionStatement(ExpressionStatementSyntaxNode *stmt) override
			{
				stmt->Expression = stmt->Expression->Accept(this).As<ExpressionSyntaxNode>();
				return stmt;
			}
			bool MatchType_BinaryImplicit(RefPtr<ExpressionType> & resultType, RefPtr<ExpressionType> &leftType, RefPtr<ExpressionType> &rightType)
			{
				if (leftType->Equals(rightType.Ptr()) && !leftType->IsTexture())
				{
					resultType = leftType;
					return true;
				}
				else if (leftType->IsVectorType() && rightType->AsBasicType() &&
					rightType->AsBasicType()->BaseType == GetVectorBaseType(leftType->AsBasicType()->BaseType))
				{
					resultType = leftType;
					return true;
				}
				else if (rightType->IsVectorType() && leftType->AsBasicType() 
					&& leftType->AsBasicType()->BaseType == GetVectorBaseType(rightType->AsBasicType()->BaseType))
				{
					resultType = rightType;
					return true;
				}
				else if ((rightType->Equals(ExpressionType::Float.Ptr()) && leftType->Equals(ExpressionType::Int.Ptr())) ||
					(leftType->Equals(ExpressionType::Float.Ptr()) && rightType->Equals(ExpressionType::Int.Ptr())))
				{
					resultType = ExpressionType::Float;
					return true;
				}
				else if ((rightType->Equals(ExpressionType::Float2.Ptr()) && leftType->Equals(ExpressionType::Int2.Ptr())) ||
					(leftType->Equals(ExpressionType::Float2.Ptr()) && rightType->Equals(ExpressionType::Int2.Ptr())))
				{
					resultType = ExpressionType::Float2;
					return true;
				}
				else if ((rightType->Equals(ExpressionType::Float3.Ptr()) && leftType->Equals(ExpressionType::Int3.Ptr())) ||
					(leftType->Equals(ExpressionType::Float3.Ptr()) && rightType->Equals(ExpressionType::Int3.Ptr())))
				{
					resultType = ExpressionType::Float3;
					return true;
				}
				else if ((rightType->Equals(ExpressionType::Float4.Ptr()) && leftType->Equals(ExpressionType::Int4.Ptr())) ||
					(leftType->Equals(ExpressionType::Float4.Ptr()) && rightType->Equals(ExpressionType::Int4.Ptr())))
				{
					resultType = ExpressionType::Float4;
					return true;
				}
				else if ((rightType->Equals(ExpressionType::Float.Ptr()) && leftType->Equals(ExpressionType::UInt.Ptr())) ||
					(leftType->Equals(ExpressionType::Float.Ptr()) && rightType->Equals(ExpressionType::UInt.Ptr())))
				{
					resultType = ExpressionType::Float;
					return true;
				}
				else if ((rightType->Equals(ExpressionType::Float2.Ptr()) && leftType->Equals(ExpressionType::UInt2.Ptr())) ||
					(leftType->Equals(ExpressionType::Float2.Ptr()) && rightType->Equals(ExpressionType::UInt2.Ptr())))
				{
					resultType = ExpressionType::Float2;
					return true;
				}
				else if ((rightType->Equals(ExpressionType::Float3.Ptr()) && leftType->Equals(ExpressionType::UInt3.Ptr())) ||
					(leftType->Equals(ExpressionType::Float3.Ptr()) && rightType->Equals(ExpressionType::UInt3.Ptr())))
				{
					resultType = ExpressionType::Float3;
					return true;
				}
				else if ((rightType->Equals(ExpressionType::Float4.Ptr()) && leftType->Equals(ExpressionType::UInt4.Ptr())) ||
					(leftType->Equals(ExpressionType::Float4.Ptr()) && rightType->Equals(ExpressionType::UInt4.Ptr())))
				{
					resultType = ExpressionType::Float4;
					return true;
				}
				else if ((rightType->Equals(ExpressionType::Int.Ptr()) && leftType->Equals(ExpressionType::UInt.Ptr())) ||
					(leftType->Equals(ExpressionType::Int.Ptr()) && rightType->Equals(ExpressionType::UInt.Ptr())))
				{
					resultType = ExpressionType::Int;
					return true;
				}
				else if ((rightType->Equals(ExpressionType::Int2.Ptr()) && leftType->Equals(ExpressionType::UInt2.Ptr())) ||
					(leftType->Equals(ExpressionType::Int2.Ptr()) && rightType->Equals(ExpressionType::UInt2.Ptr())))
				{
					resultType = ExpressionType::Int2;
					return true;
				}
				else if ((rightType->Equals(ExpressionType::Int3.Ptr()) && leftType->Equals(ExpressionType::UInt3.Ptr())) ||
					(leftType->Equals(ExpressionType::Int3.Ptr()) && rightType->Equals(ExpressionType::UInt3.Ptr())))
				{
					resultType = ExpressionType::Int3;
					return true;
				}
				else if ((rightType->Equals(ExpressionType::Int4.Ptr()) && leftType->Equals(ExpressionType::UInt4.Ptr())) ||
					(leftType->Equals(ExpressionType::Int4.Ptr()) && rightType->Equals(ExpressionType::UInt4.Ptr())))
				{
					resultType = ExpressionType::Int4;
					return true;
				}
				else if (leftType->AsBasicType() && leftType->AsBasicType()->BaseType == BaseType::Record)
				{
					resultType = leftType;
					return true;
				}
				else if (rightType->AsBasicType() && rightType->AsBasicType()->BaseType == BaseType::Record)
				{
					resultType = rightType;
					return true;
				}
				return false;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitBinaryExpression(BinaryExpressionSyntaxNode *expr) override
			{
				expr->LeftExpression = expr->LeftExpression->Accept(this).As<ExpressionSyntaxNode>();
				expr->RightExpression = expr->RightExpression->Accept(this).As<ExpressionSyntaxNode>();
				auto & leftType = expr->LeftExpression->Type;
				auto & rightType = expr->RightExpression->Type;
				RefPtr<ExpressionType> matchedType;
				auto checkAssign = [&]()
				{
					if (!(leftType->AsBasicType() && leftType->AsBasicType()->IsLeftValue) &&
						!leftType->Equals(ExpressionType::Error.Ptr()))
						Error(30011, L"left of '=' is not an l-value.", expr->LeftExpression.Ptr());
					if (expr->Operator == Operator::AndAssign ||
						expr->Operator == Operator::OrAssign ||
						expr->Operator == Operator::XorAssign ||
						expr->Operator == Operator::LshAssign ||
						expr->Operator == Operator::RshAssign)
					{
						if (!(leftType->IsIntegral() && rightType->IsIntegral()))
						{
							Error(30041, L"bit operation: operand must be integral type.", expr);
						}
					}
					expr->LeftExpression->Access = ExpressionAccess::Write;
					if (MatchType_ValueReceiver(leftType.Ptr(), expr->Type.Ptr()))
						expr->Type = leftType;
					else
						expr->Type = ExpressionType::Error;
				};
				switch (expr->Operator)
				{
				case Operator::Add:
				case Operator::Sub:
				case Operator::Div:
				case Operator::AddAssign:
				case Operator::DivAssign:
				case Operator::SubAssign:
					if (MatchType_BinaryImplicit(matchedType, leftType, rightType))
						expr->Type = matchedType;
					else
						expr->Type = ExpressionType::Error;
					if (expr->Operator == Operator::AddAssign || expr->Operator == Operator::DivAssign || expr->Operator == Operator::SubAssign)
						checkAssign();
					break;
				case Operator::Mul:
				case Operator::MulAssign:
					if (leftType->AsBasicType() && leftType->AsBasicType()->BaseType != BaseType::Shader)
					{
						auto basicType = leftType->AsBasicType();
						if (MatchType_BinaryImplicit(matchedType, leftType, rightType))
							expr->Type = matchedType;
						else if ((basicType->BaseType == BaseType::Float3x3 && rightType->Equals(ExpressionType::Float3.Ptr())) ||
							(basicType->BaseType == BaseType::Float3 && rightType->AsBasicType() && rightType->AsBasicType()->BaseType == BaseType::Float3x3))
							expr->Type = ExpressionType::Float3;
						else if ((basicType->BaseType == BaseType::Float4x4 && rightType->Equals(ExpressionType::Float4.Ptr())) ||
							(basicType->BaseType == BaseType::Float4 && rightType->AsBasicType() && rightType->AsBasicType()->BaseType == BaseType::Float4x4))
							expr->Type = ExpressionType::Float4;
						else
							expr->Type = ExpressionType::Error;
					}
					else
						expr->Type = ExpressionType::Error;
					if (expr->Operator == Operator::MulAssign)
						checkAssign();
					break;
				case Operator::Mod:
				case Operator::Rsh:
				case Operator::Lsh:
				case Operator::BitAnd:
				case Operator::BitOr:
				case Operator::BitXor:
				case Operator::And:
				case Operator::Or:
				case Operator::ModAssign:
				case Operator::AndAssign:
				case Operator::OrAssign:
				case Operator::XorAssign:
				case Operator::LshAssign:
				case Operator::RshAssign:
					if (leftType->Equals(rightType.Ptr()) && !leftType->IsArray() && !leftType->IsTexture()
						&& !leftType->IsShader() &&
						leftType->AsBasicType() && GetVectorBaseType(leftType->AsBasicType()->BaseType) != BaseType::Float)
						expr->Type = (expr->Operator == Operator::And || expr->Operator == Operator::Or ? ExpressionType::Bool : leftType);
					else if (leftType->IsIntegral() && rightType->IsIntegral())
						expr->Type = leftType;
					else
						expr->Type = ExpressionType::Error;
					if (expr->Operator == Operator::ModAssign || expr->Operator == Operator::AndAssign || expr->Operator == Operator::OrAssign ||
						expr->Operator == Operator::XorAssign || expr->Operator == Operator::LshAssign || expr->Operator == Operator::RshAssign)
						checkAssign();
					break;
				case Operator::Neq:
				case Operator::Eql:
					if (leftType->Equals(rightType.Ptr()) && !leftType->IsArray() && !leftType->IsTexture() && !leftType->IsShader())
						expr->Type = ExpressionType::Bool;
					else if ((leftType->Equals(ExpressionType::Int.Ptr()) || leftType->Equals(ExpressionType::UInt.Ptr())) &&
						(rightType->Equals(ExpressionType::Int.Ptr()) || rightType->Equals(ExpressionType::UInt.Ptr())))
						expr->Type = ExpressionType::Bool;
					else if (leftType->IsIntegral() && rightType->IsIntegral())
						expr->Type = ExpressionType::Bool;
					else if (leftType->Equals(ExpressionType::Float.Ptr()) && rightType->IsIntegral() ||
						leftType->IsIntegral() && rightType->Equals(ExpressionType::Float.Ptr()))
						expr->Type = ExpressionType::Bool;
					else
						expr->Type = ExpressionType::Error;
					break;
				case Operator::Greater:
				case Operator::Geq:
				case Operator::Less:
				case Operator::Leq:
					if ((leftType->Equals(ExpressionType::Int.Ptr()) || leftType->Equals(ExpressionType::UInt.Ptr())) && 
						(rightType->Equals(ExpressionType::Int.Ptr()) || rightType->Equals(ExpressionType::UInt.Ptr())))
						expr->Type = ExpressionType::Bool;
					else if (leftType->Equals(ExpressionType::Float.Ptr()) && rightType->Equals(ExpressionType::Float.Ptr()))
						expr->Type = ExpressionType::Bool;
					else if (leftType->Equals(ExpressionType::Float.Ptr()) && rightType->IsIntegral() ||
						leftType->IsIntegral() && rightType->Equals(ExpressionType::Float.Ptr()))
						expr->Type = ExpressionType::Bool;
					else
						expr->Type = ExpressionType::Error;
					break;
				case Operator::Assign:
					expr->Type = rightType;
					checkAssign();
					break;
				default:
					expr->Type = ExpressionType::Error;
					break;
				}
				if (expr->Type->Equals(ExpressionType::Error.Ptr()) &&
					!leftType->Equals(ExpressionType::Error.Ptr()) && 
					!rightType->Equals(ExpressionType::Error.Ptr()))
					Error(30012, L"no overload found for operator " + OperatorToString(expr->Operator)  + L" (" + leftType->ToString() + L", " 
						+ rightType->ToString() + L").", expr);
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitConstantExpression(ConstantExpressionSyntaxNode *expr) override
			{
				switch (expr->ConstType)
				{
				case ConstantExpressionSyntaxNode::ConstantType::Int:
					expr->Type = ExpressionType::Int;
					break;
				case ConstantExpressionSyntaxNode::ConstantType::Float:
					expr->Type = ExpressionType::Float;
					break;
				default:
					expr->Type = ExpressionType::Error;
					throw "Invalid constant type.";
					break;
				}
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitIndexExpression(IndexExpressionSyntaxNode *expr) override
			{
				expr->BaseExpression = expr->BaseExpression->Accept(this).As<ExpressionSyntaxNode>();
				expr->IndexExpression = expr->IndexExpression->Accept(this).As<ExpressionSyntaxNode>();
				if (expr->BaseExpression->Type->Equals(ExpressionType::Error.Ptr()))
					expr->Type = ExpressionType::Error;
				else
				{
					if (expr->BaseExpression->Type->AsGenericType() && 
						(expr->BaseExpression->Type->AsGenericType()->GenericTypeName != L"Buffer" || expr->BaseExpression->Type->AsGenericType()->GenericTypeName != L"PackedBuffer") ||
						expr->BaseExpression->Type->AsBasicType() &&
						GetVectorSize(expr->BaseExpression->Type->AsBasicType()->BaseType) == 0)
					{
						Error(30013, L"'[]' can only index on arrays.", expr);
						expr->Type = ExpressionType::Error;
					}
					if (!expr->IndexExpression->Type->Equals(ExpressionType::Int.Ptr()) && 
						!expr->IndexExpression->Type->Equals(ExpressionType::UInt.Ptr()))
					{
						Error(30014, L"index expression must evaluate to int.", expr);
						expr->Type = ExpressionType::Error;
					}
				}
				if (expr->BaseExpression->Type->IsArray())
				{
					expr->Type = expr->BaseExpression->Type->AsArrayType()->BaseType;
				}
				else if (auto genType = expr->BaseExpression->Type->AsGenericType())
				{
					expr->Type = genType->BaseType;
				}
				else if (auto basicType = expr->BaseExpression->Type->AsBasicType())
				{
					if (basicType->BaseType == BaseType::Float3x3)
						expr->Type = ExpressionType::Float3;
					else if (basicType->BaseType == BaseType::Float4x4)
						expr->Type = ExpressionType::Float4;
					else
						expr->Type = new BasicExpressionType(GetVectorBaseType(basicType->BaseType));
				}
				expr->Type = expr->Type->Clone();
				if (auto basicType = expr->Type->AsBasicType())
				{
					basicType->IsLeftValue = true;
					basicType->IsReference = true;
				}
				return expr;
			}
			bool MatchArguments(FunctionSyntaxNode * functionNode, List <RefPtr<ExpressionSyntaxNode>> &args)
			{
				if (functionNode->Parameters.Count() != args.Count())
					return false;
				for (int i = 0; i < functionNode->Parameters.Count(); i++)
				{
					if (!functionNode->Parameters[i]->Type->Equals(args[i]->Type.Ptr()))
						return false;
				}
				return true;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitInvokeExpression(InvokeExpressionSyntaxNode *expr) override
			{
				expr->FunctionExpr->Variable = TranslateHLSLTypeNames(expr->FunctionExpr->Variable);
				for (auto & arg : expr->Arguments)
					arg = arg->Accept(this).As<ExpressionSyntaxNode>();

				// check if this is an import operator call
				if (currentShader && currentCompNode && expr->Arguments.Count() > 0)
				{
					if (auto impOpList = currentShader->Pipeline->ImportOperators.TryGetValue(expr->FunctionExpr->Variable))
					{
						// component with explicit import operator call must be qualified with explicit rate
						if (!currentCompNode->Rate)
						{
							Error(33071, L"cannot call an import operator from an auto-placed component '" + currentCompNode->Name.Content + L"'. try qualify the component with explicit worlds.",
								expr);
							expr->Type = ExpressionType::Error;
							return expr;
						}
						// for now we do not support calling import operator from a multi-world component definition
						if (currentCompNode->Rate->Worlds.Count() > 1)
							Error(33073, L"cannot call an import operator from a multi-world component definition. consider qualify the component with only one explicit world.",
								expr);
						int bestMatchConversions = 1 << 30;
						bool found = false;
						RefPtr<ImportOperatorDefSyntaxNode> func;
						for (auto & f : *impOpList)
						{
							if (f->Parameters.Count() == expr->Arguments.Count() - 1 &&
								f->DestWorld.Content == currentCompNode->Rate->Worlds.First().World.Content)
							{
								int conversions = 0;
								bool match = true;
								for (int i = 1; i < expr->Arguments.Count(); i++)
								{
									auto argType = expr->Arguments[i]->Type;
									auto paramType = f->Parameters[i - 1]->Type;
									if (argType->Equals(paramType.Ptr()))
										continue;
									else if (MatchType_ValueReceiver(paramType.Ptr(), argType.Ptr()))
									{
										conversions++;
										continue;
									}
									else
									{
										match = false;
										break;
									}
								}
								if (match && conversions < bestMatchConversions)
								{
									func = f;
									found = true;
									bestMatchConversions = conversions;
								}
							}
						}
						if (found)
						{
							RefPtr<ImportExpressionSyntaxNode> importExpr = new ImportExpressionSyntaxNode();
							importExpr->Position = expr->Position;
							importExpr->Component = expr->Arguments[0];
							CloneContext cloneCtx;
							importExpr->ImportOperatorDef = func->Clone(cloneCtx);
							importExpr->ImportOperatorDef->Scope->Parent = expr->Scope->Parent;
							importExpr->Type = expr->Arguments[0]->Type->Clone();
							importExpr->Scope = expr->Scope;
							importExpr->Access = ExpressionAccess::Read;
							for (int i = 1; i < expr->Arguments.Count(); i++)
								importExpr->Arguments.Add(expr->Arguments[i]);
							return importExpr;
						}
						else
						{
							StringBuilder argList;
							for (int i = 1; i < expr->Arguments.Count(); i++)
							{
								argList << expr->Arguments[i]->Type->ToString();
								if (i != expr->Arguments.Count() - 1)
									argList << L", ";
							}
							Error(33072, L"'" + expr->FunctionExpr->Variable + L"' is an import operator defined in pipeline '" + currentShader->Pipeline->SyntaxNode->Name.Content
								+ L"', but none of the import operator overloads matches argument list '(" +
								argList.ProduceString() + L"').",
								expr);
							expr->Type = ExpressionType::Error;
						}
						return expr;
					}
				}

				// this is not an import operator call, resolve as function call
				String funcName;
				bool found = false;
				bool functionNameFound = false;
				RefPtr<FunctionSymbol> func;
				if (expr->FunctionExpr->Variable == L"texture" && expr->Arguments.Count() > 0 &&
					expr->Arguments[0]->Type->IsGenericType(L"Buffer"))
				{
					if (expr->Arguments.Count() != 2)
					{
						expr->Type = ExpressionType::Error;
						found = false;
					}
					else
					{
						if (auto genType = expr->Arguments[0]->Type->AsGenericType())
						{
							expr->Type = genType->BaseType;
							if (!expr->Arguments[1]->Type->Equals(ExpressionType::Float2.Ptr()))
							{
								found = false;
								expr->Type = ExpressionType::Error;
							}
						}
						else
						{
							expr->Type = ExpressionType::Error;
							found = false;
						}
					}
				}
				else
				{
					// find function overload with implicit argument type conversions
					auto namePrefix = expr->FunctionExpr->Variable + L"@";
					int bestMatchConversions = 1 << 30;
					for (auto & f : symbolTable->Functions)
					{
						if (f.Key.StartsWith(namePrefix))
						{
							functionNameFound = true;
							if (f.Value->SyntaxNode->Parameters.Count() == expr->Arguments.Count())
							{
								int conversions = 0;
								bool match = true;
								for (int i = 0; i < expr->Arguments.Count(); i++)
								{
									auto argType = expr->Arguments[i]->Type;
									auto paramType = f.Value->SyntaxNode->Parameters[i]->Type;
									if (argType->Equals(paramType.Ptr()))
										continue;
									else if (MatchType_ValueReceiver(paramType.Ptr(), argType.Ptr()))
									{
										conversions++;
										continue;
									}
									else
									{
										match = false;
										break;
									}
								}
								if (match && conversions < bestMatchConversions)
								{
									func = f.Value;
									funcName = f.Key;
									found = true;
									bestMatchConversions = conversions;
								}
							}
						}
					}
				}
				if (!found)
				{
					expr->Type = ExpressionType::Error;
					StringBuilder argList;
					for (int i = 0; i < expr->Arguments.Count(); i++)
					{
						argList << expr->Arguments[i]->Type->ToString();
						if (i != expr->Arguments.Count() - 1)
							argList << L", ";
					}
					if (functionNameFound)
						Error(30021, expr->FunctionExpr->Variable + L": no overload takes arguments (" + argList.ProduceString() + L")", expr);
					else
						Error(30015, L"undefined identifier '" + expr->FunctionExpr->Variable + L"'.", expr->FunctionExpr.Ptr());

				}
				else if (func)
				{
					if (!func->SyntaxNode->IsExtern)
					{
						expr->FunctionExpr->Variable = funcName;
						if (currentFunc)
							currentFunc->ReferencedFunctions.Add(funcName);
					}
					expr->Type = func->SyntaxNode->ReturnType;
				}
				return expr;
			}

			String OperatorToString(Operator op)
			{
				switch (op)
				{
				case Spire::Compiler::Operator::Neg:
					return L"-";
				case Spire::Compiler::Operator::Not:
					return L"!";
				case Spire::Compiler::Operator::PreInc:
					return L"++";
				case Spire::Compiler::Operator::PreDec:
					return L"--";
				case Spire::Compiler::Operator::PostInc:
					return L"++";
				case Spire::Compiler::Operator::PostDec:
					return L"--";
				case Spire::Compiler::Operator::Mul:
				case Spire::Compiler::Operator::MulAssign:
					return L"*";
				case Spire::Compiler::Operator::Div:
				case Spire::Compiler::Operator::DivAssign:
					return L"/";
				case Spire::Compiler::Operator::Mod:
				case Spire::Compiler::Operator::ModAssign:
					return L"%";
				case Spire::Compiler::Operator::Add:
				case Spire::Compiler::Operator::AddAssign:
					return L"+";
				case Spire::Compiler::Operator::Sub:
				case Spire::Compiler::Operator::SubAssign:
					return L"-";
				case Spire::Compiler::Operator::Lsh:
				case Spire::Compiler::Operator::LshAssign:
					return L"<<";
				case Spire::Compiler::Operator::Rsh:
				case Spire::Compiler::Operator::RshAssign:
					return L">>";
				case Spire::Compiler::Operator::Eql:
					return L"==";
				case Spire::Compiler::Operator::Neq:
					return L"!=";
				case Spire::Compiler::Operator::Greater:
					return L">";
				case Spire::Compiler::Operator::Less:
					return L"<";
				case Spire::Compiler::Operator::Geq:
					return L">=";
				case Spire::Compiler::Operator::Leq:
					return L"<=";
				case Spire::Compiler::Operator::BitAnd:
				case Spire::Compiler::Operator::AndAssign:
					return L"&";
				case Spire::Compiler::Operator::BitXor:
				case Spire::Compiler::Operator::XorAssign:
					return L"^";
				case Spire::Compiler::Operator::BitOr:
				case Spire::Compiler::Operator::OrAssign:
					return L"|";
				case Spire::Compiler::Operator::And:
					return L"&&";
				case Spire::Compiler::Operator::Or:
					return L"||";
				case Spire::Compiler::Operator::Assign:
					return L"=";
				default:
					return L"ERROR";
				}
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitUnaryExpression(UnaryExpressionSyntaxNode *expr) override
			{
				expr->Expression = expr->Expression->Accept(this).As<ExpressionSyntaxNode>();
				
				switch (expr->Operator)
				{
				case Operator::Neg:
					if (expr->Expression->Type->Equals(ExpressionType::Int.Ptr()) ||
						expr->Expression->Type->Equals(ExpressionType::Bool.Ptr()) ||
						expr->Expression->Type->Equals(ExpressionType::Float.Ptr()) ||
						expr->Expression->Type->IsVectorType())
						expr->Type = expr->Expression->Type;
					else
						expr->Type = ExpressionType::Error;
					break;
				case Operator::Not:
				case Operator::BitNot:
					if (expr->Expression->Type->Equals(ExpressionType::Int.Ptr()) || expr->Expression->Type->Equals(ExpressionType::Bool.Ptr()) ||
						expr->Expression->Type->Equals(ExpressionType::Int2.Ptr())
						|| expr->Expression->Type->Equals(ExpressionType::Int3.Ptr()) || expr->Expression->Type->Equals(ExpressionType::Int4.Ptr()))
						expr->Type = (expr->Operator == Operator::Not ? ExpressionType::Bool : expr->Expression->Type);
					else
						expr->Type = ExpressionType::Error;
					break;
				case Operator::PostDec:
				case Operator::PostInc:
				case Operator::PreDec:
				case Operator::PreInc:
					if (expr->Expression->Type->Equals(ExpressionType::Int.Ptr()))
						expr->Type = ExpressionType::Int;
					else
						expr->Type = ExpressionType::Error;
					break;
				default:
					expr->Type = ExpressionType::Error;
					break;
				}

				if(expr->Type->Equals(ExpressionType::Error.Ptr()) && !expr->Expression->Type->Equals(ExpressionType::Error.Ptr()))
					Error(30020, L"operator " + OperatorToString(expr->Operator) + L" can not be applied to " + expr->Expression->Type->ToString(), expr);
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitVarExpression(VarExpressionSyntaxNode *expr) override
			{
				VariableEntry variable;
				ShaderUsing shaderObj;
				expr->Type = ExpressionType::Error;
				if (expr->Scope->FindVariable(expr->Variable, variable))
				{
					expr->Type = variable.Type.DataType->Clone();
					if (auto basicType = expr->Type->AsBasicType())
						basicType->IsLeftValue = !variable.IsComponent;
				}
				else if (currentShader && currentShader->ShaderObjects.TryGetValue(expr->Variable, shaderObj))
				{
					auto basicType = new BasicExpressionType(BaseType::Shader);
					basicType->Shader = shaderObj.Shader;
					basicType->IsLeftValue = false;
					expr->Type = basicType;
				}
				else if (currentPipeline && currentImportOperator)
				{
					RefPtr<ShaderComponentSymbol> comp;
					if (currentPipeline->Components.TryGetValue(expr->Variable, comp))
					{
						currentImportOperator->Usings.Add(comp->Name);
						expr->Type = comp->Type->DataType;
					}
					else
						Error(30015, L"undefined identifier \'" + expr->Variable + L"\'", expr);
				}
				else if (currentShader)
				{
					auto compRef = currentShader->ResolveComponentReference(expr->Variable);
					if (compRef.IsAccessible)
					{
						expr->Type = compRef.Component->Type->DataType->Clone();
						if (auto basicType = expr->Type->AsBasicType())
							basicType->IsLeftValue = false;
					}
					else if (compRef.Component)
					{
						Error(30017, L"component \'" + expr->Variable + L"\' is not accessible from shader '" + currentShader->SyntaxNode->Name.Content + L"'.", expr);
					}
					else
						Error(30015, L"undefined identifier \'" + expr->Variable + L"\'", expr);
				}
				else
					Error(30015, L"undefined identifier \'" + expr->Variable + L"\'", expr);

				if (expr->Type->IsGenericType(L"Uniform") || expr->Type->IsGenericType(L"Patch"))
					expr->Type = expr->Type->AsGenericType()->BaseType;

				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitTypeCastExpression(TypeCastExpressionSyntaxNode * expr) override
			{
				expr->Expression = expr->Expression->Accept(this).As<ExpressionSyntaxNode>();
				auto targetType = TranslateTypeNode(expr->TargetType);
				
				if (!expr->Expression->Type->Equals(ExpressionType::Error.Ptr()) && targetType->AsBasicType())
				{
					if (!expr->Expression->Type->AsBasicType())
						expr->Type = ExpressionType::Error;
					else if (!IsNumeric(GetVectorBaseType(expr->Expression->Type->AsBasicType()->BaseType)) 
						|| !IsNumeric(GetVectorBaseType(targetType->AsBasicType()->BaseType)))
						expr->Type = ExpressionType::Error;
					else if (targetType->AsBasicType()->BaseType == BaseType::Void || expr->Expression->Type->AsBasicType()->BaseType == BaseType::Void)
						expr->Type = ExpressionType::Error;
					else
						expr->Type = targetType;
				}
				else
					expr->Type = ExpressionType::Error;
				if (expr->Type->Equals(ExpressionType::Error.Ptr()) && !expr->Expression->Type->Equals(ExpressionType::Error.Ptr()))
				{
					Error(30022, L"invalid type cast between \"" + expr->Expression->Type->ToString() + L"\" and \"" +
						targetType->ToString() + L"\".", expr);
				}
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitSelectExpression(SelectExpressionSyntaxNode * expr) override
			{
				expr->SelectorExpr = expr->SelectorExpr->Accept(this).As<ExpressionSyntaxNode>();
				if (!expr->SelectorExpr->Type->Equals(ExpressionType::Int.Ptr()) && !expr->SelectorExpr->Type->Equals(ExpressionType::Bool.Ptr()) 
					&& !expr->SelectorExpr->Type->Equals(ExpressionType::Error.Ptr()))
				{
					expr->Type = ExpressionType::Error;
					Error(30079, L"selector must evaluate to int.", expr);
				}
				expr->Expr0 = expr->Expr0->Accept(this).As<ExpressionSyntaxNode>();
				expr->Expr1 = expr->Expr1->Accept(this).As<ExpressionSyntaxNode>();
				if (expr->Expr0->Type != expr->Expr1->Type)
				{
					Error(30080, L"the two value expressions in a select clause must evaluate to same type.", expr);
				}
				expr->Type = expr->Expr0->Type;
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitMemberExpression(MemberExpressionSyntaxNode * expr) override
			{
				expr->BaseExpression = expr->BaseExpression->Accept(this).As<ExpressionSyntaxNode>();
				auto & baseType = expr->BaseExpression->Type;
				if (!baseType->AsBasicType())
					expr->Type = ExpressionType::Error;
				else if (IsVector(baseType->AsBasicType()->BaseType))
				{
					Array<int, 4> children;
					if (expr->MemberName.Length() > 4)
						expr->Type = ExpressionType::Error;
					else
					{
						bool error = false;

						for (int i = 0; i < expr->MemberName.Length(); i++)
						{
							auto ch = expr->MemberName[i];
							switch (ch)
							{
							case L'x':
							case L'r':
								children.Add(0);
								break;
							case L'y':
							case L'g':
								children.Add(1);
								break;
							case L'z':
							case L'b':
								children.Add(2);
								break;
							case L'w':
							case L'a':
								children.Add(3);
								break;
							default:
								error = true;
								expr->Type = ExpressionType::Error;
								break;
							}
						}
						int vecLen = GetVectorSize(baseType->AsBasicType()->BaseType);
						for (auto m : children)
						{
							if (m >= vecLen)
							{
								error = true;
								expr->Type = ExpressionType::Error;
								break;
							}
						}
						if ((vecLen == 9 || vecLen == 16) && children.Count() > 1)
						{
							error = true;
							expr->Type = ExpressionType::Error;
						}
						if (!error)
						{
							if (vecLen == 9)
								expr->Type = new BasicExpressionType((BaseType)((int)GetVectorBaseType(baseType->AsBasicType()->BaseType) + 2));
							else if (vecLen == 16)
								expr->Type = new BasicExpressionType((BaseType)((int)GetVectorBaseType(baseType->AsBasicType()->BaseType) + 3));
							else
							{
								expr->Type = new BasicExpressionType((BaseType)((int)GetVectorBaseType(baseType->AsBasicType()->BaseType) + children.Count() - 1));
							}
							expr->Type.As<BasicExpressionType>()->IsMaskedVector = true;
						}
						if (auto bt = expr->Type->AsBasicType())
						{
							bt->IsLeftValue = !baseType->AsBasicType()->IsMaskedVector;
							if (children.Count() > vecLen || children.Count() == 0)
								bt->IsLeftValue = false;
							int curMax = children[0];
							for (int i = 0; i < children.Count(); i++)
								if (children[i] < curMax)
								{
									bt->IsLeftValue = false;
									curMax = children[i];
								}
						}
					}
				}
				else if (baseType->AsBasicType()->BaseType == BaseType::Shader)
				{
					ShaderUsing shaderObj;
					auto refComp = baseType->AsBasicType()->Shader->ResolveComponentReference(expr->MemberName);
					if (refComp.IsAccessible)
						expr->Type = refComp.Component->Type->DataType;
					else if (baseType->AsBasicType()->Shader->ShaderObjects.TryGetValue(expr->MemberName, shaderObj))
					{
						if (shaderObj.IsPublic)
						{
							auto shaderType = new BasicExpressionType(BaseType::Shader);
							shaderType->Shader = shaderObj.Shader;
							expr->Type = shaderType;
						}
						else
							expr->Type = ExpressionType::Error;
					}
					else
						expr->Type = ExpressionType::Error;
				}
				else if (baseType->IsStruct())
				{
					int id = baseType->AsBasicType()->Struct->SyntaxNode->FindField(expr->MemberName);
					if (id == -1)
					{
						expr->Type = ExpressionType::Error;
						Error(30027, L"\'" + expr->MemberName + L"\' is not a member of \'" +
							baseType->AsBasicType()->Struct->Name + L"\'.", expr);
					}
					else
						expr->Type = baseType->AsBasicType()->Struct->SyntaxNode->Fields[id]->Type;
				}
				else
					expr->Type = ExpressionType::Error;
				if (!baseType->Equals(ExpressionType::Error.Ptr()) &&
					expr->Type->Equals(ExpressionType::Error.Ptr()))
				{
					Error(30023, L"\"" + baseType->ToString() + L"\" does not have public member \"" +
						expr->MemberName + L"\".", expr);
				}
				return expr;
			}
			SemanticsVisitor & operator = (const SemanticsVisitor &) = delete;
		};

		SyntaxVisitor * CreateSemanticsVisitor(SymbolTable * symbols, ErrorWriter * err)
		{
			return new SemanticsVisitor(symbols, err);
		}
		
	}
}