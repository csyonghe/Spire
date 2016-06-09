#ifndef IMPORT_OPERATOR_HANDLERS_H
#define IMPORT_OPERATOR_HANDLERS_H

#include "../SpireCore/ShaderCompiler.h"

void CreateGLSLImportOperatorHandlers(CoreLib::Basic::List<Spire::Compiler::ImportOperatorHandler *> & handlers);
void CreateGLSLExportOperatorHandlers(CoreLib::Basic::List<Spire::Compiler::ExportOperatorHandler *> & handlers);
void DestroyImportOperatorHanlders(CoreLib::Basic::List<Spire::Compiler::ImportOperatorHandler *> & handlers);
void DestroyExportOperatorHanlders(CoreLib::Basic::List<Spire::Compiler::ExportOperatorHandler *> & handlers);

#endif