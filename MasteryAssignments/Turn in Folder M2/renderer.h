// minimalistic code to draw a single triangle, this is not part of the API.
#include "shaderc/shaderc.h" // needed for compiling shaders at runtime
#ifdef _WIN32 // must use MT platform DLL libraries on windows
#pragma comment(lib, "shaderc_combined.lib") 
#endif
// Simple Vertex Shader
const char* vertexShaderSource = R"(
#pragma pack_matrix(row_major)
// TODO: Part 2b (inside shader)
[[vk::push_constant]]
cbuffer SHADER_VARS
{
	matrix worldMatrix;
	matrix rotationMatrix;
};


// an ultra simple hlsl vertex shader
float4 main(float2 inputVertex : POSITION) : SV_POSITION
{
	// TODO: Part 2d (inside shader)
	float4 result =  mul(float4(inputVertex, 0, 1), worldMatrix);
	result = mul(result, rotationMatrix);
	return result;
	
}
)";
// Simple Pixel Shader
const char* pixelShaderSource = R"(
// an ultra simple hlsl pixel shader
float4 main() : SV_TARGET 
{	
	// TODO: Part 4g
	return float4(1.0f, 1.0f, 1.0f, 0); 
}
)";
// TODO: Part 4b
const char* vertexShaderSource2 = R"(
#pragma pack_matrix(row_major)
// an ultra simple hlsl vertex shader
[[vk::push_constant]]
cbuffer SHADER_VARS
{
	matrix worldMatrix;
	matrix rotationMatrix;
};

struct VERTEX
{
	float2 pos : POSITION;
	float4 color : COLOR;
};

struct VERTEX_OUT
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
};

VERTEX_OUT main(VERTEX inputVertex)
{
	VERTEX_OUT output;
	output.color = inputVertex.color;
	output.pos = mul(float4(inputVertex.pos, 0, 1), worldMatrix);
	output.pos = mul(output.pos, rotationMatrix);
	return output;
}
)";
// Simple Pixel Shader
const char* pixelShaderSource2 = R"(
// an ultra simple hlsl pixel shader
struct VERTEX_OUT
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
};

float4 main(VERTEX_OUT output : COLOR) : SV_TARGET 
{	
	//return float4(1.0f, 1.0f, 1.0f, 0); // TODO: Part 1a
	return output.color;
}
)";

// Creation, Rendering & Cleanup
class Renderer
{
	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GVulkanSurface vlk;
	GW::CORE::GEventReceiver shutdown;
	// TODO: Part 2a
	GW::MATH::GMATRIXF matrix;
	GW::MATH::GMatrix matrixProxy;

	// what we need at a minimum to draw a triangle
	VkDevice device = nullptr;
	VkBuffer vertexHandle = nullptr;
	VkDeviceMemory vertexData = nullptr;
	// TODO: Part 3c
	VkDeviceMemory indexData2 = nullptr;
	VkBuffer indexHandle2 = nullptr;

	VkShaderModule vertexShader = nullptr;
	VkShaderModule pixelShader = nullptr;
	// pipeline settings for drawing (also required)
	VkPipeline pipeline = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	// TODO: Part 3a
	VkPipeline pipeline2 = nullptr;

	// TODO: Part 4b
	VkShaderModule vertexShader2 = nullptr;
	VkShaderModule pixelShader2 = nullptr;
public:
	// TODO: Part 2b
	struct SHADER_VARS
	{
		GW::MATH::GMATRIXF worldMatrix;
		GW::MATH::GMATRIXF rotationMatrix;
	};

	// TODO: Part 4a
	struct VERTEX
	{
		float pos[2];
		float color[4];
	};

	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GVulkanSurface _vlk)
	{
		win = _win;
		vlk = _vlk;
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);
		// TODO: Part 2a
		matrixProxy.Create();


		/***************** GEOMETRY INTIALIZATION ******************/
		// Grab the device & physical device so we can allocate some stuff
		VkPhysicalDevice physicalDevice = nullptr;
		vlk.GetDevice((void**)&device);
		vlk.GetPhysicalDevice((void**)&physicalDevice);
		
		// TODO: Part 3c
		int INDICES[] = {
			0, 1, 2,
			2, 11, 0,

			2, 3, 4,
			4, 5, 2,

			5, 6, 7,
			7, 8, 5,

			8, 9, 10,
			10, 11, 8,

			2, 5, 8,
			8, 11, 2
		};

		// TODO: Part 4a
		// Transfer triangle data to the vertex buffer. (staging would be prefered here)
		VERTEX crossFigure[] = {
			 {{-0.25f,  0.80f}, {rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX),rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX)}}, //0
			 {{ 0.25f,  0.80f}, {rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX),rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX)}}, //1
			 {{ 0.25f,  0.25f}, {rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX),rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX)}}, //2
			 {{ 0.80f,  0.25f}, {rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX),rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX)}}, //3
			 {{ 0.80f, -0.25f}, {rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX),rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX)}}, //4
			 {{ 0.25f, -0.25f}, {rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX),rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX)}}, //5
			 {{ 0.25f, -0.80f}, {rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX),rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX)}}, //6
			 {{-0.25f, -0.80f}, {rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX),rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX)}}, //7
			 {{-0.25f, -0.25f}, {rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX),rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX)}}, //8
			 {{-0.80f, -0.25f}, {rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX),rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX)}}, //9
			 {{-0.80f,  0.25f}, {rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX),rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX)}}, //10
			 {{-0.25f,  0.25f}, {rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX),rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX)}},//11
			 {{-0.25f,  0.80f}, {rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX),rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX)}} //12
		};


		//CROSS
		GvkHelper::create_buffer(physicalDevice, device, sizeof(crossFigure),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexHandle, &vertexData);
		GvkHelper::write_to_buffer(device, vertexData, crossFigure, sizeof(crossFigure));


		// TODO: Part 3c
		GvkHelper::create_buffer(physicalDevice, device, sizeof(INDICES),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &indexHandle2, &indexData2);
		GvkHelper::write_to_buffer(device, indexData2, INDICES, sizeof(INDICES));


		/***************** SHADER INTIALIZATION ******************/
		// Intialize runtime shader compiler HLSL -> SPIRV
		shaderc_compiler_t compiler = shaderc_compiler_initialize();
		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
		shaderc_compile_options_set_invert_y(options, true);
#ifndef NDEBUG
		shaderc_compile_options_set_generate_debug_info(options);
#endif
		// Create Vertex Shader
		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
			compiler, vertexShaderSource, strlen(vertexShaderSource),
			shaderc_vertex_shader, "main.vert", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &vertexShader);
		shaderc_result_release(result); // done
		// Create Pixel Shader
		result = shaderc_compile_into_spv( // compile
			compiler, pixelShaderSource, strlen(pixelShaderSource),
			shaderc_fragment_shader, "main.frag", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &pixelShader);
		shaderc_result_release(result); // done

		// TODO: Part 4b
		shaderc_compilation_result_t result_P4 = shaderc_compile_into_spv( // compile
			compiler, vertexShaderSource2, strlen(vertexShaderSource2),
			shaderc_vertex_shader, "main.vert", "main", options);
		if (shaderc_result_get_compilation_status(result_P4) != shaderc_compilation_status_success) // errors?
			std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result_P4) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result_P4), // load into Vulkan
			(char*)shaderc_result_get_bytes(result_P4), &vertexShader2);
		shaderc_result_release(result_P4); // done
		// Create Pixel Shader
		result_P4 = shaderc_compile_into_spv( // compile
			compiler, pixelShaderSource2, strlen(pixelShaderSource2),
			shaderc_fragment_shader, "main.frag", "main", options);
		if (shaderc_result_get_compilation_status(result_P4) != shaderc_compilation_status_success) // errors?
			std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result_P4) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result_P4), // load into Vulkan
			(char*)shaderc_result_get_bytes(result_P4), &pixelShader2);
		shaderc_result_release(result_P4); // done


		// Free runtime shader compiler resources
		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);

		/***************** PIPELINE INTIALIZATION ******************/
		// Create Pipeline & Layout (Thanks Tiny!)
		VkRenderPass renderPass;
		vlk.GetRenderPass((void**)&renderPass);
		VkPipelineShaderStageCreateInfo stage_create_info[2] = {};
		// Create Stage Info for Vertex Shader
		stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		//stage_create_info[0].module = vertexShader; // TODO: Part 4f
		stage_create_info[0].module = vertexShader2; // TODO: Part 4f
		stage_create_info[0].pName = "main";
		// Create Stage Info for Fragment Shader
		stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		//stage_create_info[1].module = pixelShader; // TODO: Part 4f // TODO: Part 4g
		stage_create_info[1].module = pixelShader; // TODO: Part 4f // TODO: Part 4g
		stage_create_info[1].pName = "main";
		// Assembly State
		VkPipelineInputAssemblyStateCreateInfo assembly_create_info = {};
		assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP; // TODO: Part 1b
		assembly_create_info.primitiveRestartEnable = false;
		// Vertex Input State
		VkVertexInputBindingDescription vertex_binding_description = {};
		vertex_binding_description.binding = 0;
		//vertex_binding_description.stride = sizeof(float) * 2; // TODO: Part 4f
		vertex_binding_description.stride = sizeof(VERTEX); // TODO: Part 4f
		// TODO: Part 4f
		vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		VkVertexInputAttributeDescription vertex_attribute_description[2] = {
			{ 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 }, //uv, normal, etc....
			{ 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VERTEX, color)}
		};
		VkPipelineVertexInputStateCreateInfo input_vertex_info = {};
		input_vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		//input_vertex_info.vertexBindingDescriptionCount = 1; // TODO: Part 4f
		input_vertex_info.vertexBindingDescriptionCount = 1; // TODO: Part 4f
		input_vertex_info.pVertexBindingDescriptions = &vertex_binding_description;
		input_vertex_info.vertexAttributeDescriptionCount = 2;
		input_vertex_info.pVertexAttributeDescriptions = vertex_attribute_description;
		// Viewport State (we still need to set this up even though we will overwrite the values)
		VkViewport viewport = {
			0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1
		};
		VkRect2D scissor = { {0, 0}, {width, height} };
		VkPipelineViewportStateCreateInfo viewport_create_info = {};
		viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_create_info.viewportCount = 1;
		viewport_create_info.pViewports = &viewport;
		viewport_create_info.scissorCount = 1;
		viewport_create_info.pScissors = &scissor;
		// Rasterizer State
		VkPipelineRasterizationStateCreateInfo rasterization_create_info = {};
		rasterization_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;
		rasterization_create_info.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization_create_info.lineWidth = 1.0f;
		rasterization_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterization_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterization_create_info.depthClampEnable = VK_FALSE;
		rasterization_create_info.depthBiasEnable = VK_FALSE;
		rasterization_create_info.depthBiasClamp = 0.0f;
		rasterization_create_info.depthBiasConstantFactor = 0.0f;
		rasterization_create_info.depthBiasSlopeFactor = 0.0f;
		// Multisampling State
		VkPipelineMultisampleStateCreateInfo multisample_create_info = {};
		multisample_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample_create_info.sampleShadingEnable = VK_FALSE;
		multisample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisample_create_info.minSampleShading = 1.0f;
		multisample_create_info.pSampleMask = VK_NULL_HANDLE;
		multisample_create_info.alphaToCoverageEnable = VK_FALSE;
		multisample_create_info.alphaToOneEnable = VK_FALSE;
		// Depth-Stencil State
		VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {};
		depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_create_info.depthTestEnable = VK_FALSE; // TODO: Part 4g
		depth_stencil_create_info.depthWriteEnable = VK_TRUE;
		depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
		depth_stencil_create_info.depthBoundsTestEnable = VK_FALSE;
		depth_stencil_create_info.minDepthBounds = 0.0f;
		depth_stencil_create_info.maxDepthBounds = 1.0f;
		depth_stencil_create_info.stencilTestEnable = VK_FALSE;
		// Color Blending Attachment & State
		VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
		color_blend_attachment_state.colorWriteMask = 0xF;
		color_blend_attachment_state.blendEnable = VK_FALSE;
		color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
		color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
		color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
		color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
		VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
		color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_create_info.logicOpEnable = VK_FALSE;
		color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
		color_blend_create_info.attachmentCount = 1;
		color_blend_create_info.pAttachments = &color_blend_attachment_state;
		color_blend_create_info.blendConstants[0] = 0.0f;
		color_blend_create_info.blendConstants[1] = 0.0f;
		color_blend_create_info.blendConstants[2] = 0.0f;
		color_blend_create_info.blendConstants[3] = 0.0f;
		// Dynamic State 
		VkDynamicState dynamic_state[2] = {
			// By setting these we do not need to re-create the pipeline on Resize
			VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamic_create_info = {};
		dynamic_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_create_info.dynamicStateCount = 2;
		dynamic_create_info.pDynamicStates = dynamic_state;
		// TODO: Part 2c
		VkPushConstantRange push_constant_create_info = {};
		push_constant_create_info.stageFlags = 1;
		push_constant_create_info.offset = 0;
		push_constant_create_info.size = sizeof(SHADER_VARS);

		// Descriptor pipeline layout
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount = 0;
		pipeline_layout_create_info.pSetLayouts = VK_NULL_HANDLE;
		pipeline_layout_create_info.pushConstantRangeCount = 1; // TODO: Part 2d 
		pipeline_layout_create_info.pPushConstantRanges = &push_constant_create_info; // TODO: Part 2d
		vkCreatePipelineLayout(device, &pipeline_layout_create_info,
			nullptr, &pipelineLayout);
		// Pipeline State... (FINALLY) 
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info.stageCount = 2;
		pipeline_create_info.pStages = stage_create_info;
		pipeline_create_info.pInputAssemblyState = &assembly_create_info;
		pipeline_create_info.pVertexInputState = &input_vertex_info;
		pipeline_create_info.pViewportState = &viewport_create_info;
		pipeline_create_info.pRasterizationState = &rasterization_create_info;
		pipeline_create_info.pMultisampleState = &multisample_create_info;
		pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
		pipeline_create_info.pColorBlendState = &color_blend_create_info;
		pipeline_create_info.pDynamicState = &dynamic_create_info;
		pipeline_create_info.layout = pipelineLayout;
		pipeline_create_info.renderPass = renderPass;
		pipeline_create_info.subpass = 0;
		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
			&pipeline_create_info, nullptr, &pipeline);
		// TODO: Part 4g
		stage_create_info[1].module = pixelShader2; // TODO: Part 4f // TODO: Part 4g
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
			&pipeline_create_info, nullptr, &pipeline2);

		// TODO: Part 3a
		assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // TODO: Part 1b
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
			&pipeline_create_info, nullptr, &pipeline2);

		/***************** CLEANUP / SHUTDOWN ******************/
		// GVulkanSurface will inform us when to release any allocated resources
		shutdown.Create(vlk, [&]() {
			if (+shutdown.Find(GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES, true)) {
				CleanUp(); // unlike D3D we must be careful about destroy timing
			}
			});
	}
	void Render()
	{
		// TODO: Part 2a
		static auto starT = std::chrono::system_clock::now();
		auto time = std::chrono::system_clock::now();
		double timeDiff = std::chrono::duration<double>(time - starT).count();

		GW::MATH::GMATRIXF ZRotationMatrix = GW::MATH::GIdentityMatrixF;
		matrixProxy.RotateZGlobalF(ZRotationMatrix, (G2D_DEGREE_TO_RADIAN(90) * timeDiff), ZRotationMatrix);

		// TODO: Part 2b
		SHADER_VARS ShaderVars = { GW::MATH::GIdentityMatrixF, ZRotationMatrix };


		// grab the current Vulkan commandBuffer
		unsigned int currentBuffer;
		vlk.GetSwapchainCurrentImage(currentBuffer);
		VkCommandBuffer commandBuffer;
		vlk.GetCommandBuffer(currentBuffer, (void**)&commandBuffer);
		// what is the current client area dimensions?
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);
		// setup the pipeline's dynamic settings
		VkViewport viewport = {
			0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1
		};
		VkRect2D scissor = { {0, 0}, {width, height} };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		// TODO: Part 2d
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SHADER_VARS), &ShaderVars);

		// now we can draw
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexHandle, offsets);
		// TODO: Part 3d
		vkCmdBindIndexBuffer(commandBuffer, indexHandle2, 0, VK_INDEX_TYPE_UINT32);
		// TODO: Part 3b
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline2);

		// TODO: Part 3d
		vkCmdDrawIndexed(commandBuffer, 30, 1, 0, 0, 0);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline); 
		vkCmdDraw(commandBuffer, 13, 1, 0, 0);
		

	}
private:
	void CleanUp()
	{
		// wait till everything has completed
		vkDeviceWaitIdle(device);
		// Release allocated buffers, shaders & pipeline
		vkDestroyBuffer(device, vertexHandle, nullptr);
		vkFreeMemory(device, vertexData, nullptr);
		// TODO: Part 3c
		vkFreeMemory(device, indexData2, nullptr);
		vkDestroyBuffer(device, indexHandle2, nullptr);
		vkDestroyShaderModule(device, vertexShader, nullptr);
		vkDestroyShaderModule(device, pixelShader, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);
		// TODO: Part 3a
		vkDestroyPipeline(device, pipeline2, nullptr);
		// TODO: Part 4b
		vkDestroyShaderModule(device, vertexShader2, nullptr);
		vkDestroyShaderModule(device, pixelShader2, nullptr);
	}
};
