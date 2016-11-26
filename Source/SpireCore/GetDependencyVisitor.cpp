#include "GetDependencyVisitor.h"

namespace Spire
{
	namespace Compiler
	{
		EnumerableHashSet<ComponentDependency> GetDependentComponents(SyntaxNode * tree)
		{
			GetDependencyVisitor visitor;
			tree->Accept(&visitor);
			return visitor.Result;
		}

		RefPtr<ExpressionSyntaxNode> GetDependencyVisitor::VisitImportExpression(ImportExpressionSyntaxNode * syntax)
		{
			for (auto & comp : syntax->ImportOperatorDef->Usings)
				Result.Add(ComponentDependency(comp, nullptr));
			Result.Add(ComponentDependency(syntax->ComponentUniqueName, syntax->ImportOperatorDef.Ptr()));
			return SyntaxVisitor::VisitImportExpression(syntax);
		}
		RefPtr<ExpressionSyntaxNode> GetDependencyVisitor::VisitMemberExpression(MemberExpressionSyntaxNode * member)
		{
			RefPtr<Object> refCompObj;
			if (member->Tags.TryGetValue("ComponentReference", refCompObj))
			{
				auto refComp = refCompObj.As<StringObject>().Ptr();
				Result.Add(ComponentDependency(refComp->Content, nullptr));
			}
			else
				member->BaseExpression->Accept(this);
			return member;
		}
		RefPtr<ExpressionSyntaxNode> GetDependencyVisitor::VisitVarExpression(VarExpressionSyntaxNode * var)
		{
			RefPtr<Object> refCompObj;
			if (var->Tags.TryGetValue("ComponentReference", refCompObj))
			{
				auto refComp = refCompObj.As<StringObject>().Ptr();
				Result.Add(ComponentDependency(refComp->Content, nullptr));
			}
			return var;
		}
	}
}

