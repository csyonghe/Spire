#ifndef SPIRE_D3D_COMPILER_H
#define SPIRE_D3D_COMPILER_H

#include "CoreLib/Basic.h"

class D3DCompiler : public CoreLib::Object
{
public:
	virtual bool Compile(CoreLib::String input, CoreLib::String stageName, CoreLib::String & errMsg) = 0;
};

D3DCompiler * LoadD3DCompiler();

#endif