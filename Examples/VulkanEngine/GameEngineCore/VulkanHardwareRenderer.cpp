#include "HardwareRenderer.h"

#include "VulkanAPI/vkel.h"
//#define VK_CPP_NO_EXCEPTIONS
#include "VulkanAPI/vk_cpp.hpp"

#include "CoreLib/WinForm/Debug.h"
#include "CoreLib/VectorMath.h"
#include "CoreLib/PerformanceCounter.h"

// Only execute actions of DEBUG_ONLY in DEBUG mode
#if _DEBUG
#define DEBUG_ONLY(x) do { x; } while(0)
//#define USE_VALIDATION_LAYER 1
#else
#define DEBUG_ONLY(x) do {    } while(0)
#endif


using namespace GameEngine;

namespace VK
{
	const int TargetVulkanVersion_Major = 1;
	const int TargetVulkanVersion_Minor = 0;

	unsigned int GpuId = 0;

	namespace VkDebug
	{
		// Print device info
		void PrintDeviceInfo(const std::vector<vk::PhysicalDevice>& physicalDevices);

		// Debug callback direct to stderr
		VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
			VkDebugReportFlagsEXT      flags,
			VkDebugReportObjectTypeEXT objectType,
			uint64_t                   object,
			size_t                     location,
			int32_t                    messageCode,
			const char*                pLayerPrefix,
			const char*                pMessage,
			void*                      pUserData);

		// VkDebug Implementation
		inline void VkDebug::PrintDeviceInfo(const std::vector<vk::PhysicalDevice>& physicalDevices)
		{
			for (uint32_t i = 0; i < physicalDevices.size(); i++)
			{
				// Print out device properties
				vk::PhysicalDeviceProperties deviceProperties = physicalDevices.at(i).getProperties();
				printf("Device ID:      %d\n", deviceProperties.deviceID);
				printf("Driver Version: %d\n", deviceProperties.driverVersion);
				printf("Device Name:    %s\n", deviceProperties.deviceName);
				printf("Device Type:    %d\n", deviceProperties.deviceType);
				printf("API Version:    %d.%d.%d\n",
					VK_VERSION_MAJOR(deviceProperties.apiVersion),
					VK_VERSION_MINOR(deviceProperties.apiVersion),
					VK_VERSION_PATCH(deviceProperties.apiVersion));

				printf("\n");

				// Print out device queue info
				std::vector<vk::QueueFamilyProperties> familyProperties = physicalDevices.at(i).getQueueFamilyProperties();

				for (size_t j = 0; j < familyProperties.size(); j++) {
					printf("Count of Queues: %d\n", familyProperties[j].queueCount);
					printf("Supported operations on this queue: %s\n", to_string(familyProperties[j].queueFlags).c_str());
					printf("\n");
				}

				// Print out device memory properties
				vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevices.at(i).getMemoryProperties();
				printf("Memory Type Count: %d\n", memoryProperties.memoryTypeCount);
				for (size_t j = 0; j < memoryProperties.memoryTypeCount; j++)
				{
					auto type = memoryProperties.memoryTypes[j];
					printf("(Heap %d) %s\n", type.heapIndex, to_string(type.propertyFlags).c_str());
				}
				printf("\n");

				printf("Memory Heap Count: %d\n", memoryProperties.memoryHeapCount);
				for (size_t j = 0; j < memoryProperties.memoryHeapCount; j++)
				{
					auto heap = memoryProperties.memoryHeaps[j];
					printf("(Heap %zu)\n", j);
					printf("\tsize: %llu MB\n", heap.size / 1024 / 1024);
					printf("\tflags:%s\n", to_string(heap.flags).c_str());
					printf("\n");
				}
				printf("\n");

				// Print out device features
				printf("Supported Features:\n");
				vk::PhysicalDeviceFeatures deviceFeatures = physicalDevices.at(i).getFeatures();
				if (deviceFeatures.shaderClipDistance == VK_TRUE) printf("Shader Clip Distance\n");
				if (deviceFeatures.textureCompressionBC == VK_TRUE) printf("BC Texture Compression\n");

				printf("\n");

				// Print out device limits
				printf("Device Limits:\n");
				printf("Max Vertex Input Attributes: %u\n", deviceProperties.limits.maxVertexInputAttributes);
				printf("Max Push Constants Size: %u\n", deviceProperties.limits.maxPushConstantsSize);

				// Readability
				printf("\n---\n\n");
			}
		}

		inline VKAPI_ATTR VkBool32 VKAPI_CALL VkDebug::DebugCallback(
			VkDebugReportFlagsEXT      flags,
			VkDebugReportObjectTypeEXT objectType,
			uint64_t                   object,
			size_t                     location,
			int32_t                    messageCode,
			const char*                pLayerPrefix,
			const char*                pMessage,
			void*                      pUserData)
		{
			(void)objectType, object, location, messageCode, pUserData;
			if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
				CoreLib::Diagnostics::Debug::Write(L"ERROR");
			if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
				CoreLib::Diagnostics::Debug::Write(L"WARNING");
			if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
				CoreLib::Diagnostics::Debug::Write(L"PERFORMANCE");
			if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
				CoreLib::Diagnostics::Debug::Write(L"INFO");
			if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
				CoreLib::Diagnostics::Debug::Write(L"DEBUG");

			CoreLib::Diagnostics::Debug::Write(L" [");
			CoreLib::Diagnostics::Debug::Write(pLayerPrefix);
			CoreLib::Diagnostics::Debug::Write(L"] ");

			CoreLib::Diagnostics::Debug::Write(L"(");
			CoreLib::Diagnostics::Debug::Write(to_string((vk::DebugReportObjectTypeEXT)objectType).c_str());
			//CoreLib::Diagnostics::Debug::Write(L" ");
			//CoreLib::Diagnostics::Debug::Write((long long)object);
			//CoreLib::Diagnostics::Debug::Write(L" at location ");
			//CoreLib::Diagnostics::Debug::Write((long long)location);
			CoreLib::Diagnostics::Debug::Write(L") ");
			CoreLib::Diagnostics::Debug::Write((long long)messageCode);
			CoreLib::Diagnostics::Debug::Write(L" ");
			CoreLib::Diagnostics::Debug::WriteLine(pMessage);
			return VK_FALSE;
		}
	}

	class DescriptorPoolObject : public CoreLib::Object
	{
	public:
		vk::DescriptorPool pool;
		DescriptorPoolObject();
		~DescriptorPoolObject();
	};

	/*
	* Internal Vulkan state
	*/
	class RendererState
	{
		//TODO: check class for multithreading safety
	private:
		bool initialized = false;
		int rendererCount = 0;
		vk::DebugReportCallbackEXT callback;

		vk::Instance instance;
		vk::PhysicalDevice physicalDevice;
		vk::Device device;

		vk::CommandPool swapchainCommandPool;
		vk::CommandPool transferCommandPool;
		vk::CommandPool renderCommandPool;
		CoreLib::RefPtr<CoreLib::List<vk::CommandBuffer>> primaryBuffers;
		CoreLib::RefPtr<CoreLib::List<vk::Fence>> primaryFences;

		int renderQueueIndex;
		int transferQueueIndex;
		vk::Queue renderQueue;
		vk::Queue transferQueue;

		CoreLib::RefPtr<CoreLib::List<DescriptorPoolObject*>> descriptorPoolChain;

		RendererState() {}

		static RendererState& State()
		{
			static RendererState singleton;
			return singleton;
		}

		static void CreateDebugCallback()
		{
			vk::DebugReportCallbackCreateInfoEXT callbackCreateInfo(
				vk::DebugReportFlagsEXT(
					//vk::DebugReportFlagBitsEXT::eInformation |
					vk::DebugReportFlagBitsEXT::eDebug |
					vk::DebugReportFlagBitsEXT::ePerformanceWarning |
					vk::DebugReportFlagBitsEXT::eWarning |
					vk::DebugReportFlagBitsEXT::eError
				),
				&VkDebug::DebugCallback,
				nullptr // userdata
			);

			State().callback = State().instance.createDebugReportCallbackEXT(callbackCreateInfo).value;
		}

		static void CreateInstance()
		{
			// Application and Instance Info
			vk::ApplicationInfo appInfo = vk::ApplicationInfo()
				.setPApplicationName("lll")
				.setPEngineName("Engine")
				.setEngineVersion(1)
				.setApiVersion(VK_API_VERSION_1_0);

			// Enabled Layers
			CoreLib::List<const char*> enabledInstanceLayers;
#ifdef _DEBUG
			bool hasValidationLayer = false;
			auto supportedLayers = vk::enumerateInstanceLayerProperties().value;
			for (auto & l : supportedLayers)
				if (strcmp(l.layerName, "VK_LAYER_LUNARG_standard_validation") == 0)
				{
					hasValidationLayer = true;
					break;
				}
#ifdef USE_VALIDATION_LAYER
			if (hasValidationLayer)
				enabledInstanceLayers.Add("VK_LAYER_LUNARG_standard_validation");
#endif
#endif
			// Enabled Extensions
			CoreLib::List<const char*> enabledInstanceExtensions;
			enabledInstanceExtensions.Add(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef _WIN32
			enabledInstanceExtensions.Add(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
			DEBUG_ONLY(enabledInstanceExtensions.Add(VK_EXT_DEBUG_REPORT_EXTENSION_NAME));

			// Instance Create Info
			vk::InstanceCreateInfo instInfo = vk::InstanceCreateInfo()
				.setPApplicationInfo(&appInfo)
				.setEnabledLayerCount(enabledInstanceLayers.Count())
				.setPpEnabledLayerNames(enabledInstanceLayers.Buffer())
				.setEnabledExtensionCount(enabledInstanceExtensions.Count())
				.setPpEnabledExtensionNames(enabledInstanceExtensions.Buffer());

			// Create the instance
			State().instance = vk::createInstance(instInfo).value;

			// Load instance level function pointers
			vkelInstanceInit((VkInstance)(State().instance));

			// Create callbacks
			DEBUG_ONLY(CreateDebugCallback());
		}

		static void SelectPhysicalDevice()
		{
			std::vector<vk::PhysicalDevice> physicalDevices = State().instance.enumeratePhysicalDevices().value;
			DEBUG_ONLY(VkDebug::PrintDeviceInfo(physicalDevices));
			if (GpuId >= physicalDevices.size())
				GpuId = 0;
			State().physicalDevice = physicalDevices[GpuId];
			printf("Using GPU %d\n", GpuId);
		}

		static void SelectPhysicalDevice(vk::SurfaceKHR surface)
		{
			std::vector<vk::PhysicalDevice> physicalDevices = State().instance.enumeratePhysicalDevices().value;
			DEBUG_ONLY(VkDebug::PrintDeviceInfo(physicalDevices));

			int k = 0;
			for (auto physDevice : physicalDevices)
			{
				for (size_t i = 0; i < physDevice.getQueueFamilyProperties().size(); i++)
				{
					if (physDevice.getSurfaceSupportKHR((uint32_t)i, surface).value)
					{
						State().physicalDevice = physDevice;
						GpuId = k;
						printf("Using GPU %d\n", GpuId);
						return;
					}
				}
				k++;
			}
		}

		static void CreateDevice()
		{
			// Initialize Queues
			CoreLib::List<float> renderQueuePriorities;
			CoreLib::List<float> transferQueuePriorities;

			renderQueuePriorities.Add(1.0f);

			std::vector<vk::QueueFamilyProperties> queueFamilyProperties = PhysicalDevice().getQueueFamilyProperties();
			if (queueFamilyProperties[0].queueCount >= 2)
			{
				renderQueuePriorities.Add(1.0f);
			}
			//transferQueuePriorities.Add(1.0f);

			//TODO: improve
			int renderQueueFamilyIndex = 0;
			int transferQueueFamilyIndex = 0;
			//NOTE: When changing this, resources created by the transferQueue will also need to have
			//      their ownership transferred to the renderQueue, if they are created with 
			//      vk::SharingMode::eExclusive. https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#resources-sharing

			CoreLib::List<vk::DeviceQueueCreateInfo> deviceQueueInfoVec;
			deviceQueueInfoVec.Add(
				vk::DeviceQueueCreateInfo()
				.setQueueFamilyIndex(renderQueueFamilyIndex)
				.setQueueCount(renderQueuePriorities.Count())
				.setPQueuePriorities(renderQueuePriorities.Buffer())
			);
			//deviceQueueInfoVec.Add(
			//	vk::DeviceQueueCreateInfo()
			//	.setQueueFamilyIndex(transferQueueFamilyIndex)
			//	.setQueueCount(transferQueuePriorities.Count())
			//	.setPQueuePriorities(transferQueuePriorities.Buffer())
			//);

			// Lambda to check if layer is present and then add it
			auto AddLayer = [](CoreLib::List<const char*>& enabledDeviceLayers, const char* layerName)
			{
				for (auto layer : State().physicalDevice.enumerateDeviceLayerProperties().value)
				{
					if (!strcmp(layerName, layer.layerName))
					{
						enabledDeviceLayers.Add(layerName);
						return;
					}
				}
				printf("Layer %s not supported\n", layerName);
			};
			CoreLib::List<const char*> enabledDeviceLayers;
#ifdef USE_VALIDATION_LAYER
			DEBUG_ONLY(AddLayer(enabledDeviceLayers, "VK_LAYER_LUNARG_standard_validation"));
#endif
			// Lambda to check if extension is present and then add it
			auto AddExtension = [](CoreLib::List<const char*>& enabledDeviceExtensions, const char* extensionName)
			{
				for (auto extension : State().physicalDevice.enumerateDeviceExtensionProperties().value)
				{
					if (!strcmp(extensionName, extension.extensionName))
					{
						enabledDeviceExtensions.Add(extensionName);
						return;
					}
				}
				printf("Extension %s not supported\n", extensionName);
			};
			CoreLib::List<const char*> enabledDeviceExtensions;
			AddExtension(enabledDeviceExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
			AddExtension(enabledDeviceExtensions, VK_NV_GLSL_SHADER_EXTENSION_NAME);
			DEBUG_ONLY(AddExtension(enabledDeviceExtensions, VK_EXT_DEBUG_MARKER_EXTENSION_NAME));

			// Device Features
			vk::PhysicalDeviceFeatures enabledDeviceFeatures = State().physicalDevice.getFeatures();

			vk::DeviceCreateInfo deviceInfo = vk::DeviceCreateInfo()
				.setQueueCreateInfoCount(deviceQueueInfoVec.Count())
				.setPQueueCreateInfos(deviceQueueInfoVec.Buffer())
				.setEnabledLayerCount(enabledDeviceLayers.Count())
				.setPpEnabledLayerNames(enabledDeviceLayers.Buffer())
				.setEnabledExtensionCount(enabledDeviceExtensions.Count())
				.setPpEnabledExtensionNames(enabledDeviceExtensions.Buffer())
				.setPEnabledFeatures(&enabledDeviceFeatures);

			State().device = State().physicalDevice.createDevice(deviceInfo).value;

			// Load device level function pointers
			vkelDeviceInit((VkDevice)(State().device));

			State().renderQueue = State().device.getQueue(renderQueueFamilyIndex, 0);
			State().transferQueue = State().device.getQueue(transferQueueFamilyIndex, renderQueuePriorities.Count() - 1);//TODO: Change the index if changing family
		}

		static void CreateCommandPool()
		{
			vk::CommandPoolCreateInfo commandPoolCreateInfo = vk::CommandPoolCreateInfo()
				.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
				.setQueueFamilyIndex(State().renderQueueIndex);

			State().swapchainCommandPool = State().device.createCommandPool(commandPoolCreateInfo).value;

			vk::CommandPoolCreateInfo setupCommandPoolCreateInfo = vk::CommandPoolCreateInfo()
				.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
				.setQueueFamilyIndex(State().transferQueueIndex);

			State().transferCommandPool = State().device.createCommandPool(setupCommandPoolCreateInfo).value;

			//TODO: multiple pools for multiple threads
			vk::CommandPoolCreateInfo renderCommandPoolCreateInfo = vk::CommandPoolCreateInfo()
				.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
				.setQueueFamilyIndex(State().renderQueueIndex);

			State().renderCommandPool = State().device.createCommandPool(renderCommandPoolCreateInfo).value;

			// Create primary command buffers
			State().primaryFences = new CoreLib::List<vk::Fence>();
			State().primaryBuffers = new CoreLib::List<vk::CommandBuffer>();
			const int numCommandBuffers = 3;
			for (int k = 0; k < numCommandBuffers; k++)
			{
				// Initially all fences are signaled
				State().primaryFences->Add(Device().createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)).value);
				State().primaryBuffers->Add(CreateCommandBuffer(RenderCommandPool()));
			}
		}

		static void CreateDescriptorPoolChain()
		{
			State().descriptorPoolChain = new CoreLib::List<DescriptorPoolObject*>();
		}

		// This function encapsulates all device-specific initialization
		static void InitDevice()
		{
			CreateDevice();
			CreateCommandPool();
			CreateDescriptorPoolChain();
		}

		static void Init()
		{
			if (State().initialized)
				return;

			State().initialized = true;
			vkelInit();
			CreateInstance();
			SelectPhysicalDevice();
			InitDevice();
		}

		static void DestroyDevice()
		{
			State().device.destroy();
		}

		static void DestroyInstance()
		{
			DEBUG_ONLY(State().instance.destroyDebugReportCallbackEXT(State().callback));
			State().instance.destroy();
		}

		static void DestroyCommandPool()
		{
			for (auto& fence : *State().primaryFences)
				State().device.destroyFence(fence);

			State().primaryFences = nullptr;
			State().primaryBuffers = nullptr;

			State().device.destroyCommandPool(State().renderCommandPool);
			State().device.destroyCommandPool(State().transferCommandPool);
			State().device.destroyCommandPool(State().swapchainCommandPool);
		}

		static void DestroyDescriptorPoolChain()
		{
			for (auto& descriptorPool : *State().descriptorPoolChain)
				delete descriptorPool;

			State().descriptorPoolChain = nullptr;
		}

		// This function encapsulates all device-specific destruction
		static void UninitDevice()
		{
			State().device.waitIdle();
			DestroyDescriptorPoolChain();
			DestroyCommandPool();
			DestroyDevice();
		}

		static void Destroy()
		{
			if (!State().initialized)
				return;

			UninitDevice();
			DestroyInstance();
			vkelUninit();
			State().initialized = false;
		};

		static void DeviceLost()
		{
			UninitDevice();
			SelectPhysicalDevice();
			InitDevice();
			//TODO: send message to user of engine that signals to recreate resources
		}

	public:
		RendererState(RendererState const&) = delete;
		void operator=(RendererState const&) = delete;

		~RendererState()
		{
			Destroy();//TODO: Does this need to be here?
		}

		// Const reference access to class variables
		static const vk::Instance& Instance()
		{
			return State().instance;
		}

		static const vk::PhysicalDevice& PhysicalDevice()
		{
			return State().physicalDevice;
		}

		static const vk::Device& Device()
		{
			return State().device;
		}

		static const vk::CommandPool& SwapchainCommandPool()
		{
			//TODO: do I want to expose these command pools?
			return State().swapchainCommandPool;
		}

		static const vk::CommandPool& TransferCommandPool()
		{
			return State().transferCommandPool;
		}

		static const vk::CommandPool& RenderCommandPool()
		{
			return State().renderCommandPool;
		}

		static const vk::Queue& TransferQueue()
		{
			return State().transferQueue;
		}

		static const vk::Queue& RenderQueue()
		{
			return State().renderQueue;
		}

		static std::pair<const vk::CommandBuffer, const vk::Fence> PrimaryBuffer()
		{
			// Wait for any fence to be signaled
			Device().waitForFences(
				vk::ArrayProxy<const vk::Fence>(State().primaryFences->Count(), State().primaryFences->Buffer()),
				vk::Bool32(false),
				UINT64_MAX
			);

			int numBuffers = State().primaryFences->Count();
			for (int k = 0; k < numBuffers; k++)
				if (Device().getFenceStatus((*State().primaryFences)[k]) == vk::Result::eSuccess)
				{
					static int usageCount[3] = { 0, 0, 0 };
					usageCount[k]++;
					// Set the fence to unsignaled state
					Device().resetFences((*State().primaryFences)[k]);
					return std::make_pair((*State().primaryBuffers)[k], (*State().primaryFences)[k]);
				}
		}

		static const vk::DescriptorPool& DescriptorPool()
		{
			if (State().descriptorPoolChain->Count() == 0)
				State().descriptorPoolChain->Add(new DescriptorPoolObject());

			return State().descriptorPoolChain->Last()->pool;
		}

		static std::pair<vk::DescriptorPool, vk::DescriptorSet> AllocateDescriptorSet(CoreLib::List<vk::DescriptorSetLayout> layouts)
		{
			std::pair<vk::DescriptorPool, vk::DescriptorSet> res;
			res.first = State().DescriptorPool();

			// Create Descriptor Set
			vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo = vk::DescriptorSetAllocateInfo()
				.setDescriptorPool(res.first)
				.setDescriptorSetCount(layouts.Count())
				.setPSetLayouts(layouts.Buffer());

			vk::ResultValue<std::vector<vk::DescriptorSet>> descriptorSets = RendererState::Device().allocateDescriptorSets(descriptorSetAllocateInfo);
			vk::Result err = descriptorSets.result;
			res.second = descriptorSets.value[0];

			// As of 1.0.25, assume that any error effectively means failure due to fragmented pool
			if (err != vk::Result::eSuccess)
			{
				State().descriptorPoolChain->Add(new DescriptorPoolObject());
				return AllocateDescriptorSet(layouts);
			}

			//TODO: add counter mechanism to DescriptorPoolObject so we know when to destruct

			return res;
		}

		// Bookkeeping for multiple instances of HardwareRenderer
		static void AddRenderer()
		{
			if (!State().initialized)
				Init();

			State().rendererCount++;
		}

		static void RemRenderer()
		{
			State().rendererCount--;

			if (RendererCount() == 0)
				Destroy();//TODO: Should I destroy the state here?
		}

		static int RendererCount()
		{
			return State().rendererCount;
		}

		// Resource creation functions
		static vk::SurfaceKHR CreateSurface(void* windowHandle)
		{
			// Create the surface
			vk::SurfaceKHR surface;

#ifdef _WIN32
			vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo = vk::Win32SurfaceCreateInfoKHR()
				.setHwnd((HWND)windowHandle)
				.setHinstance(GetModuleHandle(NULL));

			surface = State().instance.createWin32SurfaceKHR(surfaceCreateInfo).value;
#elif __ANDROID__
			vk::AndroidSurfaceCreateInfoKHR surfaceCreateInfo = vk::AndroidSurfaceCreaetInfoKHR()
				.setWindow((ANativeWindow*)window);

			surface = isntance.createAndroidSurfaceKHR(surfaceCreateInfo).value;
#endif

			// Check to see if the current physical device supports the surface
			bool supported = false;
			for (unsigned int i = 0; i < State().physicalDevice.getQueueFamilyProperties().size(); i++)
			{
				if (State().physicalDevice.getSurfaceSupportKHR(i, surface).value)
				{
					supported = true;
					break;
				}
			}

			// If not supported, recreate logical device with a compatible physical device
			if (!supported)
			{
				printf("GPU %d does not support current surface\n", GpuId);
				UninitDevice();
				SelectPhysicalDevice(surface);
				InitDevice();
				//TODO: Notify user to recreate any resources they have created prior to this point
			}

			return surface;
		}

		//TODO: I should probably wrap command buffers in a class that contain both the handle and a reference to the command pool created from
		static vk::CommandBuffer CreateCommandBuffer(vk::CommandPool commandPool, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary)
		{
			vk::CommandBufferAllocateInfo commandBufferAllocateInfo = vk::CommandBufferAllocateInfo()
				.setCommandPool(commandPool)
				.setLevel(level)
				.setCommandBufferCount(1);

			vk::CommandBuffer commandBuffer;

			//TODO: lock mutex on commandPool or allocate commandPool per thread
			commandBuffer = State().device.allocateCommandBuffers(commandBufferAllocateInfo).value.front();
			//TODO: unlock mutex on commandPool

			return commandBuffer;
		}
		static vk::CommandBuffer DestroyCommandBuffer(vk::CommandPool commandPool, vk::CommandBuffer commandBuffer)
		{
			//TODO: lock mutex on commandPool
			State().device.freeCommandBuffers(commandPool, commandBuffer);
			//TODO: unlock mutex on commandPool

			return commandBuffer;
		}
	};

	DescriptorPoolObject::DescriptorPoolObject() {
		CoreLib::List<vk::DescriptorPoolSize> poolSizes;
		poolSizes.Add(vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 20000));
		poolSizes.Add(vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 5000));
		poolSizes.Add(vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 2500));

		vk::DescriptorPoolCreateInfo poolCreateInfo = vk::DescriptorPoolCreateInfo()
			.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
			.setMaxSets(5000)
			.setPoolSizeCount(poolSizes.Count())
			.setPPoolSizes(poolSizes.Buffer());

		pool = RendererState::Device().createDescriptorPool(poolCreateInfo).value;
	}
	DescriptorPoolObject::~DescriptorPoolObject()
	{
		if (pool) RendererState::Device().destroyDescriptorPool(pool);
	}

	/*
	* Interface to Vulkan translation functions
	*/
	vk::Format TranslateStorageFormat(StorageFormat format)
	{
		switch (format)
		{
		case StorageFormat::Float16: return vk::Format::eR16Sfloat;
		case StorageFormat::Float32: return vk::Format::eR32Sfloat;
		case StorageFormat::Int8: return vk::Format::eR8Uint;
		case StorageFormat::Int16: return vk::Format::eR16Uint;
		case StorageFormat::Int32_Raw: return vk::Format::eR32Sint;//
		case StorageFormat::RG_F16: return vk::Format::eR16G16Sfloat;
		case StorageFormat::RG_F32: return vk::Format::eR32G32Sfloat;
		case StorageFormat::RG_I8: return vk::Format::eR8G8Uint;
		case StorageFormat::RG_I16: return vk::Format::eR16G16Uint;
		case StorageFormat::RG_I32_Raw: return vk::Format::eR32G32Sint;//
		case StorageFormat::RGB_F16: return vk::Format::eR16G16B16Sfloat;
		case StorageFormat::RGB_F32: return vk::Format::eR32G32B32Sfloat;
		case StorageFormat::RGB_I8: return vk::Format::eR8G8B8Uint;
		case StorageFormat::RGB_I16: return vk::Format::eR16G16B16Uint;
		case StorageFormat::RGB_I32_Raw: return vk::Format::eR32G32B32Sint;//
		case StorageFormat::RGBA_F16: return vk::Format::eR16G16B16A16Sfloat;
		case StorageFormat::RGBA_F32: return vk::Format::eR32G32B32A32Sfloat;
		case StorageFormat::RGBA_8: return vk::Format::eR8G8B8A8Unorm;
		case StorageFormat::RGBA_I8: return vk::Format::eR8G8B8A8Uint;
		case StorageFormat::RGBA_I16: return vk::Format::eR16G16B16A16Uint;
		case StorageFormat::RGBA_I32_Raw: return vk::Format::eR32G32B32A32Sint;//
		case StorageFormat::BC1: return vk::Format::eBc1RgbaUnormBlock;//
		case StorageFormat::RGBA_Compressed: return vk::Format::eBc7UnormBlock;//
		case StorageFormat::R11F_G11F_B10F: return vk::Format::eB10G11R11UfloatPack32; // need to swizzle?
		case StorageFormat::RGB10_A2: return vk::Format::eA2R10G10B10UnormPack32;//
		case StorageFormat::Depth32: return vk::Format::eD32Sfloat;
		case StorageFormat::Depth24Stencil8: return vk::Format::eD24UnormS8Uint;
		default: throw HardwareRendererException(L"Not implemented.");
		}
	};

	vk::BufferUsageFlags TranslateUsageFlags(BufferUsage usage)
	{
		switch (usage)
		{
		case BufferUsage::ArrayBuffer: return vk::BufferUsageFlagBits::eVertexBuffer;
		case BufferUsage::IndexBuffer: return vk::BufferUsageFlagBits::eIndexBuffer;
		case BufferUsage::StorageBuffer: return vk::BufferUsageFlagBits::eStorageBuffer;
		case BufferUsage::UniformBuffer: return vk::BufferUsageFlagBits::eUniformBuffer;
		default: throw HardwareRendererException(L"Not implemented.");
		}
	}

	vk::Format TranslateVertexAttribute(VertexAttributeDesc attribute)
	{
		switch (attribute.Type)
		{
		case DataType::Byte:
			return attribute.Normalized ? vk::Format::eR8Unorm : vk::Format::eR8Uint;
		case DataType::Char:
			return attribute.Normalized ? vk::Format::eR8Snorm : vk::Format::eR8Sint;
		case DataType::Byte2:
			return attribute.Normalized ? vk::Format::eR8G8Unorm : vk::Format::eR8G8Uint;
		case DataType::Char2:
			return attribute.Normalized ? vk::Format::eR8G8Snorm : vk::Format::eR8G8Sint;
		case DataType::Byte3:
			return attribute.Normalized ? vk::Format::eR8G8B8Unorm : vk::Format::eR8G8B8Uint;
		case DataType::Char3:
			return attribute.Normalized ? vk::Format::eR8G8B8Snorm : vk::Format::eR8G8B8Sint;
		case DataType::Byte4:
			return attribute.Normalized ? vk::Format::eR8G8B8A8Unorm : vk::Format::eR8G8B8A8Uint;
		case DataType::Char4:
			return attribute.Normalized ? vk::Format::eR8G8B8A8Snorm : vk::Format::eR8G8B8A8Sint;
		case DataType::Short:
			return attribute.Normalized ? vk::Format::eR16Snorm : vk::Format::eR16Sint;
		case DataType::UShort:
			return attribute.Normalized ? vk::Format::eR16Unorm : vk::Format::eR16Uint;
		case DataType::Half:
			return attribute.Normalized ? throw HardwareRendererException(L"Unsupported data type.") : vk::Format::eR16Sfloat;//
		case DataType::Short2:
			return attribute.Normalized ? vk::Format::eR16G16Snorm : vk::Format::eR16G16Sint;
		case DataType::UShort2:
			return attribute.Normalized ? vk::Format::eR16G16Unorm : vk::Format::eR16G16Uint;
		case DataType::Half2:
			return attribute.Normalized ? throw HardwareRendererException(L"Unsupported data type.") : vk::Format::eR16G16Sfloat;//
		case DataType::Short3:
			return attribute.Normalized ? vk::Format::eR16G16B16Snorm : vk::Format::eR16G16B16Sint;
		case DataType::UShort3:
			return attribute.Normalized ? vk::Format::eR16G16B16Unorm : vk::Format::eR16G16B16Uint;
		case DataType::Half3:
			return attribute.Normalized ? throw HardwareRendererException(L"Unsupported data type.") : vk::Format::eR16G16B16Sfloat;//
		case DataType::Short4:
			return attribute.Normalized ? vk::Format::eR16G16B16A16Snorm : vk::Format::eR16G16B16A16Sint;
		case DataType::UShort4:
			return attribute.Normalized ? vk::Format::eR16G16B16A16Unorm : vk::Format::eR16G16B16A16Uint;
		case DataType::Half4:
			return attribute.Normalized ? vk::Format::eR16G16B16A16Snorm : vk::Format::eR16G16B16A16Sfloat;//
		case DataType::Int:
			return attribute.Normalized ? throw HardwareRendererException(L"Unsupported data type.") : vk::Format::eR32Sint;
		case DataType::UInt:
			return attribute.Normalized ? throw HardwareRendererException(L"Unsupported data type.") : vk::Format::eR32Uint;
		case DataType::Float:
			return attribute.Normalized ? throw HardwareRendererException(L"Unsupported data type.") : vk::Format::eR32Sfloat;
		case DataType::Int2:
			return attribute.Normalized ? throw HardwareRendererException(L"Unsupported data type.") : vk::Format::eR32G32Sint;
		case DataType::Float2:
			return attribute.Normalized ? throw HardwareRendererException(L"Unsupported data type.") : vk::Format::eR32G32Sfloat;
		case DataType::Int3:
			return attribute.Normalized ? throw HardwareRendererException(L"Unsupported data type.") : vk::Format::eR32G32B32Sint;
		case DataType::Float3:
			return attribute.Normalized ? throw HardwareRendererException(L"Unsupported data type.") : vk::Format::eR32G32B32Sfloat;
		case DataType::Int4:
			return attribute.Normalized ? throw HardwareRendererException(L"Unsupported data type.") : vk::Format::eR32G32B32A32Sint;
		case DataType::UInt4_10_10_10_2:
			throw HardwareRendererException(L"Unsupported data type.");
		case DataType::Float4:
			return attribute.Normalized ? throw HardwareRendererException(L"Unsupported data type.") : vk::Format::eR32G32B32Sfloat;
		default:
			throw HardwareRendererException(L"Unimplemented data type.");
		}
	}

	vk::DescriptorType TranslateBindingType(BindingType bindType)
	{
		switch (bindType)
		{
		case BindingType::Texture: return vk::DescriptorType::eCombinedImageSampler;
		case BindingType::UniformBuffer: return vk::DescriptorType::eUniformBuffer; //TODO: dynamic?
		case BindingType::StorageBuffer: return vk::DescriptorType::eStorageBuffer; //TODO: ^
		case BindingType::Unused: throw HardwareRendererException(L"Attempting to use unused binding");
		default: throw HardwareRendererException(L"Not implemented");
		}
	}

	vk::ImageLayout LayoutFromUsage(TextureUsage usage)
	{
		//TODO: fix this function to deal with the mask correctly
		switch (usage)
		{
		case TextureUsage::ColorAttachment:
		case TextureUsage::SampledColorAttachment:
			return vk::ImageLayout::eColorAttachmentOptimal;
		case TextureUsage::DepthAttachment:
		case TextureUsage::SampledDepthAttachment:
			return vk::ImageLayout::eDepthStencilAttachmentOptimal;
		case TextureUsage::Sampled: return vk::ImageLayout::eShaderReadOnlyOptimal;
		default: throw HardwareRendererException(L"Not implemented");
		}
	}

	vk::CompareOp TranslateCompareFunc(CompareFunc compareFunc)
	{
		switch (compareFunc)
		{
		case CompareFunc::Disabled: return vk::CompareOp::eNever;
		case CompareFunc::Equal: return vk::CompareOp::eEqual;
		case CompareFunc::Less: return vk::CompareOp::eLess;
		case CompareFunc::Greater: return vk::CompareOp::eGreater;
		case CompareFunc::LessEqual: return vk::CompareOp::eLessOrEqual;
		case CompareFunc::GreaterEqual: return vk::CompareOp::eGreaterOrEqual;
		case CompareFunc::NotEqual: return vk::CompareOp::eNotEqual;
		case CompareFunc::Always: return vk::CompareOp::eAlways;
		case CompareFunc::Never: return vk::CompareOp::eNever;
		default: throw CoreLib::NotImplementedException();
		}
	}



	vk::StencilOp TranslateStencilOp(StencilOp stencilOp)
	{
		switch (stencilOp)
		{
		case StencilOp::Keep: return vk::StencilOp::eKeep;
		case StencilOp::Zero: return vk::StencilOp::eZero;
		case StencilOp::Replace: return vk::StencilOp::eReplace;
		case StencilOp::Increment: return vk::StencilOp::eIncrementAndClamp;
		case StencilOp::IncrementWrap: return vk::StencilOp::eIncrementAndWrap;
		case StencilOp::Decrement: return vk::StencilOp::eDecrementAndClamp;
		case StencilOp::DecrementWrap: return vk::StencilOp::eDecrementAndWrap;
		case StencilOp::Invert: return vk::StencilOp::eInvert;
		default: throw CoreLib::NotImplementedException();
		}
	}

	/*
	* Vulkan helper functions
	*/
	vk::AccessFlags LayoutFlags(vk::ImageLayout layout)
	{
		switch (layout)
		{
		case vk::ImageLayout::eUndefined:
			return vk::AccessFlags();
		case vk::ImageLayout::eGeneral:
			return vk::AccessFlags();
		case vk::ImageLayout::eColorAttachmentOptimal:
			return vk::AccessFlagBits::eColorAttachmentWrite;
		case vk::ImageLayout::eDepthStencilAttachmentOptimal:
			return vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
			return vk::AccessFlagBits::eDepthStencilAttachmentRead;
		case vk::ImageLayout::eShaderReadOnlyOptimal:
			return vk::AccessFlagBits::eShaderRead;
		case vk::ImageLayout::eTransferSrcOptimal:
			return vk::AccessFlagBits::eTransferRead;
		case vk::ImageLayout::eTransferDstOptimal:
			return vk::AccessFlagBits::eTransferWrite;
		case vk::ImageLayout::ePreinitialized:
			return vk::AccessFlagBits::eHostWrite;
		case vk::ImageLayout::ePresentSrcKHR:
			return vk::AccessFlags();
		default: // This is not a valid image layout
			exit(-1);
		}
	}

	vk::SampleCountFlagBits SampleCount(int samples)
	{
		if (samples <= 0 || (samples & (samples - 1)) != 0)
			throw HardwareRendererException(L"samples must be a power of 2");

		return (vk::SampleCountFlagBits)samples;
	}

	int32_t GetMemoryType(uint32_t memoryTypeBits, vk::MemoryPropertyFlags memoryPropertyFlags)
	{
		int32_t memoryTypeIndex = 0;

		for (uint32_t k = 0; k < 32; k++)
		{
			if ((memoryTypeBits & 1) == 1)
			{
				if ((RendererState::PhysicalDevice().getMemoryProperties().memoryTypes[k].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags)
				{
					memoryTypeIndex = k;
					return memoryTypeIndex;
				}
			}
			memoryTypeBits >>= 1;
		}
		throw HardwareRendererException("Could not find a valid memory type index.");
	}

	vk::ShaderStageFlagBits ShaderStage(ShaderType stage)
	{
		switch (stage)
		{
		case ShaderType::VertexShader: return vk::ShaderStageFlagBits::eVertex;
		case ShaderType::FragmentShader: return vk::ShaderStageFlagBits::eFragment;
		case ShaderType::ComputeShader: return vk::ShaderStageFlagBits::eCompute;
		default: throw HardwareRendererException(L"Not implemented.");
		}
	}

	class RenderBuffer
	{

	};

	class Texture : public CoreLib::Object
	{
	public:
		vk::Image image;
		vk::ImageView view;
		vk::DeviceMemory memory;
		StorageFormat format;
		TextureUsage usage;

		Texture(TextureUsage usage)
		{
			this->usage = usage;
			if (!!(usage & TextureUsage::DepthAttachment))
				this->format = StorageFormat::Depth32;
			else if (!!(usage & TextureUsage::ColorAttachment))
				this->format = StorageFormat::RGBA_8;
			else if (!!(usage & TextureUsage::Sampled))
				this->format = StorageFormat::RGBA_F32;
		}
		~Texture()
		{
			RendererState::Device().waitIdle(); //TODO: Remove
			if (memory) RendererState::Device().freeMemory(memory);
			if (view) RendererState::Device().destroyImageView(view);
			if (image) RendererState::Device().destroyImage(image);
		}
	};

	class Texture2D : public Texture, public GameEngine::Texture2D
	{
		//TODO: Need some way of determining layouts and performing transitions properly. 
	public:
		int width;
		int height;
		int samples = 1;
		int mipLevels = 1;
		vk::ImageLayout currentLayout;

		Texture2D(TextureUsage usage) : Texture(usage) {};

		void GetSize(int& pwidth, int& pheight)
		{
			pwidth = width;
			pheight = height;
		}
		void Resize(int newWidth, int newHeight, int newSamples, int newMipLevels = 1, bool preserveData = false)
		{
			if (this->width == newWidth && this->height == newHeight && this->samples == newSamples && this->mipLevels == newMipLevels)
				return;

			if (preserveData && this->samples != samples)
				throw HardwareRendererException(L"Sample count must be identical to preserve data");

			vk::Image oldImage = image;
			vk::ImageView oldView = view;
			vk::DeviceMemory oldMemory = memory;

			int oldWidth = this->width;
			int oldHeight = this->height;
			int oldMipLevels = this->mipLevels;

			vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor;
			vk::ImageUsageFlags usageFlags;
			if (!!(usage & TextureUsage::ColorAttachment))
			{
				aspectFlags = vk::ImageAspectFlagBits::eColor;
				usageFlags = vk::ImageUsageFlagBits::eColorAttachment;
			}
			else if (!!(usage & TextureUsage::DepthAttachment))
			{
				if (format == StorageFormat::Depth32)
				{
					aspectFlags = vk::ImageAspectFlagBits::eDepth;
					usageFlags = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
				}
				else if (format == StorageFormat::Depth24Stencil8)
				{
					aspectFlags = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
					usageFlags = vk::ImageUsageFlagBits::eDepthStencilAttachment;
				}
			}
			if (!!(usage & TextureUsage::Sampled))
			{
				usageFlags |= vk::ImageUsageFlagBits::eSampled;
			}

			// Create texture resources
			vk::ImageCreateInfo imageCreateInfo = vk::ImageCreateInfo()
				.setFlags(vk::ImageCreateFlags())
				.setImageType(vk::ImageType::e2D)
				.setFormat(TranslateStorageFormat(format))
				.setExtent(vk::Extent3D(newWidth, newHeight, 1))
				.setMipLevels(newMipLevels)
				.setArrayLayers(1)
				.setSamples(SampleCount(newSamples))
				.setTiling(vk::ImageTiling::eOptimal)
				.setUsage(vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | usageFlags)
				.setSharingMode(vk::SharingMode::eExclusive)
				.setQueueFamilyIndexCount(0)
				.setPQueueFamilyIndices(nullptr)
				.setInitialLayout(vk::ImageLayout::eUndefined);

			image = RendererState::Device().createImage(imageCreateInfo).value;

			vk::MemoryRequirements imageMemoryRequirements = RendererState::Device().getImageMemoryRequirements(image);

			vk::MemoryAllocateInfo imageAllocateInfo = vk::MemoryAllocateInfo()
				.setAllocationSize(imageMemoryRequirements.size)
				.setMemoryTypeIndex(GetMemoryType(imageMemoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

			memory = RendererState::Device().allocateMemory(imageAllocateInfo).value;
			RendererState::Device().bindImageMemory(image, memory, 0);

			vk::ImageSubresourceRange imageSubresourceRange = vk::ImageSubresourceRange()
				.setAspectMask(aspectFlags)
				.setBaseMipLevel(0)
				.setLevelCount(newMipLevels)
				.setBaseArrayLayer(0)
				.setLayerCount(1);

			vk::ImageViewCreateInfo imageViewCreateInfo = vk::ImageViewCreateInfo()
				.setFlags(vk::ImageViewCreateFlags())
				.setImage(image)
				.setViewType(vk::ImageViewType::e2D)
				.setFormat(imageCreateInfo.format)
				.setComponents(vk::ComponentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA))//
				.setSubresourceRange(imageSubresourceRange);

			view = RendererState::Device().createImageView(imageViewCreateInfo).value;

			if (preserveData)
			{
				if (!oldImage || !oldView || !oldMemory)
					throw HardwareRendererException(L"There is no data to preserve");

				(void)oldWidth, oldHeight, oldMipLevels;
				//TODO: blit the image from the old to new image here
			}

			// Free old resources
			if (oldMemory) RendererState::Device().freeMemory(oldMemory);
			if (oldView) RendererState::Device().destroyImageView(oldView);
			if (oldImage) RendererState::Device().destroyImage(oldImage);

			// Set texture parameters
			this->width = newWidth;
			this->height = newHeight;
			this->samples = newSamples;
			this->mipLevels = newMipLevels;
		}
		void SetData(StorageFormat pformat, int level, int pwidth, int pheight, int numSamples, DataType inputType, void* data, bool mipmapped = true)
		{
			if (numSamples > 1)
				throw HardwareRendererException(L"samples must be equal to 1");

			format = pformat;
			if (level == 0)
			{
				if (mipmapped)
					Resize(pwidth, pheight, numSamples, (int)std::floor(std::log2(max(pwidth, pheight))) + 1);
				else
					Resize(pwidth, pheight, numSamples);
			}

			if (data == nullptr)
			{
				// If there is no data to copy, just transition the image and return

				// Create command buffer
				//TODO: Use CommandBuffer class?
				vk::CommandBuffer transferCommandBuffer = RendererState::CreateCommandBuffer(RendererState::TransferCommandPool());

				vk::ImageAspectFlags aspectFlags;
				if (format == StorageFormat::Depth32)
					aspectFlags = vk::ImageAspectFlagBits::eDepth;
				else if (format == StorageFormat::Depth24Stencil8)
					aspectFlags = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
				else
					aspectFlags = vk::ImageAspectFlagBits::eColor;

				// Record command buffer
				vk::ImageSubresourceRange textureSubresourceRange = vk::ImageSubresourceRange()
					.setAspectMask(aspectFlags)
					.setBaseMipLevel(0)
					.setLevelCount(mipLevels)
					.setBaseArrayLayer(0)
					.setLayerCount(1);

				vk::ImageMemoryBarrier textureUsageBarrier = vk::ImageMemoryBarrier()
					.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eUndefined))
					.setDstAccessMask(LayoutFlags(LayoutFromUsage(this->usage)))
					.setOldLayout(vk::ImageLayout::eUndefined)
					.setNewLayout(LayoutFromUsage(this->usage))
					.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setImage(this->image)
					.setSubresourceRange(textureSubresourceRange);

				vk::CommandBufferBeginInfo transferBeginInfo = vk::CommandBufferBeginInfo()
					.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
					.setPInheritanceInfo(nullptr);

				transferCommandBuffer.begin(transferBeginInfo);
				transferCommandBuffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eTopOfPipe,
					vk::PipelineStageFlagBits::eTopOfPipe,
					vk::DependencyFlags(),
					nullptr,
					nullptr,
					textureUsageBarrier
				);
				transferCommandBuffer.end();

				// Submit to queue
				vk::SubmitInfo transferSubmitInfo = vk::SubmitInfo()
					.setWaitSemaphoreCount(0)
					.setPWaitSemaphores(nullptr)
					.setPWaitDstStageMask(nullptr)
					.setCommandBufferCount(1)
					.setPCommandBuffers(&transferCommandBuffer)
					.setSignalSemaphoreCount(0)
					.setPSignalSemaphores(nullptr);

				RendererState::TransferQueue().submit(transferSubmitInfo, vk::Fence());
				RendererState::TransferQueue().waitIdle(); //TODO: Remove
				RendererState::DestroyCommandBuffer(RendererState::TransferCommandPool(), transferCommandBuffer);
			}
			else
			{
				if (this->mipLevels < level)
					throw HardwareRendererException(L"Attempted to set mipmap data for invalid level");

				vk::ImageAspectFlags aspectFlags;
				if (format == StorageFormat::Depth32)
					aspectFlags = vk::ImageAspectFlagBits::eDepth;
				else if (format == StorageFormat::Depth24Stencil8)
					aspectFlags = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
				else
					aspectFlags = vk::ImageAspectFlagBits::eColor;

				bool useStagingImage = false;
				if (useStagingImage)
				{
					// Set up staging image and copy data to new image

					int inputTypeSize = DataTypeSize(inputType);

					// Create staging image
					vk::ImageCreateInfo stagingImageCreateInfo = vk::ImageCreateInfo()
						.setFlags(vk::ImageCreateFlags())
						.setImageType(vk::ImageType::e2D)
						.setFormat(TranslateStorageFormat(format))//TODO: base this off of inputType
						.setExtent(vk::Extent3D(pwidth, pheight, 1))
						.setMipLevels(1)
						.setArrayLayers(1)
						.setSamples(vk::SampleCountFlagBits::e1)
						.setTiling(vk::ImageTiling::eLinear)
						.setUsage(vk::ImageUsageFlagBits::eTransferSrc)
						.setSharingMode(vk::SharingMode::eExclusive)
						.setQueueFamilyIndexCount(0)
						.setPQueueFamilyIndices(nullptr)
						.setInitialLayout(vk::ImageLayout::ePreinitialized);

					vk::Image stagingImage = RendererState::Device().createImage(stagingImageCreateInfo).value;

					vk::MemoryRequirements stagingImageMemoryRequirements = RendererState::Device().getImageMemoryRequirements(image);

					vk::MemoryAllocateInfo imageAllocateInfo = vk::MemoryAllocateInfo()
						.setAllocationSize(stagingImageMemoryRequirements.size)
						.setMemoryTypeIndex(GetMemoryType(stagingImageMemoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible));

					vk::DeviceMemory stagingMemory = RendererState::Device().allocateMemory(imageAllocateInfo).value;

					RendererState::Device().bindImageMemory(stagingImage, stagingMemory, 0);

					vk::ImageSubresource stagingImageSubresource = vk::ImageSubresource()
						.setAspectMask(aspectFlags)
						.setMipLevel(0)
						.setArrayLayer(0);

					vk::SubresourceLayout stagingSubresourceLayout = RendererState::Device().getImageSubresourceLayout(stagingImage, stagingImageSubresource);

					// Copy data to staging image
					void* stagingMappedMemory = RendererState::Device().mapMemory(stagingMemory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags()).value;

					if (format == StorageFormat::BC1 || format == StorageFormat::BC5)
					{
						int blocks = (int)(ceil(pwidth / 4.0f) * ceil(pheight / 4.0f));
						int bufferSize = format == StorageFormat::BC5 ? blocks * 16 : blocks * 8;
						memcpy(stagingMappedMemory, data, bufferSize);
					}
					else
					{
						char* rowPtr = reinterpret_cast<char*>(stagingMappedMemory);
						for (int y = 0; y < pheight; y++)
						{
							memcpy(rowPtr, &((char*)data)[y * pwidth * inputTypeSize], pwidth * inputTypeSize);
							rowPtr += stagingSubresourceLayout.rowPitch;
						}
					}

					RendererState::Device().unmapMemory(stagingMemory);

					// Create image copy information
					//TODO: Should this be a blit?
					//TODO: Correctly handle format conversions
					vk::ImageCopy stagingCopy = vk::ImageCopy()
						.setSrcOffset(vk::Offset3D(0, 0, 0))
						.setSrcSubresource(vk::ImageSubresourceLayers().setAspectMask(aspectFlags).setMipLevel(0).setBaseArrayLayer(0).setLayerCount(1))
						.setDstOffset(vk::Offset3D(0, 0, 0))
						.setDstSubresource(vk::ImageSubresourceLayers().setAspectMask(aspectFlags).setMipLevel(level).setBaseArrayLayer(0).setLayerCount(1))
						.setExtent(vk::Extent3D(pwidth, pheight, 1));

					// Create command buffer
					//TODO: Use CommandBuffer class?
					vk::CommandBuffer transferCommandBuffer = RendererState::CreateCommandBuffer(RendererState::TransferCommandPool());

					// Record command buffer
					vk::ImageSubresourceRange textureSubresourceRange = vk::ImageSubresourceRange()
						.setAspectMask(aspectFlags)
						.setBaseMipLevel(0)
						.setLevelCount(mipLevels)
						.setBaseArrayLayer(0)
						.setLayerCount(1);

					vk::ImageMemoryBarrier textureCopyBarrier = vk::ImageMemoryBarrier()
						.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eUndefined))
						.setDstAccessMask(LayoutFlags(vk::ImageLayout::eTransferDstOptimal))
						.setOldLayout(vk::ImageLayout::eUndefined)
						.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
						.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
						.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
						.setImage(this->image)
						.setSubresourceRange(textureSubresourceRange);

					vk::ImageMemoryBarrier textureUsageBarrier = vk::ImageMemoryBarrier()
						.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eTransferDstOptimal))
						.setDstAccessMask(LayoutFlags(LayoutFromUsage(this->usage)))
						.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
						.setNewLayout(LayoutFromUsage(this->usage))
						.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
						.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
						.setImage(this->image)
						.setSubresourceRange(textureSubresourceRange);

					vk::CommandBufferBeginInfo transferBeginInfo = vk::CommandBufferBeginInfo()
						.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
						.setPInheritanceInfo(nullptr);

					transferCommandBuffer.begin(transferBeginInfo);
					transferCommandBuffer.pipelineBarrier(
						vk::PipelineStageFlagBits::eTopOfPipe,
						vk::PipelineStageFlagBits::eTopOfPipe,
						vk::DependencyFlags(),
						nullptr,
						nullptr,
						textureCopyBarrier
					);
					transferCommandBuffer.copyImage(stagingImage, vk::ImageLayout::ePreinitialized, this->image, vk::ImageLayout::eTransferDstOptimal, stagingCopy);
					transferCommandBuffer.pipelineBarrier(
						vk::PipelineStageFlagBits::eTopOfPipe,
						vk::PipelineStageFlagBits::eTopOfPipe,
						vk::DependencyFlags(),
						nullptr,
						nullptr,
						textureUsageBarrier
					);
					transferCommandBuffer.end();

					// Submit to queue
					vk::SubmitInfo transferSubmitInfo = vk::SubmitInfo()
						.setWaitSemaphoreCount(0)
						.setPWaitSemaphores(nullptr)
						.setPWaitDstStageMask(nullptr)
						.setCommandBufferCount(1)
						.setPCommandBuffers(&transferCommandBuffer)
						.setSignalSemaphoreCount(0)
						.setPSignalSemaphores(nullptr);

					RendererState::TransferQueue().submit(transferSubmitInfo, vk::Fence());
					RendererState::TransferQueue().waitIdle(); //TODO: Remove
					RendererState::DestroyCommandBuffer(RendererState::TransferCommandPool(), transferCommandBuffer);

					// Destroy staging resources
					RendererState::Device().freeMemory(stagingMemory);
					RendererState::Device().destroyImage(stagingImage);
				}
				else
				{
					// Set up staging buffer and copy data to new image

					int bufferSize;
					if (format == StorageFormat::BC1 || format == StorageFormat::BC5)
					{
						int blocks = (int)(ceil(pwidth / 4.0f) * ceil(pheight / 4.0f));
						bufferSize = format == StorageFormat::BC5 ? blocks * 16 : blocks * 8;
					}
					else
					{
						bufferSize = pwidth * pheight * DataTypeSize(inputType);
					}

					vk::BufferCreateInfo stagingBufferCreateInfo = vk::BufferCreateInfo()
						.setFlags(vk::BufferCreateFlags())
						.setSize(bufferSize)
						.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
						.setSharingMode(vk::SharingMode::eExclusive)
						.setQueueFamilyIndexCount(0)
						.setPQueueFamilyIndices(nullptr);

					vk::Buffer stagingBuffer = RendererState::Device().createBuffer(stagingBufferCreateInfo).value;

					vk::MemoryRequirements stagingBufferMemoryRequirements = RendererState::Device().getBufferMemoryRequirements(stagingBuffer);

					vk::MemoryAllocateInfo bufferAllocateInfo = vk::MemoryAllocateInfo()
						.setAllocationSize(stagingBufferMemoryRequirements.size)
						.setMemoryTypeIndex(GetMemoryType(stagingBufferMemoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible));

					vk::DeviceMemory stagingMemory = RendererState::Device().allocateMemory(bufferAllocateInfo).value;

					RendererState::Device().bindBufferMemory(stagingBuffer, stagingMemory, 0);

					void* stagingMappedMemory = RendererState::Device().mapMemory(stagingMemory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags()).value;
					memcpy(stagingMappedMemory, data, bufferSize);

					//TODO: flush?
					RendererState::Device().unmapMemory(stagingMemory);

					// Create buffer image copy information
					vk::BufferImageCopy stagingCopy = vk::BufferImageCopy()
						.setBufferOffset(0)
						.setBufferRowLength(0)
						.setBufferImageHeight(0)
						.setImageSubresource(vk::ImageSubresourceLayers().setAspectMask(aspectFlags).setMipLevel(level).setBaseArrayLayer(0).setLayerCount(1))
						.setImageOffset(vk::Offset3D())
						.setImageExtent(vk::Extent3D(pwidth, pheight, 1));

					// Create command buffer
					//TODO: Use CommandBuffer class?
					vk::CommandBuffer transferCommandBuffer = RendererState::CreateCommandBuffer(RendererState::TransferCommandPool());

					// Record command buffer
					vk::ImageSubresourceRange textureSubresourceRange = vk::ImageSubresourceRange()
						.setAspectMask(aspectFlags)
						.setBaseMipLevel(0)
						.setLevelCount(mipLevels)
						.setBaseArrayLayer(0)
						.setLayerCount(1);

					vk::ImageMemoryBarrier textureCopyBarrier = vk::ImageMemoryBarrier()
						.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eUndefined))
						.setDstAccessMask(LayoutFlags(vk::ImageLayout::eTransferDstOptimal))
						.setOldLayout(vk::ImageLayout::eUndefined)
						.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
						.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
						.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
						.setImage(this->image)
						.setSubresourceRange(textureSubresourceRange);

					vk::ImageMemoryBarrier textureUsageBarrier = vk::ImageMemoryBarrier()
						.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eTransferDstOptimal))
						.setDstAccessMask(LayoutFlags(LayoutFromUsage(this->usage)))
						.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
						.setNewLayout(LayoutFromUsage(this->usage))
						.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
						.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
						.setImage(this->image)
						.setSubresourceRange(textureSubresourceRange);

					vk::CommandBufferBeginInfo transferBeginInfo = vk::CommandBufferBeginInfo()
						.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
						.setPInheritanceInfo(nullptr);

					transferCommandBuffer.begin(transferBeginInfo);
					transferCommandBuffer.pipelineBarrier(
						vk::PipelineStageFlagBits::eTopOfPipe,
						vk::PipelineStageFlagBits::eTopOfPipe,
						vk::DependencyFlags(),
						nullptr,
						nullptr,
						textureCopyBarrier
					);
					transferCommandBuffer.copyBufferToImage(
						stagingBuffer,
						this->image,
						vk::ImageLayout::eTransferDstOptimal,
						stagingCopy
					);
					transferCommandBuffer.pipelineBarrier(
						vk::PipelineStageFlagBits::eTopOfPipe,
						vk::PipelineStageFlagBits::eTopOfPipe,
						vk::DependencyFlags(),
						nullptr,
						nullptr,
						textureUsageBarrier
					);
					transferCommandBuffer.end();

					// Submit to queue
					vk::SubmitInfo transferSubmitInfo = vk::SubmitInfo()
						.setWaitSemaphoreCount(0)
						.setPWaitSemaphores(nullptr)
						.setPWaitDstStageMask(nullptr)
						.setCommandBufferCount(1)
						.setPCommandBuffers(&transferCommandBuffer)
						.setSignalSemaphoreCount(0)
						.setPSignalSemaphores(nullptr);

					RendererState::TransferQueue().submit(transferSubmitInfo, vk::Fence());
					RendererState::TransferQueue().waitIdle(); //TODO: Remove
					RendererState::DestroyCommandBuffer(RendererState::TransferCommandPool(), transferCommandBuffer);

					// Destroy staging resources
					RendererState::Device().freeMemory(stagingMemory);
					RendererState::Device().destroyBuffer(stagingBuffer);
				}
			}
			this->currentLayout = LayoutFromUsage(this->usage);
		}
		void SetData(StorageFormat pformat, int pwidth, int pheight, int numSamples, DataType inputType, void* data, bool mipmapped = true)
		{
			SetData(pformat, 0, pwidth, pheight, numSamples, inputType, data, mipmapped);
		}
		void BuildMipmaps()
		{
			vk::Image oldImage = image;
			vk::ImageView oldView = view;
			vk::DeviceMemory oldMemory = memory;

			vk::ImageAspectFlags aspectFlags;
			if (format == StorageFormat::Depth32)
				aspectFlags = vk::ImageAspectFlagBits::eDepth;
			else if (format == StorageFormat::Depth24Stencil8)
				aspectFlags = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
			else
				aspectFlags = vk::ImageAspectFlagBits::eColor;

			if (mipLevels != (int)std::floor(std::log2(max(width, height))) + 1)
			{
				// Create new image resources that support mipmapping
				vk::ImageCreateInfo imageCreateInfo = vk::ImageCreateInfo()
					.setFlags(vk::ImageCreateFlags())
					.setImageType(vk::ImageType::e2D)
					.setFormat(TranslateStorageFormat(format))
					.setExtent(vk::Extent3D(width, height, 1))
					.setMipLevels(mipLevels)
					.setArrayLayers(1)
					.setSamples(SampleCount(samples))
					.setTiling(vk::ImageTiling::eOptimal)
					.setUsage(vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
					.setSharingMode(vk::SharingMode::eExclusive)
					.setQueueFamilyIndexCount(0)
					.setPQueueFamilyIndices(nullptr)
					.setInitialLayout(vk::ImageLayout::eUndefined);

				image = RendererState::Device().createImage(imageCreateInfo).value;

				vk::MemoryRequirements imageMemoryRequirements = RendererState::Device().getImageMemoryRequirements(image);

				vk::MemoryAllocateInfo imageAllocateInfo = vk::MemoryAllocateInfo()
					.setAllocationSize(imageMemoryRequirements.size)
					.setMemoryTypeIndex(GetMemoryType(imageMemoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

				memory = RendererState::Device().allocateMemory(imageAllocateInfo).value;

				RendererState::Device().bindImageMemory(image, memory, 0);

				vk::ImageSubresourceRange imageSubresourceRange = vk::ImageSubresourceRange()
					.setAspectMask(aspectFlags)
					.setBaseMipLevel(0)
					.setLevelCount(mipLevels)
					.setBaseArrayLayer(0)
					.setLayerCount(1);

				vk::ImageViewCreateInfo imageViewCreateInfo = vk::ImageViewCreateInfo()
					.setFlags(vk::ImageViewCreateFlags())
					.setImage(image)
					.setViewType(vk::ImageViewType::e2D)
					.setFormat(imageCreateInfo.format)
					.setComponents(vk::ComponentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA))//
					.setSubresourceRange(imageSubresourceRange);

				view = RendererState::Device().createImageView(imageViewCreateInfo).value;

				// Create blit copy regions
				std::vector<vk::ImageBlit> stagingBlitRegions(1);
				std::vector<vk::ImageBlit> blitRegions(mipLevels - 1);
				stagingBlitRegions[0] = vk::ImageBlit()
					.setSrcSubresource(vk::ImageSubresourceLayers().setAspectMask(aspectFlags).setMipLevel(0).setBaseArrayLayer(0).setLayerCount(1))
					.setSrcOffsets(std::array<vk::Offset3D, 2>{vk::Offset3D(0, 0, 0), vk::Offset3D(width, height, 1)})
					.setDstSubresource(vk::ImageSubresourceLayers().setAspectMask(aspectFlags).setMipLevel(0).setBaseArrayLayer(0).setLayerCount(1))
					.setDstOffsets(std::array<vk::Offset3D, 2>{vk::Offset3D(0, 0, 0), vk::Offset3D(width, height, 1)});
				for (int l = 1; l < mipLevels; l++)
				{
					blitRegions[l - 1] = vk::ImageBlit()
						.setSrcSubresource(vk::ImageSubresourceLayers().setAspectMask(aspectFlags).setMipLevel(l - 1).setBaseArrayLayer(0).setLayerCount(1))
						.setSrcOffsets(std::array<vk::Offset3D, 2>{vk::Offset3D(0, 0, 0), vk::Offset3D(max(1, width >> (l - 1)), max(1, height >> (l - 1)), 1)})
						.setDstSubresource(vk::ImageSubresourceLayers().setAspectMask(aspectFlags).setMipLevel(l).setBaseArrayLayer(0).setLayerCount(1))
						.setDstOffsets(std::array<vk::Offset3D, 2>{vk::Offset3D(0, 0, 0), vk::Offset3D(max(1, width >> l), max(1, height >> l), 1)});
				}

				// Create command buffer
				//TODO: Use CommandBuffer class?
				vk::CommandBuffer transferCommandBuffer = RendererState::CreateCommandBuffer(RendererState::TransferCommandPool());

				// Record command buffer
				vk::ImageSubresourceRange textureSubresourceRange = vk::ImageSubresourceRange()
					.setAspectMask(aspectFlags)
					.setBaseMipLevel(0)
					.setLevelCount(mipLevels)
					.setBaseArrayLayer(0)
					.setLayerCount(1);

				vk::ImageMemoryBarrier textureCopyBarrier = vk::ImageMemoryBarrier()
					.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eUndefined))
					.setDstAccessMask(LayoutFlags(vk::ImageLayout::eTransferDstOptimal))
					.setOldLayout(vk::ImageLayout::eUndefined)
					.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
					.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setImage(this->image)
					.setSubresourceRange(textureSubresourceRange);

				vk::ImageMemoryBarrier textureUsageBarrier = vk::ImageMemoryBarrier()
					.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eTransferDstOptimal))
					.setDstAccessMask(LayoutFlags(LayoutFromUsage(this->usage)))
					.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
					.setNewLayout(LayoutFromUsage(this->usage))
					.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setImage(this->image)
					.setSubresourceRange(textureSubresourceRange);

				vk::CommandBufferBeginInfo transferBeginInfo = vk::CommandBufferBeginInfo()
					.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
					.setPInheritanceInfo(nullptr);

				transferCommandBuffer.begin(transferBeginInfo);
				transferCommandBuffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eTopOfPipe,
					vk::PipelineStageFlagBits::eTopOfPipe,
					vk::DependencyFlags(),
					nullptr,
					nullptr,
					textureCopyBarrier
				);
				//TODO: Transition old image?
				//TODO: Transition new image?
				// Blit texture from old image to new resources
				transferCommandBuffer.blitImage(
					oldImage, currentLayout,
					image, vk::ImageLayout::eGeneral,
					stagingBlitRegions,
					vk::Filter::eNearest
				);
				// Blit texture to each mip level
				transferCommandBuffer.blitImage(
					image, vk::ImageLayout::eGeneral,
					image, vk::ImageLayout::eGeneral,
					blitRegions,
					vk::Filter::eLinear
				);
				transferCommandBuffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eTopOfPipe,
					vk::PipelineStageFlagBits::eTopOfPipe,
					vk::DependencyFlags(),
					nullptr,
					nullptr,
					textureUsageBarrier
				);
				transferCommandBuffer.end();

				// Submit to queue
				vk::SubmitInfo transferSubmitInfo = vk::SubmitInfo()
					.setWaitSemaphoreCount(0)
					.setPWaitSemaphores(nullptr)
					.setPWaitDstStageMask(nullptr)
					.setCommandBufferCount(1)
					.setPCommandBuffers(&transferCommandBuffer)
					.setSignalSemaphoreCount(0)
					.setPSignalSemaphores(nullptr);

				RendererState::TransferQueue().submit(transferSubmitInfo, vk::Fence());
				RendererState::TransferQueue().waitIdle(); //TODO: Remove
				RendererState::DestroyCommandBuffer(RendererState::TransferCommandPool(), transferCommandBuffer);

				// Free old resources
				RendererState::Device().destroyImageView(oldView);
				RendererState::Device().freeMemory(oldMemory);
				RendererState::Device().destroyImage(oldImage);

				this->currentLayout = LayoutFromUsage(this->usage);
			}
			else
			{
				// Create blit copy regions
				std::vector<vk::ImageBlit> blitRegions(mipLevels - 1);
				for (int l = 1; l < mipLevels; l++)
				{
					blitRegions[l - 1] = vk::ImageBlit()
						.setSrcSubresource(vk::ImageSubresourceLayers().setAspectMask(aspectFlags).setMipLevel(l - 1).setBaseArrayLayer(0).setLayerCount(1))
						.setSrcOffsets(std::array<vk::Offset3D, 2>{vk::Offset3D(0, 0, 0), vk::Offset3D(max(1, width >> (l - 1)), max(1, height >> (l - 1)), 1)})
						.setDstSubresource(vk::ImageSubresourceLayers().setAspectMask(aspectFlags).setMipLevel(l).setBaseArrayLayer(0).setLayerCount(1))
						.setDstOffsets(std::array<vk::Offset3D, 2>{vk::Offset3D(0, 0, 0), vk::Offset3D(max(1, width >> l), max(1, height >> l), 1)});
				}

				// Create command buffer
				//TODO: Use CommandBuffer class?
				vk::CommandBuffer transferCommandBuffer = RendererState::CreateCommandBuffer(RendererState::TransferCommandPool());

				// Record command buffer
				vk::ImageSubresourceRange textureSubresourceRange = vk::ImageSubresourceRange()
					.setAspectMask(aspectFlags)
					.setBaseMipLevel(0)
					.setLevelCount(mipLevels)
					.setBaseArrayLayer(0)
					.setLayerCount(1);

				vk::ImageMemoryBarrier textureCopyBarrier = vk::ImageMemoryBarrier()
					.setSrcAccessMask(LayoutFlags(currentLayout))
					.setDstAccessMask(LayoutFlags(vk::ImageLayout::eGeneral))
					.setOldLayout(currentLayout)
					.setNewLayout(vk::ImageLayout::eGeneral)
					.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setImage(this->image)
					.setSubresourceRange(textureSubresourceRange);

				vk::ImageMemoryBarrier textureUsageBarrier = vk::ImageMemoryBarrier()
					.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eGeneral))
					.setDstAccessMask(LayoutFlags(LayoutFromUsage(this->usage)))
					.setOldLayout(vk::ImageLayout::eGeneral)
					.setNewLayout(LayoutFromUsage(this->usage))
					.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setImage(this->image)
					.setSubresourceRange(textureSubresourceRange);

				vk::CommandBufferBeginInfo transferBeginInfo = vk::CommandBufferBeginInfo()
					.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
					.setPInheritanceInfo(nullptr);

				transferCommandBuffer.begin(transferBeginInfo);
				transferCommandBuffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eTopOfPipe,
					vk::PipelineStageFlagBits::eTopOfPipe,
					vk::DependencyFlags(),
					nullptr,
					nullptr,
					textureCopyBarrier
				);
				// Blit texture to each mip level
				transferCommandBuffer.blitImage(
					image, vk::ImageLayout::eGeneral,
					image, vk::ImageLayout::eGeneral,
					blitRegions,
					vk::Filter::eLinear
				);
				transferCommandBuffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eTopOfPipe,
					vk::PipelineStageFlagBits::eTopOfPipe,
					vk::DependencyFlags(),
					nullptr,
					nullptr,
					textureUsageBarrier
				);
				transferCommandBuffer.end();

				// Submit to queue
				vk::SubmitInfo transferSubmitInfo = vk::SubmitInfo()
					.setWaitSemaphoreCount(0)
					.setPWaitSemaphores(nullptr)
					.setPWaitDstStageMask(nullptr)
					.setCommandBufferCount(1)
					.setPCommandBuffers(&transferCommandBuffer)
					.setSignalSemaphoreCount(0)
					.setPSignalSemaphores(nullptr);

				RendererState::TransferQueue().submit(transferSubmitInfo, vk::Fence());
				RendererState::TransferQueue().waitIdle(); //TODO: Remove
				RendererState::DestroyCommandBuffer(RendererState::TransferCommandPool(), transferCommandBuffer);

				this->currentLayout = LayoutFromUsage(this->usage);
			}
		}
		void GetData(int mipLevel, void * data, int bufSize)
		{
			// Set up staging buffer and copy data to new image
			int bufferSize;
			if (format == StorageFormat::BC1 || format == StorageFormat::BC5)
			{
				int blocks = (int)(ceil(width / 4.0f) * ceil(height / 4.0f));
				bufferSize = format == StorageFormat::BC5 ? blocks * 16 : blocks * 8;
			}
			else
			{
				bufferSize = width * height * StorageFormatSize(format);
			}
			if (bufferSize > bufSize)
			{
				throw CoreLib::InvalidOperationException(L"buffer size is too small");
			}
			vk::BufferCreateInfo stagingBufferCreateInfo = vk::BufferCreateInfo()
				.setFlags(vk::BufferCreateFlags())
				.setSize(bufferSize)
				.setUsage(vk::BufferUsageFlagBits::eTransferDst)
				.setSharingMode(vk::SharingMode::eExclusive)
				.setQueueFamilyIndexCount(0)
				.setPQueueFamilyIndices(nullptr);

			vk::Buffer stagingBuffer = RendererState::Device().createBuffer(stagingBufferCreateInfo).value;

			vk::MemoryRequirements stagingBufferMemoryRequirements = RendererState::Device().getBufferMemoryRequirements(stagingBuffer);

			vk::MemoryAllocateInfo bufferAllocateInfo = vk::MemoryAllocateInfo()
				.setAllocationSize(stagingBufferMemoryRequirements.size)
				.setMemoryTypeIndex(GetMemoryType(stagingBufferMemoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible));

			vk::DeviceMemory stagingMemory = RendererState::Device().allocateMemory(bufferAllocateInfo).value;

			RendererState::Device().bindBufferMemory(stagingBuffer, stagingMemory, 0);

			// Get image aspect flags
			vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor;
			if (!!(usage & TextureUsage::ColorAttachment))
			{
				aspectFlags = vk::ImageAspectFlagBits::eColor;
			}
			else if (!!(usage & TextureUsage::DepthAttachment))
			{
				if (format == StorageFormat::Depth32)
					aspectFlags = vk::ImageAspectFlagBits::eDepth;
				else if (format == StorageFormat::Depth24Stencil8)
					aspectFlags = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
			}

			// Create buffer image copy information
			vk::BufferImageCopy stagingCopy = vk::BufferImageCopy()
				.setBufferOffset(0)
				.setBufferRowLength(0)
				.setBufferImageHeight(0)
				.setImageSubresource(vk::ImageSubresourceLayers().setAspectMask(aspectFlags).setMipLevel(mipLevel).setBaseArrayLayer(0).setLayerCount(1))
				.setImageOffset(vk::Offset3D())
				.setImageExtent(vk::Extent3D(max(1, width >> mipLevel), max(1, height >> mipLevel), 1));

			// Create command buffer
			//TODO: Use CommandBuffer class?
			vk::CommandBuffer transferCommandBuffer = RendererState::CreateCommandBuffer(RendererState::TransferCommandPool());

			// Record command buffer
			vk::ImageSubresourceRange textureSubresourceRange = vk::ImageSubresourceRange()
				.setAspectMask(aspectFlags)
				.setBaseMipLevel(0)
				.setLevelCount(mipLevels)
				.setBaseArrayLayer(0)
				.setLayerCount(1);

			vk::ImageMemoryBarrier textureCopyBarrier = vk::ImageMemoryBarrier()
				.setSrcAccessMask(LayoutFlags(currentLayout))
				.setDstAccessMask(LayoutFlags(vk::ImageLayout::eTransferSrcOptimal))
				.setOldLayout(currentLayout)
				.setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(this->image)
				.setSubresourceRange(textureSubresourceRange);

			vk::ImageMemoryBarrier textureUsageBarrier = vk::ImageMemoryBarrier()
				.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eTransferSrcOptimal))
				.setDstAccessMask(LayoutFlags(currentLayout))
				.setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
				.setNewLayout(currentLayout)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(this->image)
				.setSubresourceRange(textureSubresourceRange);

			vk::CommandBufferBeginInfo transferBeginInfo = vk::CommandBufferBeginInfo()
				.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
				.setPInheritanceInfo(nullptr);

			transferCommandBuffer.begin(transferBeginInfo);
			transferCommandBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTopOfPipe,
				vk::PipelineStageFlagBits::eTopOfPipe,
				vk::DependencyFlags(),
				nullptr,
				nullptr,
				textureCopyBarrier
			);
			transferCommandBuffer.copyImageToBuffer(
				this->image,
				vk::ImageLayout::eTransferSrcOptimal,
				stagingBuffer,
				stagingCopy
			);
			transferCommandBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTopOfPipe,
				vk::PipelineStageFlagBits::eTopOfPipe,
				vk::DependencyFlags(),
				nullptr,
				nullptr,
				textureUsageBarrier
			);
			transferCommandBuffer.end();

			// Submit to queue
			vk::SubmitInfo transferSubmitInfo = vk::SubmitInfo()
				.setWaitSemaphoreCount(0)
				.setPWaitSemaphores(nullptr)
				.setPWaitDstStageMask(nullptr)
				.setCommandBufferCount(1)
				.setPCommandBuffers(&transferCommandBuffer)
				.setSignalSemaphoreCount(0)
				.setPSignalSemaphores(nullptr);

			RendererState::TransferQueue().submit(transferSubmitInfo, vk::Fence());
			RendererState::TransferQueue().waitIdle(); //TODO: Remove
			RendererState::DestroyCommandBuffer(RendererState::TransferCommandPool(), transferCommandBuffer);

			// Map memory and copy
			assert(bufSize >= bufferSize);
			float* stagingMappedMemory = (float*)RendererState::Device().mapMemory(stagingMemory, 0, bufferSize, vk::MemoryMapFlags()).value;
			memcpy(data, stagingMappedMemory, bufferSize);
			RendererState::Device().unmapMemory(stagingMemory);

			// Destroy staging resources
			RendererState::Device().freeMemory(stagingMemory);
			RendererState::Device().destroyBuffer(stagingBuffer);
		}
	};

	class TextureSampler : public GameEngine::TextureSampler
	{
	public:
		vk::Sampler sampler;
		TextureFilter filter = TextureFilter::Nearest;
		WrapMode wrap = WrapMode::Repeat;
		CompareFunc op = CompareFunc::Never;

		void CreateSampler()
		{
			DestroySampler();

			// Default sampler create info
			vk::SamplerCreateInfo samplerCreateInfo = vk::SamplerCreateInfo()
				.setFlags(vk::SamplerCreateFlags())
				.setMinFilter(vk::Filter::eNearest)
				.setMagFilter(vk::Filter::eNearest)
				.setMipmapMode(vk::SamplerMipmapMode::eNearest)
				.setAddressModeU(vk::SamplerAddressMode::eRepeat)
				.setAddressModeV(vk::SamplerAddressMode::eRepeat)
				.setAddressModeW(vk::SamplerAddressMode::eRepeat)
				.setMipLodBias(0.0f)
				.setAnisotropyEnable(VK_FALSE)
				.setMaxAnisotropy(1.0f)
				.setCompareEnable(op != CompareFunc::Disabled)
				.setCompareOp(TranslateCompareFunc(op))
				.setMinLod(0.0f)
				.setMaxLod(0.0f)
				.setBorderColor(vk::BorderColor::eFloatTransparentBlack)
				.setUnnormalizedCoordinates(VK_FALSE);

			// Modify create info based on parameters
			switch (filter)
			{
			case TextureFilter::Nearest:
				samplerCreateInfo.setMinFilter(vk::Filter::eNearest);
				samplerCreateInfo.setMagFilter(vk::Filter::eNearest);
				break;
			case TextureFilter::Linear:
				samplerCreateInfo.setMinFilter(vk::Filter::eLinear);
				samplerCreateInfo.setMagFilter(vk::Filter::eLinear);
				break;
			case TextureFilter::Trilinear:
				samplerCreateInfo.setMinFilter(vk::Filter::eLinear);
				samplerCreateInfo.setMagFilter(vk::Filter::eLinear);
				samplerCreateInfo.setMipmapMode(vk::SamplerMipmapMode::eLinear);
				samplerCreateInfo.setMaxLod(20.0f);
				break;
			case TextureFilter::Anisotropic4x:
				samplerCreateInfo.setMinFilter(vk::Filter::eLinear);
				samplerCreateInfo.setMagFilter(vk::Filter::eLinear);
				samplerCreateInfo.setMipmapMode(vk::SamplerMipmapMode::eLinear);
				samplerCreateInfo.setMaxLod(20.0f);
				samplerCreateInfo.setAnisotropyEnable(VK_TRUE);
				samplerCreateInfo.setMaxAnisotropy(4.0f);
				break;
			case TextureFilter::Anisotropic8x:
				samplerCreateInfo.setMinFilter(vk::Filter::eLinear);
				samplerCreateInfo.setMagFilter(vk::Filter::eLinear);
				samplerCreateInfo.setMipmapMode(vk::SamplerMipmapMode::eLinear);
				samplerCreateInfo.setMaxLod(20.0f);
				samplerCreateInfo.setAnisotropyEnable(VK_TRUE);
				samplerCreateInfo.setMaxAnisotropy(8.0f);
				break;
			case TextureFilter::Anisotropic16x:
				samplerCreateInfo.setMinFilter(vk::Filter::eLinear);
				samplerCreateInfo.setMagFilter(vk::Filter::eLinear);
				samplerCreateInfo.setMipmapMode(vk::SamplerMipmapMode::eLinear);
				samplerCreateInfo.setMaxLod(20.0f);
				samplerCreateInfo.setAnisotropyEnable(VK_TRUE);
				samplerCreateInfo.setMaxAnisotropy(16.0f);
				break;
			}

			switch (wrap)
			{
			case WrapMode::Clamp:
				samplerCreateInfo.setAddressModeU(vk::SamplerAddressMode::eClampToEdge);
				samplerCreateInfo.setAddressModeV(vk::SamplerAddressMode::eClampToEdge);
				samplerCreateInfo.setAddressModeW(vk::SamplerAddressMode::eClampToEdge);
				break;
			case WrapMode::Repeat:
				samplerCreateInfo.setAddressModeU(vk::SamplerAddressMode::eRepeat);
				samplerCreateInfo.setAddressModeV(vk::SamplerAddressMode::eRepeat);
				samplerCreateInfo.setAddressModeW(vk::SamplerAddressMode::eRepeat);
				break;
			case WrapMode::Mirror:
				samplerCreateInfo.setAddressModeU(vk::SamplerAddressMode::eMirroredRepeat);
				samplerCreateInfo.setAddressModeV(vk::SamplerAddressMode::eMirroredRepeat);
				samplerCreateInfo.setAddressModeW(vk::SamplerAddressMode::eMirroredRepeat);
				break;
			}

			sampler = RendererState::Device().createSampler(samplerCreateInfo).value;
		}
		void DestroySampler()
		{
			if (sampler) RendererState::Device().destroySampler(sampler);
		}
	public:
		TextureSampler()
		{
			CreateSampler();
		}
		~TextureSampler()
		{
			DestroySampler();
		}

		TextureFilter GetFilter()
		{
			return filter;
		}
		void SetFilter(TextureFilter pfilter)
		{
			filter = pfilter;

			// If anisotropy is not supported and we want anisotropic filtering, make trilinear filter
			if (!RendererState::PhysicalDevice().getFeatures().samplerAnisotropy)
			{
				if (filter == TextureFilter::Anisotropic4x ||
					filter == TextureFilter::Anisotropic8x ||
					filter == TextureFilter::Anisotropic16x)
				{
					CoreLib::Diagnostics::Debug::WriteLine(L"Anisotropic filtering is not supported. Changing to trilinear.");
					filter = TextureFilter::Trilinear;
				}
			}
			else
			{
				TextureFilter oldFilter = filter;
				float maxAnisotropy = RendererState::PhysicalDevice().getProperties().limits.maxSamplerAnisotropy;

				switch (filter)
				{
				case TextureFilter::Anisotropic16x:
					if (maxAnisotropy < 16.0f) filter = TextureFilter::Anisotropic8x;
					// Fall through
				case TextureFilter::Anisotropic8x:
					if (maxAnisotropy < 8.0f) filter = TextureFilter::Anisotropic4x;
					// Fall through
				case TextureFilter::Anisotropic4x:
					if (maxAnisotropy < 4.0f) filter = TextureFilter::Trilinear;
					// Fall through
				default:
					break;
				}

				if (oldFilter != filter)
				{
					CoreLib::Diagnostics::Debug::Write(L"Max supported anisotropy is ");
					CoreLib::Diagnostics::Debug::WriteLine(maxAnisotropy);
				}
			}

			CreateSampler();
		}
		WrapMode GetWrapMode()
		{
			return wrap;
		}
		void SetWrapMode(WrapMode pwrap)
		{
			this->wrap = pwrap;
			CreateSampler();
		}
		CompareFunc GetCompareFunc()
		{
			return op;
		}
		void SetDepthCompare(CompareFunc pop)
		{
			this->op = pop;
			CreateSampler();
		}
	};

	class BufferObject : public GameEngine::Buffer
	{
		friend class HardwareRenderer;
	public:
		vk::Buffer buffer;
		vk::DeviceMemory memory;
		vk::BufferUsageFlags usage;
		vk::MemoryPropertyFlags location;
		int mapOffset = 0;
		int mapSize = -1;
		int size = 0;
		int backingSize = 0;

		void CreateBuffer(int psize)
		{
			vk::BufferCreateInfo bufferCreateInfo = vk::BufferCreateInfo()
				.setFlags(vk::BufferCreateFlags())
				.setSize(psize)
				.setUsage(usage)
				.setSharingMode(vk::SharingMode::eExclusive)
				.setQueueFamilyIndexCount(0)
				.setPQueueFamilyIndices(nullptr);

			this->buffer = RendererState::Device().createBuffer(bufferCreateInfo).value;

			vk::MemoryRequirements memoryRequirements = RendererState::Device().getBufferMemoryRequirements(this->buffer);
			this->backingSize = (int)memoryRequirements.size;

			this->size = this->backingSize;

			vk::BufferCreateInfo fullsizeBufferCreateInfo = vk::BufferCreateInfo()
				.setFlags(vk::BufferCreateFlags())
				.setSize(this->backingSize)
				.setUsage(usage)
				.setSharingMode(vk::SharingMode::eExclusive)
				.setQueueFamilyIndexCount(0)
				.setPQueueFamilyIndices(nullptr);

			RendererState::Device().destroyBuffer(this->buffer);
			this->buffer = RendererState::Device().createBuffer(fullsizeBufferCreateInfo).value;

			vk::MemoryRequirements fullsizeMemoryRequirements = RendererState::Device().getBufferMemoryRequirements(this->buffer);

			vk::MemoryAllocateInfo fullsizeMemoryAllocateInfo = vk::MemoryAllocateInfo()
				.setAllocationSize(fullsizeMemoryRequirements.size)
				.setMemoryTypeIndex(GetMemoryType(fullsizeMemoryRequirements.memoryTypeBits, location));

			this->memory = RendererState::Device().allocateMemory(fullsizeMemoryAllocateInfo).value;
			RendererState::Device().bindBufferMemory(this->buffer, this->memory, 0);
		}
		void DestroyBuffer()
		{
			if (this->buffer) RendererState::Device().destroyBuffer(this->buffer);
			if (this->memory) RendererState::Device().freeMemory(this->memory);
		}
		void Resize(int psize)
		{
			this->size = psize;

			if (size <= backingSize)
				return;

			DestroyBuffer();
			CreateBuffer(psize);
		}
	public:
		BufferObject(vk::BufferUsageFlags usage, vk::MemoryPropertyFlags location)
		{
			//TODO: Should pass as input a requiredLocation and an optimalLocation?
			this->location = location;
			this->usage = usage;
		}
		~BufferObject()
		{
			DestroyBuffer();
		}
		const vk::Buffer& Buffer()
		{
			return this->buffer;
		}
		void SetData(int offset, void* data, int psize)
		{
			// If we can't map the buffer, we need to specify it will be a transfer dest
			if (!(location & vk::MemoryPropertyFlagBits::eHostVisible))
				this->usage |= vk::BufferUsageFlagBits::eTransferDst;

			Resize(offset + psize);

			if (data == nullptr) return;

			//auto transferBegin = CoreLib::Diagnostics::PerformanceCounter::Start();

			// If the buffer is mappable, map and memcpy
			if (location & vk::MemoryPropertyFlagBits::eHostVisible)
			{
				//TODO: Should memcpy + flush in chunks?
				void* mappedMemory = Map(offset, psize);
				memcpy(mappedMemory, data, psize);
				Flush();
				Unmap();
			}
			// Otherwise, we need to use command buffers
			else
			{
				// Create command buffer
				//TODO: Should this use a global buffer? How to handle thread safety?
				vk::CommandBuffer transferCommandBuffer = RendererState::CreateCommandBuffer(RendererState::TransferCommandPool());

				//TODO: Improve switching logic
				bool staging = psize > (1 << 18);
				if (staging)
				{
					// Create staging buffer
					vk::BufferCreateInfo stagingCreateInfo = vk::BufferCreateInfo()
						.setFlags(vk::BufferCreateFlags())
						.setSize(psize)
						.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
						.setSharingMode(vk::SharingMode::eExclusive)
						.setQueueFamilyIndexCount(0)
						.setPQueueFamilyIndices(nullptr);

					vk::Buffer stagingBuffer = RendererState::Device().createBuffer(stagingCreateInfo).value;

					vk::MemoryRequirements stagingMemoryRequirements = RendererState::Device().getBufferMemoryRequirements(stagingBuffer);

					vk::MemoryAllocateInfo stagingMemoryAllocateInfo = vk::MemoryAllocateInfo()
						.setAllocationSize(stagingMemoryRequirements.size)
						.setMemoryTypeIndex(GetMemoryType(stagingMemoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible));

					vk::DeviceMemory stagingMemory = RendererState::Device().allocateMemory(stagingMemoryAllocateInfo).value;
					RendererState::Device().bindBufferMemory(stagingBuffer, stagingMemory, 0);

					// Copy data to staging buffer
					void* stagingMappedMemory = RendererState::Device().mapMemory(stagingMemory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags()).value;
					memcpy(stagingMappedMemory, data, psize);
					RendererState::Device().unmapMemory(stagingMemory);

					// Create copy region description
					vk::BufferCopy transferRegion = vk::BufferCopy()
						.setSrcOffset(0)
						.setDstOffset(offset)
						.setSize(psize);

					// Record command buffer
					vk::CommandBufferBeginInfo transferBeginInfo = vk::CommandBufferBeginInfo()
						.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
						.setPInheritanceInfo(nullptr);

					transferCommandBuffer.begin(transferBeginInfo);
					transferCommandBuffer.copyBuffer(stagingBuffer, this->buffer, transferRegion);
					transferCommandBuffer.end();

					// Submit to queue
					vk::SubmitInfo transferSubmitInfo = vk::SubmitInfo()
						.setWaitSemaphoreCount(0)
						.setPWaitSemaphores(nullptr)
						.setPWaitDstStageMask(nullptr)
						.setCommandBufferCount(1)
						.setPCommandBuffers(&transferCommandBuffer)
						.setSignalSemaphoreCount(0)
						.setPSignalSemaphores(nullptr);

					RendererState::TransferQueue().submit(transferSubmitInfo, vk::Fence());
					RendererState::TransferQueue().waitIdle(); //TODO: Remove
					RendererState::DestroyCommandBuffer(RendererState::TransferCommandPool(), transferCommandBuffer);

					// Destroy staging resources
					RendererState::Device().freeMemory(stagingMemory);
					RendererState::Device().destroyBuffer(stagingBuffer);
				}
				else
				{
					//TEMP
					assert(psize % 4 == 0);
					assert(offset % 4 == 0);
					//ENDTEMP

					// Record command buffer
					vk::CommandBufferBeginInfo transferBeginInfo = vk::CommandBufferBeginInfo()
						.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
						.setPInheritanceInfo(nullptr);

					transferCommandBuffer.begin(transferBeginInfo);
					int remainingSize = psize;
					while (remainingSize > 65536)
					{
						transferCommandBuffer.updateBuffer(this->buffer, offset, 65536, (uint32_t*)((char*)data + offset));
						remainingSize -= 65536;
						offset += 65536;
					}
					transferCommandBuffer.updateBuffer(this->buffer, offset, remainingSize, (uint32_t*)((char*)data + offset));
					transferCommandBuffer.end();

					// Submit to queue
					vk::SubmitInfo transferSubmitInfo = vk::SubmitInfo()
						.setWaitSemaphoreCount(0)
						.setPWaitSemaphores(nullptr)
						.setPWaitDstStageMask(nullptr)
						.setCommandBufferCount(1)
						.setPCommandBuffers(&transferCommandBuffer)
						.setSignalSemaphoreCount(0)
						.setPSignalSemaphores(nullptr);

					RendererState::TransferQueue().submit(transferSubmitInfo, vk::Fence());
					RendererState::TransferQueue().waitIdle(); //TODO: Remove
					RendererState::DestroyCommandBuffer(RendererState::TransferCommandPool(), transferCommandBuffer);
				}
			}

			//auto transferEnd = CoreLib::Diagnostics::PerformanceCounter::End(transferBegin);
			//double elapsedTime = CoreLib::Diagnostics::PerformanceCounter::ToSeconds(transferEnd);

			//#if _DEBUG
			//			// Print information about transfer rate
			//			CoreLib::Array<CoreLib::String, 4> granularity;
			//			granularity.Add(L"B");
			//			granularity.Add(L"KB");
			//			granularity.Add(L"MB");
			//			granularity.Add(L"GB");
			//
			//			int count = 0;
			//			float dataSize = (float)size;
			//			while (dataSize > 1024.0f) {
			//				dataSize /= 1024.0f;
			//				count++;
			//				if (count == 3) break;
			//			}
			//
			//			CoreLib::Diagnostics::Debug::Write(L"Transfer time (");
			//			if(count == 0) CoreLib::Diagnostics::Debug::Write(CoreLib::String(size) + L" " + granularity[count] + L"): ");
			//			else CoreLib::Diagnostics::Debug::Write(CoreLib::String(dataSize, L"%.2f") + L" " + granularity[count] + L"): ");
			//			CoreLib::Diagnostics::Debug::Write(CoreLib::String(elapsedTime*1000.0f, L"%.2f") + L"ms");
			//			CoreLib::Diagnostics::Debug::WriteLine(L" (" + CoreLib::String((float)size / elapsedTime / 1024.0f / 1024.0f / 1024.0f, L"%.2f") + L" GB/s)");
			//#endif
		}
		void SetData(void* data, int psize)
		{
			SetData(0, data, psize);
		}
		void GetData(int /*offset*/, int /*psize*/)
		{
			//TODO: Implement
		}
		void GetData()
		{
			GetData(0, -1);
		}
		int GetSize()
		{
			return size;
		}
		void* Map(int offset, int psize)
		{
			//TEMP
			assert(buffer);
			assert(this->mapSize == -1);
			//ENDTEMP

			// Buffer must have host visible flag to be mappable
			if (!(location & vk::MemoryPropertyFlagBits::eHostVisible))
				return nullptr;

			//TODO: We need to guarantee that all previous commands writing to this range have completed
			//      Can probably do this by associating each buffer with a fence, and waiting on the fence

			//TODO: Lock mutex for memory
			this->mapOffset = offset;
			this->mapSize = psize;

			return RendererState::Device().mapMemory(memory, offset, psize, vk::MemoryMapFlags()).value;
		}
		void* Map()
		{
			return Map(0, this->size);
		}
		// If the buffer is not host coherent, we need to flush to guarantee writes visible to device
		void Flush(int offset, int psize)
		{
			assert(this->mapSize >= 0);

			// We only need to flush if not host coherent
			if (!(location & vk::MemoryPropertyFlagBits::eHostCoherent))
			{
				//TEMP
				assert(offset >= this->mapOffset);
				assert(offset % (RendererState::PhysicalDevice().getProperties().limits.nonCoherentAtomSize) == 0);
				assert(psize % (RendererState::PhysicalDevice().getProperties().limits.nonCoherentAtomSize) == 0);
				assert(psize <= this->mapSize || (this->mapSize == -1 && psize <= (this->size - offset)));
				//TODO: Round down offset and round up size?
				//ENDTEMP

				//TODO: We need to make sure we do this before command buffers using the memory are submitted

				vk::MappedMemoryRange memoryRange = vk::MappedMemoryRange()
					.setMemory(this->memory)
					.setOffset(offset)
					.setSize(psize);

				RendererState::Device().flushMappedMemoryRanges(memoryRange);
			}
		}
		void Flush()
		{
			Flush(this->mapOffset, this->mapSize);
		}
		void Unmap()
		{
			assert(this->mapSize >= 0);
			RendererState::Device().unmapMemory(memory);
			this->mapSize = -1;
			this->mapOffset = 0;
			//TODO: unlock mutex
		}
	};

	class Shader : public GameEngine::Shader
	{
	public:
		ShaderType stage;
		vk::ShaderModule module;

		Shader() {}
		~Shader()
		{
			Destroy();
		}

		void Create(ShaderType pstage, const char* data, int size)
		{
			Destroy();

			this->stage = pstage;

			vk::ShaderModuleCreateInfo shaderModuleCreateInfo = vk::ShaderModuleCreateInfo()
				.setFlags(vk::ShaderModuleCreateFlags())
				.setCodeSize(size)
				.setPCode((uint32_t*)data);

			this->module = RendererState::Device().createShaderModule(shaderModuleCreateInfo).value;
		}

		void Destroy()
		{
			if (module) RendererState::Device().destroyShaderModule(module);
			module = vk::ShaderModule();
		}
	};

	class RenderTargetLayout;

	class FrameBuffer : public GameEngine::FrameBuffer
	{
	public:
		int width;
		int height;
		vk::Framebuffer framebuffer;

		FrameBuffer() {};
		~FrameBuffer()
		{
			if (framebuffer) RendererState::Device().destroyFramebuffer(framebuffer);
		}
	};

	vk::AttachmentLoadOp TranslateLoadOp(LoadOp op)
	{
		switch (op)
		{
		case LoadOp::Load: return vk::AttachmentLoadOp::eLoad;
		case LoadOp::Clear: return vk::AttachmentLoadOp::eClear;
		case LoadOp::DontCare: return vk::AttachmentLoadOp::eDontCare;
		default: throw CoreLib::NotImplementedException();
		}
	}

	vk::AttachmentStoreOp TranslateStoreOp(StoreOp op)
	{
		switch (op)
		{
		case StoreOp::Store: return vk::AttachmentStoreOp::eStore;
		case StoreOp::DontCare: return vk::AttachmentStoreOp::eDontCare;
		default: throw CoreLib::NotImplementedException();
		}
	}

	class RenderTargetLayout : public GameEngine::RenderTargetLayout
	{
	public:
		CoreLib::List<vk::AttachmentDescription> descriptions;
		CoreLib::List<vk::AttachmentReference> colorReferences;
		vk::AttachmentReference depthReference;
		vk::RenderPass renderPass;
	private:
		void Resize(int size)
		{
			if (descriptions.Count() < size)
				descriptions.SetSize(size);
		}

		void SetColorAttachment(int binding, LoadOp loadOp = LoadOp::Load, StoreOp storeOp = StoreOp::Store)
		{
			Resize(binding + 1);

			descriptions[binding] = vk::AttachmentDescription()
				.setFlags(vk::AttachmentDescriptionFlags())
				.setFormat(vk::Format::eR8G8B8A8Unorm)//
				.setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)//
				.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)//
				.setSamples(SampleCount(1))
				.setLoadOp(TranslateLoadOp(loadOp))
				.setStoreOp(TranslateStoreOp(storeOp))
				.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)//
				.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);//

																	 //if (samples > 1)
																	 //{
																	 //	//TODO: need to resolve?
																	 //}
		}

		void SetDepthAttachment(int binding, LoadOp loadOp = LoadOp::Load, StoreOp storeOp = StoreOp::Store)
		{
			if (depthReference.layout != vk::ImageLayout::eUndefined)
				throw HardwareRendererException(L"Only 1 depth/stencil attachment allowed.");

			Resize(binding + 1);

			descriptions[binding] = vk::AttachmentDescription()
				.setFlags(vk::AttachmentDescriptionFlags())
				.setFormat(vk::Format::eD32Sfloat)
				.setInitialLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
				.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
				.setSamples(SampleCount(1))
				.setLoadOp(TranslateLoadOp(loadOp))
				.setStoreOp(TranslateStoreOp(storeOp))
				.setStencilLoadOp(TranslateLoadOp(loadOp))
				.setStencilStoreOp(TranslateStoreOp(storeOp));
		}
	public:
		RenderTargetLayout(CoreLib::ArrayView<TextureUsage> bindings)
		{
			depthReference.attachment = VK_ATTACHMENT_UNUSED;

			int location = 0;
			for (auto binding : bindings)
			{
				switch (binding)
				{
				case TextureUsage::ColorAttachment:
				case TextureUsage::SampledColorAttachment:
					SetColorAttachment(location);
					break;
				case TextureUsage::DepthAttachment:
				case TextureUsage::SampledDepthAttachment:
					SetDepthAttachment(location);
					break;
				case TextureUsage::Unused:
					break;
				default:
					throw HardwareRendererException(L"Unsupported attachment usage");
				}
				location++;
			}

			int binding = 0;
			for (auto description : descriptions)
			{
				if (description.initialLayout == vk::ImageLayout::eColorAttachmentOptimal)
					colorReferences.Add(vk::AttachmentReference(binding, vk::ImageLayout::eColorAttachmentOptimal));
				else if (description.initialLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
					depthReference = vk::AttachmentReference(binding, vk::ImageLayout::eDepthStencilAttachmentOptimal);

				binding++;
			}

			// Create Subpass Descriptions
			//TODO: Subpasses need to be implemented
			CoreLib::List<vk::SubpassDescription> subpassDescriptions;
			subpassDescriptions.Add(
				vk::SubpassDescription()
				.setFlags(vk::SubpassDescriptionFlags())
				.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
				.setInputAttachmentCount(0)
				.setPInputAttachments(nullptr)
				.setColorAttachmentCount(colorReferences.Count())
				.setPColorAttachments(colorReferences.Buffer())
				.setPResolveAttachments(nullptr)
				.setPDepthStencilAttachment(&depthReference)
				.setPreserveAttachmentCount(0)
				.setPPreserveAttachments(nullptr)
			);

			// Create RenderPass
			vk::RenderPassCreateInfo renderPassCreateInfo = vk::RenderPassCreateInfo()
				.setFlags(vk::RenderPassCreateFlags())
				.setAttachmentCount(descriptions.Count())
				.setPAttachments(descriptions.Buffer())
				.setSubpassCount(subpassDescriptions.Count())
				.setPSubpasses(subpassDescriptions.Buffer())
				.setDependencyCount(0)
				.setPDependencies(nullptr);

			this->renderPass = RendererState::Device().createRenderPass(renderPassCreateInfo).value;
		}
		~RenderTargetLayout()
		{
			if (renderPass) RendererState::Device().destroyRenderPass(renderPass);
		}

		virtual FrameBuffer* CreateFrameBuffer(const RenderAttachments& renderAttachments) override
		{
#if _DEBUG
			// Ensure the RenderAttachments are compatible with this RenderTargetLayout
			for (auto colorReference : colorReferences)
			{
				if (!(dynamic_cast<Texture2D*>(renderAttachments.attachments[colorReference.attachment])->usage & TextureUsage::ColorAttachment))
					throw HardwareRendererException(L"Incompatible RenderTargetLayout and RenderAttachments");
			}
			if (depthReference.layout != vk::ImageLayout::eUndefined)
			{
				if (!(dynamic_cast<Texture2D*>(renderAttachments.attachments[depthReference.attachment])->usage & TextureUsage::DepthAttachment))
					throw HardwareRendererException(L"Incompatible RenderTargetLayout and RenderAttachments");
			}
#endif
			FrameBuffer* result = new FrameBuffer();

			CoreLib::List<vk::ImageView> framebufferAttachmentViews;
			for (auto attachment : renderAttachments.attachments)
				framebufferAttachmentViews.Add(dynamic_cast<Texture2D*>(attachment)->view);

			vk::FramebufferCreateInfo framebufferCreateInfo = vk::FramebufferCreateInfo()
				.setFlags(vk::FramebufferCreateFlags())
				.setRenderPass(renderPass)
				.setAttachmentCount(framebufferAttachmentViews.Count())
				.setPAttachments(framebufferAttachmentViews.Buffer())
				.setWidth(renderAttachments.width)
				.setHeight(renderAttachments.height)
				.setLayers(1);

			result->framebuffer = RendererState::Device().createFramebuffer(framebufferCreateInfo).value;
			result->width = renderAttachments.width;
			result->height = renderAttachments.height;

			return result;
		}
	};

	class PipelineBuilder;
	class Pipeline;

	class PipelineInstance : public GameEngine::PipelineInstance
	{
	public:
		vk::PipelineLayout& pipelineLayout;//TODO:
		vk::Pipeline& pipeline;//TODO:
		PipelineBinding pipelineBindings;
		vk::DescriptorPool descriptorPool;
		vk::DescriptorSet descriptorSet;
		CoreLib::List<vk::DescriptorBufferInfo> boundBufferInfo;
	public:
		PipelineInstance(Pipeline* pipeline, const PipelineBinding& pipelineBinding);
		~PipelineInstance()
		{
			if (descriptorSet) RendererState::Device().freeDescriptorSets(descriptorPool, descriptorSet);
		}

		void Update()
		{
			if (boundBufferInfo.Count() <= 0) return;

			// Update Descriptor Set
			CoreLib::List<vk::DescriptorBufferInfo> bufferDescriptorInfo;
			CoreLib::List<int> bindingLocations;
			CoreLib::List<BindingType> bindingTypes;
			for (auto binding : pipelineBindings.GetBindings())
			{
				switch (binding.type)
				{
				case BindingType::StorageBuffer:
				{
					int range = binding.buf.range;
					if (range == 0) range = dynamic_cast<VK::BufferObject*>(binding.buf.buffer)->backingSize;

					bufferDescriptorInfo.Add(
						vk::DescriptorBufferInfo()
						.setBuffer(dynamic_cast<VK::BufferObject*>(binding.buf.buffer)->buffer)
						.setOffset(binding.buf.offset)
						.setRange(range)
					);
					bindingLocations.Add(binding.location);
					bindingTypes.Add(BindingType::StorageBuffer);
					break;
				}
				case BindingType::UniformBuffer:
				{
					int range = binding.buf.range;
					if (range == 0) range = dynamic_cast<VK::BufferObject*>(binding.buf.buffer)->backingSize;

					bufferDescriptorInfo.Add(
						vk::DescriptorBufferInfo()
						.setBuffer(dynamic_cast<VK::BufferObject*>(binding.buf.buffer)->buffer)
						.setOffset(binding.buf.offset)
						.setRange(range)
					);
					bindingLocations.Add(binding.location);
					bindingTypes.Add(BindingType::UniformBuffer);
					break;
				}
				case BindingType::Texture:
				case BindingType::Unused:
					break;
				default:
					throw HardwareRendererException(L"Invalid binding type");
				}
			}

			std::vector<vk::WriteDescriptorSet> writeDescriptorSets;
			for (int k = 0; k < bufferDescriptorInfo.Count(); k++) {
				if (bufferDescriptorInfo[k].buffer != boundBufferInfo[k].buffer)
				{
					//TODO: We can't bind a VK_NULL_HANDLE
					if (!bufferDescriptorInfo[k].buffer) break;

					writeDescriptorSets.push_back(
						vk::WriteDescriptorSet()
						.setDstSet(this->descriptorSet)
						.setDstBinding(bindingLocations[k])
						.setDstArrayElement(0)
						.setDescriptorCount(1)
						.setDescriptorType(TranslateBindingType(bindingTypes[k]))
						.setPImageInfo(nullptr)
						.setPBufferInfo(&bufferDescriptorInfo[k])
						.setPTexelBufferView(nullptr)
					);
				}
			}

			boundBufferInfo.SwapWith(bufferDescriptorInfo);
			if (writeDescriptorSets.size() > 0)
				RendererState::Device().updateDescriptorSets(writeDescriptorSets, nullptr);
		}
	};

	class Pipeline : public GameEngine::Pipeline
	{
	public:
		CoreLib::List<vk::DescriptorSetLayout> descriptorSetLayouts;
		CoreLib::List<vk::DescriptorPoolSize> descriptorPoolSizes;
		vk::PipelineLayout pipelineLayout;
		vk::Pipeline pipeline;
	public:
		Pipeline(RenderTargetLayout* renderTargetLayout, PipelineBuilder* pipelineBuilder);
		~Pipeline()
		{
			if (pipeline) RendererState::Device().destroyPipeline(pipeline);
			if (pipelineLayout) RendererState::Device().destroyPipelineLayout(pipelineLayout);
			for (auto descriptorSetLayout : descriptorSetLayouts)
				RendererState::Device().destroyDescriptorSetLayout(descriptorSetLayout);
		}

		virtual PipelineInstance* CreateInstance(const PipelineBinding& pipelineBinding) override
		{
			return new PipelineInstance(this, pipelineBinding);
		}
	};

	class PipelineBuilder : public GameEngine::PipelineBuilder
	{
	public:
		CoreLib::List<vk::DescriptorSetLayoutBinding> layoutBindings;
		CoreLib::List<vk::PushConstantRange> pushConstantRanges;//TODO:
		CoreLib::List<vk::PipelineShaderStageCreateInfo> shaderStages;
		CoreLib::List<vk::VertexInputBindingDescription> vertexBindingDescriptions;
		CoreLib::List<vk::VertexInputAttributeDescription> vertexAttributeDescriptions;
	public:
		virtual void SetShaders(CoreLib::ArrayView<GameEngine::Shader*> shaders) override
		{
			shaderStages.Clear();

#if _DEBUG
			bool vertPresent = false;
			bool tescControlPresent = false;
			bool tescEvalPresent = false;
			bool geometryPresent = false;
			bool fragPresent = false;
			bool computePresent = false;
#endif
			for (auto shader : shaders)
			{
#if _DEBUG
				// Ensure that the device supports requested shader stages
				if (ShaderStage(dynamic_cast<VK::Shader*>(shader)->stage) == vk::ShaderStageFlagBits::eGeometry)
					assert(RendererState::PhysicalDevice().getFeatures().geometryShader);
				if (ShaderStage(dynamic_cast<VK::Shader*>(shader)->stage) == vk::ShaderStageFlagBits::eTessellationControl)
					assert(RendererState::PhysicalDevice().getFeatures().tessellationShader);
				if (ShaderStage(dynamic_cast<VK::Shader*>(shader)->stage) == vk::ShaderStageFlagBits::eTessellationEvaluation)
					assert(RendererState::PhysicalDevice().getFeatures().tessellationShader);

				// Ensure only one of any shader stage is present in the requested shader stage
				switch (ShaderStage(dynamic_cast<VK::Shader*>(shader)->stage))
				{
				case vk::ShaderStageFlagBits::eVertex:
					assert(vertPresent == false);
					vertPresent = true;
					break;
				case vk::ShaderStageFlagBits::eTessellationControl:
					assert(tescControlPresent == false);
					tescControlPresent = true;
					break;
				case vk::ShaderStageFlagBits::eTessellationEvaluation:
					assert(tescEvalPresent == false);
					tescEvalPresent = true;
					break;
				case vk::ShaderStageFlagBits::eGeometry:
					assert(geometryPresent == false);
					geometryPresent = true;
					break;
				case vk::ShaderStageFlagBits::eFragment:
					assert(fragPresent == false);
					fragPresent = true;
					break;
				case vk::ShaderStageFlagBits::eCompute:
					assert(computePresent == false);
					computePresent = true;
					break;
				default:
					throw HardwareRendererException(L"Unknown shader stage");
				}
#endif

				shaderStages.Add(
					vk::PipelineShaderStageCreateInfo()
					.setFlags(vk::PipelineShaderStageCreateFlagBits())
					.setStage(ShaderStage(dynamic_cast<VK::Shader*>(shader)->stage))
					.setModule(dynamic_cast<VK::Shader*>(shader)->module)
					.setPName("main")
					.setPSpecializationInfo(nullptr)
				);
			}
		}
		virtual void SetVertexLayout(VertexFormat vertexFormat) override
		{
			//TODO: Improve
			int location = 0;
			int maxOffset = -1;
			int stride = 0;

			vertexAttributeDescriptions.Clear();
			vertexBindingDescriptions.Clear();

			for (auto attribute : vertexFormat.Attributes)
			{
				vertexAttributeDescriptions.Add(
					vk::VertexInputAttributeDescription()
					.setLocation(attribute.Location)
					.setBinding(0)
					.setFormat(TranslateVertexAttribute(attribute))
					.setOffset(attribute.StartOffset)
				);

				location++;
				if (attribute.StartOffset > maxOffset)
				{
					maxOffset = attribute.StartOffset;
					stride = maxOffset + DataTypeSize(attribute.Type);
				}
			}

			if (vertexAttributeDescriptions.Count() > 0)
			{
				vertexBindingDescriptions.Add(
					vk::VertexInputBindingDescription()
					.setBinding(0)
					.setStride(stride)
					.setInputRate(vk::VertexInputRate::eVertex)//TODO: per instance data?
				);
			}
		}
		virtual void SetBindingLayout(int bindingId, BindingType bindType) override
		{
			if (bindType == BindingType::Unused) return;//TODO: Should do something else?

			layoutBindings.Add(
				vk::DescriptorSetLayoutBinding()
				.setBinding(bindingId)
				.setDescriptorType(TranslateBindingType(bindType))
				.setDescriptorCount(1)
				.setStageFlags(vk::ShaderStageFlagBits::eAllGraphics)//TODO: improve
				.setPImmutableSamplers(nullptr)
			);
		}
		virtual Pipeline* ToPipeline(GameEngine::RenderTargetLayout* renderTargetLayout) override
		{
			return new Pipeline(dynamic_cast<RenderTargetLayout*>(renderTargetLayout), this);
		}
	};

	vk::PrimitiveTopology TranslatePrimitiveTopology(PrimitiveType ptype)
	{
		switch (ptype)
		{
		case PrimitiveType::Triangles: return vk::PrimitiveTopology::eTriangleList;
		case PrimitiveType::TriangleFans: return vk::PrimitiveTopology::eTriangleFan;
		case PrimitiveType::Points: return vk::PrimitiveTopology::ePointList;
		case PrimitiveType::Lines: return vk::PrimitiveTopology::eLineList;
		case PrimitiveType::TriangleStrips: return vk::PrimitiveTopology::eTriangleStrip;
		case PrimitiveType::LineStrips: return vk::PrimitiveTopology::eLineStrip;
		default: throw CoreLib::NotImplementedException();
		}
	}

	Pipeline::Pipeline(RenderTargetLayout* renderTargetLayout, PipelineBuilder* pipelineBuilder)
	{
		// Prepare to create Descriptor Set
		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = vk::DescriptorSetLayoutCreateInfo()
			.setFlags(vk::DescriptorSetLayoutCreateFlags())
			.setBindingCount(pipelineBuilder->layoutBindings.Count())
			.setPBindings(pipelineBuilder->layoutBindings.Buffer());

		this->descriptorSetLayouts.Add(RendererState::Device().createDescriptorSetLayout(descriptorSetLayoutCreateInfo).value);

		for (auto setLayout : pipelineBuilder->layoutBindings)
			this->descriptorPoolSizes.Add(vk::DescriptorPoolSize(setLayout.descriptorType, setLayout.descriptorCount));

		// Create Pipeline Layout
		vk::PipelineLayoutCreateInfo layoutCreateInfo = vk::PipelineLayoutCreateInfo()
			.setFlags(vk::PipelineLayoutCreateFlags())
			.setSetLayoutCount(descriptorSetLayouts.Count())
			.setPSetLayouts(descriptorSetLayouts.Buffer())
			.setPushConstantRangeCount(pipelineBuilder->pushConstantRanges.Count())
			.setPPushConstantRanges(pipelineBuilder->pushConstantRanges.Buffer());

		this->pipelineLayout = RendererState::Device().createPipelineLayout(layoutCreateInfo).value;

		// Vertex Input Description
		vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo = vk::PipelineVertexInputStateCreateInfo()
			.setFlags(vk::PipelineVertexInputStateCreateFlags())
			.setVertexBindingDescriptionCount(pipelineBuilder->vertexBindingDescriptions.Count())
			.setPVertexBindingDescriptions(pipelineBuilder->vertexBindingDescriptions.Buffer())
			.setVertexAttributeDescriptionCount(pipelineBuilder->vertexAttributeDescriptions.Count())
			.setPVertexAttributeDescriptions(pipelineBuilder->vertexAttributeDescriptions.Buffer());

		// Create Input Assembly Description
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = vk::PipelineInputAssemblyStateCreateInfo()
			.setFlags(vk::PipelineInputAssemblyStateCreateFlags())
			.setTopology(TranslatePrimitiveTopology(pipelineBuilder->PrimitiveTopology))
			.setPrimitiveRestartEnable(pipelineBuilder->PrimitiveRestartEnabled);

		// Create Viewport Description
		//empty
		vk::PipelineViewportStateCreateInfo viewportCreateInfo = vk::PipelineViewportStateCreateInfo()
			.setFlags(vk::PipelineViewportStateCreateFlags())
			.setViewportCount(1)
			.setPViewports(nullptr)
			.setScissorCount(1)
			.setPScissors(nullptr);

		// Create Rasterization Description
		vk::PipelineRasterizationStateCreateInfo rasterizationCreateInfo = vk::PipelineRasterizationStateCreateInfo()
			.setFlags(vk::PipelineRasterizationStateCreateFlags())
			.setDepthClampEnable(VK_FALSE)
			.setRasterizerDiscardEnable(VK_FALSE)
			.setPolygonMode(vk::PolygonMode::eFill)
			.setCullMode(vk::CullModeFlagBits::eNone)
			.setFrontFace(vk::FrontFace::eClockwise)
			.setDepthBiasEnable(VK_FALSE)
			.setDepthBiasConstantFactor(0.0f)
			.setDepthBiasClamp(0.0f)
			.setDepthBiasSlopeFactor(0.0f)
			.setLineWidth(1.0f);

		// Create Multisampling Description
		//TODO: Implement multisampling
		vk::PipelineMultisampleStateCreateInfo multisampleCreateInfo = vk::PipelineMultisampleStateCreateInfo()
			.setFlags(vk::PipelineMultisampleStateCreateFlags())
			.setRasterizationSamples(vk::SampleCountFlagBits::e1)
			.setSampleShadingEnable(VK_FALSE)
			.setMinSampleShading(1.0f)
			.setPSampleMask(nullptr)
			.setAlphaToCoverageEnable(VK_FALSE)
			.setAlphaToOneEnable(VK_FALSE);

		// Create Depth Stencil Description
		vk::PipelineDepthStencilStateCreateInfo depthStencilCreateInfo = vk::PipelineDepthStencilStateCreateInfo()
			.setFlags(vk::PipelineDepthStencilStateCreateFlags())
			.setDepthTestEnable(pipelineBuilder->DepthCompareFunc != CompareFunc::Disabled)
			.setDepthWriteEnable(pipelineBuilder->DepthCompareFunc != CompareFunc::Disabled)
			.setDepthCompareOp(TranslateCompareFunc(pipelineBuilder->DepthCompareFunc))
			.setDepthBoundsTestEnable(VK_FALSE)
			.setMinDepthBounds(0.0f)
			.setMaxDepthBounds(0.0f)
			.setStencilTestEnable(pipelineBuilder->StencilCompareFunc != CompareFunc::Disabled)
			.setFront(vk::StencilOpState()
				.setCompareOp(TranslateCompareFunc(pipelineBuilder->StencilCompareFunc))
				.setPassOp(TranslateStencilOp(pipelineBuilder->StencilDepthPassOp))
				.setFailOp(TranslateStencilOp(pipelineBuilder->StencilFailOp))
				.setDepthFailOp(TranslateStencilOp(pipelineBuilder->StencilDepthFailOp))
				.setCompareMask(pipelineBuilder->StencilMask)
				.setWriteMask(pipelineBuilder->StencilMask)
				.setReference(pipelineBuilder->StencilReference))
			.setBack(vk::StencilOpState()
				.setCompareOp(TranslateCompareFunc(pipelineBuilder->StencilCompareFunc))
				.setPassOp(TranslateStencilOp(pipelineBuilder->StencilDepthPassOp))
				.setFailOp(TranslateStencilOp(pipelineBuilder->StencilFailOp))
				.setDepthFailOp(TranslateStencilOp(pipelineBuilder->StencilDepthFailOp))
				.setCompareMask(pipelineBuilder->StencilMask)
				.setWriteMask(pipelineBuilder->StencilMask)
				.setReference(pipelineBuilder->StencilReference));

		// Create Blending Description
		CoreLib::List<vk::PipelineColorBlendAttachmentState> colorBlendAttachments;
		for (int i = 0; i<renderTargetLayout->colorReferences.Count(); i++)
		{
			colorBlendAttachments.Add(
				vk::PipelineColorBlendAttachmentState()
				.setBlendEnable(pipelineBuilder->BlendMode != BlendMode::Replace)
				.setSrcColorBlendFactor(pipelineBuilder->BlendMode == BlendMode::AlphaBlend ? vk::BlendFactor::eSrcAlpha : vk::BlendFactor::eOne)
				.setDstColorBlendFactor(pipelineBuilder->BlendMode == BlendMode::AlphaBlend ? vk::BlendFactor::eOneMinusSrcAlpha : vk::BlendFactor::eOne)
				.setColorBlendOp(vk::BlendOp::eAdd)
				.setSrcAlphaBlendFactor(pipelineBuilder->BlendMode == BlendMode::AlphaBlend ? vk::BlendFactor::eSrcAlpha : vk::BlendFactor::eOne)
				.setDstAlphaBlendFactor(pipelineBuilder->BlendMode == BlendMode::AlphaBlend ? vk::BlendFactor::eSrcAlpha : vk::BlendFactor::eZero)
				.setAlphaBlendOp(vk::BlendOp::eAdd)
				.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
			);
		}

		vk::PipelineColorBlendStateCreateInfo colorBlendCreateInfo = vk::PipelineColorBlendStateCreateInfo()
			.setFlags(vk::PipelineColorBlendStateCreateFlags())
			.setLogicOpEnable(VK_FALSE)
			.setLogicOp(vk::LogicOp::eCopy)
			.setAttachmentCount(colorBlendAttachments.Count())
			.setPAttachments(colorBlendAttachments.Count() > 0 ? colorBlendAttachments.Buffer() : nullptr)
			.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

		// Create Dynamic Description
		CoreLib::List<vk::DynamicState> dynamicStates;
		dynamicStates.Add(vk::DynamicState::eViewport);
		dynamicStates.Add(vk::DynamicState::eScissor);
		vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo = vk::PipelineDynamicStateCreateInfo()
			.setFlags(vk::PipelineDynamicStateCreateFlags())
			.setDynamicStateCount(dynamicStates.Count())
			.setPDynamicStates(dynamicStates.Buffer());

		// Create Pipeline Create Info
		vk::GraphicsPipelineCreateInfo pipelineCreateInfo = vk::GraphicsPipelineCreateInfo()
			.setFlags(vk::PipelineCreateFlags())
			.setStageCount(pipelineBuilder->shaderStages.Count())
			.setPStages(pipelineBuilder->shaderStages.Buffer())
			.setPVertexInputState(&vertexInputCreateInfo)
			.setPInputAssemblyState(&inputAssemblyCreateInfo)
			.setPTessellationState(nullptr)
			.setPViewportState(&viewportCreateInfo)
			.setPRasterizationState(&rasterizationCreateInfo)
			.setPMultisampleState(&multisampleCreateInfo)
			.setPDepthStencilState(dynamic_cast<VK::RenderTargetLayout*>(renderTargetLayout)->depthReference.layout == vk::ImageLayout::eUndefined ? nullptr : &depthStencilCreateInfo)
			.setPColorBlendState(dynamic_cast<VK::RenderTargetLayout*>(renderTargetLayout)->colorReferences.Count() == 0 ? nullptr : &colorBlendCreateInfo)
			.setPDynamicState(&dynamicStateCreateInfo)
			.setLayout(pipelineLayout)
			.setRenderPass(dynamic_cast<VK::RenderTargetLayout*>(renderTargetLayout)->renderPass)
			.setSubpass(0)
			.setBasePipelineHandle(vk::Pipeline())
			.setBasePipelineIndex(-1);

		this->pipeline = RendererState::Device().createGraphicsPipelines(vk::PipelineCache(), pipelineCreateInfo).value[0];
	}

	PipelineInstance::PipelineInstance(Pipeline* pipeline, const PipelineBinding& pipelineBinding) :
		pipeline(pipeline->pipeline),
		pipelineLayout(pipeline->pipelineLayout),
		pipelineBindings(pipelineBinding)
	{
		if (pipeline->descriptorPoolSizes.Count() > 0)
		{
			std::pair<vk::DescriptorPool, vk::DescriptorSet> res = RendererState::AllocateDescriptorSet(pipeline->descriptorSetLayouts);
			this->descriptorPool = res.first;
			this->descriptorSet = res.second;

			// Update Descriptor Set
			std::vector<vk::WriteDescriptorSet> writeDescriptorSets;
			CoreLib::List<vk::DescriptorBufferInfo> bufferDescriptorInfo;
			CoreLib::List<vk::DescriptorImageInfo> textureDescriptorInfo;

			for (auto binding : pipelineBinding.GetBindings())
			{
				switch (binding.type)
				{
				case BindingType::StorageBuffer:
				{
					int range = binding.buf.range;
					if (range == 0) range = dynamic_cast<VK::BufferObject*>(binding.buf.buffer)->backingSize;

					bufferDescriptorInfo.Add(
						vk::DescriptorBufferInfo()
						.setBuffer(dynamic_cast<VK::BufferObject*>(binding.buf.buffer)->buffer)
						.setOffset(binding.buf.offset)
						.setRange(range)
					);

					//TODO: We can't bind a VK_NULL_HANDLE
					if (!dynamic_cast<VK::BufferObject*>(binding.buf.buffer)->buffer) break;

					writeDescriptorSets.push_back(
						vk::WriteDescriptorSet()
						.setDstSet(this->descriptorSet)
						.setDstBinding(binding.location)
						.setDstArrayElement(0)
						.setDescriptorCount(1)
						.setDescriptorType(vk::DescriptorType::eStorageBuffer)
						.setPImageInfo(nullptr)
						.setPBufferInfo(&bufferDescriptorInfo.Last())
						.setPTexelBufferView(nullptr)
					);
					break;
				}
				case BindingType::UniformBuffer:
				{
					int range = binding.buf.range;
					if (range == 0) range = dynamic_cast<VK::BufferObject*>(binding.buf.buffer)->backingSize;

					bufferDescriptorInfo.Add(
						vk::DescriptorBufferInfo()
						.setBuffer(dynamic_cast<VK::BufferObject*>(binding.buf.buffer)->buffer)
						.setOffset(binding.buf.offset)
						.setRange(range)
					);

					//TODO: We can't bind a VK_NULL_HANDLE
					if (!dynamic_cast<VK::BufferObject*>(binding.buf.buffer)->buffer) break;

					writeDescriptorSets.push_back(
						vk::WriteDescriptorSet()
						.setDstSet(this->descriptorSet)
						.setDstBinding(binding.location)
						.setDstArrayElement(0)
						.setDescriptorCount(1)
						.setDescriptorType(vk::DescriptorType::eUniformBuffer)
						.setPImageInfo(nullptr)
						.setPBufferInfo(&bufferDescriptorInfo.Last())
						.setPTexelBufferView(nullptr)
					);
					break;
				}
				case BindingType::Texture:
					textureDescriptorInfo.Add(
						vk::DescriptorImageInfo()
						.setImageLayout(vk::ImageLayout::eGeneral)
						.setImageView(dynamic_cast<VK::Texture2D*>(binding.tex.texture)->view)
						.setSampler(dynamic_cast<VK::TextureSampler*>(binding.tex.sampler)->sampler)
					);

					writeDescriptorSets.push_back(
						vk::WriteDescriptorSet()
						.setDstSet(this->descriptorSet)
						.setDstBinding(binding.location)
						.setDstArrayElement(0)
						.setDescriptorCount(1)
						.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
						.setPImageInfo(&textureDescriptorInfo.Last())
						.setPBufferInfo(nullptr)
						.setPTexelBufferView(nullptr)
					);
				case BindingType::Unused:
					break;
				default:
					throw HardwareRendererException(L"Invalid binding type");
				}
			}

			boundBufferInfo.SwapWith(bufferDescriptorInfo);
			RendererState::Device().updateDescriptorSets(writeDescriptorSets, nullptr);
		}
	}

#define SHARED_EVENT false
#if SHARED_EVENT
	class TestEvent
	{
	public:
		vk::Event internalEvent;

		TestEvent()
		{
			internalEvent = RendererState::Device().createEvent(vk::EventCreateInfo()).value;
		}
		~TestEvent()
		{
			RendererState::Device().destroyEvent(internalEvent);
		}
	};
#endif

	class CommandBuffer : public GameEngine::CommandBuffer
	{
	public:
		const vk::CommandPool& pool;
		vk::CommandBuffer buffer;
#if SHARED_EVENT
		CoreLib::RefPtr<TestEvent> submitEvent;
#else
		vk::Event submitEvent;
#endif

		CommandBuffer(const vk::CommandPool& commandPool) : pool(commandPool)
		{
			buffer = RendererState::CreateCommandBuffer(pool, vk::CommandBufferLevel::eSecondary);
#if SHARED_EVENT
			submitEvent = new TestEvent;
			RendererState::Device().setEvent(submitEvent->internalEvent);
#else
			submitEvent = RendererState::Device().createEvent(vk::EventCreateInfo()).value;
			RendererState::Device().setEvent(submitEvent);
#endif
		}
		CommandBuffer() : CommandBuffer(RendererState::RenderCommandPool()) {}

		~CommandBuffer()
		{
			RendererState::DestroyCommandBuffer(pool, buffer);
#if !SHARED_EVENT
			RendererState::Device().destroyEvent(submitEvent);
#endif
		}

		inline void BeginRecording(GameEngine::RenderTargetLayout* renderTargetLayout, vk::Framebuffer framebuffer)
		{
			// Wait for command buffer to no longer be in use for a prior submission
			while (true)
			{
#if SHARED_EVENT
				if (RendererState::Device().getEventStatus(submitEvent->internalEvent) == vk::Result::eEventSet)
					break;
#else
				if (RendererState::Device().getEventStatus(submitEvent) == vk::Result::eEventSet)
					break;
#endif
			}

			vk::CommandBufferInheritanceInfo inheritanceInfo = vk::CommandBufferInheritanceInfo()
				.setRenderPass(dynamic_cast<VK::RenderTargetLayout*>(renderTargetLayout)->renderPass)
				.setSubpass(0)//
				.setFramebuffer(framebuffer)
				.setOcclusionQueryEnable(VK_TRUE)//
				.setQueryFlags(vk::QueryControlFlags())//
				.setPipelineStatistics(vk::QueryPipelineStatisticFlags());//

			vk::CommandBufferBeginInfo commandBufferBeginInfo = vk::CommandBufferBeginInfo()
				.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse | vk::CommandBufferUsageFlagBits::eRenderPassContinue)
				.setPInheritanceInfo(&inheritanceInfo);

			buffer.begin(commandBufferBeginInfo);
		}

		virtual void BeginRecording(GameEngine::RenderTargetLayout* renderTargetLayout, GameEngine::FrameBuffer* frameBuffer) override
		{
			BeginRecording(renderTargetLayout, dynamic_cast<VK::FrameBuffer*>(frameBuffer)->framebuffer);
		}

		virtual void BeginRecording(GameEngine::RenderTargetLayout* renderTargetLayout) override
		{
			BeginRecording(renderTargetLayout, vk::Framebuffer());
		}

		virtual void EndRecording() override
		{
			buffer.end();
		}

		virtual void BindVertexBuffer(Buffer* vertexBuffer) override
		{
			buffer.bindVertexBuffers(0, dynamic_cast<VK::BufferObject*>(vertexBuffer)->Buffer(), { 0 });
		}
		virtual void BindIndexBuffer(Buffer* indexBuffer) override
		{
			//TODO: Can make index buffer use 16 bit ints if possible?
			buffer.bindIndexBuffer(dynamic_cast<VK::BufferObject*>(indexBuffer)->Buffer(), { 0 }, vk::IndexType::eUint32);
		}
		virtual void BindPipeline(GameEngine::PipelineInstance* pipelineInstance) override
		{
			dynamic_cast<VK::PipelineInstance*>(pipelineInstance)->Update();

			if (dynamic_cast<VK::PipelineInstance*>(pipelineInstance)->descriptorSet)
				buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, dynamic_cast<VK::PipelineInstance*>(pipelineInstance)->pipelineLayout, 0, dynamic_cast<VK::PipelineInstance*>(pipelineInstance)->descriptorSet, nullptr);//TODO: offsets

			buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, dynamic_cast<VK::PipelineInstance*>(pipelineInstance)->pipeline);
		}

		virtual void Draw(int firstVertex, int vertexCount) override
		{
			buffer.draw(vertexCount, 1, firstVertex, 0);
		}
		virtual void DrawInstanced(int numInstances, int firstVertex, int vertexCount) override
		{
			buffer.draw(vertexCount, numInstances, firstVertex, 0);
		}
		virtual void DrawIndexed(int firstIndex, int indexCount) override
		{
			buffer.drawIndexed(indexCount, 1, firstIndex, 0, 0);
		}
		virtual void DrawIndexedInstanced(int numInstances, int firstIndex, int indexCount) override
		{
			buffer.drawIndexed(indexCount, numInstances, firstIndex, 0, 0);
		}

		virtual void SetViewport(int x, int y, int width, int height) override
		{
			buffer.setViewport(0, vk::Viewport((float)x, (float)y, (float)width, (float)height, 0.0f, 1.0f));
			buffer.setScissor(0, vk::Rect2D(vk::Offset2D(x, y), vk::Extent2D(width, height)));
		}

		virtual void Blit(GameEngine::Texture2D* dstImage, GameEngine::Texture2D* srcImage) override
		{
			vk::ImageSubresourceRange imageSubresourceRange = vk::ImageSubresourceRange()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setBaseMipLevel(0)
				.setLevelCount(1)
				.setBaseArrayLayer(0)
				.setLayerCount(1);

			vk::ImageMemoryBarrier preBlitBarrier = vk::ImageMemoryBarrier()
				.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eUndefined))
				.setDstAccessMask(LayoutFlags(vk::ImageLayout::eTransferDstOptimal))
				.setOldLayout(vk::ImageLayout::eUndefined)
				.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(dynamic_cast<Texture2D*>(dstImage)->image)
				.setSubresourceRange(imageSubresourceRange);

			vk::ImageMemoryBarrier postBlitBarrier = vk::ImageMemoryBarrier()
				.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eTransferDstOptimal))
				.setDstAccessMask(LayoutFlags(LayoutFromUsage(dynamic_cast<Texture2D*>(dstImage)->usage)))
				.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
				.setNewLayout(LayoutFromUsage(dynamic_cast<Texture2D*>(dstImage)->usage))
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(dynamic_cast<Texture2D*>(dstImage)->image)
				.setSubresourceRange(imageSubresourceRange);

			buffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlags(),
				nullptr,
				nullptr,
				preBlitBarrier
			);

			vk::ImageMemoryBarrier textureTransferBarrier = vk::ImageMemoryBarrier()
				.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eColorAttachmentOptimal))
				.setDstAccessMask(LayoutFlags(vk::ImageLayout::eTransferSrcOptimal))
				.setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
				.setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(dynamic_cast<Texture2D*>(srcImage)->image)
				.setSubresourceRange(imageSubresourceRange);

			vk::ImageMemoryBarrier textureRestoreBarrier = vk::ImageMemoryBarrier()
				.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eTransferSrcOptimal))
				.setDstAccessMask(LayoutFlags(vk::ImageLayout::eColorAttachmentOptimal))
				.setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
				.setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(dynamic_cast<Texture2D*>(srcImage)->image)
				.setSubresourceRange(imageSubresourceRange);

			buffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlags(),
				nullptr,
				nullptr,
				textureTransferBarrier
			);

			// Blit
			vk::ImageSubresourceLayers subresourceLayers = vk::ImageSubresourceLayers()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setMipLevel(0)
				.setBaseArrayLayer(0)
				.setLayerCount(1);

			std::array<vk::Offset3D, 2> srcOffsets;
			srcOffsets[0] = vk::Offset3D(0, 0, 0);
			srcOffsets[1] = vk::Offset3D(dynamic_cast<VK::Texture2D*>(srcImage)->width, dynamic_cast<VK::Texture2D*>(srcImage)->height, 1);

			std::array<vk::Offset3D, 2> dstOffsets;
			dstOffsets[0] = vk::Offset3D(0, 0, 0);
			dstOffsets[1] = vk::Offset3D(dynamic_cast<VK::Texture2D*>(dstImage)->width, dynamic_cast<VK::Texture2D*>(dstImage)->height, 1);

			vk::ImageBlit blitRegions = vk::ImageBlit()
				.setSrcSubresource(subresourceLayers)
				.setSrcOffsets(srcOffsets)
				.setDstSubresource(subresourceLayers)
				.setDstOffsets(dstOffsets);

			buffer.blitImage(
				dynamic_cast<VK::Texture2D*>(srcImage)->image,
				vk::ImageLayout::eTransferSrcOptimal,
				dynamic_cast<Texture2D*>(dstImage)->image,
				vk::ImageLayout::eTransferDstOptimal,
				blitRegions,
				vk::Filter::eNearest
			);

			buffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlags(),
				nullptr,
				nullptr,
				textureRestoreBarrier
			);

			buffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eBottomOfPipe,
				vk::DependencyFlags(),
				nullptr,
				nullptr,
				postBlitBarrier
			);
		}

		virtual void ClearAttachments(RenderAttachments renderAttachments) override
		{
			CoreLib::List<vk::ClearAttachment> attachments;
			CoreLib::List<vk::ClearRect> rects;

			for (int k = 0; k < renderAttachments.attachments.Count(); k++)
			{
				Texture2D* image = dynamic_cast<VK::Texture2D*>(renderAttachments.attachments[k]);

				vk::ImageAspectFlags aspectMask;
				switch (image->usage)
				{
				case GameEngine::TextureUsage::ColorAttachment:
				case GameEngine::TextureUsage::SampledColorAttachment:
					aspectMask = vk::ImageAspectFlagBits::eColor;
					break;
				case GameEngine::TextureUsage::DepthAttachment:
				case GameEngine::TextureUsage::SampledDepthAttachment:
					aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
					break;
				}

				switch (image->currentLayout)
				{
				case vk::ImageLayout::eColorAttachmentOptimal:
					attachments.Add(
						vk::ClearAttachment()
						.setAspectMask(aspectMask)
						.setColorAttachment(k)
						.setClearValue(vk::ClearColorValue())
					);
					break;
				case vk::ImageLayout::eDepthStencilAttachmentOptimal:
					attachments.Add(
						vk::ClearAttachment()
						.setAspectMask(aspectMask)
						.setColorAttachment(k)
						.setClearValue(vk::ClearDepthStencilValue(1.0f, 0))
					);
					break;
				default:
					break;
				}

				rects.Add(
					vk::ClearRect()
					.setBaseArrayLayer(0)
					.setLayerCount(1)
					.setRect(vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(renderAttachments.width, renderAttachments.height)))
				);
			}
			
			buffer.clearAttachments(
				vk::ArrayProxy<const vk::ClearAttachment>(attachments.Count(), attachments.Buffer()),
				vk::ArrayProxy<const vk::ClearRect>(rects.Count(), rects.Buffer())
			);
		}
	};

	class HardwareRenderer : public GameEngine::HardwareRenderer
	{
	private:
		void* handle = nullptr;
		int width = -1;
		int height = -1;

		vk::SurfaceKHR surface;
		vk::SwapchainKHR swapchain;

		std::vector<vk::Image> images; //alternatively could call getSwapchainImages each time
		std::vector<vk::CommandBuffer> commandBuffers;
		vk::Semaphore imageAvailableSemaphore;
		vk::Semaphore renderingFinishedSemaphore;

		void CreateSwapchain()
		{
			std::vector<vk::SurfaceFormatKHR> surfaceFormats = RendererState::PhysicalDevice().getSurfaceFormatsKHR(surface).value;
			vk::Format format;
			vk::ColorSpaceKHR colorSpace = surfaceFormats.at(0).colorSpace;
			if ((surfaceFormats.size() == 1) && (surfaceFormats.at(0).format == vk::Format::eUndefined))
				format = vk::Format::eB8G8R8A8Unorm;
			else
				format = surfaceFormats.at(0).format;

			// Select presentation mode
			vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo; // Fifo presentation mode is guaranteed
			for (auto & mode : RendererState::PhysicalDevice().getSurfacePresentModesKHR(surface).value)
			{
				// If we can use mailbox, use it.
				if (mode == vk::PresentModeKHR::eMailbox)
				{
					presentMode = mode;
					break;
				}
			}

			vk::SurfaceCapabilitiesKHR surfaceCapabilities = RendererState::PhysicalDevice().getSurfaceCapabilitiesKHR(surface).value;

			unsigned int desiredSwapchainImages = 3;
			if (desiredSwapchainImages < surfaceCapabilities.minImageCount) {
				desiredSwapchainImages = surfaceCapabilities.minImageCount;
			}
			else if (desiredSwapchainImages > surfaceCapabilities.maxImageCount) {
				desiredSwapchainImages = surfaceCapabilities.maxImageCount;
			}

			vk::Extent2D swapchainExtent = {};
			if (surfaceCapabilities.currentExtent.width == -1) {
				swapchainExtent.width = this->width;
				swapchainExtent.height = this->height;
			}
			else {
				swapchainExtent = surfaceCapabilities.currentExtent;
			}

			// Select swapchain pre-transform
			// (Can be useful on tablets, etc.)
			vk::SurfaceTransformFlagBitsKHR preTransform = surfaceCapabilities.currentTransform;
			if (surfaceCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) {
				// Select identity transform if we can
				preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
			}

			vk::SwapchainKHR oldSwapchain = swapchain;

			vk::SwapchainCreateInfoKHR swapchainCreateInfo = vk::SwapchainCreateInfoKHR()
				.setMinImageCount(desiredSwapchainImages)
				.setSurface(surface)
				.setImageFormat(format)
				.setImageColorSpace(colorSpace)
				.setImageExtent(swapchainExtent)
				.setImageArrayLayers(1)
				.setImageUsage(vk::ImageUsageFlagBits::eTransferDst) // we only draw to screen by blit
				.setImageSharingMode(vk::SharingMode::eExclusive)
				.setQueueFamilyIndexCount(0)
				.setPQueueFamilyIndices(VK_NULL_HANDLE)
				.setPreTransform(preTransform)
				.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
				.setPresentMode(presentMode)
				.setClipped(VK_TRUE)
				.setOldSwapchain(oldSwapchain);

			swapchain = RendererState::Device().createSwapchainKHR(swapchainCreateInfo).value;
			DestroySwapchain(oldSwapchain);

			images = RendererState::Device().getSwapchainImagesKHR(swapchain).value;
		}

		virtual CoreLib::String GetSpireBackendName() override
		{
			return L"spirv";
		}

		void CreateCommandBuffers()
		{
			DestroyCommandBuffers();

			vk::CommandBufferAllocateInfo commandBufferAllocateInfo = vk::CommandBufferAllocateInfo()
				.setCommandPool(RendererState::SwapchainCommandPool())
				.setLevel(vk::CommandBufferLevel::ePrimary)
				.setCommandBufferCount((uint32_t)images.size());

			commandBuffers = RendererState::Device().allocateCommandBuffers(commandBufferAllocateInfo).value;
		}

		void CreateSemaphores()
		{
			DestroySemaphores();

			vk::SemaphoreCreateInfo semaphoreCreateInfo;
			imageAvailableSemaphore = RendererState::Device().createSemaphore(semaphoreCreateInfo).value;
			renderingFinishedSemaphore = RendererState::Device().createSemaphore(semaphoreCreateInfo).value;
		}

		void DestroySemaphores()
		{
			RendererState::Device().waitIdle();
			if (imageAvailableSemaphore) RendererState::Device().destroySemaphore(imageAvailableSemaphore);
			if (renderingFinishedSemaphore) RendererState::Device().destroySemaphore(renderingFinishedSemaphore);
		}

		void DestroyCommandBuffers()
		{
			for (auto commandBuffer : commandBuffers)
				RendererState::Device().freeCommandBuffers(RendererState::SwapchainCommandPool(), commandBuffer);
		}

		void DestroySwapchain()
		{
			RendererState::Device().waitIdle();
			DestroySwapchain(swapchain);
		}

		void DestroySwapchain(vk::SwapchainKHR pswapchain)
		{
			if (pswapchain) RendererState::Device().destroySwapchainKHR(pswapchain);// shouldn't need this if, but nvidia driver is broken.
		}

		void UnbindWindow()
		{
			DestroySemaphores();
			DestroyCommandBuffers();
			DestroySwapchain();
			if (surface) RendererState::Instance().destroySurfaceKHR(surface);
			handle = nullptr;
		}

		void Clear()
		{
			for (size_t image = 0; image < images.size(); image++)
			{
				//TODO: see if following line is beneficial
				commandBuffers[image].reset(vk::CommandBufferResetFlags()); // implicitly done by begin

				vk::CommandBufferBeginInfo commandBufferBeginInfo = vk::CommandBufferBeginInfo()
					.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse)
					.setPInheritanceInfo(nullptr);

				vk::ImageSubresourceRange imageSubresourceRange = vk::ImageSubresourceRange()
					.setAspectMask(vk::ImageAspectFlagBits::eColor)
					.setBaseMipLevel(0)
					.setLevelCount(1)
					.setBaseArrayLayer(0)
					.setLayerCount(1);

				vk::ImageMemoryBarrier postPresentBarrier = vk::ImageMemoryBarrier()
					.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eUndefined))
					.setDstAccessMask(LayoutFlags(vk::ImageLayout::eTransferDstOptimal))
					.setOldLayout(vk::ImageLayout::eUndefined)
					.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
					.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setImage(images[image])
					.setSubresourceRange(imageSubresourceRange);

				vk::ImageMemoryBarrier prePresentBarrier = vk::ImageMemoryBarrier()
					.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eTransferDstOptimal))
					.setDstAccessMask(LayoutFlags(vk::ImageLayout::ePresentSrcKHR))
					.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
					.setNewLayout(vk::ImageLayout::ePresentSrcKHR)
					.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setImage(images[image])
					.setSubresourceRange(imageSubresourceRange);

				commandBuffers[image].begin(commandBufferBeginInfo); // start recording
				commandBuffers[image].pipelineBarrier(
					vk::PipelineStageFlagBits::eTransfer,
					vk::PipelineStageFlagBits::eTransfer,
					vk::DependencyFlags(),
					nullptr,
					nullptr,
					postPresentBarrier
				);

				commandBuffers[image].clearColorImage(
					images[image],
					vk::ImageLayout::eTransferDstOptimal,
					vk::ClearColorValue(std::array<float, 4>{ 0.467f, 0.725f, 0.0f, 0.0f }),
					imageSubresourceRange
				);

				commandBuffers[image].pipelineBarrier(
					vk::PipelineStageFlagBits::eTransfer,
					vk::PipelineStageFlagBits::eBottomOfPipe,
					vk::DependencyFlags(),
					nullptr,
					nullptr,
					prePresentBarrier
				);
				commandBuffers[image].end(); // stop recording
			}
		}
	public:
		HardwareRenderer()
		{
			RendererState::AddRenderer();
		};
		~HardwareRenderer()
		{
			RendererState::Device().waitIdle();
			UnbindWindow();
			RendererState::RemRenderer();
		}

		void BindWindow(void* windowHandle, int pwidth, int pheight)
		{
			UnbindWindow();

			this->handle = windowHandle;
			this->width = pwidth;
			this->height = pheight;
			this->surface = RendererState::CreateSurface(windowHandle);
			CreateSwapchain();
			CreateCommandBuffers();
			CreateSemaphores();

			Clear();
		}

		void Resize(int pwidth, int pheight)
		{
			if (!handle) return;

			this->width = pwidth;
			this->height = pheight;

			size_t oldImageCount = images.size();

			CreateSwapchain();
			if (images.size() != oldImageCount) CreateCommandBuffers();
			//CreateSemaphores(); //TODO: Can the semaphores get stuck?

			Clear(); //TODO: Should be "re-record" with old commands and new size?
		}

		virtual void ClearTexture(GameEngine::Texture2D* texture) override
		{
			//TODO: improve
			RendererState::RenderQueue().waitIdle(); //TODO: Remove

			vk::CommandBufferBeginInfo primaryBeginInfo = vk::CommandBufferBeginInfo()
				.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse)
				.setPInheritanceInfo(nullptr);

			auto primaryBufferFence = RendererState::PrimaryBuffer();
			vk::CommandBuffer primaryBuffer = primaryBufferFence.first;
			vk::Fence primaryFence = primaryBufferFence.second;
			primaryBuffer.begin(primaryBeginInfo);

			switch (dynamic_cast<VK::Texture2D*>(texture)->currentLayout)
			{
			case vk::ImageLayout::eColorAttachmentOptimal:
				primaryBuffer.clearColorImage(
					dynamic_cast<VK::Texture2D*>(texture)->image,
					vk::ImageLayout::eGeneral,
					vk::ClearColorValue(),
					vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
				);
				break;
			case vk::ImageLayout::eDepthStencilAttachmentOptimal:
				primaryBuffer.clearDepthStencilImage(
					dynamic_cast<VK::Texture2D*>(texture)->image,
					vk::ImageLayout::eGeneral,
					vk::ClearDepthStencilValue(1.0f, 0),
					vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1)
				);
				break;
			default:
				break;
			}

			primaryBuffer.end();

			vk::SubmitInfo submitInfo = vk::SubmitInfo()
				.setWaitSemaphoreCount(0)
				.setPWaitSemaphores(nullptr)
				.setPWaitDstStageMask(nullptr)
				.setCommandBufferCount(1)
				.setPCommandBuffers(&primaryBuffer)
				.setSignalSemaphoreCount(0)
				.setPSignalSemaphores(nullptr);

			RendererState::RenderQueue().submit(submitInfo, primaryFence);
		}

		virtual void ExecuteCommandBuffers(GameEngine::RenderTargetLayout* renderTargetLayout, GameEngine::FrameBuffer* frameBuffer, CoreLib::ArrayView<GameEngine::CommandBuffer*> commands) override
		{
			// Create command buffer begin info
			vk::CommandBufferBeginInfo primaryBeginInfo = vk::CommandBufferBeginInfo()
				.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse)
				.setPInheritanceInfo(nullptr);

			// Create render pass begin info
			CoreLib::Array<vk::ClearValue, 2> clearValues;
			clearValues.Add(vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f }));
			clearValues.Add(vk::ClearDepthStencilValue(1.0f, 0));

			vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
				.setRenderPass(dynamic_cast<VK::RenderTargetLayout*>(renderTargetLayout)->renderPass)
				.setFramebuffer(dynamic_cast<VK::FrameBuffer*>(frameBuffer)->framebuffer)
				.setRenderArea(vk::Rect2D().setOffset(vk::Offset2D(0, 0)).setExtent(vk::Extent2D(dynamic_cast<VK::FrameBuffer*>(frameBuffer)->width, dynamic_cast<VK::FrameBuffer*>(frameBuffer)->height)))
				.setClearValueCount(clearValues.Count())
				.setPClearValues(clearValues.Buffer());

#if SHARED_EVENT
			CoreLib::RefPtr<TestEvent> curEvent = new TestEvent();
#endif

			// Aggregate secondary command buffers
			CoreLib::List<vk::CommandBuffer> commandBuffersToExecute;
			for (auto& buffer : commands)
			{
				commandBuffersToExecute.Add(((VK::CommandBuffer*)(buffer))->buffer);
#if SHARED_EVENT
				((VK::CommandBuffer*)(buffer))->submitEvent = curEvent;
#else
				RendererState::Device().resetEvent(((VK::CommandBuffer*)(buffer))->submitEvent);
#endif
			}

			// Record primary command buffer
			auto primaryBufferFence = RendererState::PrimaryBuffer();
			vk::CommandBuffer primaryBuffer = primaryBufferFence.first;
			vk::Fence primaryFence = primaryBufferFence.second;
			primaryBuffer.begin(primaryBeginInfo);
			primaryBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eSecondaryCommandBuffers);

			primaryBuffer.executeCommands(commandBuffersToExecute.Count(), commandBuffersToExecute.Buffer());

			primaryBuffer.endRenderPass();

#if SHARED_EVENT
			primaryBuffer.setEvent(curEvent->internalEvent, vk::PipelineStageFlagBits::eBottomOfPipe);
#else
			for (auto& buffer : commands)
				primaryBuffer.setEvent(((VK::CommandBuffer*)(buffer))->submitEvent, vk::PipelineStageFlagBits::eBottomOfPipe);
#endif

			primaryBuffer.end();

			vk::SubmitInfo submitInfo = vk::SubmitInfo()
				.setWaitSemaphoreCount(0)
				.setPWaitSemaphores(nullptr)
				.setPWaitDstStageMask(nullptr)
				.setCommandBufferCount(1)
				.setPCommandBuffers(&primaryBuffer)
				.setSignalSemaphoreCount(0)
				.setPSignalSemaphores(nullptr);

			RendererState::RenderQueue().submit(submitInfo, primaryFence);
		}
		virtual void Wait() override
		{
			RendererState::Device().waitIdle();
		}
		virtual void Blit(GameEngine::Texture2D* dstImage, GameEngine::Texture2D* srcImage) override
		{
			vk::CommandBuffer transferCommandBuffer = RendererState::CreateCommandBuffer(RendererState::TransferCommandPool());

			vk::CommandBufferBeginInfo commandBufferBeginInfo = vk::CommandBufferBeginInfo()
				.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse)
				.setPInheritanceInfo(nullptr);

			vk::ImageSubresourceRange imageSubresourceRange = vk::ImageSubresourceRange()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setBaseMipLevel(0)
				.setLevelCount(1)
				.setBaseArrayLayer(0)
				.setLayerCount(1);

			vk::ImageMemoryBarrier preBlitBarrier = vk::ImageMemoryBarrier()
				.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eUndefined))
				.setDstAccessMask(LayoutFlags(vk::ImageLayout::eTransferDstOptimal))
				.setOldLayout(vk::ImageLayout::eUndefined)
				.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(dynamic_cast<Texture2D*>(dstImage)->image)
				.setSubresourceRange(imageSubresourceRange);

			vk::ImageMemoryBarrier postBlitBarrier = vk::ImageMemoryBarrier()
				.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eTransferDstOptimal))
				.setDstAccessMask(LayoutFlags(LayoutFromUsage(dynamic_cast<Texture2D*>(dstImage)->usage)))
				.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
				.setNewLayout(LayoutFromUsage(dynamic_cast<Texture2D*>(dstImage)->usage))
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(dynamic_cast<Texture2D*>(dstImage)->image)
				.setSubresourceRange(imageSubresourceRange);

			transferCommandBuffer.begin(commandBufferBeginInfo); // start recording
			transferCommandBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlags(),
				nullptr,
				nullptr,
				preBlitBarrier
			);

			vk::ImageMemoryBarrier textureTransferBarrier = vk::ImageMemoryBarrier()
				.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eColorAttachmentOptimal))
				.setDstAccessMask(LayoutFlags(vk::ImageLayout::eTransferSrcOptimal))
				.setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
				.setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(dynamic_cast<Texture2D*>(srcImage)->image)
				.setSubresourceRange(imageSubresourceRange);

			vk::ImageMemoryBarrier textureRestoreBarrier = vk::ImageMemoryBarrier()
				.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eTransferSrcOptimal))
				.setDstAccessMask(LayoutFlags(vk::ImageLayout::eColorAttachmentOptimal))
				.setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
				.setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(dynamic_cast<Texture2D*>(srcImage)->image)
				.setSubresourceRange(imageSubresourceRange);

			transferCommandBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlags(),
				nullptr,
				nullptr,
				textureTransferBarrier
			);

			// Blit
			vk::ImageSubresourceLayers subresourceLayers = vk::ImageSubresourceLayers()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setMipLevel(0)
				.setBaseArrayLayer(0)
				.setLayerCount(1);

			std::array<vk::Offset3D, 2> srcOffsets;
			srcOffsets[0] = vk::Offset3D(0, 0, 0);
			srcOffsets[1] = vk::Offset3D(dynamic_cast<VK::Texture2D*>(srcImage)->width, dynamic_cast<VK::Texture2D*>(srcImage)->height, 1);

			std::array<vk::Offset3D, 2> dstOffsets;
			dstOffsets[0] = vk::Offset3D(0, 0, 0);
			dstOffsets[1] = vk::Offset3D(dynamic_cast<VK::Texture2D*>(dstImage)->width, dynamic_cast<VK::Texture2D*>(dstImage)->height, 1);

			vk::ImageBlit blitRegions = vk::ImageBlit()
				.setSrcSubresource(subresourceLayers)
				.setSrcOffsets(srcOffsets)
				.setDstSubresource(subresourceLayers)
				.setDstOffsets(dstOffsets);

			transferCommandBuffer.blitImage(
				dynamic_cast<VK::Texture2D*>(srcImage)->image,
				vk::ImageLayout::eTransferSrcOptimal,
				dynamic_cast<Texture2D*>(dstImage)->image,
				vk::ImageLayout::eTransferDstOptimal,
				blitRegions,
				vk::Filter::eNearest
			);

			transferCommandBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlags(),
				nullptr,
				nullptr,
				textureRestoreBarrier
			);

			transferCommandBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eBottomOfPipe,
				vk::DependencyFlags(),
				nullptr,
				nullptr,
				postBlitBarrier
			);
			transferCommandBuffer.end(); // stop recording

										 // Transfer queue submit
			vk::SubmitInfo transferSubmitInfo = vk::SubmitInfo()
				.setWaitSemaphoreCount(0)
				.setPWaitSemaphores(nullptr)
				.setPWaitDstStageMask(nullptr)
				.setCommandBufferCount(1)
				.setPCommandBuffers(&transferCommandBuffer)
				.setSignalSemaphoreCount(0)
				.setPSignalSemaphores(nullptr);

			RendererState::RenderQueue().waitIdle(); //TODO: Remove
			RendererState::RenderQueue().submit(transferSubmitInfo, vk::Fence());
			RendererState::RenderQueue().waitIdle(); //TODO: Remove
			RendererState::DestroyCommandBuffer(RendererState::TransferCommandPool(), transferCommandBuffer);
		}
		virtual void Present(GameEngine::Texture2D* srcImage) override
		{
			auto nextImage = RendererState::Device().acquireNextImageKHR(swapchain, UINT64_MAX, imageAvailableSemaphore, vk::Fence()).value;

			//TODO: see if following line is beneficial
			commandBuffers[nextImage].reset(vk::CommandBufferResetFlags()); // implicitly done by begin

			vk::CommandBufferBeginInfo commandBufferBeginInfo = vk::CommandBufferBeginInfo()
				.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse)
				.setPInheritanceInfo(nullptr);

			vk::ImageSubresourceRange imageSubresourceRange = vk::ImageSubresourceRange()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setBaseMipLevel(0)
				.setLevelCount(1)
				.setBaseArrayLayer(0)
				.setLayerCount(1);

			vk::ImageMemoryBarrier postPresentBarrier = vk::ImageMemoryBarrier()
				.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eUndefined))
				.setDstAccessMask(LayoutFlags(vk::ImageLayout::eTransferDstOptimal))
				.setOldLayout(vk::ImageLayout::eUndefined)
				.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(images[nextImage])
				.setSubresourceRange(imageSubresourceRange);

			vk::ImageMemoryBarrier prePresentBarrier = vk::ImageMemoryBarrier()
				.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eTransferDstOptimal))
				.setDstAccessMask(LayoutFlags(vk::ImageLayout::ePresentSrcKHR))
				.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
				.setNewLayout(vk::ImageLayout::ePresentSrcKHR)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(images[nextImage])
				.setSubresourceRange(imageSubresourceRange);

			commandBuffers[nextImage].begin(commandBufferBeginInfo); // start recording
			commandBuffers[nextImage].pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlags(),
				nullptr,
				nullptr,
				postPresentBarrier
			);

			if (srcImage == nullptr)
			{
				// If no source image, clear to debug purple
				commandBuffers[nextImage].clearColorImage(
					images[nextImage],
					vk::ImageLayout::eTransferDstOptimal,
					vk::ClearColorValue(std::array<float, 4>{ 1.0f, 0.0f, 1.0f, 0.0f }),
					imageSubresourceRange
				);
			}
			else
			{
				vk::ImageMemoryBarrier textureTransferBarrier = vk::ImageMemoryBarrier()
					.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eColorAttachmentOptimal))
					.setDstAccessMask(LayoutFlags(vk::ImageLayout::eTransferSrcOptimal))
					.setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
					.setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
					.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setImage(dynamic_cast<Texture2D*>(srcImage)->image)
					.setSubresourceRange(imageSubresourceRange);

				vk::ImageMemoryBarrier textureRestoreBarrier = vk::ImageMemoryBarrier()
					.setSrcAccessMask(LayoutFlags(vk::ImageLayout::eTransferSrcOptimal))
					.setDstAccessMask(LayoutFlags(vk::ImageLayout::eColorAttachmentOptimal))
					.setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
					.setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
					.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setImage(dynamic_cast<Texture2D*>(srcImage)->image)
					.setSubresourceRange(imageSubresourceRange);

				commandBuffers[nextImage].pipelineBarrier(
					vk::PipelineStageFlagBits::eTransfer,
					vk::PipelineStageFlagBits::eTransfer,
					vk::DependencyFlags(),
					nullptr,
					nullptr,
					textureTransferBarrier
				);

				// Blit
				vk::ImageSubresourceLayers subresourceLayers = vk::ImageSubresourceLayers()
					.setAspectMask(vk::ImageAspectFlagBits::eColor)
					.setMipLevel(0)
					.setBaseArrayLayer(0)
					.setLayerCount(1);

				std::array<vk::Offset3D, 2> srcOffsets;
				srcOffsets[0] = vk::Offset3D(0, 0, 0);
				srcOffsets[1] = vk::Offset3D(dynamic_cast<VK::Texture2D*>(srcImage)->width, dynamic_cast<VK::Texture2D*>(srcImage)->height, 1);

				// We need to flip y coordinate because Vulkan is left-handed, origin at top-left
				std::array<vk::Offset3D, 2> dstOffsets;
				dstOffsets[0] = vk::Offset3D(0, height, 0);
				dstOffsets[1] = vk::Offset3D(width, 0, 1);

				vk::ImageBlit blitRegions = vk::ImageBlit()
					.setSrcSubresource(subresourceLayers)
					.setSrcOffsets(srcOffsets)
					.setDstSubresource(subresourceLayers)
					.setDstOffsets(dstOffsets);

				commandBuffers[nextImage].blitImage(
					dynamic_cast<VK::Texture2D*>(srcImage)->image,
					vk::ImageLayout::eTransferSrcOptimal,
					images[nextImage],
					vk::ImageLayout::eTransferDstOptimal,
					blitRegions,
					vk::Filter::eNearest
				);

				commandBuffers[nextImage].pipelineBarrier(
					vk::PipelineStageFlagBits::eTransfer,
					vk::PipelineStageFlagBits::eTransfer,
					vk::DependencyFlags(),
					nullptr,
					nullptr,
					textureRestoreBarrier
				);
			}

			commandBuffers[nextImage].pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eBottomOfPipe,
				vk::DependencyFlags(),
				nullptr,
				nullptr,
				prePresentBarrier
			);
			commandBuffers[nextImage].end(); // stop recording

			vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);

			vk::SubmitInfo submitInfo = vk::SubmitInfo()
				.setWaitSemaphoreCount(1)
				.setPWaitSemaphores(&imageAvailableSemaphore)
				.setPWaitDstStageMask(&waitDstStageMask)
				.setCommandBufferCount(1)
				.setPCommandBuffers(&commandBuffers[nextImage])
				.setSignalSemaphoreCount(1)
				.setPSignalSemaphores(&renderingFinishedSemaphore);

			RendererState::RenderQueue().waitIdle(); //TODO: Remove
			RendererState::RenderQueue().submit(submitInfo, vk::Fence());

			vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
				.setWaitSemaphoreCount(1)
				.setPWaitSemaphores(&renderingFinishedSemaphore)
				.setSwapchainCount(1)
				.setPSwapchains(&swapchain)
				.setPImageIndices(&nextImage)
				.setPResults(nullptr);

			RendererState::RenderQueue().presentKHR(presentInfo);
		}

		BufferObject* CreateBuffer(BufferUsage usage)
		{
			return new BufferObject(TranslateUsageFlags(usage), vk::MemoryPropertyFlagBits::eDeviceLocal);
		}


		virtual BufferObject* CreateMappedBuffer(BufferUsage usage) override
		{
			return new BufferObject(TranslateUsageFlags(usage), vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		}

		Texture2D* CreateTexture2D(TextureUsage usage)
		{
			return new Texture2D(usage);
		}

		TextureSampler* CreateTextureSampler()
		{
			return new TextureSampler();
		}

		Shader* CreateShader(ShaderType stage, const char* data, int size)
		{
			Shader* result = new Shader();
			result->Create(stage, data, size);
			return result;
		}

		virtual RenderTargetLayout* CreateRenderTargetLayout(CoreLib::ArrayView<TextureUsage> bindings) override
		{
			return new RenderTargetLayout(bindings);
		}

		virtual PipelineBuilder* CreatePipelineBuilder() override
		{
			return new PipelineBuilder();
		}

		CommandBuffer* CreateCommandBuffer()
		{
			return new CommandBuffer();
		}

		virtual int UniformBufferAlignment() override
		{
			return (int)RendererState::PhysicalDevice().getProperties().limits.minUniformBufferOffsetAlignment;
		}
		virtual int StorageBufferAlignment() override
		{
			return (int)RendererState::PhysicalDevice().getProperties().limits.minStorageBufferOffsetAlignment;
		}

		virtual void * GetWindowHandle() override
		{
			return handle;
		}
	};
}

HardwareRenderer* GameEngine::CreateVulkanHardwareRenderer(int gpuId)
{
	VK::GpuId = gpuId;
	return new VK::HardwareRenderer();
}