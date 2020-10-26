

#include "ToolFunc.h"




DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
	ErrorCode(hr),
	FunctionName(functionName),
	Filename(filename),
	LineNumber(lineNumber)
{
}

std::wstring DxException::ToString() const
{
	//获得错误代码的string描述
	_com_error err(ErrorCode);
	std::wstring msg = err.ErrorMessage();
	return FunctionName + L" failed in " + Filename + L"; Line " + std::to_wstring(LineNumber) + L"; error" + msg;
}

void OutputDebugPrintf(const char* strOutputString, ...)
{
	char strBuffer[4096] = { 0 };
	va_list vlArgs;
	va_start(vlArgs, strOutputString);
	_vsnprintf_s(strBuffer, sizeof(strBuffer) - 1, strOutputString, vlArgs);
	//vsprintf(strBuffer, strOutputString, vlArgs);
	va_end(vlArgs);
	OutputDebugString(CA2W(strBuffer)+"\n");
}

UINT CalcConstantBufferByteSize(UINT byteSize)
{

	return (byteSize + 255) & ~255;

}

ComPtr<ID3DBlob> CompileShader(
	const std::wstring& fileName,
	const D3D_SHADER_MACRO* defines,
	const std::string& enteryPoint,
	const std::string& target)
{
	//若处于调试模式，则使用调试标志
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	//用调试模式来编译着色器 | 指示编译器跳过优化阶段
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // defined(DEBUG) || defined(_DEBUG)

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(fileName.c_str(), //hlsl源文件名
		defines,	//高级选项，指定为空指针
		D3D_COMPILE_STANDARD_FILE_INCLUDE,	//高级选项，可以指定为空指针
		enteryPoint.c_str(),	//着色器的入口点函数名
		target.c_str(),		//指定所用着色器类型和版本的字符串
		compileFlags,	//指示对着色器断代码应当如何编译的标志
		0,	//高级选项
		&byteCode,	//编译好的字节码
		&errors);	//错误信息

	if (errors != nullptr)
	{
		OutputDebugStringA((char*)errors->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	return byteCode;
}