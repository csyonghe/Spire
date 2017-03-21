#include "D3DCompiler.h"
#ifdef _WIN32

#include <d3dcompiler.h>

using namespace CoreLib::Basic;

typedef HRESULT (WINAPI *D3DCompile2Func)(
	LPCVOID pSrcData,
	SIZE_T SrcDataSize,
	LPCSTR pSourceName,
	D3D_SHADER_MACRO * pDefines,
	ID3DInclude * pInclude,
	LPCSTR pEntrypoint,
	LPCSTR pTarget,
	UINT Flags1,
	UINT Flags2,
	UINT SecondaryDataFlags,
	LPCVOID pSecondaryData,
	SIZE_T SecondaryDataSize,
	ID3DBlob **ppCode,
	ID3DBlob **ppErrorMsgs
);
typedef HRESULT (WINAPI *D3DCreateBlobFunc)(
	SIZE_T Size,
	ID3DBlob **ppBlob
);


class D3DCompilerImpl : public D3DCompiler
{
public:
	D3DCompile2Func D3DCompile2;
	D3DCreateBlobFunc D3DCreateBlob;
	HMODULE Lib;
	virtual bool Compile(CoreLib::String input, CoreLib::String stageName, CoreLib::String & errMsg) override
	{
		auto entryPoint = "main";
		char * profile = "ps_5_0";
		if (stageName == "vs")
			profile = "vs_5_0";
		else if (stageName == "tes")
			profile = "hs_5_0";
		else if (stageName == "tcs")
			profile = "ds_5_0";
		ID3DBlob *code, *err = nullptr;
		D3DCompile2(input.Buffer(), input.Length(), "", nullptr, nullptr, entryPoint, profile, 0, 0, 0, 0, 0, &code, &err);
		if (err != nullptr)
		{
			errMsg = (char*)err->GetBufferPointer();
			return false;
		}
		return true;
	}
};

D3DCompiler * LoadD3DCompiler()
{
	auto d3dLib = LoadLibraryW(L"D3DCompiler_47.dll");
	if (d3dLib)
	{
		auto compileFunc = (D3DCompile2Func)GetProcAddress(d3dLib, "D3DCompile2");
		auto createBlobFunc = (D3DCreateBlobFunc)GetProcAddress(d3dLib, "D3DCreateBlob");
		if (compileFunc && createBlobFunc)
		{
			D3DCompilerImpl* result = new D3DCompilerImpl();
			result->D3DCompile2 = compileFunc;
			result->D3DCreateBlob = createBlobFunc;
			result->Lib = d3dLib;
			return result;
		}
		else
			FreeLibrary(d3dLib);
	}
	return nullptr;
}

#else

D3DCompiler * LoadD3DCompiler()
{
	return nullptr;
}

#endif
