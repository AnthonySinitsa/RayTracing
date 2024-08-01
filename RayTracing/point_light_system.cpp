#include "point_light_system.hpp"

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
	 * @brief A system for managing and rendering point lights in a Vulkan application.
	 *
	 * The `PointLightSystem` class is responsible for setting up the Vulkan pipeline and rendering point lights in a scene.
	 * It handles the creation of pipeline layouts, pipelines, and updates the point light properties for each frame.
	 *
	 * The system uses Vulkan's push constants to pass point light data to the shaders and updates the uniform buffer object
	 * (UBO) with the latest point light information every frame. It also binds the appropriate descriptor sets and issues
	 * draw commands for each point light.
	 *
	 * The class provides the following functionalities:
	 * - **Constructor & Destructor**: Initializes the system by creating necessary Vulkan resources and cleans up
	 *   resources when destroyed.
	 * - **Pipeline Creation**: Sets up the Vulkan pipeline layout and pipeline specifically for rendering point lights.
	 * - **Light Update**: Updates the point light positions and properties based on frame information and applies transformations.
	 * - **Rendering**: Binds the pipeline and descriptor sets, pushes light constants to the shaders, and issues draw commands
	 *   to render point lights.
	 *
	 * @see LveDevice
	 * @see LvePipeline
	 * @see FrameInfo
	 * @see GlobalUbo
	 * @see VkPipelineLayout
	 * @see VkDescriptorSetLayout
	 */
	struct PointLightPushConstants {
		glm::vec4 position{};
		glm::vec4 color{};
		float radius;
	};

	/**
		 * @brief Constructs a `PointLightSystem` instance.
		 *
		 * @param device A reference to the `LveDevice` used to create Vulkan resources.
		 * @param renderPass A Vulkan `VkRenderPass` used for rendering.
		 * @param globalSetLayout A Vulkan `VkDescriptorSetLayout` for global descriptor sets.
		 */
	PointLightSystem::PointLightSystem(LveDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
		: lveDevice{ device } {
		createPipelineLayout(globalSetLayout);
		createPipeline(renderPass);
	}

	/**
		 * @brief Destructs the `PointLightSystem` instance.
		 *
		 * Cleans up Vulkan resources associated with the pipeline layout.
		 */
	PointLightSystem::~PointLightSystem() {
		vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
	}

	/**
		 * @brief Creates the pipeline layout for the point light system.
		 *
		 * @param globalSetLayout A Vulkan `VkDescriptorSetLayout` used in the pipeline layout.
		 *
		 * Configures the pipeline layout with push constants for point light properties and descriptor set layouts.
		 * Throws an exception if pipeline layout creation fails.
		 */
	void PointLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PointLightPushConstants);

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
		 * @brief Creates the Vulkan pipeline for rendering point lights.
		 *
		 * @param renderPass A Vulkan `VkRenderPass` used for rendering.
		 *
		 * Sets up the Vulkan graphics pipeline, including shaders and pipeline configuration.
		 * Throws an exception if pipeline creation fails.
		 */
	void PointLightSystem::createPipeline(VkRenderPass renderPass) {
		assert(pipelineLayout != nullptr && "Cannot create pipeline before pipline layout.");

		PipelineConfigInfo pipelineConfig{};
		LvePipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		lvePipeline = std::make_unique<LvePipeline>(
			lveDevice,
			"point_light.vert.spv",
			"point_light.frag.spv",
			pipelineConfig);
	}

	/**
		 * @brief Updates the point light properties for the current frame.
		 *
		 * @param frameInfo A reference to the `FrameInfo` struct containing frame-specific data.
		 * @param ubo A reference to the `GlobalUbo` struct containing global uniform buffer data.
		 *
		 * Updates the positions, colors, and other properties of point lights based on the current frame's data.
		 */
	void PointLightSystem::update(FrameInfo& frameInfo, GlobalUbo& ubo) {
		auto rotateLight = glm::rotate(glm::mat4(1.f), frameInfo.frameTime, { 0.f, -1.f, 0.f });

		int lightIndex = 0;
		for (auto& kv : frameInfo.gameObjects) {
			auto& obj = kv.second;
			if (obj.pointLight == nullptr) continue;

			assert(lightIndex < MAX_LIGHTS && "Point lights exceed maximum specified.");

			// update light position
			obj.transform.translation = glm::vec3(rotateLight * glm::vec4(obj.transform.translation, 1.f));

			// copy light to ubo
			ubo.pointLights[lightIndex].position = glm::vec4(obj.transform.translation, 1.f);
			ubo.pointLights[lightIndex].color = glm::vec4(obj.color, obj.pointLight->lightIntensity);

			lightIndex += 1;
		}
		ubo.numLights = lightIndex;
	}

	/**
		 * @brief Renders point lights for the current frame.
		 *
		 * @param frameInfo A reference to the `FrameInfo` struct containing frame-specific data.
		 *
		 * Binds the pipeline, descriptor sets, and pushes point light data to the shaders. Issues draw commands to render
		 * the point lights.
		 */
	void PointLightSystem::render(FrameInfo &frameInfo) {
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
			if (obj.pointLight == nullptr) continue;

			PointLightPushConstants push{};
			push.position = glm::vec4(obj.transform.translation, 1.f);
			push.color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
			push.radius = obj.transform.scale.x;

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(PointLightPushConstants),
				&push
			);
			vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
		}
	}
}