#include <windows.h>
#include <windowsx.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <chrono>
#include <cmath>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

void set_blend_state(D3D12_BLEND_DESC& blend_desc) {
    blend_desc = {};
    blend_desc.AlphaToCoverageEnable = FALSE;
    blend_desc.IndependentBlendEnable = FALSE;

    D3D12_RENDER_TARGET_BLEND_DESC rt = {};
    rt.BlendEnable = FALSE;
    rt.LogicOpEnable = FALSE;
    rt.SrcBlend = D3D12_BLEND_ONE;
    rt.DestBlend = D3D12_BLEND_ZERO;
    rt.BlendOp = D3D12_BLEND_OP_ADD;
    rt.SrcBlendAlpha = D3D12_BLEND_ONE;
    rt.DestBlendAlpha = D3D12_BLEND_ZERO;
    rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    rt.LogicOp = D3D12_LOGIC_OP_NOOP;
    rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++) blend_desc.RenderTarget[i] = rt;
}

void set_rasterizer_state(D3D12_RASTERIZER_DESC& rs) {
    rs = {};
    rs.FillMode = D3D12_FILL_MODE_SOLID;
    rs.CullMode = D3D12_CULL_MODE_BACK;
    rs.FrontCounterClockwise = FALSE;
    rs.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rs.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rs.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rs.DepthClipEnable = TRUE;
    rs.MultisampleEnable = FALSE;
    rs.AntialiasedLineEnable = FALSE;
    rs.ForcedSampleCount = 0;
    rs.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
}

void set_depth_stencil_state(D3D12_DEPTH_STENCIL_DESC& ds) {
    ds = {};
    ds.DepthEnable = TRUE;
    ds.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    ds.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    ds.StencilEnable = FALSE;
    ds.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    ds.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    ds.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    ds.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    ds.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    ds.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    ds.BackFace = ds.FrontFace;
}

struct AppInput {
    bool lookActive = false;
    POINT lastMouse{};
    float yaw = 0.0f;
    float pitch = 0.0f;
    XMFLOAT3 camPos{ 0.0f, 1.0f, -4.5f };
    float mouseSens = 0.0025f;
    float moveSpeed = 2.5f;
};

static AppInput gInput;
static HWND gHwnd = nullptr;
static int gWidth = 800, gHeight = 800;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_RBUTTONDOWN: {
        gInput.lookActive = true;
        ShowCursor(FALSE);

        RECT rc; GetClientRect(hWnd, &rc);
        POINT center = { (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 };
        ClientToScreen(hWnd, &center);
        SetCursorPos(center.x, center.y);
        gInput.lastMouse = center;

        RECT clip; GetClientRect(hWnd, &clip);
        POINT tl{ clip.left, clip.top }, br{ clip.right, clip.bottom };
        ClientToScreen(hWnd, &tl); ClientToScreen(hWnd, &br);
        RECT clipScr{ tl.x, tl.y, br.x, br.y };
        ClipCursor(&clipScr);
    } return 0;

    case WM_RBUTTONUP: {
        gInput.lookActive = false;
        ClipCursor(nullptr);
        ShowCursor(TRUE);
    } return 0;

    case WM_MOUSEMOVE: {
        if (!gInput.lookActive) break;
        POINT p{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        POINT pScreen = p; ClientToScreen(hWnd, &pScreen);

        LONG dx = pScreen.x - gInput.lastMouse.x;
        LONG dy = pScreen.y - gInput.lastMouse.y;

        gInput.yaw += dx * gInput.mouseSens;
        gInput.pitch += dy * gInput.mouseSens;

        const float kLimit = XMConvertToRadians(89.0f);
        if (gInput.pitch > kLimit) gInput.pitch = kLimit;
        if (gInput.pitch < -kLimit) gInput.pitch = -kLimit;

        RECT rc; GetClientRect(hWnd, &rc);
        POINT center = { (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 };
        ClientToScreen(hWnd, &center);
        SetCursorPos(center.x, center.y);
        gInput.lastMouse = center;
    } return 0;

    case WM_ACTIVATEAPP:
        if (!wParam && gInput.lookActive) {
            gInput.lookActive = false;
            ClipCursor(nullptr);
            ShowCursor(TRUE);
        }
        return 0;

    case WM_SIZE:
        gWidth = LOWORD(lParam);
        gHeight = HIWORD(lParam);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// Vertex for cube
struct Vertex {
    XMFLOAT3 pos;
    XMFLOAT3 color;
};

// 8 corners with colors
static const Vertex gCubeVerts[] = {
    {{-1,-1,-1},{1,0,0}}, {{-1, 1,-1},{0,1,0}},
    {{ 1, 1,-1},{0,0,1}}, {{ 1,-1,-1},{1,1,0}},
    {{-1,-1, 1},{1,0,1}}, {{-1, 1, 1},{0,1,1}},
    {{ 1, 1, 1},{1,1,1}}, {{ 1,-1, 1},{0.2f,0.2f,0.2f}},
};

// 12 triangles
static const uint16_t gCubeIndices[] = {
    0,1,2, 0,2,3,       // -Z
    4,6,5, 4,7,6,       // +Z
    4,5,1, 4,1,0,       // -X
    3,2,6, 3,6,7,       // +X
    4,0,3, 4,3,7,       // -Y
    1,5,6, 1,6,2        // +Y
};

__declspec(align(256))
struct VSConstants {
    XMFLOAT4X4 mvp;
};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    gWidth = 800; gHeight = 800;

    // Window
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"D3D12Cube";
    RegisterClass(&wc);

    gHwnd = CreateWindow(wc.lpszClassName, L"Cube",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        gWidth, gHeight, nullptr, nullptr, hInstance, nullptr);
    ShowWindow(gHwnd, nCmdShow);

    // Device
    ID3D12Device* device = nullptr;
    HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));

    // Queue/allocator/list
    ID3D12CommandQueue* command_queue = nullptr;
    {
        D3D12_COMMAND_QUEUE_DESC q = {};
        q.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        hr = device->CreateCommandQueue(&q, IID_PPV_ARGS(&command_queue));
    }
    ID3D12CommandAllocator* command_allocator = nullptr;
    hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator));
    hr = command_allocator->Reset();

    ID3D12GraphicsCommandList* command_list = nullptr;
    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator, nullptr, IID_PPV_ARGS(&command_list));
    hr = command_list->Close();

    // Swap chain
    IDXGIFactory4* factory = nullptr;
    hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));

    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 2;
    scd.BufferDesc.Width = gWidth;
    scd.BufferDesc.Height = gHeight;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scd.OutputWindow = gHwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;

    IDXGISwapChain* temp_sc = nullptr;
    hr = factory->CreateSwapChain(command_queue, &scd, &temp_sc);
    IDXGISwapChain3* swap_chain = nullptr;
    temp_sc->QueryInterface(IID_PPV_ARGS(&swap_chain));
    temp_sc->Release();

    // RTVs
    ID3D12DescriptorHeap* rtv_heap = nullptr;
    {
        D3D12_DESCRIPTOR_HEAP_DESC d = {};
        d.NumDescriptors = 2;
        d.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        hr = device->CreateDescriptorHeap(&d, IID_PPV_ARGS(&rtv_heap));
    }
    ID3D12Resource* backbuffers[2] = {};
    UINT rtvInc = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    {
        D3D12_CPU_DESCRIPTOR_HANDLE h = rtv_heap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < 2; ++i) {
            hr = swap_chain->GetBuffer(i, IID_PPV_ARGS(&backbuffers[i]));
            device->CreateRenderTargetView(backbuffers[i], nullptr, h);
            h.ptr += rtvInc;
        }
    }

    ID3D12DescriptorHeap* dsv_heap = nullptr;
    {
        D3D12_DESCRIPTOR_HEAP_DESC d = {};
        d.NumDescriptors = 1;
        d.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        hr = device->CreateDescriptorHeap(&d, IID_PPV_ARGS(&dsv_heap));
    }

    ID3D12Resource* depth = nullptr;
    {
        D3D12_HEAP_PROPERTIES hp = {}; hp.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC tex = {};
        tex.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        tex.Width = gWidth; tex.Height = gHeight;
        tex.DepthOrArraySize = 1; tex.MipLevels = 1;
        tex.Format = DXGI_FORMAT_D32_FLOAT;
        tex.SampleDesc = { 1,0 };
        tex.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        tex.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clear = {};
        clear.Format = DXGI_FORMAT_D32_FLOAT;
        clear.DepthStencil.Depth = 1.0f;

        hr = device->CreateCommittedResource(
            &hp, D3D12_HEAP_FLAG_NONE, &tex,
            D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear, IID_PPV_ARGS(&depth));

        D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
        dsv.Format = DXGI_FORMAT_D32_FLOAT;
        dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        device->CreateDepthStencilView(depth, &dsv, dsv_heap->GetCPUDescriptorHandleForHeapStart());
    }

    // Fence
    ID3D12Fence* fence = nullptr; UINT64 fenceValue = 0;
    HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    auto waitGPU = [&]() {
        const UINT64 v = ++fenceValue;
        command_queue->Signal(fence, v);
        if (fence->GetCompletedValue() < v) {
            fence->SetEventOnCompletion(v, fenceEvent);
            WaitForSingleObject(fenceEvent, INFINITE);
        }
        };

    // Root signature
    D3D12_ROOT_PARAMETER root_params[1] = {};
    root_params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    root_params[0].Descriptor.ShaderRegister = 0;
    root_params[0].Descriptor.RegisterSpace = 0;
    root_params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    ID3D12RootSignature* root_signature = nullptr;
    {
        D3D12_ROOT_SIGNATURE_DESC rs = {};
        rs.NumParameters = _countof(root_params);
        rs.pParameters = root_params;
        rs.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ID3DBlob* sig = nullptr; ID3DBlob* err = nullptr;
        D3D12SerializeRootSignature(&rs, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &err);
        device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&root_signature));
        if (sig) sig->Release();
        if (err) err->Release();
    }

    // Shaders
    ID3DBlob* vs = nullptr; ID3DBlob* ps = nullptr; ID3DBlob* ce = nullptr;
    D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vs, &ce);
    if (ce) { OutputDebugStringA((char*)ce->GetBufferPointer()); ce->Release(); }
    D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &ps, &ce);
    if (ce) { OutputDebugStringA((char*)ce->GetBufferPointer()); ce->Release(); }

    // Input layout for POSITION/COLOR
    D3D12_INPUT_ELEMENT_DESC il[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // PSO
    ID3D12PipelineState* pso = nullptr;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
    pso_desc.pRootSignature = root_signature;
    pso_desc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
    pso_desc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
    set_blend_state(pso_desc.BlendState);
    pso_desc.SampleMask = UINT_MAX;
    set_rasterizer_state(pso_desc.RasterizerState);
    set_depth_stencil_state(pso_desc.DepthStencilState);
    pso_desc.InputLayout = { il, _countof(il) };
    pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso_desc.NumRenderTargets = 1;
    pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pso_desc.SampleDesc.Count = 1;
    device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pso));
    if (vs) vs->Release();
    if (ps) ps->Release();

    D3D12_HEAP_PROPERTIES heapDefault = {}; heapDefault.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_HEAP_PROPERTIES heapUpload = {}; heapUpload.Type = D3D12_HEAP_TYPE_UPLOAD;

    auto makeBufDesc = [](UINT64 bytes) {
        D3D12_RESOURCE_DESC b{};
        b.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        b.Width = bytes; b.Height = 1; b.DepthOrArraySize = 1; b.MipLevels = 1;
        b.Format = DXGI_FORMAT_UNKNOWN; b.SampleDesc = { 1,0 };
        b.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        return b;
        };

    // Vertex buffer
    ID3D12Resource* vb = nullptr; ID3D12Resource* vbUpload = nullptr;
    const UINT vbSize = (UINT)sizeof(gCubeVerts);
    {
        auto desc = makeBufDesc(vbSize);
        device->CreateCommittedResource(&heapDefault, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&vb));
        device->CreateCommittedResource(&heapUpload, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vbUpload));

        void* p = nullptr; D3D12_RANGE r{ 0,0 };
        vbUpload->Map(0, &r, &p); memcpy(p, gCubeVerts, vbSize); vbUpload->Unmap(0, nullptr);

        command_allocator->Reset();
        command_list->Reset(command_allocator, nullptr);
        command_list->CopyBufferRegion(vb, 0, vbUpload, 0, vbSize);

        D3D12_RESOURCE_BARRIER b{};
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource = vb;
        b.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        b.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        command_list->ResourceBarrier(1, &b);

        command_list->Close();
        ID3D12CommandList* lists[] = { command_list };
        command_queue->ExecuteCommandLists(1, lists);
        waitGPU();
        vbUpload->Release(); vbUpload = nullptr;
    }
    D3D12_VERTEX_BUFFER_VIEW vbv{};
    vbv.BufferLocation = vb->GetGPUVirtualAddress();
    vbv.StrideInBytes = sizeof(Vertex);
    vbv.SizeInBytes = vbSize;

    // Index buffer
    ID3D12Resource* ib = nullptr; ID3D12Resource* ibUpload = nullptr;
    const UINT ibSize = (UINT)sizeof(gCubeIndices);
    {
        auto desc = makeBufDesc(ibSize);
        device->CreateCommittedResource(&heapDefault, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&ib));
        device->CreateCommittedResource(&heapUpload, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&ibUpload));

        void* p = nullptr; D3D12_RANGE r{ 0,0 };
        ibUpload->Map(0, &r, &p); memcpy(p, gCubeIndices, ibSize); ibUpload->Unmap(0, nullptr);

        command_allocator->Reset();
        command_list->Reset(command_allocator, nullptr);
        command_list->CopyBufferRegion(ib, 0, ibUpload, 0, ibSize);

        D3D12_RESOURCE_BARRIER b{};
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource = ib;
        b.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        b.Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        command_list->ResourceBarrier(1, &b);

        command_list->Close();
        ID3D12CommandList* lists[] = { command_list };
        command_queue->ExecuteCommandLists(1, lists);
        waitGPU();
        ibUpload->Release(); ibUpload = nullptr;
    }
    D3D12_INDEX_BUFFER_VIEW ibv{};
    ibv.BufferLocation = ib->GetGPUVirtualAddress();
    ibv.Format = DXGI_FORMAT_R16_UINT;
    ibv.SizeInBytes = ibSize;

    // Constant buffer
    ID3D12Resource* cb = nullptr;
    VSConstants* cbMapped = nullptr;
    {
        UINT cbBytes = (sizeof(VSConstants) + 255) & ~255u;
        D3D12_HEAP_PROPERTIES hp = {}; hp.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC rd = makeBufDesc(cbBytes);

        device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&cb));

        D3D12_RANGE r{ 0,0 };
        cb->Map(0, &r, reinterpret_cast<void**>(&cbMapped));
    }

    auto tPrev = std::chrono::steady_clock::now();

    // Main loop
    bool running = true;
    while (running) {
        MSG msg = {};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { running = false; break; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!running) break;

        // --- Time step ---
        auto tNow = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(tNow - tPrev).count();
        tPrev = tNow;

        // --- WASD movement (Shift to sprint) ---
        float speed = gInput.moveSpeed * ((GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 2.0f : 1.0f);
        float cy = cosf(gInput.yaw), sy = sinf(gInput.yaw);
        float cp = cosf(gInput.pitch), sp = sinf(gInput.pitch);
        XMFLOAT3 fwd = { cp * sy, sp, cp * cy };

        XMFLOAT3 up = { 0,1,0 };
        XMVECTOR vUp = XMLoadFloat3(&up);
        XMVECTOR vF = XMLoadFloat3(&fwd);
        XMVECTOR vR = XMVector3Normalize(XMVector3Cross(vUp, vF));
        XMFLOAT3 right; XMStoreFloat3(&right, vR);

        if (GetAsyncKeyState('W') & 0x8000) {
            gInput.camPos.x += fwd.x * speed * dt;
            gInput.camPos.y += fwd.y * speed * dt;
            gInput.camPos.z += fwd.z * speed * dt;
        }
        if (GetAsyncKeyState('S') & 0x8000) {
            gInput.camPos.x -= fwd.x * speed * dt;
            gInput.camPos.y -= fwd.y * speed * dt;
            gInput.camPos.z -= fwd.z * speed * dt;
        }
        if (GetAsyncKeyState('D') & 0x8000) {
            gInput.camPos.x += right.x * speed * dt;
            gInput.camPos.y += right.y * speed * dt;
            gInput.camPos.z += right.z * speed * dt;
        }
        if (GetAsyncKeyState('A') & 0x8000) {
            gInput.camPos.x -= right.x * speed * dt;
            gInput.camPos.y -= right.y * speed * dt;
            gInput.camPos.z -= right.z * speed * dt;
        }

        static float spinT = 0.0f; spinT += dt * 0.7f;
        XMMATRIX world = XMMatrixRotationY(spinT) * XMMatrixRotationX(spinT * 0.5f);

        XMVECTOR pos = XMVectorSet(gInput.camPos.x, gInput.camPos.y, gInput.camPos.z, 0.0f);
        XMVECTOR dir = XMVector3Normalize(vF);
        XMMATRIX view = XMMatrixLookToLH(pos, dir, vUp);

        float aspect = (gHeight > 0) ? float(gWidth) / float(gHeight) : 1.0f;
        XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(60.0f), aspect, 0.1f, 100.0f);

        XMMATRIX mvp = world * view * proj;
        XMStoreFloat4x4(&cbMapped->mvp, XMMatrixTranspose(mvp));

        // --- Record ---
        hr = command_allocator->Reset();
        hr = command_list->Reset(command_allocator, nullptr);

        UINT bb = swap_chain->GetCurrentBackBufferIndex();

        // Present -> RT
        {
            D3D12_RESOURCE_BARRIER b = {};
            b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            b.Transition.pResource = backbuffers[bb];
            b.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            b.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            command_list->ResourceBarrier(1, &b);
        }

        D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtv_heap->GetCPUDescriptorHandleForHeapStart();
        rtv.ptr += bb * rtvInc;
        D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsv_heap->GetCPUDescriptorHandleForHeapStart();

        float clearColor[] = { 0.02f, 0.09f, 0.18f, 1.0f };
        command_list->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
        command_list->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        D3D12_VIEWPORT vp{ 0,0,(float)gWidth,(float)gHeight,0,1 };
        D3D12_RECT sc{ 0,0,gWidth,gHeight };
        command_list->RSSetViewports(1, &vp);
        command_list->RSSetScissorRects(1, &sc);

        command_list->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
        command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        command_list->IASetVertexBuffers(0, 1, &vbv);
        command_list->IASetIndexBuffer(&ibv);

        command_list->SetGraphicsRootSignature(root_signature);
        command_list->SetPipelineState(pso);
        command_list->SetGraphicsRootConstantBufferView(0, cb->GetGPUVirtualAddress());

        // Draw the cube
        command_list->DrawIndexedInstanced(_countof(gCubeIndices), 1, 0, 0, 0);

        {
            D3D12_RESOURCE_BARRIER b = {};
            b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            b.Transition.pResource = backbuffers[bb];
            b.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            b.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            command_list->ResourceBarrier(1, &b);
        }

        command_list->Close();
        ID3D12CommandList* lists[] = { command_list };
        command_queue->ExecuteCommandLists(1, lists);
        swap_chain->Present(1, 0);

        waitGPU();
    }

    // Cleanup
    if (cb) { cb->Unmap(0, nullptr); cb->Release(); }
    if (ib) ib->Release();
    if (vb) vb->Release();
    if (depth) depth->Release();
    if (dsv_heap) dsv_heap->Release();
    for (auto* bb : backbuffers) if (bb) bb->Release();
    if (rtv_heap) rtv_heap->Release();
    if (pso) pso->Release();
    if (root_signature) root_signature->Release();
    if (swap_chain) swap_chain->Release();
    if (factory) factory->Release();
    if (command_list) command_list->Release();
    if (command_allocator) command_allocator->Release();
    if (command_queue) command_queue->Release();
    if (fenceEvent) CloseHandle(fenceEvent);
    if (fence) fence->Release();
    if (device) device->Release();

    return 0;
}
