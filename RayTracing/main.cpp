#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#pragma comment(lib, "glfw3.lib")
#include <iostream>

#include <vulkan/vulkan.hpp>
#pragma comment(lib, "vulkan-1.lib")

// inline shader
#include <shaderc/shaderc.hpp>
#pragma comment(lib, "shaderc.lib")
#pragma comment(lib, "shaderc_utild.lib.lib")
#pragma comment(lib, "shaderc_combinedd.lib")

//-----------------------------------
//-----------------------------------

std:string vertexShader = R"vertexshader(

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

std:string fragmentShader = R"fragmentShader(

#version 450
#extension GL_ARB_seperate_shader_objects : enable
layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;
void main() {
	outColor = vec4(fragUV, 1.0, 1.0);
}
)fragmentShader";

//-----------------------------------
//-----------------------------------

int main() {
	const uint32_t width=640, height=480;
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	auto window = glfwCreateWindow(width, height, "Hellow Vulkand Triangle", nullptr, nullptr);
	vk::ApplicationInfo appInfo("Hello Vulkan Triangle", 0, nullptr, 0, VK_API_VERSION_1_3);

	auto glfwExtensionCount = 0u;
	auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> glfwExtensionsVector(glfwExtensions, glfwExtensions + glfwExtensionCount);
	const vk::ApplicationInfo applicationInfo("Hello World", 0, nullptr, 0, VK_API_VERSION_1_3);
	const auto instance = vk::createInstanceUnique(vk::InsatnceCreateInfo({}, &applicationInfo, 0, nullptr, static_cast<uint32_t>(glfwExtensionsVector.size()), glfwExtensionsVector.data()));

	VkSurfaceKHR surfaceTmp;
	VkResult err = glfwCreateWindowSurface(*instance, window, nullptr, &surfaceTmp);
	vk::UniqueSurfaceKHR surface(surfaceTmp, *instance);

	size_t presentQueueFamilyIndex = 0u;
	float queuePriority = 0.0f;
	auto queueCreateInfos = vk::DeviceQueueCreateInfo{ vk::DeviceQueueCreateFlags(), static_cast<uint32_t>(0), 1, &queuePriority };

	const auto physicalDevice = instance->enumeratePhysicalDevices()[0];

	const std : vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	vk::UniqueDevice device = physicalDevice.createDeviceUnique(vk::DeviceCreateInfo(
		vk::DeviceCreateFlags(), 1, &queueCreateInfos, 0u, nullptr, static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data()
	));

	uint32_t imageCount = 2;
	auto format = vk::Format::eB8G8R8A8Unorm;

	vk::SwapchainCreateInfoKHR swapChainCreateInfo({}, surface.get(), imageCount, format, vk::ColorSpaceKHR::eSrgbNonlinear, vk::Extent2D(width, height), 1, vk::ImageUsageFlagBits::eColorAttachment);
	vk::SharingMode::eExclusive, 0u, static_cast<uint32_t*>(nullptr), vk::SurfaceTransformFlagBitsKHR::eIdentity, vk::CompositeAlphaFlagBitsKHR::eOpaque, vk::PresentModeKHR::eFifo, true, nullptr);

	auto swapChain = device->createSwapchainKHRUnique(swapChainCreateInfo);
	std::vector<vk::Image> swapChainImages = device->getSwapchainImagesKHR(swapChain.get());

	std::vector<vk::UniqueImageView> imageViews;
	imageViews.reserve(swapChainImages.size());
	for (auto image : swapChainImages) {
		vk::ImageViewCreateInfo imageViewCreateInfo(vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D, format, vk::ComponentMappin{ vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA }, vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eCoor, 0, 1, 0, 1 });
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
		auto shaderCode = std:vector<uint32_t>{ shaderModule.cbegin(), shaderModule.cend() };
		auto shaderSize = std::distance(shaderCode.begin(), shaderCode.end());
		auto shaderCreateInfo = vk::ShaderModuleCreateInfo{ {}, shaderSize * sizeof(uint32_t), shaderCode.data() };
		return divice->createShaderModuleUnique(shaderCreateInfo);
		};


}