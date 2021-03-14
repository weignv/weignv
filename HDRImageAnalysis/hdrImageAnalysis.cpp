// run a compute shader to calculate the image stats.

// write the results to a static buffer as meta data for panel analysis
// The output buffer (created below) is on a default heap, so only the GPU can access it.

D3D12_HEAP_PROPERTIES defaultHeapProperties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };
D3D12_RESOURCE_DESC outputBufferDesc{ CD3DX12_RESOURCE_DESC::Buffer(outputBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) };
winrt::com_ptr<::ID3D12Resource> outputBuffer;
winrt::check_hresult(d3d12Device->CreateCommittedResource(
    &defaultHeapProperties,
    D3D12_HEAP_FLAG_NONE,
    &outputBufferDesc,
    D3D12_RESOURCE_STATE_COPY_DEST,
    nullptr,
    __uuidof(outputBuffer),
    outputBuffer.put_void()));

// The readback buffer (created below) is on a readback heap, so that the CPU can access it.

D3D12_HEAP_PROPERTIES readbackHeapProperties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK) };
D3D12_RESOURCE_DESC readbackBufferDesc{ CD3DX12_RESOURCE_DESC::Buffer(outputBufferSize) };
winrt::com_ptr<::ID3D12Resource> readbackBuffer;
winrt::check_hresult(d3d12Device->CreateCommittedResource(
    &readbackHeapProperties,
    D3D12_HEAP_FLAG_NONE,
    &readbackBufferDesc,
    D3D12_RESOURCE_STATE_COPY_DEST,
    nullptr,
    __uuidof(readbackBuffer),
    readbackBuffer.put_void()));

{
    D3D12_RESOURCE_BARRIER outputBufferResourceBarrier
    {
        CD3DX12_RESOURCE_BARRIER::Transition(
            outputBuffer.get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_COPY_SOURCE)
    };
    commandList->ResourceBarrier(1, &outputBufferResourceBarrier);
}

commandList->CopyResource(readbackBuffer.get(), outputBuffer.get());

// Code goes here to close, execute (and optionally reset) the command list, and also
// to use a fence to wait for the command queue.

// Wait for the compute shader to complete the simulation.
UINT64 threadFenceValue = InterlockedIncrement(&m_threadFenceValues[threadIndex]);
ThrowIfFailed(pCommandQueue->Signal(pFence, threadFenceValue));
ThrowIfFailed(pFence->SetEventOnCompletion(threadFenceValue, m_threadFenceEvents[threadIndex]));
WaitForSingleObject(m_threadFenceEvents[threadIndex], INFINITE);

// The code below assumes that the GPU wrote FLOATs to the buffer.

D3D12_RANGE readbackBufferRange{ 0, outputBufferSize };
FLOAT * pReadbackBufferData{};
winrt::check_hresult(
    readbackBuffer->Map
    (
        0,
        &readbackBufferRange,
        reinterpret_cast<void**>(&pReadbackBufferData)
    )
);

// Code goes here to access the data via pReadbackBufferData.

D3D12_RANGE emptyRange{ 0, 0 };
readbackBuffer->Unmap
(
    0,
    &emptyRange
);
