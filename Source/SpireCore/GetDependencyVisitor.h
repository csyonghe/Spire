#ifndef GET_DEPENDENCY_VISITOR_H
#define GET_DEPENDENCY_VISITOR_H

#include "VariantIR.h"
#include "Closure.h"
#include "StringObject.h"

namespace Spire
{
	namespace Compiler
	{
		class ComponentDependency
		{
		public:
			String ReferencedComponent;
			ImportOperatorDefSyntaxNode * ImportOperator = nullptr;
			ComponentDependency() = default;
			ComponentDependency(String compName, ImportOperatorDefSyntaxNode * impOp)
				: ReferencedComponent(compName), ImportOperator(impOp)
			{}
			int GetHashCode()
			{
				return ReferencedComponent.GetHashCode() ^ (int)(CoreLib::PtrInt)(void*)(ImportOperator);
			}
			bool operator == (const ComponentDependency & other)
			{
				return ReferencedComponent == other.ReferencedComponent && ImportOperator == other.ImportOperator;
			}
		};

		class GetDependencyVisitor : public SyntaxVisitor
		{
		public:
			EnumerableHashSet<ComponentDependency> Result;
			GetDependencyVisitor()
				: SyntaxVisitor(nullptr)
			{}

			RefPtr<ExpressionSyntaxNode> VisitVarExpression(VarExpressionSyntaxNode * var) override;

			RefPtr<ExpressionSyntaxNode> VisitMemberExpression(MemberExpressionSyntaxNode * member) override;

			RefPtr<ExpressionSyntaxNode> VisitImportExpression(ImportExpressionSyntaxNode * syntax) override;
		};

		EnumerableHashSet<ComponentDependency> GetDependentComponents(SyntaxNode * tree);
	}
}
#endif