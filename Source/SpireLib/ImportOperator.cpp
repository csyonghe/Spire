#include "ImportOperator.h"
using namespace Spire::Compiler;
using namespace CoreLib::Basic;

void DestroyImportOperatorHanlders(CoreLib::Basic::List<Spire::Compiler::ImportOperatorHandler *> & handlers)
{
	for (auto handler : handlers)
		delete handler;
	handlers.Clear();
}

void DestroyExportOperatorHanlders(CoreLib::Basic::List<Spire::Compiler::ExportOperatorHandler *> & handlers)
{
	for (auto handler : handlers)
		delete handler;
	handlers.Clear();
}