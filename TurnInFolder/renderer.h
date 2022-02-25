// minimalistic code to draw a single triangle, this is not part of the API.
// TODO: Part 1b
#include "FSLogo.h"
#include <fstream>
#include <string>
#include <vector>
#include "shaderc/shaderc.h" // needed for compiling shaders at runtime
#ifdef _WIN32 // must use MT platform DLL libraries on windows
#pragma comment(lib, "shaderc_combined.lib") 
#endif
#include "h2bParser.h"
#define MAX_SUBMESH_PER_DRAW 1024
// Simple Vertex Shader
const char* vertexShaderSource = R"(
#pragma pack_matrix(row_major)
#define MAX_SUBMESH_PER_DRAW 1024
struct OBJ_ATTRIBUTES
{
    float3 Kd;
    float d;
    float3 Ks;
    float Ns;
    float3 Ka;
    float shaprness;
    float3 Tf;
    float Ni;
    float3 Ke;
    int illum;
};
struct SHADER_MODEL_DATA
{
    float4 sunDirection;
    float4 sunColor;

    matrix viewMatrix;
    matrix projectionMatrix;

    matrix matricies[MAX_SUBMESH_PER_DRAW];
    OBJ_ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];
	float4 sunAmbient;
	float4 camPos;
};
StructuredBuffer<SHADER_MODEL_DATA> SceneData;
[[vk::push_constant]]
cbuffer MEXH_INDEX
{
	matrix meshes_WorldMatrix;
	uint mesh_ID;
};
struct OBJ_VERT
{
    float3 pos : POSITION; // Left-handed +Z forward coordinate w not provided, assumed to be 1.
	float3 uvw : UVW; // D3D/Vulkan style top left 0,0 coordinate.
	float3 nrm : NORMAL; // Provided direct from obj file, may or may not be normalized.
};
struct OBJ_VERT_OUT
{
    float4 pos : SV_POSITION;
	float4 nrm : NORMAL;
	float4 wPos : POSITION;
};
OBJ_VERT_OUT main(OBJ_VERT inputVertex)
{
    OBJ_VERT_OUT result;
    result.pos = float4(inputVertex.pos, 1);
	result.nrm = float4(inputVertex.nrm, 0);

	result.pos = mul(result.pos, meshes_WorldMatrix);
	result.wPos = result.pos;
    result.pos = mul(result.pos, SceneData[0].viewMatrix);
    result.pos = mul(result.pos, SceneData[0].projectionMatrix);
	result.nrm = mul(result.nrm, SceneData[0].matricies[mesh_ID]);
		

    return result;
}
)";
// Simple Pixel Shader
const char* pixelShaderSource = R"(
#define MAX_SUBMESH_PER_DRAW 1024
struct OBJ_ATTRIBUTES
{
    float3 Kd;
    float d;
    float3 Ks;
    float Ns;
    float3 Ka;
    float shaprness;
    float3 Tf;
    float Ni;
    float3 Ke;
    int illum;
};
struct SHADER_MODEL_DATA
{
    float4 sunDirection;
    float4 sunColor;

    matrix viewMatrix;
    matrix projectionMatrix;

    matrix matricies[MAX_SUBMESH_PER_DRAW];
    OBJ_ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];
	
	// TODO: Part 4g
    float4 sunAmbient;
    float4 camPos;
};
StructuredBuffer<SHADER_MODEL_DATA> SceneData;
[[vk::push_constant]]
cbuffer MEXH_INDEX
{
	matrix meshes_WorldMatrix;
    uint mesh_ID;
};
struct OBJ_VERT_OUT
{
    float4 pos : SV_POSITION;
    float4 nrm : NORMAL;
	float4 wPos : POSITION;
};
float4 main(OBJ_VERT_OUT inputVertex) : SV_TARGET
{
	
    float3 lightDirection = normalize(SceneData[0].sunDirection.xyz);
    float4 lightColor = SceneData[0].sunColor;
    float3 surfacePos = inputVertex.pos.xyz;
    float3 surfaceNormal = normalize(inputVertex.nrm.xyz);
    float4 surfaceColor = float4(SceneData[0].materials[mesh_ID].Kd, 1);
    
   
    float4 finalColor;
    float lightRatio = clamp(dot( -lightDirection, surfaceNormal), 0, 1);
    float4 Ambient =  SceneData[0].sunAmbient * surfaceColor;
    
	finalColor = lightRatio * lightColor * surfaceColor;
	finalColor += Ambient; 

    float3 reflect = reflect(lightDirection, surfaceNormal);
    float3 toCam = normalize(SceneData[0].camPos.xyz - inputVertex.wPos.xyz);
    float specDot = dot(reflect, toCam);
    specDot = pow(specDot, SceneData[0].materials[mesh_ID].Ns * 0.5f);
    float4 specFinalColor = float4(1.0f, 1.0f, 1.0f, 0) * lightColor * saturate(specDot);
    
	finalColor += specFinalColor;
	return finalColor;

    
}
)";


class LevelRenderer
{
public:

	//file io 
	std::fstream myFile;

	//Necessary stuff to get things drawn
	VkBuffer indexHandle = nullptr;
	VkDeviceMemory indexData = nullptr;
	VkDevice device = nullptr;
	VkBuffer vertexHandle = nullptr;
	VkDeviceMemory vertexData = nullptr;

	//Matrices
	GW::MATH::GMATRIXF _WorldMatrixForMeshes;

	//H2BPARSER DATA MEMBERS
	H2B::VERTEX* logoVerts;
	int* logoIndices;
	unsigned vertexCount;
	unsigned indexCount;
	unsigned materialCount;
	unsigned meshCount;
	std::vector<H2B::VERTEX> vertices;
	std::vector<unsigned> indices;
	std::vector<H2B::MATERIAL> materials;
	std::vector<H2B::BATCH> batches;
	std::vector<H2B::MESH> meshes;

	LevelRenderer(std::string objName)
	{
		//Looking for the clone number on each mesh ex. CoffeTable , CoffeTable".001" and gets rid of anything after period
		std::string h2bmeshes;
		int cloneID = objName.find_first_of('.');
		h2bmeshes = objName.substr(0, cloneID);
		//Adding the name convection to the model names
		std::string ModelAddress = "../Models/";
		std::string meshNameforH2B = ModelAddress.append(h2bmeshes);
		meshNameforH2B = meshNameforH2B.append(".h2b");
		//Passing in the data to the h2b parser
		H2B::Parser thingsWeArePassingIn;
		const char* parsedMeshNames = meshNameforH2B.c_str();
		thingsWeArePassingIn.Parse(parsedMeshNames);

		//Assining the parsed data to our data members
		vertexCount = thingsWeArePassingIn.vertexCount;
		indexCount = thingsWeArePassingIn.indexCount;
		materialCount = thingsWeArePassingIn.materialCount;
		meshCount = thingsWeArePassingIn.meshCount;
		vertices = thingsWeArePassingIn.vertices;
		indices = thingsWeArePassingIn.indices;
		materials = thingsWeArePassingIn.materials;
		batches = thingsWeArePassingIn.batches;
		meshes = thingsWeArePassingIn.meshes;

		//Populating the materials attrib with the values with parsed in
		for (size_t i = 0; i < thingsWeArePassingIn.materialCount; i++)
		{
			materials[i].attrib = thingsWeArePassingIn.materials[i].attrib;
		}
		//Opens the text file with my blenders level matrices
			std::string line;
		myFile.open("../Blender Protype/GameLevel.txt", std::ios::in);
		if (myFile.is_open())
		{
			int counter = 0;
			//Goes through the entire file and reads it
			while (std::getline(myFile, line))
			{
				//looks for the word MESH
				if (line == "MESH")
				{
					//reads everything after the first instance of the word MESH
					while (std::getline(myFile, line))
					{
						//looks for the model after
						if (line == objName)
						{
							//reads everything below that
							while (std::getline(myFile, line) && counter != 4)
							{
								counter++;
								GW::MATH::GMATRIXF modelMatrix;
								if (counter == 1)
								{
									std::vector<std::string> coordinates;
										std::string newline = line.substr(13);
										std::stringstream stream(newline);
										while (stream.good())
										{
											std::string numbers;
											std::getline(stream, numbers, ',');
											coordinates.push_back(numbers);
										}
										modelMatrix.row1.x = std::stof(coordinates[0]);
										modelMatrix.row1.y = std::stof(coordinates[1]);
										modelMatrix.row1.z = std::stof(coordinates[2]);
										modelMatrix.row1.w = std::stof(coordinates[3]);
								}
								if (counter == 2)
								{
									std::vector<std::string> coordinates;
									std::string newline = line.substr(13);
									std::stringstream stream(newline);
									while (stream.good())
									{
										std::string numbers;
										std::getline(stream, numbers, ',');
										coordinates.push_back(numbers);
									}
									modelMatrix.row2.x = std::stof(coordinates[0]);
									modelMatrix.row2.y = std::stof(coordinates[1]);
									modelMatrix.row2.z = std::stof(coordinates[2]);
									modelMatrix.row2.w = std::stof(coordinates[3]);
								}
								if (counter == 3)
								{
									std::vector<std::string> coordinates;
									std::string newline = line.substr(13);
									std::stringstream stream(newline);
									while (stream.good())
									{
										std::string numbers;
										std::getline(stream, numbers, ',');
										coordinates.push_back(numbers);
									}
									modelMatrix.row3.x = std::stof(coordinates[0]);
									modelMatrix.row3.y = std::stof(coordinates[1]);
									modelMatrix.row3.z = std::stof(coordinates[2]);
									modelMatrix.row3.w = std::stof(coordinates[3]);
								}
								if (counter == 4)
								{
									std::vector<std::string> coordinates;
									std::string newline = line.substr(13);
									std::stringstream stream(newline);
									while (stream.good())
									{
										std::string numbers;
										std::getline(stream, numbers, ',');
										coordinates.push_back(numbers);
									}
									modelMatrix.row4.x = std::stof(coordinates[0]);
									modelMatrix.row4.y = std::stof(coordinates[1]);
									modelMatrix.row4.z = std::stof(coordinates[2]);
									modelMatrix.row4.w = std::stof(coordinates[3]);
									_WorldMatrixForMeshes = modelMatrix;
									counter = 0;
									break;
								}

								 
								// for some reason switch statements was breaking my code with the counter reset at the bottom;

								//switch (counter)
								//{
								//case 1:
								//{
								//	std::vector<std::string> coordinates;
								//	std::string newline = line.substr(13);
								//	std::stringstream stream(newline);
								//	while (stream.good())
								//	{
								//		std::string numbers;
								//		std::getline(stream, numbers, ',');
								//		coordinates.push_back(numbers);
								//	}
								//	modelMatrix.row1.x = std::stof(coordinates[0]);
								//	modelMatrix.row1.y = std::stof(coordinates[1]);
								//	modelMatrix.row1.z = std::stof(coordinates[2]);
								//	modelMatrix.row1.w = std::stof(coordinates[3]);
								//	break;
								//}
								//case 2:
								//{
								//	std::vector<std::string> coordinates;
								//	std::string newline = line.substr(13);
								//	std::stringstream stream(newline);
								//	while (stream.good())
								//	{
								//		std::string numbers;
								//		std::getline(stream, numbers, ',');
								//		coordinates.push_back(numbers);
								//	}
								//	modelMatrix.row2.x = std::stof(coordinates[0]);
								//	modelMatrix.row2.y = std::stof(coordinates[1]);
								//	modelMatrix.row2.z = std::stof(coordinates[2]);
								//	modelMatrix.row2.w = std::stof(coordinates[3]);
								//	break;
								//}
								//case 3:
								//{
								//	std::vector<std::string> coordinates;
								//	std::string newline = line.substr(13);
								//	std::stringstream stream(newline);
								//	while (stream.good())
								//	{
								//		std::string numbers;
								//		std::getline(stream, numbers, ',');
								//		coordinates.push_back(numbers);
								//	}
								//	modelMatrix.row3.x = std::stof(coordinates[0]);
								//	modelMatrix.row3.y = std::stof(coordinates[1]);
								//	modelMatrix.row3.z = std::stof(coordinates[2]);
								//	modelMatrix.row3.w = std::stof(coordinates[3]);
								//	break;
								//}
								//case 4:
								//{
								//	std::vector<std::string> coordinates;
								//	std::string newline = line.substr(13);
								//	std::stringstream stream(newline);
								//	while (stream.good())
								//	{
								//		std::string numbers;
								//		std::getline(stream, numbers, ',');
								//		coordinates.push_back(numbers);
								//	}
								//	modelMatrix.row4.x = std::stof(coordinates[0]);
								//	modelMatrix.row4.y = std::stof(coordinates[1]);
								//	modelMatrix.row4.z = std::stof(coordinates[2]);
								//	modelMatrix.row4.w = std::stof(coordinates[3]);
								//	_WorldMatrixForMeshes = modelMatrix;
								//	//counter = 0;
								//	break;
								//}

								//}

							}
						}
						else { break; }
					}

				}
			}
			myFile.close();
		}

		//Populating logoVerts
		logoVerts = new H2B::VERTEX[vertexCount];
		for (size_t i = 0; i < vertexCount; i++)
		{
			logoVerts[i] = vertices[i];
		}
		//Populating logoIndices
		logoIndices = new int[indexCount];
		for (size_t i = 0; i < indexCount; i++)
		{
			logoIndices[i] = indices[i];
		}

	}
};


// Creation, Rendering & Cleanup
class Renderer
{
	//Controller inputs
	GW::INPUT::GInput inputProxy;
	GW::INPUT::GController controllerProxy;
	//Instance of my LevelRendere Class
	std::vector<LevelRenderer*>OBJMESHES;

	struct SHADER_MODEL_DATA
	{
		GW::MATH::GVECTORF sunDirection, sunColor; //lighting info
		GW::MATH::GMATRIXF viewMatrix, projectionMartix; //viewing info
		//Pre sub-mesh transform and material data
		GW::MATH::GMATRIXF matrices[MAX_SUBMESH_PER_DRAW];
		H2B::ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];

		GW::MATH::GVECTORF sunAmbient, camPos;
	};


	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GVulkanSurface vlk;
	GW::CORE::GEventReceiver shutdown;

	// what we need at a minimum to draw a triangle
	VkDevice device = nullptr;
	VkBuffer vertexHandle = nullptr;
	VkDeviceMemory vertexData = nullptr;
	VkBuffer indexHandle = nullptr;
	VkDeviceMemory indexData = nullptr;

	std::vector<VkBuffer> storageHandle;
	std::vector<VkDeviceMemory> storageData;//loop through size in cleanup

	VkShaderModule vertexShader = nullptr;
	VkShaderModule pixelShader = nullptr;

	// pipeline settings for drawing (also required)
	VkPipeline pipeline = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	VkDescriptorSetLayout descriptorLayout;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSet;

	GW::MATH::GMatrix MatrixProxy;
	GW::MATH::GMATRIXF WorldMatrix = GW::MATH::GIdentityMatrixF;
	GW::MATH::GMATRIXF ViewMatrix = GW::MATH::GIdentityMatrixF;
	GW::MATH::GMATRIXF ProjectionMatrix = GW::MATH::GIdentityMatrixF;
	GW::MATH::GVECTORF LightDirection = { -1.0f, -1.0f, 2.0f, 0.0f };
	GW::MATH::GVECTORF LightColor = { 0.9f, 0.9, 1.0f, 1.0f };

	//ALL CONTAINERS USED FOR THE LEVELRENDERER
	SHADER_MODEL_DATA ModelData;
	std::vector<GW::MATH::GMATRIXF> matricesContainer;
	std::vector<unsigned int> mat_ID; //stores the materials id
	std::vector<H2B::ATTRIBUTES> mats; // stores the materials information or attributes 


	std::chrono::steady_clock::time_point lastUpdate;
	float deltaTime;


public:
	struct MESH_INDEX
	{
		GW::MATH::GMATRIXF meshes_WorldMatrix;
		unsigned int mesh_ID;
	};


	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GVulkanSurface _vlk)
	{
		win = _win;
		vlk = _vlk;
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);

		inputProxy.Create(win);
		controllerProxy.Create();

		MatrixProxy.Create();

		GW::MATH::GVECTORF eye = { 0.75f, 0.25f, -1.5f, 0.0f };
		GW::MATH::GVECTORF at = { 0.15f, 0.75f, 0.0f, 0.0f };
		GW::MATH::GVECTORF up = { 0.0f, 1.0f, 0.0f, 0.0f };
		MatrixProxy.LookAtLHF(eye, at, up, ViewMatrix);
		ModelData.viewMatrix = ViewMatrix;
		//Set View Matrix to Cameras true World Matrix
		MatrixProxy.InverseF(ViewMatrix, ViewMatrix);


		float aspectRatio = 0.0f;
		vlk.GetAspectRatio(aspectRatio);
		MatrixProxy.ProjectionVulkanLHF(G2D_DEGREE_TO_RADIAN(65.0f), aspectRatio, 0.1f, 100.0f, ProjectionMatrix);
		ModelData.projectionMartix = ProjectionMatrix;

		float lightVectLength = std::sqrtf(
			(LightDirection.x * LightDirection.x) +
			(LightDirection.y * LightDirection.y) +
			(LightDirection.z * LightDirection.z));
		LightDirection.x /= lightVectLength;
		LightDirection.y /= lightVectLength;
		LightDirection.z /= lightVectLength;
		ModelData.sunDirection = LightDirection;
		ModelData.sunColor = LightColor;

		ModelData.camPos = ViewMatrix.row4;
		ModelData.sunAmbient = { 0.25f, 0.25f, 0.35f, 0.0f };

		std::string address = "../Blender Protype/GameLevel.txt";
		std::string line;
		std::fstream myFile(address);
		if (myFile.is_open())
		{
			int counter = 0;
			//Goes through the entire file and reads it
			while (std::getline(myFile, line))
			{
				//looks for the word MESH
				if (line == "MESH")
				{
					while (std::getline(myFile, line))
					{
						counter++;
						GW::MATH::GMATRIXF objMatr;

						if (counter == 1)
						{
							std::string mesh;
							mesh += line;
							LevelRenderer* instance = new LevelRenderer(mesh);
							OBJMESHES.push_back(instance);
							counter = 0;
							break;
						}
					}

				}


				/*if (line == "CAMERA")
				{
					while (std::getline(myFile, line))
					{

						GW::MATH::GMATRIXF camMatr;

						switch (counter)
						{
						case 1:
						{
							std::vector<std::string> coordinates;
							std::string newline = line.substr(13);
							std::stringstream stream(newline);
							while (stream.good())
							{
								std::string numbers;
								std::getline(stream, numbers, ',');
								coordinates.push_back(numbers);
							}
							camMatr.row1.x = std::stof(coordinates[0]);
							camMatr.row1.y = std::stof(coordinates[1]);
							camMatr.row1.z = std::stof(coordinates[2]);
							camMatr.row1.w = std::stof(coordinates[3]);
							break;
						}
						case 2:
						{
							std::vector<std::string> coordinates;
							std::string newline = line.substr(13);
							std::stringstream stream(newline);
							while (stream.good())
							{
								std::string numbers;
								std::getline(stream, numbers, ',');
								coordinates.push_back(numbers);
							}
							camMatr.row2.x = std::stof(coordinates[0]);
							camMatr.row2.y = std::stof(coordinates[1]);
							camMatr.row2.z = std::stof(coordinates[2]);
							camMatr.row2.w = std::stof(coordinates[3]);
							break;
						}
						case 3:
						{
							std::vector<std::string> coordinates;
							std::string newline = line.substr(13);
							std::stringstream stream(newline);
							while (stream.good())
							{
								std::string numbers;
								std::getline(stream, numbers, ',');
								coordinates.push_back(numbers);
							}
							camMatr.row3.x = std::stof(coordinates[0]);
							camMatr.row3.y = std::stof(coordinates[1]);
							camMatr.row3.z = std::stof(coordinates[2]);
							camMatr.row3.w = std::stof(coordinates[3]);
							break;
						}
						case 4:
						{
							std::vector<std::string> coordinates;
							std::string newline = line.substr(13);
							std::stringstream stream(newline);
							while (stream.good())
							{
								std::string numbers;
								std::getline(stream, numbers, ',');
								coordinates.push_back(numbers);
							}
							camMatr.row4.x = std::stof(coordinates[0]);
							camMatr.row4.y = std::stof(coordinates[1]);
							camMatr.row4.z = std::stof(coordinates[2]);
							camMatr.row4.w = std::stof(coordinates[3]);
							ViewMatrix = camMatr;
							break;
						}
						}
						counter++;
					}
				}*/

			}
			myFile.close();
		}

		int meshcount = 0;
		for (size_t i = 0; i < OBJMESHES.size(); i++)
		{
			for (size_t j = 0; j < OBJMESHES[i]->meshCount; j++)
			{
				ModelData.matrices[meshcount] = OBJMESHES[i]->_WorldMatrixForMeshes;
				meshcount++;
			}
		}

		// sets materials that get sent to the buffer
		for (size_t i = 0; i < OBJMESHES.size(); i++)
		{
			for (size_t j = 0; j < OBJMESHES[i]->meshCount; j++)
			{
				mats.push_back(OBJMESHES[i]->materials[j].attrib);
			}
		}

		for (size_t i = 0; i < mats.size(); i++)
		{
			ModelData.materials[i] = mats[i];
		}

		/***************** GEOMETRY INTIALIZATION ******************/
		// Grab the device & physical device so we can allocate some stuff
		VkPhysicalDevice physicalDevice = nullptr;
		vlk.GetDevice((void**)&device);
		vlk.GetPhysicalDevice((void**)&physicalDevice);

		for (size_t i = 0; i < OBJMESHES.size(); i++)
		{
			GvkHelper::create_buffer(physicalDevice, device, sizeof(H2B::VERTEX) * OBJMESHES[i]->vertexCount,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &OBJMESHES[i]->vertexHandle, &OBJMESHES[i]->vertexData);
			GvkHelper::write_to_buffer(device, OBJMESHES[i]->vertexData, OBJMESHES[i]->logoVerts, sizeof(H2B::VERTEX) * OBJMESHES[i]->vertexCount);
		
			GvkHelper::create_buffer(physicalDevice, device, sizeof(int) * OBJMESHES[i]->indexCount,
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &OBJMESHES[i]->indexHandle, &OBJMESHES[i]->indexData);
			GvkHelper::write_to_buffer(device, OBJMESHES[i]->indexData, OBJMESHES[i]->logoIndices, sizeof(int) * OBJMESHES[i]->indexCount);
		}


		unsigned int maxframes;
		vlk.GetSwapchainImageCount(maxframes);
		storageData.resize(maxframes);
		storageHandle.resize(maxframes);
		descriptorSet.resize(maxframes);
		for (size_t i = 0; i < maxframes; i++)
		{
			GvkHelper::create_buffer(physicalDevice, device, sizeof(ModelData),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &storageHandle[i], &storageData[i]);
			GvkHelper::write_to_buffer(device, storageData[i], &ModelData, sizeof(ModelData));
		}


		/***************** SHADER INTIALIZATION ******************/
		// Intialize runtime shader compiler HLSL -> SPIRV
		shaderc_compiler_t compiler = shaderc_compiler_initialize();
		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
		shaderc_compile_options_set_invert_y(options, false);
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
		stage_create_info[0].module = vertexShader;
		stage_create_info[0].pName = "main";
		// Create Stage Info for Fragment Shader
		stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage_create_info[1].module = pixelShader;
		stage_create_info[1].pName = "main";
		// Assembly State
		VkPipelineInputAssemblyStateCreateInfo assembly_create_info = {};
		assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		assembly_create_info.primitiveRestartEnable = false;

		// Vertex Input State
		VkVertexInputBindingDescription vertex_binding_description = {};
		vertex_binding_description.binding = 0;
		vertex_binding_description.stride = sizeof(H2B::VERTEX);
		vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		VkVertexInputAttributeDescription vertex_attribute_description[3] = {
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }, //uv, normal, etc....
			{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(H2B::VERTEX, uvw)},
			{ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(H2B::VERTEX, nrm)}
		};
		VkPipelineVertexInputStateCreateInfo input_vertex_info = {};
		input_vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		input_vertex_info.vertexBindingDescriptionCount = 1;
		input_vertex_info.pVertexBindingDescriptions = &vertex_binding_description;
		input_vertex_info.vertexAttributeDescriptionCount = 3;
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
		depth_stencil_create_info.depthTestEnable = VK_TRUE;
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

		VkDescriptorSetLayoutBinding descriptor_layout_binding = {};
		descriptor_layout_binding.binding = 0;
		descriptor_layout_binding.descriptorCount = 1;
		descriptor_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		descriptor_layout_binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info = {};
		descriptor_layout_create_info.bindingCount = 1;
		descriptor_layout_create_info.flags = 0;
		descriptor_layout_create_info.pBindings = &descriptor_layout_binding;
		descriptor_layout_create_info.pNext = nullptr;
		descriptor_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		VkResult r = vkCreateDescriptorSetLayout(device, &descriptor_layout_create_info,
			nullptr, &descriptorLayout);

		VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
		VkDescriptorPoolSize descriptor_pool_size = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxframes }; // come back to this later
		descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptor_pool_create_info.poolSizeCount = 1;// sizeof(storageData); //1;
		descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		descriptor_pool_create_info.pPoolSizes = &descriptor_pool_size;
		descriptor_pool_create_info.maxSets = maxframes;
		descriptor_pool_create_info.pNext = nullptr;
		vkCreateDescriptorPool(device, &descriptor_pool_create_info, nullptr, &descriptorPool);

		VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
		descriptor_set_allocate_info.descriptorPool = descriptorPool;
		descriptor_set_allocate_info.descriptorSetCount = 1;
		descriptor_set_allocate_info.pNext = nullptr;
		descriptor_set_allocate_info.pSetLayouts = &descriptorLayout;
		descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		for (int i = 0; i < maxframes; ++i) {
			vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &descriptorSet[i]);
		}

		for (int i = 0; i < maxframes; ++i) {

			VkDescriptorBufferInfo dbinfo = { storageHandle[i], 0, VK_WHOLE_SIZE };
			VkWriteDescriptorSet write_descriptor_set = {};
			write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write_descriptor_set.descriptorCount = 1;
			write_descriptor_set.dstArrayElement = 0;
			write_descriptor_set.dstBinding = 0;
			write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write_descriptor_set.dstSet = descriptorSet[i];
			write_descriptor_set.pBufferInfo = &dbinfo;
			vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, nullptr);
		}

		VkDescriptorBufferInfo descriptor_buffer_info = {};


		// Descriptor pipeline layout
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		pipeline_layout_create_info.pSetLayouts = &descriptorLayout;
		pipeline_layout_create_info.setLayoutCount = 1;

		VkPushConstantRange push_constant_range = {};
		push_constant_range.offset = 0;
		push_constant_range.size = sizeof(MESH_INDEX);
		push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		pipeline_layout_create_info.pushConstantRangeCount = 1; // 0 to 1
		pipeline_layout_create_info.pPushConstantRanges = &push_constant_range; // was a nullptr
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

		auto now = std::chrono::steady_clock::now();
		deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(now - lastUpdate).count() / 1000000.0f;
		lastUpdate = now;

		//updatea camerea information
		//Inverse View Matrix For Rendering Objects
		MatrixProxy.InverseF(ViewMatrix, ModelData.viewMatrix);
		ModelData.camPos = ViewMatrix.row4;


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
		VkDeviceSize offsets[] = { 0 };

		MESH_INDEX daIndex;
		unsigned int meshID = 0;

		for (size_t i = 0; i < OBJMESHES.size(); i++)
		{
			GvkHelper::write_to_buffer(device, storageData[currentBuffer], &ModelData, sizeof(SHADER_MODEL_DATA));
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout, 0, 1, &descriptorSet[currentBuffer], 0, nullptr);

			for (size_t j = 0; j < OBJMESHES[i]->meshCount; j++)
			{
				daIndex = { OBJMESHES[i]->_WorldMatrixForMeshes, meshID };
				meshID++;

				vkCmdBindVertexBuffers(commandBuffer, 0, 1, &OBJMESHES[i]->vertexHandle, offsets);
				vkCmdBindIndexBuffer(commandBuffer, OBJMESHES[i]->indexHandle, 0, VK_INDEX_TYPE_UINT32);
				vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MESH_INDEX), &daIndex);
				vkCmdDrawIndexed(commandBuffer, OBJMESHES[i]->meshes[j].drawInfo.indexCount, 1, OBJMESHES[i]->meshes[j].drawInfo.indexOffset, 0, 0);
			}
		}



		//// now we can draw
		//VkDeviceSize offsets[] = { 0 };
		//vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexHandle, offsets);

		//vkCmdBindIndexBuffer(commandBuffer, indexHandle, 0, VK_INDEX_TYPE_UINT32);

		//vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		//	pipelineLayout, 0, 1, &descriptorSet[currentBuffer], 0, nullptr);

		//for (size_t i = 0; i < FSLogo_meshcount; i++)
		//{
		//	// TODO: Part 3d
		//	//MeshIndex.mesh_ID = FSLogo_meshes[i].materialIndex;
		//	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(unsigned int), &FSLogo_meshes[i].materialIndex);
		//	vkCmdDrawIndexed(commandBuffer, FSLogo_meshes[i].indexCount, 1, FSLogo_meshes[i].indexOffset, 0, 0);
		//}

	}

	void UpdateCamera()
	{
		using namespace std::chrono;
		static auto starT = std::chrono::system_clock::now();
		auto time = std::chrono::system_clock::now();
		double timeChange = std::chrono::duration<double>(time - starT).count() * 500.0;

		GW::MATH::GMATRIXF tempView = ViewMatrix;

#pragma region MOVE UP AND DOWN

		//MOVE UP AND DOWN
		float YchangeU = 0.0f;
		float YchangeD = 0.0f;
		float YchangeUcontroller = 0.0f;
		float YchangeDcontroller = 0.0f;
		const float Camera_Speed = 0.003f;

		inputProxy.GetState(G_KEY_SPACE, YchangeU);
		inputProxy.GetState(G_KEY_LEFTSHIFT, YchangeD);
		controllerProxy.GetState(0, G_RIGHT_TRIGGER_AXIS, YchangeUcontroller);
		controllerProxy.GetState(0, G_LEFT_TRIGGER_AXIS, YchangeDcontroller);
		float YChange = YchangeU - YchangeD + YchangeUcontroller - YchangeDcontroller;
		GW::MATH::GVECTORF CameraPosY = { 0, (YChange * Camera_Speed) * static_cast<float>(timeChange), 0, 0 };
		MatrixProxy.TranslateGlobalF(tempView, CameraPosY, tempView);
#pragma endregion


#pragma region MOVE(FORWARD-BACKWARD, LEFT-RIGHT)
		// MOVE FORWARD-BACKWARD/LEFT-RIGHT
		float perFrameSpeed = Camera_Speed * static_cast<float>(timeChange);

		float ZchangeF = 0.0f;
		float ZchangeB = 0.0f;
		float ZchangeController = 0.0f;
		inputProxy.GetState(G_KEY_W, ZchangeF);
		inputProxy.GetState(G_KEY_S, ZchangeB);
		controllerProxy.GetState(0, G_LY_AXIS, ZchangeController);
		float total_Z_Change = ZchangeF - ZchangeB + ZchangeController;

		if (ZchangeF != 0.0f)
			int nDebug = 0;

		float XchangeL = 0.0f;
		float XchangeR = 0.0f;
		float XchangeController = 0.0f;
		inputProxy.GetState(G_KEY_A, XchangeL);
		inputProxy.GetState(G_KEY_D, XchangeR);
		controllerProxy.GetState(0, G_LX_AXIS, XchangeController);
		float total_X_Change = XchangeR - XchangeL + XchangeController;

		GW::MATH::GMATRIXF TranslationMatrix = GW::MATH::GIdentityMatrixF;
		GW::MATH::GVECTORF TranslationVec = { (total_X_Change * perFrameSpeed), 0, (total_Z_Change * perFrameSpeed), 0 };
		MatrixProxy.TranslateGlobalF(TranslationMatrix, TranslationVec, TranslationMatrix);
		MatrixProxy.MultiplyMatrixF(TranslationMatrix, tempView, tempView);
#pragma endregion


#pragma region MOUSE LOOK(UP-DOWN, LEFT-RIGHT)
		float MouseDeltaX = 0.0f;
		float MouseDelataY = 0.0f;
		float Look_UD_Controller = 0.0f;
		float Look_RL_Controller = 0.0f;

		unsigned int screenHeight;
		win.GetClientHeight(screenHeight);
		unsigned int screenWidth;
		win.GetClientWidth(screenWidth);
		float aspectRatio = 0.0f;
		vlk.GetAspectRatio(aspectRatio);
		float Thumb_Speed = 0.001;

		controllerProxy.GetState(0, G_RY_AXIS, Look_UD_Controller);
		controllerProxy.GetState(0, G_RX_AXIS, Look_RL_Controller);
		Look_UD_Controller *= 0.03;
		Look_RL_Controller *= 0.03;

		GW::GReturn UpDownresult = inputProxy.GetMouseDelta(MouseDeltaX, MouseDelataY);
		if ((G_PASS(UpDownresult) && UpDownresult != GW::GReturn::REDUNDANT) || Look_UD_Controller != 0 || Look_RL_Controller != 0)
		{
			//LOOK UP AND DOWN
			float Total_Pitch = G_DEGREE_TO_RADIAN(65.0f) * (MouseDelataY) / screenHeight + Look_UD_Controller * (Thumb_Speed);
			GW::MATH::GMATRIXF PitchMatrix = GW::MATH::GIdentityMatrixF;
			MatrixProxy.RotateXLocalF(PitchMatrix, Total_Pitch, PitchMatrix);
			MatrixProxy.MultiplyMatrixF(PitchMatrix, tempView, tempView);
			//LOOK LEFT AND RIGHT
			float Total_Yaw = G_DEGREE_TO_RADIAN(65.0f) * aspectRatio * (MouseDeltaX) / screenWidth + Look_RL_Controller * Thumb_Speed;
			GW::MATH::GMATRIXF YawMatrix = GW::MATH::GIdentityMatrixF;
			MatrixProxy.RotateYLocalF(YawMatrix, Total_Yaw, YawMatrix);
			GW::MATH::GVECTORF saveCamPos = tempView.row4;
			MatrixProxy.MultiplyMatrixF(tempView, YawMatrix, tempView);
			tempView.row4 = saveCamPos;
		}
#pragma endregion
		ViewMatrix = tempView;
		starT = std::chrono::system_clock::now();
	}

private:
	void CleanUp()
	{
		// wait till everything has completed
		vkDeviceWaitIdle(device);
		// Release allocated buffers, shaders & pipeline
		vkDestroyBuffer(device, indexHandle, nullptr);
		vkFreeMemory(device, indexData, nullptr);
		for (int i = 0; i < storageData.size(); ++i) {
			vkDestroyBuffer(device, storageHandle[i], nullptr);
			vkFreeMemory(device, storageData[i], nullptr);
		}
		storageData.clear();
		storageHandle.clear();

		vkDestroyBuffer(device, vertexHandle, nullptr);
		vkFreeMemory(device, vertexData, nullptr);
		vkDestroyShaderModule(device, vertexShader, nullptr);
		vkDestroyShaderModule(device, pixelShader, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorLayout, nullptr);
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);

		for (size_t i = 0; i < OBJMESHES.size(); i++)
		{
			vkDestroyBuffer(device, OBJMESHES[i]->vertexHandle, nullptr);
			vkFreeMemory(device, OBJMESHES[i]->vertexData, nullptr);

			vkDestroyBuffer(device, OBJMESHES[i]->indexHandle, nullptr);
			vkFreeMemory(device, OBJMESHES[i]->indexData, nullptr);
		}

		unsigned uiMeshes = OBJMESHES.size();
		for (unsigned i = 0; i < uiMeshes; ++i)
			delete OBJMESHES[i];
	}
};