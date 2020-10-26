

#include "D3D12App.h"
#include <assert.h>



//窗口过程函数
LRESULT CALLBACK  MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lparam)
{

	return D3D12App::GetApp()->MsgProc(hwnd, msg, wParam, lparam);
}

D3D12App* D3D12App::mApp = nullptr;

D3D12App::D3D12App(HINSTANCE hInstance):mhAppInst(hInstance)
{
	mApp = this;
}

D3D12App::D3D12App()
{

}

D3D12App::~D3D12App()
{
	if (d3dDevice!=nullptr)
	{
		FlushCmdQueue();
	}
}

D3D12App* D3D12App::GetApp()
{
	return mApp;
}

bool D3D12App::initWindow(HINSTANCE hInstance, int nShowCmd)
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;  //表示工作区区域宽高改变，则重新绘制窗口
	wc.lpfnWndProc = MainWndProc;		 //指定窗口过程
	wc.cbClsExtra = 0;                   //借助这两个字段来为当前应用分配额外的内存空间（这里不分配，所以置为0）
	wc.cbWndExtra = 0;					 //借助这两个字段来为当前应用分配额外的内存空间（这里不分配，所以置为0）
	wc.hInstance = hInstance;			 //应用程序实例句柄
	wc.hIcon = LoadIcon(0, IDC_ARROW);   //使用默认的应用程序图标
	wc.hCursor = LoadCursor(0, IDC_ARROW); //使用标准的鼠标样式
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); //指定白色背景画笔刷句柄
	wc.lpszMenuName = 0;                 //没有菜单栏
	wc.lpszClassName = L"MainWnd";		 //窗口名

	//窗口类注册失败
	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegsisterClass Failed", 0, 0);
		return false;
	}

	//窗口类注册成功
	RECT R; //裁剪矩形
	R.left = 0;
	R.top = 0;
	R.right = clientWidth;
	R.bottom = clientHeight;
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);   //根据窗口的客户去大小计算窗口大小
	int width = R.right - R.left;
	int hight = R.bottom - R.top;

	//创建主窗口
	mhMainWnd = CreateWindow(L"MainWnd", L"DX12Init", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, hight, 0, 0, hInstance, 0);
	//窗口创建失败
	if (!mhMainWnd)
	{
		MessageBox(0, L"CreateWindow Failed", 0, 0);
		return 0;
	}

	ShowWindow(mhMainWnd, nShowCmd);
	UpdateWindow(mhMainWnd);
	

	return true;
}

int D3D12App::Run()
{
	//消息循环
	//定义消息结构体
	MSG msg = { 0 };

	//每次循环开始重置计数器
	gt.Reset();

	

	while (msg.message!=WM_QUIT)
	{
		

		//如果有窗口消息就进行处理
		if (PeekMessage(&msg,0,0,0,PM_REMOVE) )  //Peek函数会自动填充msg机构体元素
		{
			TranslateMessage(&msg);  //键盘按键转换，将虚拟信息转换为字符信息
			DispatchMessage(&msg);   //把信息分派给相应的窗口过程
		}
		//否则就执行动画和游戏逻辑
		else
		{

			gt.Tick();  //计算每两帧间隔时间

			

			if (!gt.IsStoped()) //如果不是暂停状态，才运行游戏
			{
				CalculateFrameState();
				Update();
				Draw();
				
			}
			//如果是暂停状态，则休眠100秒
			else
			{
				Sleep(100);
			}
			
		}

	}

	return (int)msg.wParam;
}

void D3D12App::OnResize()
{
	assert(d3dDevice);
	assert(swapChain);
	assert(cmdAllocator);


	FlushCmdQueue();
	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr));

	//释放之前的资源，为重新创建做准备
	for (int i=0;i < 2;i++ )
	{
		swapChainBuffer[i].Reset();
	}

	//重新调整后台缓冲区资源的大小
	ThrowIfFailed(swapChain->ResizeBuffers(2,
		clientWidth,
		clientHeight,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrentBackBuffer = 0;

	CreateRTV();
	CreateDSV();
	CreateViewPortAndScissorRect();

}

HRESULT D3D12App::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

	switch (msg)
	{

	
	case WM_SIZE:
		clientWidth = LOWORD(lParam);
		clientHeight = HIWORD(lParam);

		if (d3dDevice)
		{
				//最小化
				if (wParam==SIZE_MINIMIZED)
				{
					isAppPaused = true;
					isMinimized = true;
					isMaximized = false;

				}
				else if (wParam==SIZE_MAXHIDE)
				{
					isAppPaused = false;
					isMaximized = false;
					OnResize();
				}else if (wParam==SIZE_RESTORED)
				{
					if (isMinimized)
					{
						isAppPaused = false;
						isMinimized = false;
						OnResize();
					}
					else if (isMaximized)
					{
						isAppPaused = false;
						isMaximized = false;
						OnResize();
					}

					else if (isResizing)
					{

					}
					else
					{
						OnResize();
					}
				}
		}
	

		

		return 0;

		//鼠标按下
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:

		//wParam为输入的虚拟键代码
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

		//鼠标按键抬起时触发
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

		//鼠标移动时触发
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

		//窗口销毁时，终止循环
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	default:
		break;
	}


	//将上面没有处理的消息转发给默认的窗口过程
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool  D3D12App::init(HINSTANCE hinstance,int nShowCmd)
{

	if (!initWindow(hinstance,nShowCmd))
	{
		return false;
	}else if (!initDirecX3D())
	{
		return false;
	}

	OnResize();

	return true;
}




#pragma region D3D

void  D3D12App::CreateDevice()
{
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));
	ThrowIfFailed(D3D12CreateDevice(nullptr,   //D3Dadpater 显卡适配器 若为0则是主适配器
	D3D_FEATURE_LEVEL_12_1,					   //特征等级  表示支持的d3d版本  这里12表示最低为dx12
	IID_PPV_ARGS(&d3dDevice)));				   //返回所创建的设备
}

void  D3D12App::CreateFence()
{
	ThrowIfFailed(d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
}

void D3D12App::GetDescriptorSize()
{
	rtvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	dsvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	cbv_srv_uavDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void  D3D12App::SetMSAA()
{
	msaaQualityLevels.Format = DXGI_FORMAT_B8G8R8X8_UNORM; //UNORM是归一化处理的无符号整数
	msaaQualityLevels.SampleCount = 4;
	msaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msaaQualityLevels.NumQualityLevels = 0;

	//当前图形驱动对Msaa多重采样的支持（注意：第二个参数又是输入，又是输出）
	ThrowIfFailed(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaQualityLevels, sizeof(msaaQualityLevels)));

	//NumQualityLevels在check函数里会进行设置
	//如果支持MSAA,则Check函数会返回的NumQualityLevel>0
	//expression为假（为0），则程序终止，并打印一条信息
	assert(msaaQualityLevels.NumQualityLevels > 0);

}

void  D3D12App::CreateCommandObject()
{
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;


	ThrowIfFailed(d3dDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&cmdQueue)));
	ThrowIfFailed(d3dDevice->CreateCommandAllocator(commandQueueDesc.Type, IID_PPV_ARGS(&cmdAllocator)));
	ThrowIfFailed(d3dDevice->CreateCommandList(0,   //掩码值为0，表示单cpu
		commandQueueDesc.Type,  //命令列表类型
		cmdAllocator.Get(),     //命令分配器接口指针
		nullptr,                //流水线状态对象PSO,这里不绘制，所以为空
		IID_PPV_ARGS(&cmdList)	//返回创建的命令列表
	));
	cmdList->Close(); //重置命令列表之前必须把它关闭

}

void  D3D12App::CreateSwapChain()
{
	swapChain.Reset();

	swapChainDesc.BufferDesc.Width = clientWidth;   //缓冲区分辨率宽度
	swapChainDesc.BufferDesc.Height = clientHeight;   //缓冲区分辨率高度
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;   //缓冲区的显示格式
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;  //刷新率的分子
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;   //刷新率的分母
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;   //逐行扫描vs隔行扫描（未指定）
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;  //图像相对屏幕的拉伸（未指定）
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;  //将数据渲染至后台缓冲区（即作为渲染目标）
	swapChainDesc.OutputWindow = mhMainWnd;    //渲染窗口句柄
	swapChainDesc.SampleDesc.Count =1;			//多重采样数量
	swapChainDesc.SampleDesc.Quality = 0;		//多重采样质量
	swapChainDesc.Windowed = true;             //是否窗口化
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;  //固定写法
	swapChainDesc.BufferCount = 2;    //后台缓冲区数量
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;   //自适应窗口模式

	//利用DXGI接口下的工厂类创建交换链
	ThrowIfFailed(dxgiFactory->CreateSwapChain(cmdQueue.Get(), &swapChainDesc, swapChain.GetAddressOf()));

}

void  D3D12App::CreateDescriptorHeap()
{
	rtvDescriptorHeapDesc.NumDescriptors = 2;
	rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptorHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvHeap)));

	dsvDescriptorHeapDesc.NumDescriptors = 1;
	dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvDescriptorHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&dsvHeap)));

}

void  D3D12App::CreateRTV()
{
	rtvHeapHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < 2; i++) {

		//获得存于交换链中的后台缓冲资源
		HRESULT hr= swapChain->GetBuffer(i,IID_PPV_ARGS(swapChainBuffer[i].GetAddressOf()) );

		//创建RTV
		d3dDevice->CreateRenderTargetView(swapChainBuffer[i].Get(),
			nullptr,				//在交换链的创建中已经定义了该资源的数据格式，所以这里是空指针
			rtvHeapHandle			//描述符的句柄结构体
		);

		rtvHeapHandle.Offset(1, rtvDescriptorSize);
	}


}

void  D3D12App::CreateDSV()
{
	dsvResourDesc.Alignment = 0; //指定对齐
	dsvResourDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; //指定资源维度
	dsvResourDesc.DepthOrArraySize = 1; //纹理深度为1
	dsvResourDesc.Width = clientWidth; //资源宽
	dsvResourDesc.Height = clientHeight; //资源高
	dsvResourDesc.MipLevels = 1; //MIPMAP层级数量
	dsvResourDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;  //指定纹理布局（这里不指定）
	dsvResourDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; //深度模板资源的Flag
	dsvResourDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; //24位深度，8位模板，还有个无类型格式
	dsvResourDesc.SampleDesc.Count = 1;	//多重采样数量为4 (because of error now set is 1)
	dsvResourDesc.SampleDesc.Quality = 0;

	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//24位深度，8位模板,还有个无类型的格式DXGI_FORMAT_R24G8_TYPELESS也可以使用
	optClear.DepthStencil.Depth = 1; //初始深度为1
	optClear.DepthStencil.Stencil = 0; //初始模板值为0
	


	ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), //堆类型为默认堆（不能写入）
		D3D12_HEAP_FLAG_NONE,  //Flag
		&dsvResourDesc,    //DSV资源指针
		D3D12_RESOURCE_STATE_COMMON,  //资源状态为初始状态
		&optClear,  //优化值指针
		IID_PPV_ARGS(&depthStencilBuffer)  //深度模板资源
	));

	//创建DSV(必须弹出DSV属性结构体，和创建rtv不同，rtv是通过句柄)
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;

	d3dDevice->CreateDepthStencilView(depthStencilBuffer.Get(),
		nullptr,           //如上D3D12_DEPTH_STENCIL_VIEW_DESC类型的结构体指针
							//因为在创建深度模板资源的时候已经定义深度模板数据属性，所以这里指定为空指针
		dsvHeap->GetCPUDescriptorHandleForHeapStart()
	);

	//标记DS状态 因为资源在不同时期有着不同的作用
	cmdList->ResourceBarrier(1,   //Barrier屏障个数
		&CD3DX12_RESOURCE_BARRIER::Transition(depthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,  //转换前状态（创建时的状态，即CreateCommittedResource函数中定义的状态）
			D3D12_RESOURCE_STATE_DEPTH_WRITE //转换后为可写入的深度图 _Read是只读的深度图
		)
	);

	//将命令都进入cmdList，需要用ExecuteCommandLists函数，将命令从命令列表传入命令队列，也是从CPU传入GPU的过程。注意：在传入命令队列前必须关闭命令列表
	ThrowIfFailed(cmdList->Close());    //命令添加完后将其关闭
	ID3D12CommandList* cmdLists[] = { cmdList.Get() };	//声明并定义命令列表数组
	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists); //将命令从命令列表传至命令队列

	FlushCmdQueue();
}


void  D3D12App::CreateViewPortAndScissorRect()
{
	//视口设置
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.Width = clientWidth;
	viewPort.Height = clientHeight;
	viewPort.MaxDepth = 1.0f;
	viewPort.MinDepth = 0.0f;

	//裁剪矩形（矩形以外的像素将被剔除）
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = clientWidth;
	scissorRect.bottom = clientHeight;
}

void  D3D12App::FlushCmdQueue()
{
	
	mCurrentFence++;		//cpuc传完命令并关闭后，将当前围栏值+1
	cmdQueue->Signal(fence.Get(), mCurrentFence);    //当GPU处理完CPU传入的命令后，将Fence接口的围栏值+1，即fence->GetCompletedValue()+1

	if (fence->GetCompletedValue()<mCurrentFence)  //如果小于GPU没有处理完所有命令
	{
		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");   //创建事件
		fence->SetEventOnCompletion(mCurrentFence, eventHandle);  //当围栏达到CurrentFence值的时候，执行eventHandle事件
		WaitForSingleObject(eventHandle, INFINITE);  //等待GPU命中围栏，激活事件（阻塞当前线程直到事件触发，注意此event需要先设置再等待）
													//如果没有Set就Wait就死锁了

		CloseHandle(eventHandle);


	}


	OutputDebugPrintf(  std::to_string(fence->GetCompletedValue()).c_str());


	
	

}



#pragma endregion D3D

void  D3D12App::CalculateFrameState()
{
	static int frameCnt = 0; //总帧数
	static float timeElapsed = 0.0f; //流逝的时间
	frameCnt++; //每帧++，经过一秒后即为FPS值

	

	//判断模块
	if ((gt.TotalTime()-timeElapsed)>=1.f)  //一旦》=0，说明刚好过了一秒
	{
		float fps = (float)frameCnt;   // 多少帧

		float mspf = 1000.0f / fps; //每帧多少毫秒

		//将帧数显示再窗口上
		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);
		std::wstring windowText = L"D3D12Init  fps:" + fpsStr + L" " + L"mspf:" + mspfStr;

		SetWindowText(mhMainWnd, windowText.c_str());

		//为一帧重置数值
		frameCnt = 0;
		timeElapsed += 1.f;
	}

}

D3D12InitApp::D3D12InitApp(HINSTANCE hInstance)
	: D3D12App(hInstance)
{

}

D3D12InitApp::~D3D12InitApp()
{

}

ComPtr<ID3D12Resource> D3D12InitApp::CreateDefaultBuff(UINT64 byteSize, const void* initData, ComPtr<ID3D12Resource>& uploadBuffer)
{
	///创建上传堆，作用是：写入CPU内存数据，并传输给默认堆
	ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), //创建上传堆类型的堆
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),//变体的构造函数，传入byteSize，其他均为默认值，简化书写
		D3D12_RESOURCE_STATE_GENERIC_READ,	//上传堆里的资源需要复制给默认堆，所以是可读状态
		nullptr,	//不是深度模板资源，不用指定优化值
		IID_PPV_ARGS(&uploadBuffer)));

	//创建默认堆，作为上传堆的数据传输对象
	ComPtr<ID3D12Resource> defaultBuffer;
	ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),//创建默认堆类型的堆
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,//默认堆为最终存储数据的地方，所以暂时初始化为普通状态
		nullptr,
		IID_PPV_ARGS(&defaultBuffer)));

	//将资源从COMMON状态转换到COPY_DEST状态（默认堆此时作为接收数据的目标）
	cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST));

	//将数据从CPU内存拷贝到GPU缓存
	D3D12_SUBRESOURCE_DATA subResourceData;
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;
	//核心函数UpdateSubresources，将数据从CPU内存拷贝至上传堆，再从上传堆拷贝至默认堆。1是最大的子资源的下标（模板中定义，意为有2个子资源）
	UpdateSubresources<1>(cmdList.Get(), defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

	//再次将资源从COPY_DEST状态转换到GENERIC_READ状态(现在只提供给着色器访问)
	cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_GENERIC_READ));

	return defaultBuffer;
}


void D3D12InitApp::BuildFrameResources()
{
	for (int i=0;i<frameResourcesCount;++i)
	{
		FrameResourcesArray.push_back(std::make_unique<FrameResources>(
			d3dDevice.Get(),
			1,
			(UINT)allRitems.size()
			)
		);
	}
}

void D3D12InitApp::BuildRenderItem()
{
	auto boxRitem = std::make_unique<RenderItem>();

	XMStoreFloat4x4(&(boxRitem->world), XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
	boxRitem->objCBIndex = 0; //box常量数据（world矩阵）在objConstantBuffer索引0上
	boxRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->indexCount = DrawArgs["box"].indexCount;
	boxRitem->baseVertexLocation = DrawArgs["box"].baseVertexLocation;
	boxRitem->startIndexLocation = DrawArgs["box"].startIndexLocation;
	allRitems.push_back(std::move(boxRitem));


	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->world = MathHelper::Identity4x4();
	gridRitem->objCBIndex = 1;//BOX常量数据（world矩阵）在objConstantBuffer索引1上
	gridRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->indexCount = DrawArgs["grid"].indexCount;
	gridRitem->baseVertexLocation = DrawArgs["grid"].baseVertexLocation;
	gridRitem->startIndexLocation = DrawArgs["grid"].startIndexLocation;
	allRitems.push_back(std::move(gridRitem));


	UINT fllowObjCBIndex = 2;
	for (int i = 0; i < 5; i++)
	{
		auto leftCylinderRitem = std::make_unique<RenderItem>();
		auto rightCylinderRitem = std::make_unique<RenderItem>();
		auto leftSphereRitem = std::make_unique<RenderItem>();
		auto rightSphereRitem = std::make_unique<RenderItem>();


		XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
		XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);
		XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
		XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);
		//左边5个圆柱
		XMStoreFloat4x4(&(leftCylinderRitem->world), leftCylWorld);
		//此处的索引随着循环不断加1（注意：这里是先赋值再++）
		leftCylinderRitem->objCBIndex = fllowObjCBIndex++;
		leftCylinderRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylinderRitem->indexCount = DrawArgs["cylinder"].indexCount;
		leftCylinderRitem->baseVertexLocation = DrawArgs["cylinder"].baseVertexLocation;
		leftCylinderRitem->startIndexLocation = DrawArgs["cylinder"].startIndexLocation;
		//右边5个圆柱
		XMStoreFloat4x4(&(rightCylinderRitem->world), rightCylWorld);
		rightCylinderRitem->objCBIndex = fllowObjCBIndex++;
		rightCylinderRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylinderRitem->indexCount = DrawArgs["cylinder"].indexCount;
		rightCylinderRitem->baseVertexLocation = DrawArgs["cylinder"].baseVertexLocation;
		rightCylinderRitem->startIndexLocation = DrawArgs["cylinder"].startIndexLocation;
		//左边5个球
		XMStoreFloat4x4(&(leftSphereRitem->world), leftSphereWorld);
		leftSphereRitem->objCBIndex = fllowObjCBIndex++;
		leftSphereRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->indexCount = DrawArgs["sphere"].indexCount;
		leftSphereRitem->baseVertexLocation = DrawArgs["sphere"].baseVertexLocation;
		leftSphereRitem->startIndexLocation = DrawArgs["sphere"].startIndexLocation;
		//右边5个球
		XMStoreFloat4x4(&(rightSphereRitem->world), rightSphereWorld);
		rightSphereRitem->objCBIndex = fllowObjCBIndex++;
		rightSphereRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRitem->indexCount = DrawArgs["sphere"].indexCount;
		rightSphereRitem->baseVertexLocation = DrawArgs["sphere"].baseVertexLocation;
		rightSphereRitem->startIndexLocation = DrawArgs["sphere"].startIndexLocation;

		allRitems.push_back(std::move(leftCylinderRitem));
		allRitems.push_back(std::move(rightCylinderRitem));
		allRitems.push_back(std::move(leftSphereRitem));
		allRitems.push_back(std::move(rightSphereRitem));
	}
}

void D3D12InitApp::CreateConstantBufferView()
{
	//设置常量缓冲区
	UINT objectCount = (UINT)allRitems.size();//物体总个数（包括实例）
	UINT objConstSize = CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT passConstSize = CalcConstantBufferByteSize(sizeof(PassConstants));

	//创建CBV堆
	D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc;
	cbHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbHeapDesc.NumDescriptors = (objectCount+ 1)*frameResourcesCount;
	cbHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&cbHeapDesc, IID_PPV_ARGS(&cbvHeap)));

	//创建第一个cbv

	//定义并获得物体的常量缓冲区，然后得到其首地址
	//std::unique_ptr<UploadBufferResource<ObjectConstants>> objCB = nullptr;
	//elementCount为1（1个子物体常量缓冲元素），isConstantBuffer为ture（是常量缓冲区）
	//objCB = std::make_unique<UploadBufferResource<ObjectConstants>>(d3dDevice.Get(), objectCount, true);
	

	for (int frameIndex=0;frameIndex<frameResourcesCount;++frameIndex)
	{
		for (int i = 0; i < objectCount; ++i)
		{
			//获得常量缓冲区首地址
			D3D12_GPU_VIRTUAL_ADDRESS address_obj;
			address_obj =FrameResourcesArray[frameIndex]->objCB->Resource()->GetGPUVirtualAddress();
			//通过常量缓冲区元素偏移值计算最终的元素地址
			int cbElementIndex = i;	//常量缓冲区子物体个数（子缓冲区个数）下标
			address_obj += (cbElementIndex * objConstSize);

			int heapIndex =objectCount*frameIndex+i;
			auto handle_obj = CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvHeap->GetCPUDescriptorHandleForHeapStart());
			handle_obj.Offset(heapIndex, cbv_srv_uavDescriptorSize);

			//创建CBV描述符
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc_obj;
			cbvDesc_obj.BufferLocation = address_obj;
			cbvDesc_obj.SizeInBytes = objConstSize;
			d3dDevice->CreateConstantBufferView(&cbvDesc_obj, handle_obj);

		}
	}


	//创建第二个cbv
	for (int frameIndex = 0; frameIndex < frameResourcesCount; ++frameIndex)
	{

		//passCB = std::make_unique<UploadBufferResource<PassConstants>>(d3dDevice.Get(), 1, true);

		D3D12_GPU_VIRTUAL_ADDRESS address_pass;
		address_pass = FrameResourcesArray[frameIndex]->passCB->Resource()->GetGPUVirtualAddress();
		int passElementIndex = 0;	//常量缓冲区元素下标
		address_pass += (passElementIndex * passConstSize);
		//由于是第二个所以heapIndex为1
		int passheapIndex = objectCount*frameResourcesCount+ frameIndex;
		auto handle_pass = CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvHeap->GetCPUDescriptorHandleForHeapStart());
		handle_pass.Offset(passheapIndex, cbv_srv_uavDescriptorSize);
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc_pass;
		cbvDesc_pass.BufferLocation = address_pass;
		cbvDesc_pass.SizeInBytes = passConstSize;
		d3dDevice->CreateConstantBufferView(&cbvDesc_pass, handle_pass);

	}


}

//将各种资源设置到渲染流水线上，并最终发出绘制命令。
void D3D12InitApp::Draw()
{

	//重置cmdAllocator以及cmdList
	auto currCmdAllocator = currFrameResources->cmdAllocator;
	ThrowIfFailed(currCmdAllocator->Reset());  //重复使用记录命令的相关内存
	ThrowIfFailed(cmdList->Reset(currCmdAllocator.Get(), PSO.Get())); //重复使用命令列表以及内存


	//将后台缓冲区从呈现状态到渲染目标状态

	cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[mCurrentBackBuffer].Get(),//转换资源为后台缓冲区资源
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET    //从呈现到渲染目标的转化
		)
	);

	//设置视口和裁剪矩形
	cmdList->RSSetViewports(1, &viewPort);
	cmdList->RSSetScissorRects(1, &scissorRect);


	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(), mCurrentBackBuffer, rtvDescriptorSize);
	cmdList->ClearRenderTargetView(rtvHandle, Colors::AliceBlue, 0, nullptr);//清除RT，并且不设置裁剪矩形


	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
	cmdList->ClearDepthStencilView(dsvHandle,	//DSV描述符句柄
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,	//FLAG
		1.0f,	//默认深度值
		0,	//默认模板值
		0,	//裁剪矩形数量
		nullptr);	//裁剪矩形指针



	//指定要渲染的缓冲区  
	cmdList->OMSetRenderTargets(1,//待绑定的RTV数量
		&rtvHandle,				//指向RTV数组的指针
		true,					//RTV对象再堆内存中是连续存放的
		&dsvHandle				//指向DSV的指针
	);



	//设置CBV描述符堆
	ID3D12DescriptorHeap* descriHeaps[] = { cbvHeap.Get() };//注意这里之所以是数组，是因为还可能包含SRV和UAV，而这里我们只用到了CBV
	cmdList->SetDescriptorHeaps(_countof(descriHeaps), descriHeaps);
	//设置根签名
	cmdList->SetGraphicsRootSignature(rootSignature.Get());

	//pass描述符
	int passCbvIndex = (int)allRitems.size()*frameResourcesCount+currFrameResourcesIndex;
	auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvHeap->GetGPUDescriptorHandleForHeapStart());
	passCbvHandle.Offset(passCbvIndex, cbv_srv_uavDescriptorSize);
	cmdList->SetGraphicsRootDescriptorTable(1, //根参数的起始索引
		passCbvHandle);

	//obj描述符
	DrawRenderIitem();



	////设置顶点缓冲区
	//cmdList->IASetVertexBuffers(0, 1, &GetVbv());

	///*cmdList->IASetVertexBuffers(0, 1, &GetVbv_Pos());
	//cmdList->IASetVertexBuffers(1, 1, &GetVbv_Color());*/
	////设置索引缓冲区
	//cmdList->IASetIndexBuffer(&GetIbv());
	////将图元拓扑类型传入流水线
	//cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	////设置根描述符表

	//auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvHeap->GetGPUDescriptorHandleForHeapStart());
	//int HeadIndex = 0;
	//handle.Offset(HeadIndex, cbv_srv_uavDescriptorSize);
	//cmdList->SetGraphicsRootDescriptorTable(0, //根参数的起始索引
	//	handle);

	//handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvHeap->GetGPUDescriptorHandleForHeapStart());
	//HeadIndex = 1;
	//handle.Offset(HeadIndex, cbv_srv_uavDescriptorSize);
	//cmdList->SetGraphicsRootDescriptorTable(1, //根参数的起始索引
	//	handle);


	////绘制顶点（通过索引缓冲区绘制）
	////cmdList->DrawIndexedInstanced(sizeof(indices), //每个实例要绘制的索引数
	////	1,	//实例化个数
	////	0,	//起始索引位置
	////	0,	//子物体起始索引在全局索引中的位置
	////	0);
	//cmdList->DrawIndexedInstanced(DrawArgs["box"].indexCount,
	//	1,
	//	DrawArgs["box"].startIndexLocation, //起始索引位置
	//	DrawArgs["box"].baseVertexLocation, //子物体的顶点起始位置
	//	0 //实例化的高级技术，暂时设置为0
	//	);
	//cmdList->DrawIndexedInstanced(DrawArgs["grid"].indexCount, //每个实例要绘制的索引数
	//	1,	//实例化个数
	//	DrawArgs["grid"].startIndexLocation,	//起始索引位置
	//	DrawArgs["grid"].baseVertexLocation,	//子物体起始索引在全局索引中的位置
	//	0);	//实例化的高级技术，暂时设置为0
	//cmdList->DrawIndexedInstanced(DrawArgs["sphere"].indexCount, //每个实例要绘制的索引数
	//	1,	//实例化个数
	//	DrawArgs["sphere"].startIndexLocation,	//起始索引位置
	//	DrawArgs["sphere"].baseVertexLocation,	//子物体起始索引在全局索引中的位置
	//	0);	//实例化的高级技术，暂时设置为0
	//cmdList->DrawIndexedInstanced(DrawArgs["cylinder"].indexCount, //每个实例要绘制的索引数
	//	1,	//实例化个数
	//	DrawArgs["cylinder"].startIndexLocation,	//起始索引位置
	//	DrawArgs["cylinder"].baseVertexLocation,	//子物体起始索引在全局索引中的位置
	//	0);	//实例化的高级技术，暂时设置为0

	//等待渲染完成，将后台缓冲区该为呈现状态，使之推到前台缓冲区显示。完了，关闭命令列表，等待传入命令队列
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[mCurrentBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	)
	);

	//完成命令后关闭列表
	ThrowIfFailed(cmdList->Close());

	//等CPU将命令都准备好后，需要将执行的命令列表加入GPU的命令队列
	ID3D12CommandList* commandists[] = { cmdList.Get() }; //声明并定义命令列表数组
	cmdQueue->ExecuteCommandLists(_countof(commandists), commandists); //将命令从命令列表传至命令队列

	//然后交换前后台缓冲区引索
	ThrowIfFailed(swapChain->Present(0, 0));
	mCurrentBackBuffer = (mCurrentBackBuffer + 1) % 2;

	//FlushCmdQueue();

	mCurrentFence++;
	currFrameResources->fenceCPU = mCurrentFence;
	cmdQueue->Signal(fence.Get(), mCurrentFence);
}


void D3D12InitApp::DrawRenderIitem()
{
	std::vector<RenderItem*> ritems;
	for (auto& e:allRitems)
	{
		ritems.push_back(e.get());
	}


	for (size_t i=0;i<ritems.size();i++)
	{
		auto ritem = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &GetVbv());
		cmdList->IASetIndexBuffer(&GetIbv());
		cmdList->IASetPrimitiveTopology(ritem->primitiveType);

		UINT objCbvIndex = currFrameResourcesIndex*(UINT)ritems.size()+ritem->objCBIndex;
		auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvHeap->GetGPUDescriptorHandleForHeapStart());
		handle.Offset(objCbvIndex, cbv_srv_uavDescriptorSize);
		cmdList->SetGraphicsRootDescriptorTable(0, //根参数起始索引
			handle);

		cmdList->DrawIndexedInstanced(ritem->indexCount,
			1,
			ritem->startIndexLocation,
			ritem->baseVertexLocation,
			0
		);
	}

}

void D3D12InitApp::BuildRootSignature()
{

	//根参数可以是描述符表，根描述符，根常量
	//根签名作用是将常量数据绑定至寄存器槽，供着色器程序访问。因为现在我们有2个常量数据结构体，所以要创建2个元素的根参数，即2个CBV表，并绑定2个寄存器槽。
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];


	//创建由单个CBV所组成的描述符表
	CD3DX12_DESCRIPTOR_RANGE cbvTable0;
	cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,  //描述符类型
		1, //描述符数量
		0  //描述符所绑定的寄存器槽号
	);

	slotRootParameter[0].InitAsDescriptorTable(1,     // Number of ranges
		&cbvTable0
	);

	CD3DX12_DESCRIPTOR_RANGE cbvTable1;
	cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,  //描述符类型
		1, //描述符数量
		1  //描述符所绑定的寄存器槽号
	);

	slotRootParameter[1].InitAsDescriptorTable(1,     // Number of ranges
		&cbvTable1
	);


	//根签名由一根参数构成
	CD3DX12_ROOT_SIGNATURE_DESC rootSig(2, //根参数的数量
		slotRootParameter, //根参数指针
		0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	//由单个寄存器来创建一个根签名，该槽位指向一个仅包含单个常量缓冲区的描述符区域
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSig, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob);

	if (errorBlob!=nullptr)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(hr);

	

	ThrowIfFailed(d3dDevice->CreateRootSignature(0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature)

	)
	);
}

void D3D12InitApp::BuildByteCodeAndInputLayout()
{
	HRESULT hr = S_OK;
	vsBytecode = CompileShader(L"color.hlsl", nullptr, "VS", "vs_5_0");
	psBytecode = CompileShader(L"color.hlsl", nullptr, "PS", "ps_5_0");
}

void D3D12InitApp::BuildGeometry()
{

	//创建多个几何体
	ProceduralGeometry proceGeo;
	MeshData grid = proceGeo.CreateGrid(160.0f, 160.0f, 50, 50);

	//计算顶点数地址偏移	
	SubmeshGeometry gridSubMesh;
	gridSubMesh.indexCount=(UINT)gridSubmesh


	//计算在索引数组中的偏移
	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = (UINT)grid.Indices32.size() + gridIndexOffset;
	UINT cylinderIndexOffset = (UINT)sphere.Indices32.size() + sphereIndexOffset;





	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), box.GetIndices16().begin(), box.GetIndices16().end());
	indices.insert(indices.end(), grid.GetIndices16().begin(), grid.GetIndices16().end());
	indices.insert(indices.end(), sphere.GetIndices16().begin(), sphere.GetIndices16().end());
	indices.insert(indices.end(), cylinder.GetIndices16().begin(), cylinder.GetIndices16().end());

	DrawArgs["box"] = boxSubmesh;
	DrawArgs["grid"] = gridSubmesh;
	DrawArgs["sphere"] = sphereSubmesh;
	DrawArgs["cylinder"] = cylinderSubmesh;

	vbByteSize = (UINT)verties.size() * sizeof(Vertex_Old);
	/*vbByteSize_Pos=(UINT)verties_pos.size() * sizeof(Vertex_Pos);
	vbByteSize_Color= (UINT)verties_color.size() * sizeof(Vertex_Color);*/

	ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &vertexBufferCpu));	//创建顶点数据内存空间
	//ThrowIfFailed(D3DCreateBlob(vbByteSize_Pos, &vertexPosBufferCpu));	//创建顶点数据内存空间
	//ThrowIfFailed(D3DCreateBlob(vbByteSize_Color, &vertexColorBufferCpu));

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &indexBufferCpu));	//创建索引数据内存空间

	CopyMemory(vertexBufferCpu->GetBufferPointer(), verties.data(), vbByteSize);	//将顶点数据拷贝至顶点系统内存中
	//CopyMemory(vertexPosBufferCpu->GetBufferPointer(), verties_pos.data(), vbByteSize_Pos);
	//CopyMemory(vertexColorBufferCpu->GetBufferPointer(), verties_color.data(), vbByteSize_Color);

	CopyMemory(indexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);	//将索引数据拷贝至索引系统内存中

	vertexBufferGpu = CreateDefaultBuff(vbByteSize, verties.data(), vertexBufferUpLoader);
	/*vertexPosBufferGpu = CreateDefaultBuff(vbByteSize_Pos, verties_pos.data(), vertexPosBufferUpLoader);
	vertexColorBufferGpu = CreateDefaultBuff(vbByteSize_Color, verties_color.data(), vertexColorBufferUpLoader);*/

	indexBufferGpu = CreateDefaultBuff(ibByteSize, indices.data(), indexBufferUpLoader);
}

//建PSO（PipeLineStateObject），将之前定义的顶点布局描述、着色器程序字节码、光栅器状态、根签名、
//图元拓扑方式、采样方式、混合方式、深度模板状态、RTV格式、DSV格式等等对象绑定到图形流水线上
void D3D12InitApp::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { inputLayoutDesc.data(), (UINT)inputLayoutDesc.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(vsBytecode->GetBufferPointer()), vsBytecode->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(psBytecode->GetBufferPointer()), psBytecode->GetBufferSize() };

	//光删化描述
	CD3DX12_RASTERIZER_DESC rdesc(D3D12_DEFAULT);
	//只描述线段
	//rdesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
	//禁用背面消除
	//rdesc.CullMode = D3D12_CULL_MODE_NONE;

	psoDesc.RasterizerState = rdesc;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;	//0xffffffff,全部采样，没有遮罩
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;	//归一化的无符号整型
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;	//不使用4XMSAA
	psoDesc.SampleDesc.Quality = 0;	////不使用4XMSAA

	
	//ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&PSO)));

	HRESULT hr = d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&PSO));
}


bool D3D12InitApp::Init(HINSTANCE hInstance, int nShowCmd, std::wstring customCaption)
{
	if (!D3D12App::init(hInstance,nShowCmd))
	{
		return false;
	}

	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr));

	
	BuildRootSignature();
	BuildByteCodeAndInputLayout();
	BuildGeometry();
	BuildRenderItem();
	BuildFrameResources();
	CreateConstantBufferView();
	BuildPSO();
	

	ThrowIfFailed(cmdList->Close());
	ID3D12CommandList* cmdLists[] = { cmdList.Get() };
	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCmdQueue();

	return true;
}

D3D12_VERTEX_BUFFER_VIEW D3D12InitApp::GetVbv() const
{
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = vertexBufferGpu->GetGPUVirtualAddress();//顶点缓冲区资源虚拟地址
	vbv.SizeInBytes = vbByteSize;	//顶点缓冲区大小（所有顶点数据大小）
	vbv.StrideInBytes = sizeof(Vertex_Old);	//每个顶点元素所占用的字节数

	return vbv;
}

//D3D12_VERTEX_BUFFER_VIEW D3D12InitApp::GetVbv_Pos() const
//{
//	D3D12_VERTEX_BUFFER_VIEW vbv_pos;
//	vbv_pos.BufferLocation = vertexPosBufferGpu->GetGPUVirtualAddress();//顶点缓冲区资源虚拟地址
//	vbv_pos.SizeInBytes = vbByteSize_Pos;	//顶点缓冲区大小（所有顶点数据大小）
//	vbv_pos.StrideInBytes = sizeof(Vertex_Pos);	//每个顶点元素所占用的字节数
//
//	return vbv_pos;
//}
//
//D3D12_VERTEX_BUFFER_VIEW D3D12InitApp::GetVbv_Color() const
//{
//	D3D12_VERTEX_BUFFER_VIEW vbv_color;
//	vbv_color.BufferLocation = vertexColorBufferGpu->GetGPUVirtualAddress();//顶点缓冲区资源虚拟地址
//	vbv_color.SizeInBytes = vbByteSize_Color;	//顶点缓冲区大小（所有顶点数据大小）
//	vbv_color.StrideInBytes = sizeof(Vertex_Color);	//每个顶点元素所占用的字节数
//
//	return vbv_color;
//}


D3D12_INDEX_BUFFER_VIEW D3D12InitApp::GetIbv() const
{
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = indexBufferGpu->GetGPUVirtualAddress();
	ibv.Format = DXGI_FORMAT_R16_UINT;
	ibv.SizeInBytes = ibByteSize;

	return ibv;
}



void D3D12InitApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	lastMousePos.x = x;
	lastMousePos.y = y;
	SetCapture(mhMainWnd); //在当前线程的指定窗口里，设置鼠标获取
}

void D3D12InitApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture(); //按键抬起后释放鼠标获取
}

void D3D12InitApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState&MK_LBUTTON)!=0) //如果在左键按下状态
	{
		float dx = XMConvertToRadians((x-lastMousePos.x )* 0.25f);
		float dy= XMConvertToRadians((y-lastMousePos.y ) * 0.25f);

		theta += dx;
		phi += dy;

		phi = MathHelper::Clamp(phi, -1.5708f + 0.1f, 1.5708f - 0.1f);
		
	}
	else if((btnState & MK_RBUTTON) != 0)
	{
		float dx = (x - lastMousePos.x) * 0.005f;
		float dy =(y - lastMousePos.y) * 0.005f;

		radius += dx - dy;

		radius = MathHelper::Clamp(radius, 1.0f, 20.0f);
	}



	lastMousePos.x = x;
	lastMousePos.y = y;
}

void D3D12InitApp::OnResize()
{
	D3D12App::OnResize();

	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f*3.1416f, clientWidth/clientHeight,1.0f,1000.0f);
	XMStoreFloat4x4(&proj, p);

}

void D3D12InitApp::Update()
{

	currFrameResourcesIndex = (currFrameResourcesIndex + 1) % frameResourcesCount;
	currFrameResources = FrameResourcesArray[currFrameResourcesIndex].get();

	if (currFrameResources->fenceCPU!=0&&currFrameResources->fenceCPU>fence->GetCompletedValue())
	{
		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");
		ThrowIfFailed(fence->SetEventOnCompletion(currFrameResources->fenceCPU, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}


	ObjectConstants objConstants;
	PassConstants passConstants;




	//构建观察矩阵
	float y = radius * sinf(phi);
	float x = radius * cosf(phi) * sinf(theta);
	float z = radius * cosf(phi) * cosf(theta);


	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX v = XMMatrixLookAtLH(pos, target, up);

	//构建投影矩阵
	XMMATRIX p = XMLoadFloat4x4(&proj);
	////构建世界矩阵
	XMMATRIX w = XMLoadFloat4x4(&world);
	//w *= XMMatrixTranslation(0.5f, 0.0f, 0.0f);


	//矩阵计算
	XMMATRIX WVP_Matrix = v * p*w;

	//XMMATRIX赋值给XMFLOAT4X4
	XMStoreFloat4x4(&passConstants.worldViewProj, XMMatrixTranspose(WVP_Matrix));
	//将数据拷贝至GPU缓存
	currFrameResources->passCB->CopyData(0, passConstants);

	for (auto& e : allRitems)
	{

		if (e->numFramesDirty)
		{
			XMFLOAT4X4 eworld = e->world;
			XMMATRIX w = XMLoadFloat4x4(&eworld);
			XMStoreFloat4x4(&objConstants.world, XMMatrixTranspose(w));
			currFrameResources->objCB->CopyData(e->objCBIndex, objConstants);

			e->numFramesDirty--;
		}
	}
	
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nShowCmd) {
#if defined(DEBUG)|defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif


	//第一步 创建并显示窗口
	//第二步 消息循环中检测信息
	//第三步 将接受到的消息分派到窗口过程

	D3D12InitApp theApp(hInstance);
	try
	{
		if (!theApp.Init(hInstance, nShowCmd,L"None"))
		{
			return 0;
		}

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"Falied", MB_OK);
		return 0;
	}






	////消息循环
	////定义消息结构体
	//MSG msg = { 0 };
	//BOOL bRet = 0;
	//
	//while (bRet=GetMessage(&msg,0,0,0)!=0)
	//{
	//	//如果等于-1，说明GetMessage()函数出错，弹出错误框
	//	if (bRet==-1)
	//	{
	//		MessageBox(0, L"GetMessage Failed", L"Error", MB_OK);
	//	}
	//	//如果等于其他值，说明收到消息
	//	else
	//	{
	//		TranslateMessage(&msg);  //键盘按键转换，将虚拟信息转换为字符信息
	//		DispatchMessage(&msg);   //把信息分派给相应的窗口过程
	//	}

	//}

	//return (int)msg.wParam;
	
}


FrameResources::FrameResources(ID3D12Device* device, UINT passCount, UINT objCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&cmdAllocator)));

	objCB = std::make_unique<UploadBufferResource<ObjectConstants>>(device, objCount, true);
	passCB = std::make_unique<UploadBufferResource<PassConstants>>(device, passCount, true);
}

FrameResources::~FrameResources()
{

}
