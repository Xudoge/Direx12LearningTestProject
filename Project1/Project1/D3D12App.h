
#pragma once

#define D3D_DEBUG_INFO

#include "ToolFunc.h"
#include "GameTime.h"




//定义结构体
struct Vertex
{
	XMFLOAT3 Pos;
	XMCOLOR Color;
};

struct Vertex_Pos
{
	XMFLOAT3 Pos;
};

struct Vertex_Color
{
	XMCOLOR Color;
};



//实例化顶点结构并填充
std::array<Vertex, 8> verties = {
	Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMCOLOR(Colors::White) }),
	Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMCOLOR(Colors::Black) }),
	Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMCOLOR(Colors::Red) }),
	Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMCOLOR(Colors::Green) }),
	Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMCOLOR(Colors::Blue) }),
	Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMCOLOR(Colors::Yellow) }),
	Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMCOLOR(Colors::Cyan) }),
	Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMCOLOR(Colors::Magenta) })
};

std::array<Vertex_Pos, 8> verties_pos = {
	Vertex_Pos({ XMFLOAT3(-1.0f, -1.0f, -1.0f) }),
	Vertex_Pos({ XMFLOAT3(-1.0f, +1.0f, -1.0f) }),
	Vertex_Pos({ XMFLOAT3(+1.0f, +1.0f, -1.0f) }),
	Vertex_Pos({ XMFLOAT3(+1.0f, -1.0f, -1.0f) }),
	Vertex_Pos({ XMFLOAT3(-1.0f, -1.0f, +1.0f) }),
	Vertex_Pos({ XMFLOAT3(-1.0f, +1.0f, +1.0f) }),
	Vertex_Pos({ XMFLOAT3(+1.0f, +1.0f, +1.0f) }),
	Vertex_Pos({ XMFLOAT3(+1.0f, -1.0f, +1.0f) })
};



std::array<Vertex_Color, 8> verties_color = {
	Vertex_Color({ XMCOLOR(Colors::White) }),
	Vertex_Color({ XMCOLOR(Colors::Black) }),
	Vertex_Color({ XMCOLOR(Colors::Red) }),
	Vertex_Color({ XMCOLOR(Colors::Green) }),
	Vertex_Color({ XMCOLOR(Colors::Blue) }),
	Vertex_Color({ XMCOLOR(Colors::Yellow) }),
	Vertex_Color({ XMCOLOR(Colors::Cyan) }),
	Vertex_Color({ XMCOLOR(Colors::Magenta) })
};

//索引数据
std::array<std::uint16_t, 36> indices = {
	//前
	0,1,2,
	0,2,3,

	//后
	4,6,5,
	4,7,6,

	//左
	4,5,1,
	4,1,0,

	//右
	3,2,6,
	3,6,7,

	//上
	1,5,6,
	1,6,2,

	//下
	4,0,3,
	4,3,7
};

std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutDesc =
{
	  { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	 // { "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }

	   { "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
};


//单个物体的常量数据
struct ObjectConstants
{
	//初始化物体空间变换到裁剪空间矩阵，Identity4x4()是单位矩阵，需要包含MathHelper头文件
	XMFLOAT4X4 worldViewProj=MathHelper::Identity4x4();
		
};


LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lparam);

class D3D12App{

private:
	static D3D12App* mApp;
	HINSTANCE mhAppInst = nullptr;

public:
	D3D12App();
	D3D12App(HINSTANCE hInstance);
	virtual ~D3D12App();

public:

	virtual void OnMouseDown(WPARAM btnState, int x, int y) = 0;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) = 0;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) = 0;

public:
	static D3D12App* GetApp();


	bool isAppPaused = false;
	bool isMinimized = false;
	bool isMaximized = false;
	bool isResizing = false;

public:
	float clientWidth = 1280.0f;
	float clientHeight = 720.0f;
public:
	HWND mhMainWnd = 0;

	

	bool initWindow(HINSTANCE hInstance, int nShowCmd);
	int Run();
	virtual void OnResize();
	HRESULT MsgProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);


	UINT mCurrentBackBuffer =0;

	virtual void Draw()=0;
	virtual void Update()=0;
	bool init(HINSTANCE hinstance, int nShowCmd);

	//初始化DirectX12。大致步骤如下

	//1.开启D3D12调试层
	//2.创建设备
	//3.创建围栏，同步CPU和GPU
	//4.获取描述符大小
	//5.设置MSAA抗锯齿属性
	//6.创建命令队列，命令列表，命令分配器
	//7.创建交换链
	//8.创建描述符堆
	//9.创建描述符
	//10.资源转换
	//11.设置视口和裁剪矩形
	//12.设置围栏刷新命令队列
	//13.将命令从列表传至队列


	//---------------------
	//Beign init
	//---------------------

	//2
	ComPtr<IDXGIFactory4> dxgiFactory;
	ComPtr<ID3D12Device> d3dDevice;   //设备
	void CreateDevice();


	//3
	ComPtr<ID3D12Fence> fence;    //围栏
	void CreateFence();

	//4
	UINT rtvDescriptorSize;              //渲染目标缓冲区描述符
	UINT dsvDescriptorSize;				 //深度模板缓冲区描述符
	UINT cbv_srv_uavDescriptorSize;		 //常量缓冲区描述符，着色器资源描述符和随机访问缓冲描述符
	void GetDescriptorSize();			 //获取描述符大小，之后通过偏移找到堆中的描述符元素

	//5
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels;     //多重采样属性结构体
	void SetMSAA();


	//6  首先CPU创建命令列表，然后将关联在命令分配器上的命令传入命令列表，最后将命令传入命令队列给GPU处理。
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	ComPtr<ID3D12CommandQueue> cmdQueue;
	ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> cmdList;  //注意此处接口是GraphicsCommandList
	void CreateCommandObject();

	//7 由IDXGIFactory创建交换链  这里禁用MSAA多重采样，因为设置比较麻烦，这里直接设置MSAA会出错
	ComPtr<IDXGISwapChain> swapChain;  //交换链结构体
	DXGI_SWAP_CHAIN_DESC swapChainDesc; //交换链描述结构体
	void CreateSwapChain();

	//8 描述符堆是存放描述符的一段连续空间。因为是双后台缓冲，所以我们存放2个PTV的PTV堆，一个DSV缓存(深度缓存只有一个)
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc;
	ComPtr<ID3D12DescriptorHeap> dsvHeap;
	void CreateDescriptorHeap();

	//9 
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle;
	ComPtr<ID3D12Resource> swapChainBuffer[2];
	void CreateRTV();

	ComPtr<ID3D12Resource>  depthStencilBuffer;
	D3D12_RESOURCE_DESC dsvResourDesc;   //资源描述符
	CD3DX12_CLEAR_VALUE optClear;	//清理资源优化值
	void CreateDSV();


	//10
	//void SetTransition();


	//11
	D3D12_VIEWPORT viewPort;
	D3D12_RECT scissorRect;
	void CreateViewPortAndScissorRect();


	//12 创建围栏 

	int mCurrentFence = 0;  //初始化CPU上的围栏点为0
	void FlushCmdQueue();


	//13 
	//void ExecuteLists();




	bool initDirecX3D(){

		/*开启D3D12调试层*/
		#if defined(DEBUG) || defined(_DEBUG)
		{
			ComPtr<ID3D12Debug> debugController;
			ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
			debugController->EnableDebugLayer();
		}
		#endif


		CreateDevice();
		CreateFence();
		GetDescriptorSize();
		SetMSAA();
		CreateCommandObject();
		CreateSwapChain();
		CreateDescriptorHeap();
		//CreateRTV();
		//CreateDSV();
		//
		
		//CreateLists();
		/*CreateViewPortAndScissorRect();*/
		
		return true;
	}

	

	//计算fps
	GameTime gt;
	void CalculateFrameState();

};

class D3D12InitApp :public D3D12App {

public:
	D3D12InitApp(HINSTANCE  hInstance);
	~D3D12InitApp();

	//将cpu的顶点数据复制到GPU上，先创建上传堆，然后创建gpu只读的默认堆，将数据复制。返回默认堆
	ComPtr<ID3D12Resource> CreateDefaultBuff(UINT64 byteSize, const void* initData, ComPtr<ID3D12Resource>& uploadBuffer);



	void CreateConstantBufferView();
	void BuildRootSignature();
	void BuildByteCodeAndInputLayout();
	void BuildGeometry();
	void BuildPSO();

	bool Init(HINSTANCE hInstance, int nShowCmd, std::wstring customCaption);

	D3D12_VERTEX_BUFFER_VIEW GetVbv() const;

	D3D12_VERTEX_BUFFER_VIEW GetVbv_Pos() const;
	D3D12_VERTEX_BUFFER_VIEW GetVbv_Color() const;

	D3D12_INDEX_BUFFER_VIEW GetIbv() const;

	
	
	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;

	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;

	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;


	virtual void OnResize() override;

private:
	virtual void Draw() override; 
	virtual void Update() override;




	//UINT vbByteSize;

	UINT vbByteSize_Pos;
	UINT vbByteSize_Color;

	UINT ibByteSize;
	//上传堆
	//ComPtr<ID3D12Resource> vertexBufferUpLoader = nullptr;

	ComPtr<ID3D12Resource> vertexPosBufferUpLoader = nullptr;
	ComPtr<ID3D12Resource> vertexColorBufferUpLoader = nullptr;

	ComPtr<ID3D12Resource> indexBufferUpLoader = nullptr;
	//内存空间缓冲区
	//ComPtr<ID3DBlob> vertexBufferCpu = nullptr;


	ComPtr<ID3DBlob> vertexPosBufferCpu = nullptr;
	ComPtr<ID3DBlob> vertexColorBufferCpu = nullptr;

	ComPtr<ID3DBlob> indexBufferCpu = nullptr;
	//GPU创建的缓冲区
	//ComPtr<ID3D12Resource> vertexBufferGpu =nullptr;

	ComPtr<ID3D12Resource> vertexPosBufferGpu = nullptr;
	ComPtr<ID3D12Resource> vertexColorBufferGpu = nullptr;


	ComPtr<ID3D12Resource> indexBufferGpu = nullptr;

	//常量缓冲区
	std::unique_ptr<UploadBufferResource<ObjectConstants>> objCB = nullptr;

	//cbv堆
	ComPtr<ID3D12DescriptorHeap> cbvHeap;


	ComPtr<ID3D12RootSignature> rootSignature = nullptr;

	ComPtr<ID3DBlob> vsBytecode = nullptr;
	ComPtr<ID3DBlob> psBytecode = nullptr;

	ComPtr<ID3D12PipelineState> PSO = nullptr;

	XMFLOAT4X4 world = MathHelper::Identity4x4();
	XMFLOAT4X4 view = MathHelper::Identity4x4();
	XMFLOAT4X4 proj = MathHelper::Identity4x4();


	POINT lastMousePos;
	float theta = (float)1.5 * XM_PI;
	float phi= XM_PIDIV4;
	float radius = 10.0F;
};






