#ifndef VARIANT_IR_H
#define VARIANT_IR_H

#include "Syntax.h"

namespace Spire
{
	namespace Compiler
	{
		class ShaderClosure;
		class ModuleInstanceIR : public RefObject
		{
		public:
			ShaderSyntaxNode * SyntaxNode;
			String BindingName;
		};
		class ComponentDefinitionIR : public RefObject
		{
		private:
			EnumerableHashSet<ComponentDefinitionIR *> dependencyClosure;
		public:
			String OriginalName, UniqueName, UniqueKey;
			RefPtr<ComponentSyntaxNode> SyntaxNode;
			RefPtr<ExpressionType> Type;
			ModuleInstanceIR * ModuleInstance = nullptr;
			String World;
			bool IsEntryPoint = false;
			EnumerableHashSet<ComponentDefinitionIR *> Users, Dependency; // Bidirectional dependency;
			EnumerableHashSet<ComponentDefinitionIR *> & GetComponentFunctionDependencyClosure();
			void ClearDependency()
			{
				Dependency.Clear();
				dependencyClosure.Clear();
			}
		};

		class ShaderIR : public RefObject
		{
		public:
			ShaderClosure * Shader;
			SymbolTable * SymbolTable;
			List<RefPtr<ModuleInstanceIR>> ModuleInstances;
			List<RefPtr<ComponentDefinitionIR>> Definitions;
			EnumerableDictionary<String, EnumerableDictionary<String, ComponentDefinitionIR*>> DefinitionsByComponent;
			void EliminateDeadCode(); // returns remaining definitions in reverse dependency order
			void ResolveComponentReference(); // resolve reference and build dependency map
			List<String> GetComponentDependencyOrder(); // returns a list of all components' unique names in dependency order
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
	}
}

#endif