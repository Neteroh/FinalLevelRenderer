// minimalistic code to draw a single triangle, this is not part of the API.
#include "shaderc/shaderc.h" // needed for compiling shaders at runtime
#include <vector>
#include <cstdlib>
#include <ctime>
#ifdef _WIN32 // must use MT platform DLL libraries on windows
#pragma comment(lib, "shaderc_combined.lib") 
#endif
// TODO: Part 2b
// Simple Vertex Shader
const char* vertexShaderSource = R"(
// an ultra simple hlsl vertex shader
float4 main(float2 inputVertex : POSITION) : SV_POSITION
{
	return float4(inputVertex, 0, 1);
}
)";
// Simple Pixel Shader
const char* pixelShaderSource = R"(
// an ultra simple hlsl pixel shader
float4 main() : SV_TARGET 
{	
	return float4(1.0f ,1.0f, 1.0f, 0); // TODO: Part 1a 
}
)";

const char* vertexShaderSource2 = R"(
// an ultra simple hlsl vertex shader

struct vecColor
{

	float2  pos : POSITION;

	float4 clr : COLOR;

};
struct vecColor_out
{

	float4  pos : SV_POSITION;

	float4 clr : COLOR;

};
vecColor_out main(vecColor inputVertex : POSITION) : SV_POSITION
{
	vecColor_out output;
	output.pos = float4(inputVertex.pos, 0, 1);
	output.clr = inputVertex.clr;
	/*return vecColor_out(inputVertex, 0, 1);*/
	return output;
}
)";
// Simple Pixel Shader
const char* pixelShaderSource2 = R"(
// an ultra simple hlsl pixel shader
struct vecColor_out
{

	float4  pos : SV_POSITION;

	float4 clr : COLOR;

};
float4 main(vecColor_out output) : SV_TARGET 
{	
	return output.clr; // TODO: Part 1a 
}
)";

struct vecPosition
{
	float x, y, z, w;
};
struct vecColor
{
	float pos[2];
	float color[4];
};

// TODO: Part 4b
// TODO: Part 4c (in new shader)
// TODO: Part 4d (in new shader)
// TODO: Part 4e
// Creation, Rendering & Cleanup
class Renderer
{
	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GVulkanSurface vlk;
	GW::CORE::GEventReceiver shutdown;
	// what we need at a minimum to draw a triangle
	VkDevice device = nullptr;
	VkBuffer vertexHandle = nullptr;
	VkDeviceMemory vertexData = nullptr;

	VkBuffer vertexHandle2 = nullptr;
	VkDeviceMemory vertexData2 = nullptr;
	// TODO: Part 3a
	VkShaderModule vertexShader = nullptr;
	VkShaderModule pixelShader = nullptr;

	VkShaderModule vertexShader2 = nullptr;
	VkShaderModule pixelShader2 = nullptr;
	// TODO: Part 4b
	// pipeline settings for drawing (also required)
	VkPipeline pipeline = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;

	VkPipeline pipeline2 = nullptr;
public:
	// TODO: Part 4a
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GVulkanSurface _vlk)
	{
		win = _win;
		vlk = _vlk;
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);
		/***************** GEOMETRY INTIALIZATION ******************/
		// Grab the device & physical device so we can allocate some stuff
		VkPhysicalDevice physicalDevice = nullptr;
		vlk.GetDevice((void**)&device);
		vlk.GetPhysicalDevice((void**)&physicalDevice);
		// TODO: Part 2b
		// Create Vertex Buffer
		srand(static_cast <unsigned> (time(0)));
		/*std::vector<vec> stars;*/
		vecPosition stars[20000];
		srand(static_cast <unsigned> (time(0)));
		for (size_t i = 0; i < 20000; i++)
		{
			float Rx = -1 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1 - -1)));
			float Ry = -1 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1 - -1)));
			stars[i].x = Rx;
			stars[i].y = Ry;
		}

			float R = rand() / static_cast<float>(RAND_MAX);
			float G = rand() / static_cast<float>(RAND_MAX);
			float B = rand() / static_cast<float>(RAND_MAX);
			float A = rand() / static_cast<float>(RAND_MAX);
		vecColor star2D[22] =
		{
				star2D[0].pos[0] = 0.0f,		star2D[0].pos[1] = 0.9f,  star2D[0].color[0] = rand() / static_cast<float>(RAND_MAX), star2D[0].color[1] = rand() / static_cast<float>(RAND_MAX),star2D[0].color[2] = rand() / static_cast<float>(RAND_MAX),star2D[0].color[3] = rand() / static_cast<float>(RAND_MAX),
				star2D[1].pos[0] = 0.25f,		star2D[1].pos[1] = 0.25f, star2D[1].color[0] = rand() / static_cast<float>(RAND_MAX), star2D[1].color[1] = rand() / static_cast<float>(RAND_MAX),star2D[1].color[2] = rand() / static_cast<float>(RAND_MAX),star2D[1].color[3] = rand() / static_cast<float>(RAND_MAX),
				star2D[2].pos[0] = 0.95f,		star2D[2].pos[1] = 0.125f, star2D[2].color[0] = rand() / static_cast<float>(RAND_MAX), star2D[2].color[1] = rand() / static_cast<float>(RAND_MAX),star2D[2].color[2] = rand() / static_cast<float>(RAND_MAX),star2D[2].color[3] = rand() / static_cast<float>(RAND_MAX),
				star2D[3].pos[0] = 0.35f,		star2D[3].pos[1] = -0.15, star2D[3].color[0] = rand() / static_cast<float>(RAND_MAX), star2D[3].color[1] = rand() / static_cast<float>(RAND_MAX),star2D[3].color[2] = rand() / static_cast<float>(RAND_MAX),star2D[3].color[3] = rand() / static_cast<float>(RAND_MAX),
				star2D[4].pos[0] = 0.5f,		star2D[4].pos[1] = -0.9f, star2D[4].color[0] = rand() / static_cast<float>(RAND_MAX), star2D[4].color[1] = rand() / static_cast<float>(RAND_MAX),star2D[4].color[2] = rand() / static_cast<float>(RAND_MAX),star2D[4].color[3] = rand() / static_cast<float>(RAND_MAX),
				star2D[5].pos[0] = 0.0f,		star2D[5].pos[1] = -0.3f, star2D[5].color[0] = rand() / static_cast<float>(RAND_MAX), star2D[5].color[1] = rand() / static_cast<float>(RAND_MAX),star2D[5].color[2] = rand() / static_cast<float>(RAND_MAX),star2D[5].color[3] = rand() / static_cast<float>(RAND_MAX),
				star2D[6].pos[0] = -0.5f,		star2D[6].pos[1] = -0.9f, star2D[6].color[0] = rand() / static_cast<float>(RAND_MAX), star2D[6].color[1] = rand() / static_cast<float>(RAND_MAX),star2D[6].color[2] = rand() / static_cast<float>(RAND_MAX),star2D[6].color[3] = rand() / static_cast<float>(RAND_MAX),
				star2D[7].pos[0] = -0.35f,		star2D[7].pos[1] = -0.15, star2D[7].color[0] = rand() / static_cast<float>(RAND_MAX), star2D[7].color[1] = rand() / static_cast<float>(RAND_MAX),star2D[7].color[2] = rand() / static_cast<float>(RAND_MAX),star2D[7].color[3] = rand() / static_cast<float>(RAND_MAX),
				star2D[8].pos[0] = -0.95f,		star2D[8].pos[1] = 0.125f, star2D[8].color[0] = rand() / static_cast<float>(RAND_MAX), star2D[8].color[1] = rand() / static_cast<float>(RAND_MAX),star2D[8].color[2] = rand() / static_cast<float>(RAND_MAX),star2D[8].color[3] = rand() / static_cast<float>(RAND_MAX),
				star2D[9].pos[0] = -0.25f,		star2D[9].pos[1] = 0.25f, star2D[9].color[0] = rand() / static_cast<float>(RAND_MAX), star2D[9].color[1] = rand() / static_cast<float>(RAND_MAX),star2D[9].color[2] = rand() / static_cast<float>(RAND_MAX),star2D[9].color[3] = rand() / static_cast<float>(RAND_MAX),
				star2D[10].pos[0] = 0.0f,		star2D[10].pos[1] = 0.9f, star2D[10].color[0] = rand() / static_cast<float>(RAND_MAX), star2D[10].color[1] = rand() / static_cast<float>(RAND_MAX),star2D[10].color[2] = rand() / static_cast<float>(RAND_MAX),star2D[10].color[3] = rand() / static_cast<float>(RAND_MAX)
		};

		// Transfer triangle data to the vertex buffer. (staging would be prefered here)
		GvkHelper::create_buffer(physicalDevice, device, sizeof(star2D),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexHandle, &vertexData);
		GvkHelper::write_to_buffer(device, vertexData, star2D, sizeof(star2D));
		// TODO: Part 3a
		GvkHelper::create_buffer(physicalDevice, device, sizeof(stars),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexHandle2, &vertexData2);
		GvkHelper::write_to_buffer(device, vertexData2, stars, sizeof(stars));
		// TODO: Part 4a

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


		// Create Vertex Shader
		shaderc_compilation_result_t result2 = shaderc_compile_into_spv( // compile
			compiler, vertexShaderSource2, strlen(vertexShaderSource2),
			shaderc_vertex_shader, "main.vert", "main", options);
		if (shaderc_result_get_compilation_status(result2) != shaderc_compilation_status_success) // errors?
			std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result2) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result2), // load into Vulkan
			(char*)shaderc_result_get_bytes(result2), &vertexShader2);
		shaderc_result_release(result2); // done
		// Create Pixel Shader
		result2 = shaderc_compile_into_spv( // compile
			compiler, pixelShaderSource2, strlen(pixelShaderSource2),
			shaderc_fragment_shader, "main.frag", "main", options);
		if (shaderc_result_get_compilation_status(result2) != shaderc_compilation_status_success) // errors?
			std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result2) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result2), // load into Vulkan
			(char*)shaderc_result_get_bytes(result2), &pixelShader2);
		shaderc_result_release(result2); // done

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
		stage_create_info[0].module = vertexShader2;
		stage_create_info[0].pName = "main";
		// Create Stage Info for Fragment Shader
		stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage_create_info[1].module = pixelShader2;
		stage_create_info[1].pName = "main";
		// Assembly State
		VkPipelineInputAssemblyStateCreateInfo assembly_create_info = {};
		assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		assembly_create_info.primitiveRestartEnable = false;
		// Vertex Input State
		VkVertexInputBindingDescription vertex_binding_description = {};
		vertex_binding_description.binding = 0;
		vertex_binding_description.stride = sizeof(vecColor);
		vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		VkVertexInputAttributeDescription vertex_attribute_description[2] = {
			{ 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 },{1,0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vecColor,color)} //uv, normal, etc....
		};
		VkPipelineVertexInputStateCreateInfo input_vertex_info = {};
		input_vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		input_vertex_info.vertexBindingDescriptionCount = 1;
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
		rasterization_create_info.polygonMode = VK_POLYGON_MODE_LINE; // TODO: Part 2a
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
		depth_stencil_create_info.depthTestEnable = VK_FALSE;
		depth_stencil_create_info.depthWriteEnable = VK_FALSE;
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
		// Descriptor pipeline layout
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount = 0;
		pipeline_layout_create_info.pSetLayouts = VK_NULL_HANDLE;
		pipeline_layout_create_info.pushConstantRangeCount = 0;
		pipeline_layout_create_info.pPushConstantRanges = nullptr;
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

		// TODO: Part 3b
		/***************** PIPELINE INTIALIZATION ******************/
		// Create Pipeline & Layout (Thanks Tiny!)
		VkRenderPass renderPass2;
		vlk.GetRenderPass((void**)&renderPass2);
		VkPipelineShaderStageCreateInfo stage_create_info2[2] = {};
		// Create Stage Info for Vertex Shader
		stage_create_info2[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info2[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage_create_info2[0].module = vertexShader;
		stage_create_info2[0].pName = "main";
		// Create Stage Info for Fragment Shader
		stage_create_info2[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info2[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage_create_info2[1].module = pixelShader;
		stage_create_info2[1].pName = "main";
		// Assembly State
		VkPipelineInputAssemblyStateCreateInfo assembly_create_info2 = {};
		assembly_create_info2.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assembly_create_info2.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		assembly_create_info2.primitiveRestartEnable = false;
		// Vertex Input State
		VkVertexInputBindingDescription vertex_binding_description2 = {};
		vertex_binding_description2.binding = 0;
		vertex_binding_description2.stride = sizeof(float) * 2;
		vertex_binding_description2.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		VkVertexInputAttributeDescription vertex_attribute_description2[1] = {
			{ 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 } //uv, normal, etc....
		};
		VkPipelineVertexInputStateCreateInfo input_vertex_info2 = {};
		input_vertex_info2.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		input_vertex_info2.vertexBindingDescriptionCount = 1;
		input_vertex_info2.pVertexBindingDescriptions = &vertex_binding_description2;
		input_vertex_info2.vertexAttributeDescriptionCount = 1;
		input_vertex_info2.pVertexAttributeDescriptions = vertex_attribute_description2;
		// Viewport State (we still need to set this up even though we will overwrite the values)
		VkViewport viewport2 = {
			0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1
		};
		VkRect2D scissor2 = { {0, 0}, {width, height} };
		VkPipelineViewportStateCreateInfo viewport_create_info2 = {};
		viewport_create_info2.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_create_info2.viewportCount = 1;
		viewport_create_info2.pViewports = &viewport2;
		viewport_create_info2.scissorCount = 1;
		viewport_create_info2.pScissors = &scissor2;
		// Rasterizer State
		VkPipelineRasterizationStateCreateInfo rasterization_create_info2 = {};
		rasterization_create_info2.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization_create_info2.rasterizerDiscardEnable = VK_FALSE;
		rasterization_create_info2.polygonMode = VK_POLYGON_MODE_POINT; // TODO: Part 2a
		rasterization_create_info2.lineWidth = 1.0f;
		rasterization_create_info2.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterization_create_info2.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterization_create_info2.depthClampEnable = VK_FALSE;
		rasterization_create_info2.depthBiasEnable = VK_FALSE;
		rasterization_create_info2.depthBiasClamp = 0.0f;
		rasterization_create_info2.depthBiasConstantFactor = 0.0f;
		rasterization_create_info2.depthBiasSlopeFactor = 0.0f;
		// Multisampling State
		VkPipelineMultisampleStateCreateInfo multisample_create_info2 = {};
		multisample_create_info2.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample_create_info2.sampleShadingEnable = VK_FALSE;
		multisample_create_info2.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisample_create_info2.minSampleShading = 1.0f;
		multisample_create_info2.pSampleMask = VK_NULL_HANDLE;
		multisample_create_info2.alphaToCoverageEnable = VK_FALSE;
		multisample_create_info2.alphaToOneEnable = VK_FALSE;
		// Depth-Stencil State
		VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info2 = {};
		depth_stencil_create_info2.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_create_info2.depthTestEnable = VK_FALSE;
		depth_stencil_create_info2.depthWriteEnable = VK_FALSE;
		depth_stencil_create_info2.depthCompareOp = VK_COMPARE_OP_LESS;
		depth_stencil_create_info2.depthBoundsTestEnable = VK_FALSE;
		depth_stencil_create_info2.minDepthBounds = 0.0f;
		depth_stencil_create_info2.maxDepthBounds = 1.0f;
		depth_stencil_create_info2.stencilTestEnable = VK_FALSE;
		// Color Blending Attachment & State
		VkPipelineColorBlendAttachmentState color_blend_attachment_state2 = {};
		color_blend_attachment_state2.colorWriteMask = 0xF;
		color_blend_attachment_state2.blendEnable = VK_FALSE;
		color_blend_attachment_state2.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
		color_blend_attachment_state2.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
		color_blend_attachment_state2.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment_state2.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment_state2.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
		color_blend_attachment_state2.alphaBlendOp = VK_BLEND_OP_ADD;
		VkPipelineColorBlendStateCreateInfo color_blend_create_info2 = {};
		color_blend_create_info2.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_create_info2.logicOpEnable = VK_FALSE;
		color_blend_create_info2.logicOp = VK_LOGIC_OP_COPY;
		color_blend_create_info2.attachmentCount = 1;
		color_blend_create_info2.pAttachments = &color_blend_attachment_state2;
		color_blend_create_info2.blendConstants[0] = 0.0f;
		color_blend_create_info2.blendConstants[1] = 0.0f;
		color_blend_create_info2.blendConstants[2] = 0.0f;
		color_blend_create_info2.blendConstants[3] = 0.0f;
		// Dynamic State 
		VkDynamicState dynamic_state2[2] = {
			// By setting these we do not need to re-create the pipeline on Resize
			VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamic_create_info2 = {};
		dynamic_create_info2.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_create_info2.dynamicStateCount = 2;
		dynamic_create_info2.pDynamicStates = dynamic_state2;
		// Descriptor pipeline layout
		VkPipelineLayoutCreateInfo pipeline_layout_create_info2 = {};
		pipeline_layout_create_info2.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info2.setLayoutCount = 0;
		pipeline_layout_create_info2.pSetLayouts = VK_NULL_HANDLE;
		pipeline_layout_create_info2.pushConstantRangeCount = 0;
		pipeline_layout_create_info2.pPushConstantRanges = nullptr;
		vkCreatePipelineLayout(device, &pipeline_layout_create_info2,
			nullptr, &pipelineLayout);
		// Pipeline State... (FINALLY) 
		VkGraphicsPipelineCreateInfo pipeline_create_info2 = {};
		pipeline_create_info2.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info2.stageCount = 2;
		pipeline_create_info2.pStages = stage_create_info2;
		pipeline_create_info2.pInputAssemblyState = &assembly_create_info2;
		pipeline_create_info2.pVertexInputState = &input_vertex_info2;
		pipeline_create_info2.pViewportState = &viewport_create_info2;
		pipeline_create_info2.pRasterizationState = &rasterization_create_info2;
		pipeline_create_info2.pMultisampleState = &multisample_create_info2;
		pipeline_create_info2.pDepthStencilState = &depth_stencil_create_info2;
		pipeline_create_info2.pColorBlendState = &color_blend_create_info2;
		pipeline_create_info2.pDynamicState = &dynamic_create_info2;
		pipeline_create_info2.layout = pipelineLayout;
		pipeline_create_info2.renderPass = renderPass2;
		pipeline_create_info2.subpass = 0;
		pipeline_create_info2.basePipelineHandle = VK_NULL_HANDLE;
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
			&pipeline_create_info2, nullptr, &pipeline2);
		 
		
		// TODO: Part 4f
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
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		// now we can draw
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexHandle, offsets);
		vkCmdDraw(commandBuffer, 11, 1, 0, 0); // TODO: Part 2b

		// TODO: Part 3b
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline2);
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexHandle2, offsets);
		vkCmdDraw(commandBuffer, 20000, 1, 0, 0);
	}
private:
	void CleanUp()
	{
		// wait till everything has completed
		vkDeviceWaitIdle(device);
		// Release allocated buffers, shaders & pipeline
		vkDestroyBuffer(device, vertexHandle, nullptr);
		vkFreeMemory(device, vertexData, nullptr);

		vkDestroyBuffer(device, vertexHandle2, nullptr);
		vkFreeMemory(device, vertexData2, nullptr);
		// TODO: Part 3a
		vkDestroyShaderModule(device, vertexShader, nullptr);
		vkDestroyShaderModule(device, pixelShader, nullptr);

		vkDestroyShaderModule(device, vertexShader2, nullptr);
		vkDestroyShaderModule(device, pixelShader2, nullptr);
		// TODO: Part 4b
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipeline(device, pipeline2, nullptr);
	}
};
