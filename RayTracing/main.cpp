#define _CRT_SECURE_NO_WARNINGS
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#pragma comment(lib, "glfw3.lib")
#include <iostream>

#include <vulkan/vulkan.hpp>
#pragma comment(lib, "vulkan-1.lib")

// inline shader
#include <shaderc/shaderc.hpp>
#pragma comment(lib, "shadercd.lib")
#pragma comment(lib, "shaderc_utild.lib")
#pragma comment(lib, "shaderc_combinedd.lib")

//-----------------------------------
//-----------------------------------

std::string vertexShader = R"vertexshader(

#version 450
#extension GL_ARB_seperate_shader_objects : enable
out gl_PerVertex {
	vec4 gl_Position;
};
layout(location = 0) out vec2 fragUV;
vec2 positions[4] = vec2[](
vec2(-1.0, 1.0),
vec2(-1.0, -1.0),
vec2(1.0, 1.0),
vec2(1.0, -1.0)
);

vec2 uvs[4] = vec2[](
vec2(1.0, 1.0),
vec2(1.0, -1.0),
vec2(-1.0, 1.0),
vec2(-1.0, -1.0)
);

void main() {
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	fragUV = uvs[gl_VertexIndex] * 0.5 + 0.5; // 0->1
}
)vertexshader";

std::string fragmentShader = R"fragmentShader(

#version 450
#extension GL_ARB_seperate_shader_objects : enable
layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;
void main() {
	vec2 uv = (fragUV * 2.0) - 1.0;

	// normalize and adjust for ratio
	uv *= .8;

	// initialize some colors
	vec4 color1 = vec4(.4, .6, .7, 1.0);
	vec4 color2 = vec4(.9, .7, .6, 1.0);

	// shade with 2 faux lights
	color1 *= .8 - distance(uv, vec2(-.1, -.1));
	color2 *= .6 - distance(uv, vec2(.25, .3));
	vec4 sphere = color1 + color2;

	// limit edges to circle shape
	float d = distance(uv, vec2(0.0));
	// smooth edges
	float t = 1.0 - smoothstep(.6, .61, d);
	// apply shape to color
	sphere *= t + 0.2 * uv.y;

	// output final color and brighten
	vec4 fragColor = sphere * 1.6;
	outColor = vec4(fragColor.xyz, 1.0);
}
)fragmentShader";

//-----------------------------------
//-----------------------------------

int main() {
	const uint32_t width = 640, height = 480;
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	auto window = glfwCreateWindow(width, height, "Hello Vulkan Triangle", nullptr, nullptr);
	vk::ApplicationInfo appInfo("Hello Vulkan Triangle", 0, nullptr, 0, VK_API_VERSION_1_3);

	auto glfwExtensionCount = 0u;
	auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> glfwExtensionsVector(glfwExtensions, glfwExtensions + glfwExtensionCount);
	const vk::ApplicationInfo applicationInfo("Hello World", 0, nullptr, 0, VK_API_VERSION_1_3);
	const auto instance = vk::createInstanceUnique(vk::InstanceCreateInfo({}, &applicationInfo, 0, nullptr, static_cast<uint32_t>(glfwExtensionsVector.size()), glfwExtensionsVector.data()));

	VkSurfaceKHR surfaceTmp;
	VkResult err = glfwCreateWindowSurface(*instance, window, nullptr, &surfaceTmp);
	vk::UniqueSurfaceKHR surface(surfaceTmp, *instance);

	size_t presentQueueFamilyIndex = 0u;
	float queuePriority = 0.0f;
	auto queueCreateInfos = vk::DeviceQueueCreateInfo{ vk::DeviceQueueCreateFlags(), static_cast<uint32_t>(0), 1, &queuePriority };

	const auto physicalDevice = instance->enumeratePhysicalDevices()[0];

	const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	vk::UniqueDevice device = physicalDevice.createDeviceUnique(vk::DeviceCreateInfo(
		vk::DeviceCreateFlags(), 1, &queueCreateInfos, 0u, nullptr, static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data()
	));

	uint32_t imageCount = 2;
	auto format = vk::Format::eB8G8R8A8Unorm;

	vk::SwapchainCreateInfoKHR swapChainCreateInfo({}, surface.get(), imageCount, format, vk::ColorSpaceKHR::eSrgbNonlinear, vk::Extent2D(width, height), 1, vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, 0u, static_cast<uint32_t*>(nullptr), vk::SurfaceTransformFlagBitsKHR::eIdentity, vk::CompositeAlphaFlagBitsKHR::eOpaque, vk::PresentModeKHR::eFifo, true, nullptr);

	auto swapChain = device->createSwapchainKHRUnique(swapChainCreateInfo);
	std::vector<vk::Image> swapChainImages = device->getSwapchainImagesKHR(swapChain.get());

	std::vector<vk::UniqueImageView> imageViews;
	imageViews.reserve(swapChainImages.size());
	for (auto image : swapChainImages) {
		vk::ImageViewCreateInfo imageViewCreateInfo(vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D, format, vk::ComponentMapping{ vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA }, vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
		imageViews.push_back(device->createImageViewUnique(imageViewCreateInfo));
	}

	auto compileShader = [&](const std::string& srcGLSL, shaderc_shader_kind shaderKind) {
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetOptimizationLevel(shaderc_optimization_level_performance);
		shaderc::SpvCompilationResult shaderModule = compiler.CompileGlslToSpv(srcGLSL, shaderKind, "shader", options);
		if (shaderModule.GetCompilationStatus() != shaderc_compilation_status_success) {
			std::cerr << shaderModule.GetErrorMessage() << std::endl;
		}
		auto shaderCode = std::vector<uint32_t>{ shaderModule.cbegin(), shaderModule.cend() };
		auto shaderSize = std::distance(shaderCode.begin(), shaderCode.end());
		auto shaderCreateInfo = vk::ShaderModuleCreateInfo{ {}, shaderSize * sizeof(uint32_t), shaderCode.data() };
		return device->createShaderModuleUnique(shaderCreateInfo);
		};

	auto vertexShaderModule = compileShader(vertexShader, shaderc_glsl_vertex_shader);
	auto fragmentShaderModule = compileShader(fragmentShader, shaderc_glsl_fragment_shader);
	auto vertShaderStageInfo = vk::PipelineShaderStageCreateInfo{ {}, vk::ShaderStageFlagBits::eVertex, *vertexShaderModule, "main" };
	auto fragShaderStageInfo = vk::PipelineShaderStageCreateInfo{ {}, vk::ShaderStageFlagBits::eFragment, *fragmentShaderModule, "main" };

	auto pipelineShaderStages = std::vector<vk::PipelineShaderStageCreateInfo>{vertShaderStageInfo, fragShaderStageInfo};
	
	auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo{ {}, 0u, nullptr, 0u, nullptr };

	auto inputAssembly = vk::PipelineInputAssemblyStateCreateInfo{ {}, vk::PrimitiveTopology::eTriangleStrip, false };

	auto viewport = vk::Viewport{ 0.0f, 0.09f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };

	auto scissor = vk::Rect2D{ { 0, 0 }, vk::Extent2D(width, height) };

	auto viewportState = vk::PipelineViewportStateCreateInfo{ {}, 1, &viewport, 1, &scissor };

	auto rasterizer = vk::PipelineRasterizationStateCreateInfo{ 
		{}, /*depthClamp*/ false, 
		/*rasterizerDiscard*/ false, vk::PolygonMode::eFill, {}, 
		/*frontFace*/ vk::FrontFace::eCounterClockwise, {}, {}, {}, {}, 1.0f };

	auto colorBlendAttachment = vk::PipelineColorBlendAttachmentState{ {},
		/*srcCol*/ vk::BlendFactor::eOne,
		/*dstCol*/ vk::BlendFactor::eZero, /*colBlend*/ vk::BlendOp::eAdd,
		/*srcAlpha*/ vk::BlendFactor::eOne, /*dstAlpha*/ vk::BlendFactor::eZero,
		/*alphaBlend*/ vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
		};

	auto colorBlending = vk::PipelineColorBlendStateCreateInfo{ {}, false, vk::LogicOp::eCopy, 1, &colorBlendAttachment };

	auto pipelineLayout = device->createPipelineLayoutUnique({}, nullptr);

	auto colorAttachment = vk::AttachmentDescription{ {}, format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, {}, {}, {}, vk::ImageLayout::ePresentSrcKHR };

	auto colourAttachmentRef = vk::AttachmentReference{ 0, vk::ImageLayout::eColorAttachmentOptimal };

	auto subpass = vk::SubpassDescription{ {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colourAttachmentRef };

	auto semaphoreCreateInfo = vk::SemaphoreCreateInfo{};
	auto imageAvailableSemaphore = device->createSemaphoreUnique(semaphoreCreateInfo);
	auto renderFinishedSemaphore = device->createSemaphoreUnique(semaphoreCreateInfo);

	auto subpassDependency = vk::SubpassDependency{ VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite };

	auto renderPass = device->createRenderPassUnique(
		vk::RenderPassCreateInfo{ {}, 1, &colorAttachment, 1, &subpass, 1, &subpassDependency }
	);

	auto pipelineCreateInfo = vk::GraphicsPipelineCreateInfo{ {}, 2, pipelineShaderStages.data(), &vertexInputInfo, &inputAssembly, nullptr, &viewportState, &rasterizer, nullptr, nullptr, &colorBlending, nullptr, *pipelineLayout, *renderPass, 0 };

	auto pipeline = device->createGraphicsPipelineUnique({}, pipelineCreateInfo).value;

	auto framebuffers = std::vector<vk::UniqueFramebuffer>(imageCount);
	for (size_t i = 0; i < imageViews.size(); i++) {
		framebuffers[i] = device->createFramebufferUnique(vk::FramebufferCreateInfo{ {}, *renderPass, 1, &(*imageViews[i]), width, height, 1 });
	}

	auto commandPoolUnique = device->createCommandPoolUnique({ {}, static_cast<uint32_t>(presentQueueFamilyIndex) });

	std::vector<vk::UniqueCommandBuffer> commandBuffers = device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(
		commandPoolUnique.get(), vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(framebuffers.size())
	));

	auto deviceQueue = device->getQueue(static_cast<uint32_t>(presentQueueFamilyIndex), 0);

	for (size_t i = 0; i < commandBuffers.size(); i++) {
		auto beginInfo = vk::CommandBufferBeginInfo{};
		commandBuffers[i]->begin(beginInfo);
		vk::ClearValue clearValues{};
		auto renderPassBeginInfo = vk::RenderPassBeginInfo{ renderPass.get(), framebuffers[i].get(), vk::Rect2D{ { 0, 0 }, vk::Extent2D{ width, height }}, 1, &clearValues };

		commandBuffers[i]->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
		commandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
		commandBuffers[i]->draw(4, 1, 0, 0);
		commandBuffers[i]->endRenderPass();
		commandBuffers[i]->end();
	}

	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		auto imageIndex = device->acquireNextImageKHR(swapChain.get(), std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore.get(), {});
		vk::PipelineStageFlags waitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		auto submitInfo = vk::SubmitInfo{ 1, &imageAvailableSemaphore.get(), &waitStageMask, 1, &commandBuffers[imageIndex.value].get(), 1, &renderFinishedSemaphore.get() };

		deviceQueue.submit(submitInfo, {});

		auto presentInfo = vk::PresentInfoKHR{ 1, &renderFinishedSemaphore.get(), 1, &swapChain.get(), &imageIndex.value };
		auto result = deviceQueue.presentKHR(presentInfo);

		device->waitIdle();
	}
}