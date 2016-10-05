#include "Closure.h"
#include "VariantIR.h"
#include "StringObject.h"

namespace Spire
{
	namespace Compiler
	{
		class InsertImplicitImportOperatorVisitor : public SyntaxVisitor
		{
		private:
			ShaderIR * shaderIR;
		public:
			ComponentDefinitionIR * currentCompDef = nullptr;
			EnumerableDictionary<String, RefPtr<ComponentDefinitionIR>> passThroughComponents;
		public:
			InsertImplicitImportOperatorVisitor(ShaderIR * ir, ErrorWriter* err)
				: SyntaxVisitor(err), shaderIR(ir)
			{}

			ComponentDefinitionIR * MakeComponentAvailableAtWorld(String componentUniqueName, String world)
			{
				HashSet<String> visitedComponents;
				return MakeComponentAvailableAtWorldInternal(visitedComponents, componentUniqueName, world);
			}

			ComponentDefinitionIR * MakeComponentAvailableAtWorldInternal(HashSet<String> & visitedComponents, String componentUniqueName, String world)
			{
				RefPtr<ComponentDefinitionIR> refDef;
				if (passThroughComponents.TryGetValue(componentUniqueName + L"I_at_I" + world, refDef))
					return refDef.Ptr();
				if (visitedComponents.Contains(componentUniqueName + "@" + world))
				{
					StringBuilder refs;
					int count = 0;
					for (auto & comp : visitedComponents)
					{
						refs << comp;
						if (count != visitedComponents.Count() - 1)
							refs << L", ";
						count++;
					}
					Error(34062, L"cyclic reference: " + refs.ProduceString(), currentCompDef->SyntaxNode.Ptr());
					return nullptr;
				}
				visitedComponents.Add(componentUniqueName);
				ImportPath importPath;
				int currentPathLength = 1 << 30;
				ComponentDefinitionIR * referencedDef = nullptr;
				for (auto & compDef : shaderIR->DefinitionsByComponent[componentUniqueName]())
				{
					if (compDef.Value->World == world)
						return compDef.Value;
				}
				for (auto & compDef : shaderIR->DefinitionsByComponent[componentUniqueName]())
				{
					auto path = shaderIR->Shader->Pipeline->FindImplicitImportOperatorChain(compDef.Value->World, world);
					if (path.Count() && path.First().Nodes.Count() < currentPathLength)
					{
						importPath = path.First();
						currentPathLength = importPath.Nodes.Count();
						referencedDef = compDef.Value;
					}
				}
				if (referencedDef)
				{
					auto & node = importPath.Nodes.Last();
					RefPtr<ComponentDefinitionIR> thruDef;
					auto thruDefName = componentUniqueName + L"I_at_I" + node.TargetWorld;
					if (!passThroughComponents.TryGetValue(thruDefName, thruDef))
					{
						auto srcDef = MakeComponentAvailableAtWorldInternal(visitedComponents, componentUniqueName, node.ImportOperator->SourceWorld.Content);
						thruDef = new ComponentDefinitionIR();
						thruDef->World = world;
						thruDef->Dependency.Add(srcDef);
						srcDef->Users.Add(thruDef.Ptr());
						thruDef->OriginalName = referencedDef->OriginalName;
						thruDef->UniqueName = thruDefName;
						thruDef->UniqueKey = referencedDef->UniqueKey + L"@" + node.TargetWorld;
						thruDef->IsEntryPoint = false;
						thruDef->SyntaxNode = new ComponentSyntaxNode();
						thruDef->SyntaxNode->Type = thruDef->Type = srcDef->SyntaxNode->Type;
						thruDef->SyntaxNode->Rate = new RateSyntaxNode();
						thruDef->SyntaxNode->Rate->Worlds.Add(RateWorld(node.TargetWorld));
						thruDef->SyntaxNode->Name.Content = thruDefName;
						CloneContext cloneCtx;
						thruDef->SyntaxNode->TypeNode = srcDef->SyntaxNode->TypeNode->Clone(cloneCtx);
						auto importExpr = new ImportExpressionSyntaxNode();
						importExpr->Type = thruDef->Type;
						importExpr->ImportOperatorDef = node.ImportOperator->Clone(cloneCtx);
						importExpr->ImportOperatorDef->Scope->Parent = thruDef->SyntaxNode->Scope.Ptr();
						importExpr->ComponentUniqueName = srcDef->UniqueName;
						for (auto & arg : importExpr->Arguments)
							arg->Accept(this);
						importExpr->ImportOperatorDef->Body->Accept(this);
						thruDef->SyntaxNode->Expression = importExpr;
						passThroughComponents[thruDefName] = thruDef;
					}
					visitedComponents.Remove(componentUniqueName + "@" + world);
					return thruDef.Ptr();
				}
				else
				{
					throw InvalidProgramException(L"import operator not found, should have been checked in semantics pass.");
				}
			}

			RefPtr<ExpressionSyntaxNode> ProcessComponentReference(String componentUniqueName)
			{
				auto refDef = MakeComponentAvailableAtWorld(componentUniqueName, currentCompDef->World);
				auto refNode = new VarExpressionSyntaxNode();
				if (refDef)
				{
					refNode->Variable = refDef->UniqueName;
					refNode->Type = refDef->Type;
					refNode->Tags[L"ComponentReference"] = new StringObject(refDef->UniqueName);
					currentCompDef->Dependency.Add(refDef);
					refDef->Users.Add(currentCompDef);
				}
				return refNode;
			}
			RefPtr<ExpressionSyntaxNode> VisitVarExpression(VarExpressionSyntaxNode * var) override
			{
				RefPtr<Object> refCompObj;
				if (var->Tags.TryGetValue(L"ComponentReference", refCompObj))
				{
					auto refComp = refCompObj.As<StringObject>().Ptr();
					return ProcessComponentReference(refComp->Content);
				}
				return var;
			}

			RefPtr<ExpressionSyntaxNode> VisitMemberExpression(MemberExpressionSyntaxNode * member) override
			{
				RefPtr<Object> refCompObj;
				if (member->Tags.TryGetValue(L"ComponentReference", refCompObj))
				{
					auto refComp = refCompObj.As<StringObject>().Ptr();
					return ProcessComponentReference(refComp->Content);
				}
				else
					member->BaseExpression->Accept(this);
				return member;
			}
			RefPtr<ExpressionSyntaxNode> VisitImportExpression(ImportExpressionSyntaxNode * import) override
			{
				auto refDef = MakeComponentAvailableAtWorld(import->ComponentUniqueName, import->ImportOperatorDef->SourceWorld.Content);
				if (refDef)
					import->ComponentUniqueName = refDef->UniqueName;
				return import;
			}
		};
		void InsertImplicitImportOperators(ShaderIR * shader)
		{
			InsertImplicitImportOperatorVisitor visitor(shader, nullptr);
			for (auto & comp : shader->Definitions)
			{
				visitor.currentCompDef = comp.Ptr();
				comp->SyntaxNode->Accept(&visitor);
			}
			for (auto & comp : visitor.passThroughComponents)
			{
				shader->Definitions.Add(comp.Value);
				EnumerableDictionary<String, ComponentDefinitionIR*> defs;
				defs[comp.Value->World] = comp.Value.Ptr();
				shader->DefinitionsByComponent[comp.Key] = defs;
			}
		}
	}
}