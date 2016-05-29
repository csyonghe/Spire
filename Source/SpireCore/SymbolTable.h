#ifndef RASTER_RENDERER_SYMBOL_TABLE_H
#define RASTER_RENDERER_SYMBOL_TABLE_H

#include "CoreLib/Basic.h"
#include "Syntax.h"

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
			EnumerableHashSet<ShaderComponentSymbol *> DependentComponents;
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
			EnumerableHashSet<ShaderComponentSymbol *> DependentComponents, UserComponents;
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

		class ComponentDefinitionIR
		{
		public:
			ShaderComponentSymbol * Component;
			ShaderComponentImplSymbol * Implementation;
			String World;
			bool IsEntryPoint = false;
			EnumerableHashSet<ComponentDefinitionIR*> Users, Dependency; // Bidirectional dependency;
		};
		
		class ShaderClosure;

		class ShaderIR
		{
		public:
			ShaderClosure * Shader;
			List<RefPtr<ComponentDefinitionIR>> Definitions;
			EnumerableDictionary<String, EnumerableDictionary<String, ComponentDefinitionIR*>> DefinitionsByComponent;
			void EliminateDeadCode(); // returns remaining definitions in reverse dependency order
			void ResolveComponentReference(); // resolve reference and build dependency map
			List<ShaderComponentSymbol*> GetComponentDependencyOrder();
			template<typename ShouldRemoveFunc>
			void RemoveDefinitions(const ShouldRemoveFunc &shouldRemove)
			{
				List<RefPtr<ComponentDefinitionIR>> newDefinitions;
				for (auto & def : Definitions)
				{
					if (!shouldRemove(def.Ptr()))
					{
						newDefinitions.Add(def);
					}
				}
				Definitions = _Move(newDefinitions);
				for (auto & kv : DefinitionsByComponent)
				{
					for (auto & def : kv.Value)
						if (shouldRemove(def.Value))
							kv.Value.Remove(def.Key);
				}
			}

		};
		
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
				ShaderComponentSymbol * Component;
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
			RefPtr<ShaderComponentSymbol> FindComponent(String name, bool findInPrivate = false);
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
			EnumerableDictionary<String, RefPtr<ShaderComponentSymbol>> Components;
			EnumerableDictionary<String, EnumerableHashSet<String>> ReachableWorlds;
			EnumerableDictionary<String, EnumerableHashSet<String>> WorldDependency;
			EnumerableDictionary<String, WorldSymbol> Worlds;
			bool IsAbstractWorld(String world);
			bool IsWorldReachable(EnumerableHashSet<String> & src, String targetWorld);
			bool IsWorldReachable(String src, String targetWorld);
			bool IsWorldDirectlyReachable(String src, String targetWorld);
			List<String> & GetWorldTopologyOrder();
			List<ImportPath> FindImportOperatorChain(String worldSrc, String worldDest);
			List<ImportOperatorDefSyntaxNode*> GetImportOperatorsFromSourceWorld(String worldSrc);
		};

		class CompileResult;

		class SymbolTable
		{
		public:
			EnumerableDictionary<String, RefPtr<FunctionSymbol>> Functions;
			EnumerableDictionary<String, RefPtr<ShaderSymbol>> Shaders;
			EnumerableDictionary<String, RefPtr<PipelineSymbol>> Pipelines;
			List<ShaderSymbol*> ShaderDependenceOrder;
			bool SortShaders(); // return true if success, return false if dependency is cyclic
			void EvalFunctionReferenceClosure();
		};

		class GUID
		{
		private:
			static int currentGUID;
		public:
			static void Clear();
			static int Next();
		};

		bool CheckComponentImplementationConsistency(ErrorWriter * err, ShaderComponentSymbol * comp, ShaderComponentImplSymbol * impl);

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