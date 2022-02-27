#include <iostream>
#include "emscripten.h"
#include <stdio.h>
#include <SDL/SDL.h>
#include "webgpu/webgpu.h"
#include "emscripten/html5_webgpu.h"

#include "./shaders/triangle_vert.h"
#include "./shaders/triangle_frag.h"

WGPUDevice device;
WGPUQueue queue;
WGPUSwapChain swapchain;
WGPURenderPipeline pipeline;
WGPUBuffer vertBuf;
WGPUBuffer indxBuf;
WGPUBuffer uRotBuf;
WGPUBindGroup bindGroup;

float rotDeg = 0.0f;

static WGPUSwapChain createSwapchain() {
    WGPUSurfaceDescriptorFromCanvasHTMLSelector canvDesc = {};
    canvDesc.chain.sType = WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector;
    canvDesc.selector = "canvas";
    
    WGPUSurfaceDescriptor surfDesc = {};
    surfDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&canvDesc);
    
    WGPUSurface surface = wgpuInstanceCreateSurface(nullptr, &surfDesc);
    
    WGPUSwapChainDescriptor swapDesc = {};
    swapDesc.usage  = WGPUTextureUsage_RenderAttachment;
    swapDesc.format = WGPUTextureFormat_BGRA8Unorm;
    swapDesc.width  = 800;
    swapDesc.height = 450;
    swapDesc.presentMode = WGPUPresentMode_Fifo;
    
    return wgpuDeviceCreateSwapChain(device, surface, &swapDesc);
}

static WGPUShaderModule createShader(const char* const code, const char* label = nullptr) {
	WGPUShaderModuleWGSLDescriptor wgsl = {};
	wgsl.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
	wgsl.source = code;
	WGPUShaderModuleDescriptor desc = {};
	desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgsl);
	desc.label = label;
	return wgpuDeviceCreateShaderModule(device, &desc);
}

static WGPUBuffer createBuffer(const void* data, size_t size, WGPUBufferUsage usage) {
	WGPUBufferDescriptor desc = {};
	desc.usage = WGPUBufferUsage_CopyDst | usage;
	desc.size  = size;
	WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &desc);
	wgpuQueueWriteBuffer(queue, buffer, 0, data, size);
	return buffer;
}

static void createPipeline() {
	// compile shaders
	WGPUShaderModule vertMod = createShader(triangle_vert_wgsl);
	WGPUShaderModule fragMod = createShader(triangle_frag_wgsl);

	WGPUBufferBindingLayout buf = {};
	buf.type = WGPUBufferBindingType_Uniform;

	// bind group layout (used by both the pipeline layout and uniform bind group, released at the end of this function)
	WGPUBindGroupLayoutEntry bglEntry = {};
	bglEntry.binding = 0;
	bglEntry.visibility = WGPUShaderStage_Vertex;
	bglEntry.buffer = buf;

	WGPUBindGroupLayoutDescriptor bglDesc = {};
	bglDesc.entryCount = 1;
	bglDesc.entries = &bglEntry;
	WGPUBindGroupLayout bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bglDesc);

	// pipeline layout (used by the render pipeline, released after its creation)
	WGPUPipelineLayoutDescriptor layoutDesc = {};
	layoutDesc.bindGroupLayoutCount = 1;
	layoutDesc.bindGroupLayouts = &bindGroupLayout;
	WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc);

	// describe buffer layouts
	WGPUVertexAttribute vertAttrs[2] = {};
	vertAttrs[0].format = WGPUVertexFormat_Float32x2;
	vertAttrs[0].offset = 0;
	vertAttrs[0].shaderLocation = 0;
	vertAttrs[1].format = WGPUVertexFormat_Float32x3;
	vertAttrs[1].offset = 2 * sizeof(float);
	vertAttrs[1].shaderLocation = 1;
	WGPUVertexBufferLayout vertexBufferLayout = {};
	vertexBufferLayout.arrayStride = 5 * sizeof(float);
	vertexBufferLayout.attributeCount = 2;
	vertexBufferLayout.attributes = vertAttrs;

	// Fragment state
	WGPUBlendState blend = {};
	blend.color.operation = WGPUBlendOperation_Add;
	blend.color.srcFactor = WGPUBlendFactor_One;
	blend.color.dstFactor = WGPUBlendFactor_One;
	blend.alpha.operation = WGPUBlendOperation_Add;
	blend.alpha.srcFactor = WGPUBlendFactor_One;
	blend.alpha.dstFactor = WGPUBlendFactor_One;

	WGPUColorTargetState colorTarget = {};
	colorTarget.format = WGPUTextureFormat_BGRA8Unorm;
	colorTarget.blend = &blend;
	colorTarget.writeMask = WGPUColorWriteMask_All;

	WGPUFragmentState fragment = {};
	fragment.module = fragMod;
	fragment.entryPoint = "main";
	fragment.targetCount = 1;
	fragment.targets = &colorTarget;

	WGPURenderPipelineDescriptor desc = {};
	desc.fragment = &fragment;

	// Other state
	desc.layout = pipelineLayout;
	desc.depthStencil = nullptr;

	desc.vertex.module = vertMod;
	desc.vertex.entryPoint = "main";
	desc.vertex.bufferCount = 1;//0;
	desc.vertex.buffers = &vertexBufferLayout;

	desc.multisample.count = 1;
	desc.multisample.mask = 0xFFFFFFFF;
	desc.multisample.alphaToCoverageEnabled = false;

	desc.primitive.frontFace = WGPUFrontFace_CCW;
	desc.primitive.cullMode = WGPUCullMode_None;
	desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
	desc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;

	pipeline = wgpuDeviceCreateRenderPipeline(device, &desc);

	// partial clean-up (just move to the end, no?)
	wgpuPipelineLayoutRelease(pipelineLayout);

	wgpuShaderModuleRelease(fragMod);
	wgpuShaderModuleRelease(vertMod);

	// create the buffers (x, y, r, g, b)
	float const vertData[] = {
		-0.8f, -0.8f, 0.0f, 0.0f, 1.0f, // BL
		 0.8f, -0.8f, 0.0f, 1.0f, 0.0f, // BR
		-0.0f,  0.8f, 1.0f, 0.0f, 0.0f, // top
	};
	uint16_t const indxData[] = {
		0, 1, 2,
		0 // padding (better way of doing this?)
	};
	vertBuf = createBuffer(vertData, sizeof(vertData), WGPUBufferUsage_Vertex);
	indxBuf = createBuffer(indxData, sizeof(indxData), WGPUBufferUsage_Index);

	// create the uniform bind group (note 'rotDeg' is copied here, not bound in any way)
	uRotBuf = createBuffer(&rotDeg, sizeof(rotDeg), WGPUBufferUsage_Uniform);

	WGPUBindGroupEntry bgEntry = {};
	bgEntry.binding = 0;
	bgEntry.buffer = uRotBuf;
	bgEntry.offset = 0;
	bgEntry.size = sizeof(rotDeg);

	WGPUBindGroupDescriptor bgDesc = {};
	bgDesc.layout = bindGroupLayout;
	bgDesc.entryCount = 1;
	bgDesc.entries = &bgEntry;

	bindGroup = wgpuDeviceCreateBindGroup(device, &bgDesc);

	// last bit of clean-up
	wgpuBindGroupLayoutRelease(bindGroupLayout);
}

static bool draw() {
	WGPUTextureView backBufView = wgpuSwapChainGetCurrentTextureView(swapchain);			// create textureView

	WGPURenderPassColorAttachment colorDesc = {};
	colorDesc.view    = backBufView;
	colorDesc.loadOp  = WGPULoadOp_Clear;
	colorDesc.storeOp = WGPUStoreOp_Store;
	colorDesc.clearColor.r = 0.3f;
	colorDesc.clearColor.g = 0.3f;
	colorDesc.clearColor.b = 0.3f;
	colorDesc.clearColor.a = 1.0f;

	WGPURenderPassDescriptor renderPass = {};
	renderPass.colorAttachmentCount = 1;
	renderPass.colorAttachments = &colorDesc;

	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);			// create encoder
	WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPass);	// create pass

	// update the rotation
	rotDeg += 0.1f;
	wgpuQueueWriteBuffer(queue, uRotBuf, 0, &rotDeg, sizeof(rotDeg));

	// draw the triangle (comment these five lines to simply clear the screen)
	wgpuRenderPassEncoderSetPipeline(pass, pipeline);
	wgpuRenderPassEncoderSetBindGroup(pass, 0, bindGroup, 0, 0);
	wgpuRenderPassEncoderSetVertexBuffer(pass, 0, vertBuf, 0, WGPU_WHOLE_SIZE);
	wgpuRenderPassEncoderSetIndexBuffer(pass, indxBuf, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
	wgpuRenderPassEncoderDrawIndexed(pass, 3, 1, 0, 0, 0);

	wgpuRenderPassEncoderEnd(pass);
	wgpuRenderPassEncoderRelease(pass);														// release pass
	WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, nullptr);				// create commands
	wgpuCommandEncoderRelease(encoder);														// release encoder

	wgpuQueueSubmit(queue, 1, &commands);
	wgpuCommandBufferRelease(commands);														// release commands

	/* wgpuSwapChainPresent(swapchain); */
	wgpuTextureViewRelease(backBufView);													// release textureView

	return true;
}

extern "C" {
    int main(int argc, char *argv[])
    {
        std::cout << "Hello world" << std::endl;

        device = emscripten_webgpu_get_device();
        queue = wgpuDeviceGetQueue(device);
        swapchain = createSwapchain();
        createPipeline();
        draw();

        std::cout << "Bye world" << std::endl;
        return 0;
    }
}
