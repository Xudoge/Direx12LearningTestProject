#pragma once

#define D3D_DEBUG_INFO

#include <Windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <DirectXPackedVector.h>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <windowsx.h>
#include <comdef.h>

#include "MathHelper.h"
#include "d3dx12.h"


#define ThrowIfFailed
#define DEBUG
#define _DEBUG

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")



#include "atlbase.h"
#include "atlstr.h"
void OutputDebugPrintf(const char* strOutputString, ...);


#ifndef ThrowIfFailed
#define ThrowIfFailed(x)	\
{\
	HRESULT hr__=(x);\
	std::wstring wfn = AnsiToWString(__FILE__);\
	if(FAILED(hr__)){throw DxException(hr__,L#x,wfn,__FILE__);}\
}
#endif // !ThrowIfFailed



//AnsiToWString函数（转换成宽字符类型的字符串，wstring）
//在Window平台上，我们应该都使用wstring和wchar_t,处理方式是在字符串前面+L
inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}


//DxExpception类
class DxException {
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);
	std::wstring ToString() const;
	HRESULT ErrorCode = S_OK;
	std::wstring FunctionName;
	std::wstring Filename;
	int LineNumber = -1;
};

//宏定义ThrowIfFailed


// (a+b)~b 返回 （b+1）倍数的 相对于a的最小上界
UINT CalcConstantBufferByteSize(UINT byteSize);



template<typename T>
class UploadBufferResource {

	public:

		UploadBufferResource(ComPtr<ID3D12Device> d3dDevice, UINT elementCount, bool isConstantBuffer) : mIsConstantBuffer(isConstantBuffer)
		{
			//由于硬件原因，常量的长度必须是256的倍数
			if (!isConstantBuffer)
			{
				elementByteSize = sizeof(T);
			}
			else
			{
				elementByteSize = CalcConstantBufferByteSize(sizeof(T));
			}
			
			//创建上传堆
			ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),  //上传堆
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(elementByteSize * elementCount),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&uploadBuffer)
			));

			//返回欲更新资源的指针
			uploadBuffer->Map(0,   //子资源索引，对于缓冲区，其子资源就是他自己
				nullptr, //对整个资源进行映射
				ThrowIfFailed(reinterpret_cast<void**>(&mappedData)) //带返回映射的资源数
			);
		};

		~UploadBufferResource()
		{
			if (uploadBuffer != nullptr)
				uploadBuffer->Unmap(0, nullptr);//取消映射

			mappedData = nullptr;//释放映射内存
		};

		void CopyData(int elementIndex, const T& Data)
		{
			memcpy(&mappedData[elementIndex * elementByteSize], &Data, sizeof(T));
		};
		
		ComPtr<ID3D12Resource> Resource()const
		{
			return uploadBuffer;
		};

private:
	

	BYTE* mappedData =nullptr;
	bool mIsConstantBuffer = false;
	ComPtr<ID3D12Resource> uploadBuffer = nullptr;
	UINT elementByteSize = 0;
};

ComPtr<ID3DBlob>  CompileShader(
	const std::wstring& fileName,
	const D3D_SHADER_MACRO* defines,
	const std::string& enteryPoint,
	const std::string& target);
