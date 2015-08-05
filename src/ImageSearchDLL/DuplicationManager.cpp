// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
#include "stdafx.h"
#include <algorithm>
#include "DuplicationManager.h"

#ifdef _DEBUG

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new  new(_NORMAL_BLOCK, __FILE__, __LINE__)

#endif

_Post_satisfies_(return != DUPL_RETURN_SUCCESS)
DUPL_RETURN ProcessFailure(_In_opt_ ID3D11Device* Device, _In_ LPCWSTR Str, _In_ LPCWSTR Title, HRESULT hr, _In_opt_z_ HRESULT* ExpectedErrors)
{
	HRESULT TranslatedHr;

	// On an error check if the DX device is lost
	if (Device)
	{
		HRESULT DeviceRemovedReason = Device->GetDeviceRemovedReason();

		switch (DeviceRemovedReason)
		{
		case DXGI_ERROR_DEVICE_REMOVED:
		case DXGI_ERROR_DEVICE_RESET:
		case static_cast<HRESULT>(E_OUTOFMEMORY) :
		{
			// Our device has been stopped due to an external event on the GPU so map them all to
			// device removed and continue processing the condition
			TranslatedHr = DXGI_ERROR_DEVICE_REMOVED;
			break;
		}

		case S_OK:
		{
			// Device is not removed so use original error
			TranslatedHr = hr;
			break;
		}

		default:
		{
			// Device is removed but not a error we want to remap
			TranslatedHr = DeviceRemovedReason;
		}
		}
	}
	else
	{
		TranslatedHr = hr;
	}

	// Check if this error was expected or not
	if (ExpectedErrors)
	{
		HRESULT* CurrentResult = ExpectedErrors;

		while (*CurrentResult != S_OK)
		{
			if (*(CurrentResult++) == TranslatedHr)
			{
				return DUPL_RETURN_ERROR_EXPECTED;
			}
		}
	}

	// Error was not expected so display the message box
	//DisplayMsg(Str, Title, TranslatedHr);
	MessageBoxW(NULL, Str, Title, MB_OK);
	return DUPL_RETURN_ERROR_UNEXPECTED;
}


//
// Get DX_RESOURCES
//
DUPL_RETURN InitializeDx(_Out_ DX_RESOURCES* Data)
{
	HRESULT hr = S_OK;

	// Driver types supported
	D3D_DRIVER_TYPE DriverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

	// Feature levels supported
	D3D_FEATURE_LEVEL FeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_1
	};
	UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);

	D3D_FEATURE_LEVEL FeatureLevel;

	// Create device
	for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
	{
		hr = D3D11CreateDevice(nullptr, DriverTypes[DriverTypeIndex], nullptr, 0, FeatureLevels, NumFeatureLevels,
			D3D11_SDK_VERSION, &Data->Device, &FeatureLevel, &Data->Context);
		if (SUCCEEDED(hr))
		{
			// Device creation success, no need to loop anymore
			break;
		}
	}
	if (FAILED(hr))
	{
		return ProcessFailure(nullptr, L"Failed to create device in InitializeDx", L"Error", hr);
	}

	// VERTEX shader
	UINT Size = ARRAYSIZE(g_VS);
	hr = Data->Device->CreateVertexShader(g_VS, Size, nullptr, &Data->VertexShader);
	if (FAILED(hr))
	{
		return ProcessFailure(Data->Device, L"Failed to create vertex shader in InitializeDx", L"Error", hr, SystemTransitionsExpectedErrors);
	}

	// Input layout
	D3D11_INPUT_ELEMENT_DESC Layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT NumElements = ARRAYSIZE(Layout);
	hr = Data->Device->CreateInputLayout(Layout, NumElements, g_VS, Size, &Data->InputLayout);
	if (FAILED(hr))
	{
		return ProcessFailure(Data->Device, L"Failed to create input layout in InitializeDx", L"Error", hr, SystemTransitionsExpectedErrors);
	}
	Data->Context->IASetInputLayout(Data->InputLayout);

	// Pixel shader
	Size = ARRAYSIZE(g_PS);
	hr = Data->Device->CreatePixelShader(g_PS, Size, nullptr, &Data->PixelShader);
	if (FAILED(hr))
	{
		return ProcessFailure(Data->Device, L"Failed to create pixel shader in InitializeDx", L"Error", hr, SystemTransitionsExpectedErrors);
	}

	// Set up sampler
	D3D11_SAMPLER_DESC SampDesc;
	RtlZeroMemory(&SampDesc, sizeof(SampDesc));
	SampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	SampDesc.MinLOD = 0;
	SampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = Data->Device->CreateSamplerState(&SampDesc, &Data->SamplerLinear);
	if (FAILED(hr))
	{
		return ProcessFailure(Data->Device, L"Failed to create sampler state in InitializeDx", L"Error", hr, SystemTransitionsExpectedErrors);
	}

	return DUPL_RETURN_SUCCESS;
}

//
// Clean up DX_RESOURCES
//
void CleanDx(_Inout_ DX_RESOURCES* Data)
{
	if (Data->Device)
	{
		Data->Device->Release();
		Data->Device = nullptr;
	}

	if (Data->Context)
	{
		Data->Context->Release();
		Data->Context = nullptr;
	}

	if (Data->VertexShader)
	{
		Data->VertexShader->Release();
		Data->VertexShader = nullptr;
	}

	if (Data->PixelShader)
	{
		Data->PixelShader->Release();
		Data->PixelShader = nullptr;
	}

	if (Data->InputLayout)
	{
		Data->InputLayout->Release();
		Data->InputLayout = nullptr;
	}

	if (Data->SamplerLinear)
	{
		Data->SamplerLinear->Release();
		Data->SamplerLinear = nullptr;
	}
}


#ifdef _DEBUG
//VC下把HBITMAP保存为bmp图片 
BOOL  SaveBmp(HBITMAP     hBitmap, LPCTSTR     FileName)
{
	HDC     hDC;
	//当前分辨率下每象素所占字节数       
	int     iBits;
	//位图中每象素所占字节数       
	WORD     wBitCount;
	//定义调色板大小，     位图中像素字节大小     ，位图文件大小     ，     写入文件字节数           
	DWORD     dwPaletteSize = 0, dwBmBitsSize = 0, dwDIBSize = 0, dwWritten = 0;
	//位图属性结构           
	BITMAP     Bitmap;
	//位图文件头结构       
	BITMAPFILEHEADER     bmfHdr;
	//位图信息头结构           
	BITMAPINFOHEADER     bi;
	//指向位图信息头结构               
	LPBITMAPINFOHEADER     lpbi;
	//定义文件，分配内存句柄，调色板句柄           
	HANDLE     fh, hDib, hPal, hOldPal = NULL;

	//计算位图文件每个像素所占字节数           
	hDC = CreateDC("DISPLAY", NULL, NULL, NULL);
	iBits = GetDeviceCaps(hDC, BITSPIXEL)     *     GetDeviceCaps(hDC, PLANES);
	DeleteDC(hDC);
	if (iBits <= 1)
		wBitCount = 1;
	else  if (iBits <= 4)
		wBitCount = 4;
	else if (iBits <= 8)
		wBitCount = 8;
	else
		wBitCount = 24;

	GetObject(hBitmap, sizeof(Bitmap), (LPSTR)&Bitmap);
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = Bitmap.bmWidth;
	bi.biHeight = Bitmap.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = wBitCount;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrImportant = 0;
	bi.biClrUsed = 0;

	dwBmBitsSize = ((Bitmap.bmWidth *wBitCount + 31) / 32) * 4 * Bitmap.bmHeight;

	//为位图内容分配内存           
	hDib = GlobalAlloc(GHND, dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER));
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
	*lpbi = bi;

	//     处理调色板               
	hPal = GetStockObject(DEFAULT_PALETTE);
	if (hPal)
	{
		hDC = ::GetDC(NULL);
		hOldPal = ::SelectPalette(hDC, (HPALETTE)hPal, FALSE);
		RealizePalette(hDC);
	}

	//     获取该调色板下新的像素值           
	GetDIBits(hDC, hBitmap, 0, (UINT)Bitmap.bmHeight,
		(LPSTR)lpbi + sizeof(BITMAPINFOHEADER) + dwPaletteSize,
		(BITMAPINFO *)lpbi, DIB_RGB_COLORS);

	//恢复调色板               
	if (hOldPal)
	{
		::SelectPalette(hDC, (HPALETTE)hOldPal, TRUE);
		RealizePalette(hDC);
		::ReleaseDC(NULL, hDC);
	}

	//创建位图文件               
	fh = CreateFile(FileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (fh == INVALID_HANDLE_VALUE)         return     FALSE;

	//     设置位图文件头           
	bmfHdr.bfType = 0x4D42;     //     "BM"           
	dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwPaletteSize + dwBmBitsSize;
	bmfHdr.bfSize = dwDIBSize;
	bmfHdr.bfReserved1 = 0;
	bmfHdr.bfReserved2 = 0;
	bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize;
	//     写入位图文件头           
	WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
	//     写入位图文件其余内容           
	WriteFile(fh, (LPSTR)lpbi, dwDIBSize, &dwWritten, NULL);
	//清除               
	GlobalUnlock(hDib);
	GlobalFree(hDib);
	CloseHandle(fh);

	return     TRUE;
}
#endif

HGDIOBJ extractBitmap(ID3D11Device* D3D11Device, void* texture) {

	HBITMAP hBitmapTexture = NULL;
	HGDIOBJ hBitmap = INVALID_HANDLE_VALUE;

	ID3D11Texture2D* d3dtex = (ID3D11Texture2D*)texture;
	D3D11_TEXTURE2D_DESC desc;
	d3dtex->GetDesc(&desc);

	ID3D11Texture2D* pNewTexture = NULL;
	D3D11_TEXTURE2D_DESC description;
	d3dtex->GetDesc(&description);

	description.BindFlags = 0;
	description.MiscFlags = 0;
	description.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	description.Usage = D3D11_USAGE_STAGING;

	HRESULT hr = D3D11Device->CreateTexture2D(&description, NULL, &pNewTexture);

	ID3D11DeviceContext* ctx = NULL;
	D3D11Device->GetImmediateContext(&ctx);

	ctx->CopyResource(pNewTexture, d3dtex);

	D3D11_MAPPED_SUBRESOURCE resource;
	UINT subresource = D3D11CalcSubresource(0, 0, 0);
	ctx->Map(pNewTexture, subresource, D3D11_MAP_READ_WRITE, 0, &resource);

	// COPY from texture to bitmap buffer
	uint8_t* sptr = reinterpret_cast<uint8_t*>(resource.pData);
	uint8_t* dptr = new uint8_t[desc.Width*desc.Height * 4];

	for (size_t h = 0; h < desc.Height; ++h)
	{
		size_t msize = std::min<size_t>(desc.Width * 4, resource.RowPitch);
		memcpy_s(dptr, desc.Width * 4, sptr, msize);
		sptr += resource.RowPitch;
		dptr += desc.Width * 4;
	}

	dptr -= desc.Width*desc.Height * 4;

	// SWAP BGR to RGB bitmap
	uint32_t *dPtr = reinterpret_cast<uint32_t*>(dptr);
	for (size_t count = 0; count < desc.Width*desc.Height * 4; count += 4) {
		uint32_t t = *(dPtr);
		uint32_t t1 = (t & 0x00ff0000);// >> 16;
		uint32_t t2 = (t & 0x000000ff);// << 16;
		uint32_t t3 = (t & 0x0000ff00);
		uint32_t ta = (t & 0xFF000000);

		*(dPtr++) = t1 | t2 | t3 | ta;
	}

	hBitmapTexture = CreateCompatibleBitmap(GetDC(NULL), desc.Width, desc.Height);
	SetBitmapBits(hBitmapTexture, desc.Width*desc.Height * 4, dptr);

	hBitmap = CopyImage(hBitmapTexture, IMAGE_BITMAP, desc.Width, desc.Height, LR_CREATEDIBSECTION);

#ifdef _DEBUG

	SaveBmp((HBITMAP)hBitmap, "ScreenShot.bmp");
#endif

extractBitmap_EXIT:
	delete[] dptr;
	pNewTexture->Release();
	ctx->Release();
	DeleteObject(hBitmapTexture);
	return hBitmap;
}

//
// Entry point for new duplication threads
//
DWORD WINAPI DDProc(_In_ void* Param)
{
	// Classes
	DUPLICATIONMANAGER DuplMgr;

	// Data passed in from thread creation
	THREAD_DATA* TData = reinterpret_cast<THREAD_DATA*>(Param);

	// Get desktop
	DUPL_RETURN Ret;
	HDESK CurrentDesktop = nullptr;
	CurrentDesktop = OpenInputDesktop(0, FALSE, GENERIC_ALL);
	if (!CurrentDesktop)
	{
		// We do not have access to the desktop so request a retry
		Ret = DUPL_RETURN_ERROR_EXPECTED;
		goto Exit;
	}

	// Attach desktop to this thread
	bool DesktopAttached = SetThreadDesktop(CurrentDesktop) != 0;
	CloseDesktop(CurrentDesktop);
	CurrentDesktop = nullptr;
	if (!DesktopAttached)
	{
		// We do not have access to the desktop so request a retry
		Ret = DUPL_RETURN_ERROR_EXPECTED;
		goto Exit;
	}

	// Obtain handle to sync shared Surface
	HRESULT hr;

	// Make duplication manager
	Ret = DuplMgr.InitDupl(TData->DxRes.Device, 0);
	if (Ret != DUPL_RETURN_SUCCESS)
	{
		goto Exit;
	}

	// Get output description
	DXGI_OUTPUT_DESC DesktopDesc;
	RtlZeroMemory(&DesktopDesc, sizeof(DXGI_OUTPUT_DESC));
	DuplMgr.GetOutputDesc(&DesktopDesc);

	FRAME_DATA CurrentData;
	TData->out_HBitmap = (HBITMAP)INVALID_HANDLE_VALUE;
	int i = 0;
	while (i<2)
	{
		// Get new frame from desktop duplication
		bool TimeOut;
		Ret = DuplMgr.GetFrame(&CurrentData, &TimeOut);

		if (Ret == DUPL_RETURN_SUCCESS && !TimeOut)
			++i;
		// Check for timeout
		if (TimeOut)
			continue;

		if (TData->out_HBitmap != INVALID_HANDLE_VALUE) {
			DeleteObject(TData->out_HBitmap);
			TData->out_HBitmap = (HBITMAP)INVALID_HANDLE_VALUE;
		}

		TData->out_HBitmap = (HBITMAP)extractBitmap(TData->DxRes.Device, CurrentData.Frame);

		// Release frame back to desktop duplication
		Ret = DuplMgr.DoneWithFrame();
		if (Ret != DUPL_RETURN_SUCCESS)
		{
			break;
		}
	}

Exit:
	if (Ret != DUPL_RETURN_SUCCESS)
	{
		MessageBoxA(NULL, "SomeError", "SomeError", MB_OK);
	}
	return Ret == DUPL_RETURN_SUCCESS;
}

HBITMAP GetScreenShot()
{
	UINT ThreadCount = 1;
	HANDLE ThreadHandles=INVALID_HANDLE_VALUE;
	THREAD_DATA ThreadData;

	// Create appropriate # of threads for duplication
	DUPL_RETURN Ret = DUPL_RETURN_SUCCESS;
	ThreadData.out_HBitmap = (HBITMAP)INVALID_HANDLE_VALUE;
	ThreadData.OffsetX = 0;
	ThreadData.OffsetY = 0;

	RtlZeroMemory(&ThreadData.DxRes, sizeof(DX_RESOURCES));
	Ret = InitializeDx(&ThreadData.DxRes);

	if (Ret != DUPL_RETURN_SUCCESS)
		goto GetScreenShot_EXIT;

	DWORD ThreadId;
	ThreadHandles = CreateThread(nullptr, 0, DDProc, &ThreadData, 0, &ThreadId);
	if (ThreadHandles == nullptr){
		ProcessFailure(nullptr, L"Failed to create thread", L"Error", E_FAIL);
		goto GetScreenShot_EXIT;
	}
	WaitForSingleObject(ThreadHandles, INFINITE);

GetScreenShot_EXIT:
	CloseHandle(ThreadHandles);
	CleanDx(&ThreadData.DxRes);
	return ThreadData.out_HBitmap;
}

// Below are lists of errors expect from Dxgi API calls when a transition event like mode change, PnpStop, PnpStart
// desktop switch, TDR or session disconnect/reconnect. In all these cases we want the application to clean up the threads that process
// the desktop updates and attempt to recreate them.
// If we get an error that is not on the appropriate list then we exit the application

// These are the errors we expect from general Dxgi API due to a transition
HRESULT SystemTransitionsExpectedErrors[] = {
	DXGI_ERROR_DEVICE_REMOVED,
	DXGI_ERROR_ACCESS_LOST,
	static_cast<HRESULT>(WAIT_ABANDONED),
	S_OK                                    // Terminate list with zero valued HRESULT
};

// These are the errors we expect from IDXGIOutput1::DuplicateOutput due to a transition
HRESULT CreateDuplicationExpectedErrors[] = {
	DXGI_ERROR_DEVICE_REMOVED,
	static_cast<HRESULT>(E_ACCESSDENIED),
	DXGI_ERROR_UNSUPPORTED,
	DXGI_ERROR_SESSION_DISCONNECTED,
	S_OK                                    // Terminate list with zero valued HRESULT
};

// These are the errors we expect from IDXGIOutputDuplication methods due to a transition
HRESULT FrameInfoExpectedErrors[] = {
	DXGI_ERROR_DEVICE_REMOVED,
	DXGI_ERROR_ACCESS_LOST,
	S_OK                                    // Terminate list with zero valued HRESULT
};

// These are the errors we expect from IDXGIAdapter::EnumOutputs methods due to outputs becoming stale during a transition
HRESULT EnumOutputsExpectedErrors[] = {
	DXGI_ERROR_NOT_FOUND,
	S_OK                                    // Terminate list with zero valued HRESULT
};



//
// Constructor sets up references / variables
//
DUPLICATIONMANAGER::DUPLICATIONMANAGER() : m_DeskDupl(nullptr),
                                           m_AcquiredDesktopImage(nullptr),
                                           m_MetaDataBuffer(nullptr),
                                           m_MetaDataSize(0),
                                           m_OutputNumber(0),
                                           m_Device(nullptr)
{
    RtlZeroMemory(&m_OutputDesc, sizeof(m_OutputDesc));
}

//
// Destructor simply calls CleanRefs to destroy everything
//
DUPLICATIONMANAGER::~DUPLICATIONMANAGER()
{
    if (m_DeskDupl)
    {
        m_DeskDupl->Release();
        m_DeskDupl = nullptr;
    }

    if (m_AcquiredDesktopImage)
    {
        m_AcquiredDesktopImage->Release();
        m_AcquiredDesktopImage = nullptr;
    }

    if (m_MetaDataBuffer)
    {
        delete [] m_MetaDataBuffer;
        m_MetaDataBuffer = nullptr;
    }

    if (m_Device)
    {
        m_Device->Release();
        m_Device = nullptr;
    }
}

//
// Initialize duplication interfaces
//
DUPL_RETURN DUPLICATIONMANAGER::InitDupl(_In_ ID3D11Device* Device, UINT Output)
{
    m_OutputNumber = Output;

    // Take a reference on the device
    m_Device = Device;
    m_Device->AddRef();

    // Get DXGI device
    IDXGIDevice* DxgiDevice = nullptr;
    HRESULT hr = m_Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));
    if (FAILED(hr))
    {
        return ProcessFailure(nullptr, L"Failed to QI for DXGI Device", L"Error", hr);
    }

    // Get DXGI adapter
    IDXGIAdapter* DxgiAdapter = nullptr;
    hr = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
    DxgiDevice->Release();
    DxgiDevice = nullptr;
    if (FAILED(hr))
    {
        return ProcessFailure(m_Device, L"Failed to get parent DXGI Adapter", L"Error", hr, SystemTransitionsExpectedErrors);
    }

    // Get output
    IDXGIOutput* DxgiOutput = nullptr;
    hr = DxgiAdapter->EnumOutputs(Output, &DxgiOutput);
    DxgiAdapter->Release();
    DxgiAdapter = nullptr;
    if (FAILED(hr))
    {
        return ProcessFailure(m_Device, L"Failed to get specified output in DUPLICATIONMANAGER", L"Error", hr, EnumOutputsExpectedErrors);
    }

    DxgiOutput->GetDesc(&m_OutputDesc);

    // QI for Output 1
    IDXGIOutput1* DxgiOutput1 = nullptr;
    hr = DxgiOutput->QueryInterface(__uuidof(DxgiOutput1), reinterpret_cast<void**>(&DxgiOutput1));
    DxgiOutput->Release();
    DxgiOutput = nullptr;
    if (FAILED(hr))
    {
        return ProcessFailure(nullptr, L"Failed to QI for DxgiOutput1 in DUPLICATIONMANAGER", L"Error", hr);
    }

    // Create desktop duplication
    hr = DxgiOutput1->DuplicateOutput(m_Device, &m_DeskDupl);
    DxgiOutput1->Release();
    DxgiOutput1 = nullptr;
    if (FAILED(hr))
    {
        if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
        {
            MessageBoxW(nullptr, L"There is already the maximum number of applications using the Desktop Duplication API running, please close one of those applications and then try again.", L"Error", MB_OK);
            return DUPL_RETURN_ERROR_UNEXPECTED;
        }
        return ProcessFailure(m_Device, L"Failed to get duplicate output in DUPLICATIONMANAGER", L"Error", hr, CreateDuplicationExpectedErrors);
    }

    return DUPL_RETURN_SUCCESS;
}


//
// Get next frame and write it into Data
//
_Success_(*Timeout == false && return == DUPL_RETURN_SUCCESS)
DUPL_RETURN DUPLICATIONMANAGER::GetFrame(_Out_ FRAME_DATA* Data, _Out_ bool* Timeout)
{
    IDXGIResource* DesktopResource = nullptr;
    DXGI_OUTDUPL_FRAME_INFO FrameInfo;
	
    // Get new frame
    HRESULT hr = m_DeskDupl->AcquireNextFrame(500, &FrameInfo, &DesktopResource);
    if (hr == DXGI_ERROR_WAIT_TIMEOUT)
    {
        *Timeout = true;
        return DUPL_RETURN_SUCCESS;
    }
    *Timeout = false;

    if (FAILED(hr))
    {
        return ProcessFailure(m_Device, L"Failed to acquire next frame in DUPLICATIONMANAGER", L"Error", hr, FrameInfoExpectedErrors);
    }

    // If still holding old frame, destroy it
    if (m_AcquiredDesktopImage)
    {
        m_AcquiredDesktopImage->Release();
        m_AcquiredDesktopImage = nullptr;
    }

    // QI for IDXGIResource
    hr = DesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&m_AcquiredDesktopImage));
    DesktopResource->Release();
    DesktopResource = nullptr;
    if (FAILED(hr))
    {
        return ProcessFailure(nullptr, L"Failed to QI for ID3D11Texture2D from acquired IDXGIResource in DUPLICATIONMANAGER", L"Error", hr);
    }

    // Get metadata
    if (FrameInfo.TotalMetadataBufferSize)
    {
        // Old buffer too small
        if (FrameInfo.TotalMetadataBufferSize > m_MetaDataSize)
        {
            if (m_MetaDataBuffer)
            {
                delete [] m_MetaDataBuffer;
                m_MetaDataBuffer = nullptr;
            }
            m_MetaDataBuffer = new  BYTE[FrameInfo.TotalMetadataBufferSize];
            if (!m_MetaDataBuffer)
            {
                m_MetaDataSize = 0;
                Data->MoveCount = 0;
                Data->DirtyCount = 0;
                return ProcessFailure(nullptr, L"Failed to allocate memory for metadata in DUPLICATIONMANAGER", L"Error", E_OUTOFMEMORY);
            }
            m_MetaDataSize = FrameInfo.TotalMetadataBufferSize;
        }

        UINT BufSize = FrameInfo.TotalMetadataBufferSize;

        // Get move rectangles
        hr = m_DeskDupl->GetFrameMoveRects(BufSize, reinterpret_cast<DXGI_OUTDUPL_MOVE_RECT*>(m_MetaDataBuffer), &BufSize);
        if (FAILED(hr))
        {
            Data->MoveCount = 0;
            Data->DirtyCount = 0;
            return ProcessFailure(nullptr, L"Failed to get frame move rects in DUPLICATIONMANAGER", L"Error", hr, FrameInfoExpectedErrors);
        }
        Data->MoveCount = BufSize / sizeof(DXGI_OUTDUPL_MOVE_RECT);

        BYTE* DirtyRects = m_MetaDataBuffer + BufSize;
        BufSize = FrameInfo.TotalMetadataBufferSize - BufSize;

        // Get dirty rectangles
        hr = m_DeskDupl->GetFrameDirtyRects(BufSize, reinterpret_cast<RECT*>(DirtyRects), &BufSize);
        if (FAILED(hr))
        {
            Data->MoveCount = 0;
            Data->DirtyCount = 0;
            return ProcessFailure(nullptr, L"Failed to get frame dirty rects in DUPLICATIONMANAGER", L"Error", hr, FrameInfoExpectedErrors);
        }
        Data->DirtyCount = BufSize / sizeof(RECT);

        Data->MetaData = m_MetaDataBuffer;
    }

    Data->Frame = m_AcquiredDesktopImage;
    Data->FrameInfo = FrameInfo;

    return DUPL_RETURN_SUCCESS;
}

//
// Release frame
//
DUPL_RETURN DUPLICATIONMANAGER::DoneWithFrame()
{
    HRESULT hr = m_DeskDupl->ReleaseFrame();
    if (FAILED(hr))
    {
        return ProcessFailure(m_Device, L"Failed to release frame in DUPLICATIONMANAGER", L"Error", hr, FrameInfoExpectedErrors);
    }

    if (m_AcquiredDesktopImage)
    {
        m_AcquiredDesktopImage->Release();
        m_AcquiredDesktopImage = nullptr;
    }

    return DUPL_RETURN_SUCCESS;
}

//
// Gets output desc into DescPtr
//
void DUPLICATIONMANAGER::GetOutputDesc(_Out_ DXGI_OUTPUT_DESC* DescPtr)
{
    *DescPtr = m_OutputDesc;
}