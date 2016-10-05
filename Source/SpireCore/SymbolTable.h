#ifndef RASTER_RENDERER_SYMBOL_TABLE_H
#define RASTER_RENDERER_SYMBOL_TABLE_H

#include "../CoreLib/Basic.h"
#include "Syntax.h"
#include "IL.h"
#include "VariantIR.h"

namespace Spire
{
	namespace Compiler
	{
		
		class FunctionSymbol
		{
		public:
			FunctionSyntaxNode * SyntaxNode;
			EnumerableHashSet<String> ReferencedFunctions;
		};
		class ShaderComponentSymbol;
		class ShaderComponentImplSymbol : public Object
		{
		public:
			String AlternateName;
			EnumerableHashSet<String> Worlds, ExportWorlds, SrcPinnedWorlds;
			RefPtr<ComponentSyntaxNode> SyntaxNode;
			EnumerableDictionary<ShaderComponentSymbol *, EnumerableHashSet<RefPtr<ImportExpressionSyntaxNode>>> DependentComponents; // key: dependent components, value: set of import expression nodes (null means implicit reference)
			EnumerableDictionary<ShaderComponentSymbol *, CodePosition> ComponentReferencePositions;
			ShaderComponentImplSymbol() = default;
			ShaderComponentImplSymbol(const ShaderComponentImplSymbol & other)
			{
				AlternateName = other.AlternateName;
				Worlds = other.Worlds;
				ExportWorlds = other.ExportWorlds;
				SrcPinnedWorlds = other.SrcPinnedWorlds;
				CloneContext ctx;
				SyntaxNode = other.SyntaxNode->Clone(ctx);
			}
		};

		class ShaderComponentSymbol : public Object
		{
		public:
			bool IsDceEntryPoint = false;
			String Name, UniqueName, UniqueKey;
			List<String> ChoiceNames;
			EnumerableDictionary<ShaderComponentSymbol *, EnumerableHashSet<RefPtr<ImportExpressionSyntaxNode>>> DependentComponents;
			EnumerableHashSet<ShaderComponentSymbol *>UserComponents;
			List<RefPtr<ShaderComponentImplSymbol>> Implementations;
			RefPtr<Type> Type;
			bool IsParam()
			{
				for (auto & impl : Implementations)
					if (impl->SyntaxNode->IsParam)
						return true;
				return false;
			}
			ShaderComponentSymbol() = default;
			ShaderComponentSymbol(const ShaderComponentSymbol & other)
			{
				Type = new Spire::Compiler::Type(*other.Type);
				for (auto &impl : other.Implementations)
					this->Implementations.Add(new ShaderComponentImplSymbol(*impl));
				this->Name = other.Name;
			}
		};
		
		class WorldSymbol
		{
		public:
			bool IsAbstract = false;
			WorldSyntaxNode * SyntaxNode = nullptr;
		};

		class PipelineSymbol;
		class ShaderClosure;
		
		class ShaderSymbol;

		class ShaderUsing
		{
		public:
			ShaderSymbol * Shader;
			bool IsPublic;
		};

		class ShaderSymbol
		{
		public:
			bool IsAbstract = false;
			ShaderSyntaxNode * SyntaxNode = nullptr;
			PipelineSymbol * Pipeline = nullptr;
		
			EnumerableDictionary<String, RefPtr<ShaderComponentSymbol>> Components;
			List<ShaderComponentSymbol*> GetComponentDependencyOrder();
			EnumerableHashSet<ShaderSymbol*> DependentShaders;
			List<ShaderUsing> ShaderUsings;
			EnumerableDictionary<String, ShaderUsing> ShaderObjects;
			void SortComponents(List<ShaderComponentSymbol*> & comps);
			struct ComponentReference
			{
				ShaderComponentSymbol * Component = nullptr;
				bool IsAccessible = false;
			};
			ComponentReference ResolveComponentReference(String compName, bool topLevel = true);
		};

		class ShaderClosure : public Object
		{
		public:
			ShaderClosure * Parent = nullptr;
			CodePosition Position;
			PipelineSymbol * Pipeline = nullptr;
			bool IsInPlace = false;
			bool IsPublic = false;
			String Name;
			CodePosition UsingPosition;
			Dictionary<String, RefPtr<ShaderComponentSymbol>> RefMap;
			EnumerableDictionary<String, RefPtr<ShaderComponentSymbol>> Components;
			EnumerableDictionary<String, ShaderComponentSymbol *> AllComponents;
			EnumerableDictionary<String, RefPtr<ShaderClosure>> SubClosures;
			RefPtr<ShaderComponentSymbol> FindComponent(String name, bool findInPrivate = false, bool includeParams = true);
			RefPtr<ShaderClosure> FindClosure(String name);
			List<ShaderComponentSymbol*> GetDependencyOrder();
			RefPtr<ShaderIR> IR;
		};

		class ImportPath
		{
		public:
			class Node
			{
			public:
				String TargetWorld;
				ImportOperatorDefSyntaxNode * ImportOperator;
				Node() = default;
				Node(String world, ImportOperatorDefSyntaxNode * imp)
					: TargetWorld(world), ImportOperator(imp)
				{}
			};
			List<Node> Nodes;
		};

		class PipelineSymbol
		{
		private:
			List<String> WorldTopologyOrder;
		public:
			PipelineSyntaxNode * SyntaxNode;
			EnumerableDictionary<String, List<RefPtr<ImportOperatorDefSyntaxNode>>> ImportOperators;
			EnumerableDictionary<String, RefPtr<ShaderComponentSymbol>> Components;
			EnumerableDictionary<String, EnumerableHashSet<String>> ReachableWorlds, ImplicitlyReachableWorlds;
			EnumerableDictionary<String, EnumerableHashSet<String>> WorldDependency, ImplicitWorldDependency;
			EnumerableDictionary<String, WorldSymbol> Worlds;
			bool IsAbstractWorld(String world);
			bool IsWorldReachable(EnumerableHashSet<String> & src, String targetWorld);
			bool IsWorldReachable(String src, String targetWorld);
			bool IsWorldImplicitlyReachable(EnumerableHashSet<String> & src, String targetWorld);
			bool IsWorldImplicitlyReachable(String src, String targetWorld);
			List<String> & GetWorldTopologyOrder();
			List<ImportPath> FindImplicitImportOperatorChain(String worldSrc, String worldDest);
			List<ImportOperatorDefSyntaxNode*> GetImportOperatorsFromSourceWorld(String worldSrc);
		};

		class CompileResult;

		class StructSymbol
		{
		public:
			String Name;
			RefPtr<StructSyntaxNode> SyntaxNode;
			RefPtr<ILStructType> Type;
		};

		class SymbolTable
		{
		public:
			EnumerableDictionary<String, RefPtr<FunctionSymbol>> Functions;
			EnumerableDictionary<String, RefPtr<ShaderSymbol>> Shaders;
			EnumerableDictionary<String, RefPtr<PipelineSymbol>> Pipelines;
			EnumerableDictionary<String, RefPtr<StructSymbol>> Structs;
			List<ShaderSymbol*> ShaderDependenceOrder;
			bool SortShaders(); // return true if success, return false if dependency is cyclic
			void EvalFunctionReferenceClosure();
			bool CheckComponentImplementationConsistency(ErrorWriter * err, ShaderComponentSymbol * comp, ShaderComponentImplSymbol * impl);
		};

		class GUID
		{
		private:
			static int currentGUID;
		public:
			static void Clear();
			static int Next();
		};


		template<typename T, typename GetDependencyFunc>
		void DependencySort(List<T> & list, const GetDependencyFunc & getDep)
		{
			HashSet<T> allSymbols, addedSymbols;
			for (auto & comp : list)
				allSymbols.Add(comp);
			List<T> sorted;
			bool changed = true;
			while (changed)
			{
				changed = false;
				for (auto & comp : list)
				{
					if (!addedSymbols.Contains(comp))
					{
						bool isFirst = true;
						auto && dependency = getDep(comp);
						for (auto & dep : dependency)
							if (allSymbols.Contains(dep) && !addedSymbols.Contains(dep))
							{
								isFirst = false;
								break;
							}
						if (isFirst)
						{
							addedSymbols.Add(comp);
							sorted.Add(comp);
							changed = true;
						}
					}
				}
			}
			list = _Move(sorted);
		}

	}
}
#endif