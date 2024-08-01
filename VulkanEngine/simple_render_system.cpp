#include "simple_render_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <stdexcept>
#include <array>
#include <cassert>

namespace lve {

	/**
	 * @brief A system for rendering simple game objects with Vulkan.
	 *
	 * The `SimpleRenderSystem` class provides functionality for rendering game objects using a basic shader pipeline
	 * and push constants. It is designed to work with a Vulkan-based rendering engine and handles the creation and
	 * management of Vulkan pipelines and pipeline layouts for rendering operations.
	 *
	 * The system uses Vulkan's push constants to pass model and normal matrices to the shaders, and updates the shader
	 * with the current transformation data for each game object. It binds the necessary descriptor sets and issues
	 * draw commands to render the objects.
	 *
	 * The class provides the following functionalities:
	 * - **Constructor & Destructor**: Initializes Vulkan resources required for rendering and cleans up resources
	 *   when the system is destroyed.
	 * - **Pipeline Creation**: Sets up the Vulkan pipeline layout and pipeline specifically for rendering simple game objects.
	 * - **Rendering**: Binds the pipeline, descriptor sets, and push constants for each game object. Issues draw commands
	 *   to render the objects.
	 *
	 * @see LveDevice
	 * @see LvePipeline
	 * @see FrameInfo
	 * @see VkPipelineLayout
	 * @see VkDescriptorSetLayout
	 */

	struct SimplePushConstantData {
		glm::mat4 modelMatrix{ 1.f };
		glm::mat4 normalMatrix{ 1.f };
	};

	/**
		 * @brief Constructs a `SimpleRenderSystem` instance.
		 *
		 * @param device A reference to the `LveDevice` used to create Vulkan resources.
		 * @param renderPass A Vulkan `VkRenderPass` used for rendering.
		 * @param globalSetLayout A Vulkan `VkDescriptorSetLayout` for global descriptor sets.
		 */
	SimpleRenderSystem::SimpleRenderSystem(LveDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
		: lveDevice{ device } {
		createPipelineLayout(globalSetLayout);
		createPipeline(renderPass);
	}

	/**
		 * @brief Destructs the `SimpleRenderSystem` instance.
		 *
		 * Cleans up Vulkan resources associated with the pipeline layout.
		 */
	SimpleRenderSystem::~SimpleRenderSystem() { 
		vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
	}

	/**
		 * @brief Creates the pipeline layout for the simple render system.
		 *
		 * @param globalSetLayout A Vulkan `VkDescriptorSetLayout` used in the pipeline layout.
		 *
		 * Configures the pipeline layout with push constants for model and normal matrices and descriptor set layouts.
		 * Throws an exception if pipeline layout creation fails.
		 */
	void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		if (vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
			VK_SUCCESS) {
			throw std::runtime_error("Failed to create pipeline layout!");
		}
	}

	/**
		 * @brief Creates the Vulkan pipeline for rendering simple game objects.
		 *
		 * @param renderPass A Vulkan `VkRenderPass` used for rendering.
		 *
		 * Sets up the Vulkan graphics pipeline, including shaders and pipeline configuration. The pipeline is configured
		 * without attribute and binding descriptions.
		 * Throws an exception if pipeline creation fails.
		 */
	void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
		assert(pipelineLayout != nullptr && "Cannot create pipeline before pipline layout.");

		PipelineConfigInfo pipelineConfig{};
		LvePipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.attributeDescriptions.clear();
		pipelineConfig.bindingDescriptions.clear();
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		lvePipeline = std::make_unique<LvePipeline>(
			lveDevice,
			"simple_shader.vert.spv",
			"simple_shader.frag.spv",
			pipelineConfig);
	}

	/**
		 * @brief Renders game objects for the current frame.
		 *
		 * @param frameInfo A reference to the `FrameInfo` struct containing frame-specific data.
		 *
		 * Binds the pipeline and descriptor sets, pushes transformation matrices to the shaders, and issues draw commands
		 * for each game object with a model. Objects without a model are skipped.
		 */
	void SimpleRenderSystem::renderGameObjects(FrameInfo &frameInfo) {
		lvePipeline->bind(frameInfo.commandBuffer);

		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0, 
			1,
			&frameInfo.globalDescriptorSet,
			0, 
			nullptr
		);

		for (auto& kv : frameInfo.gameObjects) {
			auto& obj = kv.second;
			if (obj.model == nullptr) continue;
			SimplePushConstantData push{};
			push.modelMatrix = obj.transform.mat4();
			push.normalMatrix = obj.transform.normalMatrix();

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&push);
			obj.model->bind(frameInfo.commandBuffer);
			obj.model->draw(frameInfo.commandBuffer);
		}
	}
}