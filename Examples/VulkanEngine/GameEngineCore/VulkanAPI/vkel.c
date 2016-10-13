/*========================================================================
/* Name
/*     Vulkan (Cross-Platform) Extension Loader
/*
/* Repository
/*
/* Overview
/*     This is a simple, dynamic and tiny cross-platform Vulkan
/*     extension loader.
/*
/* Dependencies
/*     Vulkan (library)
/*     Windows (header) - needed for library loading on Windows
/*     dlfcn (header) - needed for library loading on non-Windows OS'
/*     Standard C Libraries (stdio, stdlib, string, assert) - needed for NULL, malloc()
/*                                                 calloc(), free(), memset(), assert()
/*
/* Notice
/*     Copyright (c) 2016 Christian Vallentin <mail@vallentinsource.com>
/*
/* Developers & Contributors
/*     Christian Vallentin <mail@vallentinsource.com>
/*
/* Version History
/*     Last Modified Date: May 16, 2016
/*     Revision: 14
/*     Version: 2.0.11
/*
/* Revision History
/*     Revision 14, 2016/05/16
/*       - Updated support for Vulkan 1.0.14
/*
/*     Revision 13, 2016/05/15
/*       - Updated support for Vulkan 1.0.13
/*
/*     Revision 12, 2016/05/02
/*       - Fixed "for loop initial declarations are only
/*         allowed in C99 mode".
/*
/*     Revision 11, 2016/05/01
/*       - Updated support for Vulkan 1.0.12
/*
/*     Revision 10, 2016/04/22
/*       - Updated support for Vulkan 1.0.11
/*
/*     Revision 9, 2016/04/18
/*       - Updated support for Vulkan 1.0.10
/*
/*     Revision 8, 2016/04/08
/*       - Updated support for Vulkan 1.0.9
/*
/*     Revision 7, 2016/04/03
/*       - Updated support for Vulkan 1.0.8
/*
/*     Revision 6, 2016/03/26
/*       - Updated support for Vulkan 1.0.7
/*
/*     Revision 5, 2016/03/12
/*       - Updated support for Vulkan 1.0.6
/*
/*     Revision 4, 2016/03/06
/*       - Updated support for Vulkan 1.0.5
/*
/*     Revision 3, 2016/02/26
/*       - Rewrote vkel_gen.py, now it parses and directly
/*         adds vulkan.h and vk_platform.h into vkel.h,
/*         along with moving the appropriate copyrights
/*         to the top of vkel.h.
/*       - Fixed/added better differentiation for instance
/*         and device related calls.
/*       - Removed the need for having the vukan.h and
/*         vk_platform.h headers.
/*       - Updated support for Vulkan 1.0.4
/*
/*     Revision 2, 2016/02/24
/*       - Created a Python script for automatically generating
/*         all the extensions and their functions. (Tested with
/*         Python 3.5.1)
/*       - Added cross-platform support, for loading libraries
/*         and getting the function addresses.
/*       - Fixed so platform specific functions defaults to NULL
/*       - Added missing include for dlfcn (used on non-Window OS')
/*
/*     Revision 1, 2016/02/23
/*       - Implemented the basic version supporting a few (manually
/*         written) dynamically loaded functions.
/*
/*------------------------------------------------------------------------
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
/* IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
/* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
/* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
/* OTHER DEALINGS IN THE SOFTWARE.
/*
/* Permission is granted to anyone to use this software for any purpose,
/* including commercial applications, and to alter it and redistribute it
/* freely, subject to the following restrictions:
/*
/* 1. The origin of this software must not be misrepresented; you must not
/*    claim that you wrote the original software. If you use this software
/*    in a product, an acknowledgment in the product documentation would
/*    be appreciated but is not required.
/*
/* 2. Altered source versions must be plainly marked as such, and must not
/*    be misrepresented as being the original software.
/*
/* 3. This notice may not be removed or altered from any source
/*    distribution.
/*========================================================================*/

/*
** Copyright (c) 2015-2016 The Khronos Group Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include "vkel.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include <stdio.h> /* NULL, printf() */
#include <stdlib.h> /* malloc(), calloc(), free() */
#include <string.h> /* memset() */
#include <assert.h> /* assert() */


#ifdef VK_USE_PLATFORM_WIN32_KHR

#ifndef VC_EXTRALEAN
#	define VC_EXTRALEAN
#endif

#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN
#endif

	/* STOP THAT, BAD WINDOWS! */
#ifndef NOMINMAX
#	define NOMINMAX
#endif

#include <Windows.h>
#else

#include <dlfcn.h>

#endif


#ifdef VK_USE_PLATFORM_WIN32_KHR
#	define vkelPlatformOpenLibrary(name) LoadLibraryA(name)
#	define vkelPlatformCloseLibrary(handle) FreeLibrary((HMODULE) handle)
#	define vkelPlatformGetProcAddr(handle, name) GetProcAddress((HMODULE) handle, name)
	/* #elif defined(__APPLE__) || defined(__linux__) || defined(__ANDROID__) || defined(__unix__ ) */
#else
#define vkelPlatformOpenLibrary(name) dlopen(name, RTLD_LAZY | RTLD_LOCAL)
#define vkelPlatformCloseLibrary(handle) dlclose(handle)
#define vkelPlatformGetProcAddr(handle, name) dlsym(handle, name)
	// #else
	// #	error VKEL Unsupported Platform
#endif

	static void *vkelVkLibHandle;


	// Instance and device extension names
	VkBool32 VKEL_AMD_gcn_shader;
	VkBool32 VKEL_AMD_rasterization_order;
	VkBool32 VKEL_AMD_shader_explicit_vertex_parameter;
	VkBool32 VKEL_AMD_shader_trinary_minmax;
	VkBool32 VKEL_EXT_debug_marker;
	VkBool32 VKEL_EXT_debug_report;
	VkBool32 VKEL_IMG_filter_cubic;
	VkBool32 VKEL_KHR_android_surface;
	VkBool32 VKEL_KHR_display;
	VkBool32 VKEL_KHR_display_swapchain;
	VkBool32 VKEL_KHR_mir_surface;
	VkBool32 VKEL_KHR_sampler_mirror_clamp_to_edge;
	VkBool32 VKEL_KHR_surface;
	VkBool32 VKEL_KHR_swapchain;
	VkBool32 VKEL_KHR_wayland_surface;
	VkBool32 VKEL_KHR_win32_surface;
	VkBool32 VKEL_KHR_xcb_surface;
	VkBool32 VKEL_KHR_xlib_surface;
	VkBool32 VKEL_NV_glsl_shader;

	// Instance and device layer names
	VkBool32 VKEL_LAYER_GOOGLE_unique_objects;
	VkBool32 VKEL_LAYER_LUNARG_api_dump;
	VkBool32 VKEL_LAYER_LUNARG_device_limits;
	VkBool32 VKEL_LAYER_LUNARG_draw_state;
	VkBool32 VKEL_LAYER_LUNARG_image;
	VkBool32 VKEL_LAYER_LUNARG_mem_tracker;
	VkBool32 VKEL_LAYER_LUNARG_object_tracker;
	VkBool32 VKEL_LAYER_LUNARG_param_checker;
	VkBool32 VKEL_LAYER_LUNARG_screenshot;
	VkBool32 VKEL_LAYER_LUNARG_swapchain;
	VkBool32 VKEL_LAYER_LUNARG_threading;
	VkBool32 VKEL_LAYER_LUNARG_vktrace;


	// Functions
	PFN_vkAcquireNextImageKHR __vkAcquireNextImageKHR;
	PFN_vkAllocateCommandBuffers __vkAllocateCommandBuffers;
	PFN_vkAllocateDescriptorSets __vkAllocateDescriptorSets;
	PFN_vkAllocateMemory __vkAllocateMemory;
	PFN_vkAllocationFunction __vkAllocationFunction;
	PFN_vkBeginCommandBuffer __vkBeginCommandBuffer;
	PFN_vkBindBufferMemory __vkBindBufferMemory;
	PFN_vkBindImageMemory __vkBindImageMemory;
	PFN_vkCmdBeginQuery __vkCmdBeginQuery;
	PFN_vkCmdBeginRenderPass __vkCmdBeginRenderPass;
	PFN_vkCmdBindDescriptorSets __vkCmdBindDescriptorSets;
	PFN_vkCmdBindIndexBuffer __vkCmdBindIndexBuffer;
	PFN_vkCmdBindPipeline __vkCmdBindPipeline;
	PFN_vkCmdBindVertexBuffers __vkCmdBindVertexBuffers;
	PFN_vkCmdBlitImage __vkCmdBlitImage;
	PFN_vkCmdClearAttachments __vkCmdClearAttachments;
	PFN_vkCmdClearColorImage __vkCmdClearColorImage;
	PFN_vkCmdClearDepthStencilImage __vkCmdClearDepthStencilImage;
	PFN_vkCmdCopyBuffer __vkCmdCopyBuffer;
	PFN_vkCmdCopyBufferToImage __vkCmdCopyBufferToImage;
	PFN_vkCmdCopyImage __vkCmdCopyImage;
	PFN_vkCmdCopyImageToBuffer __vkCmdCopyImageToBuffer;
	PFN_vkCmdCopyQueryPoolResults __vkCmdCopyQueryPoolResults;
	PFN_vkCmdDebugMarkerBeginEXT __vkCmdDebugMarkerBeginEXT;
	PFN_vkCmdDebugMarkerEndEXT __vkCmdDebugMarkerEndEXT;
	PFN_vkCmdDebugMarkerInsertEXT __vkCmdDebugMarkerInsertEXT;
	PFN_vkCmdDispatch __vkCmdDispatch;
	PFN_vkCmdDispatchIndirect __vkCmdDispatchIndirect;
	PFN_vkCmdDraw __vkCmdDraw;
	PFN_vkCmdDrawIndexed __vkCmdDrawIndexed;
	PFN_vkCmdDrawIndexedIndirect __vkCmdDrawIndexedIndirect;
	PFN_vkCmdDrawIndirect __vkCmdDrawIndirect;
	PFN_vkCmdEndQuery __vkCmdEndQuery;
	PFN_vkCmdEndRenderPass __vkCmdEndRenderPass;
	PFN_vkCmdExecuteCommands __vkCmdExecuteCommands;
	PFN_vkCmdFillBuffer __vkCmdFillBuffer;
	PFN_vkCmdNextSubpass __vkCmdNextSubpass;
	PFN_vkCmdPipelineBarrier __vkCmdPipelineBarrier;
	PFN_vkCmdPushConstants __vkCmdPushConstants;
	PFN_vkCmdResetEvent __vkCmdResetEvent;
	PFN_vkCmdResetQueryPool __vkCmdResetQueryPool;
	PFN_vkCmdResolveImage __vkCmdResolveImage;
	PFN_vkCmdSetBlendConstants __vkCmdSetBlendConstants;
	PFN_vkCmdSetDepthBias __vkCmdSetDepthBias;
	PFN_vkCmdSetDepthBounds __vkCmdSetDepthBounds;
	PFN_vkCmdSetEvent __vkCmdSetEvent;
	PFN_vkCmdSetLineWidth __vkCmdSetLineWidth;
	PFN_vkCmdSetScissor __vkCmdSetScissor;
	PFN_vkCmdSetStencilCompareMask __vkCmdSetStencilCompareMask;
	PFN_vkCmdSetStencilReference __vkCmdSetStencilReference;
	PFN_vkCmdSetStencilWriteMask __vkCmdSetStencilWriteMask;
	PFN_vkCmdSetViewport __vkCmdSetViewport;
	PFN_vkCmdUpdateBuffer __vkCmdUpdateBuffer;
	PFN_vkCmdWaitEvents __vkCmdWaitEvents;
	PFN_vkCmdWriteTimestamp __vkCmdWriteTimestamp;
	PFN_vkCreateBuffer __vkCreateBuffer;
	PFN_vkCreateBufferView __vkCreateBufferView;
	PFN_vkCreateCommandPool __vkCreateCommandPool;
	PFN_vkCreateComputePipelines __vkCreateComputePipelines;
	PFN_vkCreateDebugReportCallbackEXT __vkCreateDebugReportCallbackEXT;
	PFN_vkCreateDescriptorPool __vkCreateDescriptorPool;
	PFN_vkCreateDescriptorSetLayout __vkCreateDescriptorSetLayout;
	PFN_vkCreateDevice __vkCreateDevice;
	PFN_vkCreateDisplayModeKHR __vkCreateDisplayModeKHR;
	PFN_vkCreateDisplayPlaneSurfaceKHR __vkCreateDisplayPlaneSurfaceKHR;
	PFN_vkCreateEvent __vkCreateEvent;
	PFN_vkCreateFence __vkCreateFence;
	PFN_vkCreateFramebuffer __vkCreateFramebuffer;
	PFN_vkCreateGraphicsPipelines __vkCreateGraphicsPipelines;
	PFN_vkCreateImage __vkCreateImage;
	PFN_vkCreateImageView __vkCreateImageView;
	PFN_vkCreateInstance __vkCreateInstance;
	PFN_vkCreatePipelineCache __vkCreatePipelineCache;
	PFN_vkCreatePipelineLayout __vkCreatePipelineLayout;
	PFN_vkCreateQueryPool __vkCreateQueryPool;
	PFN_vkCreateRenderPass __vkCreateRenderPass;
	PFN_vkCreateSampler __vkCreateSampler;
	PFN_vkCreateSemaphore __vkCreateSemaphore;
	PFN_vkCreateShaderModule __vkCreateShaderModule;
	PFN_vkCreateSharedSwapchainsKHR __vkCreateSharedSwapchainsKHR;
	PFN_vkCreateSwapchainKHR __vkCreateSwapchainKHR;
	PFN_vkDebugMarkerSetObjectNameEXT __vkDebugMarkerSetObjectNameEXT;
	PFN_vkDebugMarkerSetObjectTagEXT __vkDebugMarkerSetObjectTagEXT;
	PFN_vkDebugReportCallbackEXT __vkDebugReportCallbackEXT;
	PFN_vkDebugReportMessageEXT __vkDebugReportMessageEXT;
	PFN_vkDestroyBuffer __vkDestroyBuffer;
	PFN_vkDestroyBufferView __vkDestroyBufferView;
	PFN_vkDestroyCommandPool __vkDestroyCommandPool;
	PFN_vkDestroyDebugReportCallbackEXT __vkDestroyDebugReportCallbackEXT;
	PFN_vkDestroyDescriptorPool __vkDestroyDescriptorPool;
	PFN_vkDestroyDescriptorSetLayout __vkDestroyDescriptorSetLayout;
	PFN_vkDestroyDevice __vkDestroyDevice;
	PFN_vkDestroyEvent __vkDestroyEvent;
	PFN_vkDestroyFence __vkDestroyFence;
	PFN_vkDestroyFramebuffer __vkDestroyFramebuffer;
	PFN_vkDestroyImage __vkDestroyImage;
	PFN_vkDestroyImageView __vkDestroyImageView;
	PFN_vkDestroyInstance __vkDestroyInstance;
	PFN_vkDestroyPipeline __vkDestroyPipeline;
	PFN_vkDestroyPipelineCache __vkDestroyPipelineCache;
	PFN_vkDestroyPipelineLayout __vkDestroyPipelineLayout;
	PFN_vkDestroyQueryPool __vkDestroyQueryPool;
	PFN_vkDestroyRenderPass __vkDestroyRenderPass;
	PFN_vkDestroySampler __vkDestroySampler;
	PFN_vkDestroySemaphore __vkDestroySemaphore;
	PFN_vkDestroyShaderModule __vkDestroyShaderModule;
	PFN_vkDestroySurfaceKHR __vkDestroySurfaceKHR;
	PFN_vkDestroySwapchainKHR __vkDestroySwapchainKHR;
	PFN_vkDeviceWaitIdle __vkDeviceWaitIdle;
	PFN_vkEndCommandBuffer __vkEndCommandBuffer;
	PFN_vkEnumerateDeviceExtensionProperties __vkEnumerateDeviceExtensionProperties;
	PFN_vkEnumerateDeviceLayerProperties __vkEnumerateDeviceLayerProperties;
	PFN_vkEnumerateInstanceExtensionProperties __vkEnumerateInstanceExtensionProperties;
	PFN_vkEnumerateInstanceLayerProperties __vkEnumerateInstanceLayerProperties;
	PFN_vkEnumeratePhysicalDevices __vkEnumeratePhysicalDevices;
	PFN_vkFlushMappedMemoryRanges __vkFlushMappedMemoryRanges;
	PFN_vkFreeCommandBuffers __vkFreeCommandBuffers;
	PFN_vkFreeDescriptorSets __vkFreeDescriptorSets;
	PFN_vkFreeFunction __vkFreeFunction;
	PFN_vkFreeMemory __vkFreeMemory;
	PFN_vkGetBufferMemoryRequirements __vkGetBufferMemoryRequirements;
	PFN_vkGetDeviceMemoryCommitment __vkGetDeviceMemoryCommitment;
	PFN_vkGetDeviceProcAddr __vkGetDeviceProcAddr;
	PFN_vkGetDeviceQueue __vkGetDeviceQueue;
	PFN_vkGetDisplayModePropertiesKHR __vkGetDisplayModePropertiesKHR;
	PFN_vkGetDisplayPlaneCapabilitiesKHR __vkGetDisplayPlaneCapabilitiesKHR;
	PFN_vkGetDisplayPlaneSupportedDisplaysKHR __vkGetDisplayPlaneSupportedDisplaysKHR;
	PFN_vkGetEventStatus __vkGetEventStatus;
	PFN_vkGetFenceStatus __vkGetFenceStatus;
	PFN_vkGetImageMemoryRequirements __vkGetImageMemoryRequirements;
	PFN_vkGetImageSparseMemoryRequirements __vkGetImageSparseMemoryRequirements;
	PFN_vkGetImageSubresourceLayout __vkGetImageSubresourceLayout;
	PFN_vkGetInstanceProcAddr __vkGetInstanceProcAddr;
	PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR __vkGetPhysicalDeviceDisplayPlanePropertiesKHR;
	PFN_vkGetPhysicalDeviceDisplayPropertiesKHR __vkGetPhysicalDeviceDisplayPropertiesKHR;
	PFN_vkGetPhysicalDeviceFeatures __vkGetPhysicalDeviceFeatures;
	PFN_vkGetPhysicalDeviceFormatProperties __vkGetPhysicalDeviceFormatProperties;
	PFN_vkGetPhysicalDeviceImageFormatProperties __vkGetPhysicalDeviceImageFormatProperties;
	PFN_vkGetPhysicalDeviceMemoryProperties __vkGetPhysicalDeviceMemoryProperties;
	PFN_vkGetPhysicalDeviceProperties __vkGetPhysicalDeviceProperties;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties __vkGetPhysicalDeviceQueueFamilyProperties;
	PFN_vkGetPhysicalDeviceSparseImageFormatProperties __vkGetPhysicalDeviceSparseImageFormatProperties;
	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR __vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR __vkGetPhysicalDeviceSurfaceFormatsKHR;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR __vkGetPhysicalDeviceSurfacePresentModesKHR;
	PFN_vkGetPhysicalDeviceSurfaceSupportKHR __vkGetPhysicalDeviceSurfaceSupportKHR;
	PFN_vkGetPipelineCacheData __vkGetPipelineCacheData;
	PFN_vkGetQueryPoolResults __vkGetQueryPoolResults;
	PFN_vkGetRenderAreaGranularity __vkGetRenderAreaGranularity;
	PFN_vkGetSwapchainImagesKHR __vkGetSwapchainImagesKHR;
	PFN_vkInternalAllocationNotification __vkInternalAllocationNotification;
	PFN_vkInternalFreeNotification __vkInternalFreeNotification;
	PFN_vkInvalidateMappedMemoryRanges __vkInvalidateMappedMemoryRanges;
	PFN_vkMapMemory __vkMapMemory;
	PFN_vkMergePipelineCaches __vkMergePipelineCaches;
	PFN_vkQueueBindSparse __vkQueueBindSparse;
	PFN_vkQueuePresentKHR __vkQueuePresentKHR;
	PFN_vkQueueSubmit __vkQueueSubmit;
	PFN_vkQueueWaitIdle __vkQueueWaitIdle;
	PFN_vkReallocationFunction __vkReallocationFunction;
	PFN_vkResetCommandBuffer __vkResetCommandBuffer;
	PFN_vkResetCommandPool __vkResetCommandPool;
	PFN_vkResetDescriptorPool __vkResetDescriptorPool;
	PFN_vkResetEvent __vkResetEvent;
	PFN_vkResetFences __vkResetFences;
	PFN_vkSetEvent __vkSetEvent;
	PFN_vkUnmapMemory __vkUnmapMemory;
	PFN_vkUpdateDescriptorSets __vkUpdateDescriptorSets;
	PFN_vkVoidFunction __vkVoidFunction;
	PFN_vkWaitForFences __vkWaitForFences;

#ifdef VK_USE_PLATFORM_ANDROID_KHR
	PFN_vkCreateAndroidSurfaceKHR __vkCreateAndroidSurfaceKHR;
#endif /* VK_USE_PLATFORM_ANDROID_KHR */

#ifdef VK_USE_PLATFORM_MIR_KHR
	PFN_vkCreateMirSurfaceKHR __vkCreateMirSurfaceKHR;
	PFN_vkGetPhysicalDeviceMirPresentationSupportKHR __vkGetPhysicalDeviceMirPresentationSupportKHR;
#endif /* VK_USE_PLATFORM_MIR_KHR */

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
	PFN_vkCreateWaylandSurfaceKHR __vkCreateWaylandSurfaceKHR;
	PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR __vkGetPhysicalDeviceWaylandPresentationSupportKHR;
#endif /* VK_USE_PLATFORM_WAYLAND_KHR */

#ifdef VK_USE_PLATFORM_WIN32_KHR
	PFN_vkCreateWin32SurfaceKHR __vkCreateWin32SurfaceKHR;
	PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR __vkGetPhysicalDeviceWin32PresentationSupportKHR;
#endif /* VK_USE_PLATFORM_WIN32_KHR */

#ifdef VK_USE_PLATFORM_XCB_KHR
	PFN_vkCreateXcbSurfaceKHR __vkCreateXcbSurfaceKHR;
	PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR __vkGetPhysicalDeviceXcbPresentationSupportKHR;
#endif /* VK_USE_PLATFORM_XCB_KHR */

#ifdef VK_USE_PLATFORM_XLIB_KHR
	PFN_vkCreateXlibSurfaceKHR __vkCreateXlibSurfaceKHR;
	PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR __vkGetPhysicalDeviceXlibPresentationSupportKHR;
#endif /* VK_USE_PLATFORM_XLIB_KHR */


	PFN_vkVoidFunction vkelGetProcAddr(const char *name)
	{
		return (PFN_vkVoidFunction)vkelPlatformGetProcAddr(vkelVkLibHandle, name);
	}

	PFN_vkVoidFunction vkelGetInstanceProcAddr(VkInstance instance, const char *pName)
	{
		PFN_vkVoidFunction proc = (PFN_vkVoidFunction)vkGetInstanceProcAddr(instance, pName);

		if (!proc)
			proc = (PFN_vkVoidFunction)vkelGetProcAddr(pName);

		return proc;
	}

	PFN_vkVoidFunction vkelGetDeviceProcAddr(VkDevice device, const char *pName)
	{
		PFN_vkVoidFunction proc = (PFN_vkVoidFunction)vkGetDeviceProcAddr(device, pName);

		if (!proc)
			proc = (PFN_vkVoidFunction)vkelGetInstanceProcAddr(NULL, pName);

		return proc;
	}


	static int vkel_strcmp(const char *str1, const char *str2)
	{
		while (*str1 && (*str1 == *str2))
		{
			str1++, str2++;
		}

		return *(const unsigned char*)str1 - *(const unsigned char*)str2;
	}

	static void vkel_strpy(char *dest, char *src)
	{
		while (*src) {
			*dest = *src;
			dest++;
			src++;
		}
	}

	VkBool32 vkelInit(void)
	{
		vkelUninit();

#ifdef VK_USE_PLATFORM_WIN32_KHR
		const char *name = "vulkan-1.dll";
#else
		const char *name = "libvulkan.so.1";
#endif

		vkelVkLibHandle = vkelPlatformOpenLibrary(name);

		if (!vkelVkLibHandle)
			return VK_FALSE;

		__vkCreateInstance = (PFN_vkCreateInstance)vkelGetProcAddr("vkCreateInstance");
		__vkDestroyInstance = (PFN_vkDestroyInstance)vkelGetProcAddr("vkDestroyInstance");
		__vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)vkelGetProcAddr("vkEnumerateInstanceExtensionProperties");
		__vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)vkelGetProcAddr("vkEnumerateInstanceLayerProperties");
		__vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)vkelGetProcAddr("vkGetInstanceProcAddr");

		return VK_TRUE;
	}

	VkBool32 vkelInstanceInit(VkInstance instance)
	{
		if (!vkelVkLibHandle && !vkelInit())
			return VK_FALSE;
		__vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkelGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
		__vkCreateDevice = (PFN_vkCreateDevice)vkelGetInstanceProcAddr(instance, "vkCreateDevice");
		__vkCreateDisplayModeKHR = (PFN_vkCreateDisplayModeKHR)vkelGetInstanceProcAddr(instance, "vkCreateDisplayModeKHR");
		__vkCreateDisplayPlaneSurfaceKHR = (PFN_vkCreateDisplayPlaneSurfaceKHR)vkelGetInstanceProcAddr(instance, "vkCreateDisplayPlaneSurfaceKHR");
		__vkDebugReportCallbackEXT = (PFN_vkDebugReportCallbackEXT)vkelGetInstanceProcAddr(instance, "vkDebugReportCallbackEXT");
		__vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkelGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
		__vkDestroyDevice = (PFN_vkDestroyDevice)vkelGetInstanceProcAddr(instance, "vkDestroyDevice");
		__vkDestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)vkelGetInstanceProcAddr(instance, "vkDestroySurfaceKHR");
		__vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)vkelGetInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties");
		__vkEnumerateDeviceLayerProperties = (PFN_vkEnumerateDeviceLayerProperties)vkelGetInstanceProcAddr(instance, "vkEnumerateDeviceLayerProperties");
		__vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)vkelGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices");
		__vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vkelGetInstanceProcAddr(instance, "vkGetDeviceProcAddr");
		__vkGetPhysicalDeviceDisplayPlanePropertiesKHR = (PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR)vkelGetInstanceProcAddr(instance, "vkGetPhysicalDeviceDisplayPlanePropertiesKHR");
		__vkGetPhysicalDeviceDisplayPropertiesKHR = (PFN_vkGetPhysicalDeviceDisplayPropertiesKHR)vkelGetInstanceProcAddr(instance, "vkGetPhysicalDeviceDisplayPropertiesKHR");
		__vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures)vkelGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures");
		__vkGetPhysicalDeviceFormatProperties = (PFN_vkGetPhysicalDeviceFormatProperties)vkelGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFormatProperties");
		__vkGetPhysicalDeviceImageFormatProperties = (PFN_vkGetPhysicalDeviceImageFormatProperties)vkelGetInstanceProcAddr(instance, "vkGetPhysicalDeviceImageFormatProperties");
		__vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)vkelGetInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties");
		__vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)vkelGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties");
		__vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)vkelGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties");
		__vkGetPhysicalDeviceSparseImageFormatProperties = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties)vkelGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSparseImageFormatProperties");
		__vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkelGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
		__vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vkelGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
		__vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)vkelGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");
		__vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkelGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");

#ifdef VK_USE_PLATFORM_ANDROID_KHR
		__vkCreateAndroidSurfaceKHR = (PFN_vkCreateAndroidSurfaceKHR)vkelGetInstanceProcAddr(instance, "vkCreateAndroidSurfaceKHR");
#endif /* VK_USE_PLATFORM_ANDROID_KHR */

#ifdef VK_USE_PLATFORM_MIR_KHR
		__vkCreateMirSurfaceKHR = (PFN_vkCreateMirSurfaceKHR)vkelGetInstanceProcAddr(instance, "vkCreateMirSurfaceKHR");
		__vkGetPhysicalDeviceMirPresentationSupportKHR = (PFN_vkGetPhysicalDeviceMirPresentationSupportKHR)vkelGetInstanceProcAddr(instance, "vkGetPhysicalDeviceMirPresentationSupportKHR");
#endif /* VK_USE_PLATFORM_MIR_KHR */

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
		__vkCreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR)vkelGetInstanceProcAddr(instance, "vkCreateWaylandSurfaceKHR");
		__vkGetPhysicalDeviceWaylandPresentationSupportKHR = (PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR)vkelGetInstanceProcAddr(instance, "vkGetPhysicalDeviceWaylandPresentationSupportKHR");
#endif /* VK_USE_PLATFORM_WAYLAND_KHR */

#ifdef VK_USE_PLATFORM_WIN32_KHR
		__vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkelGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
		__vkGetPhysicalDeviceWin32PresentationSupportKHR = (PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR)vkelGetInstanceProcAddr(instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR");
#endif /* VK_USE_PLATFORM_WIN32_KHR */

#ifdef VK_USE_PLATFORM_XCB_KHR
		__vkCreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR)vkelGetInstanceProcAddr(instance, "vkCreateXcbSurfaceKHR");
		__vkGetPhysicalDeviceXcbPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR)vkelGetInstanceProcAddr(instance, "vkGetPhysicalDeviceXcbPresentationSupportKHR");
#endif /* VK_USE_PLATFORM_XCB_KHR */

#ifdef VK_USE_PLATFORM_XLIB_KHR
		__vkCreateXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR)vkelGetInstanceProcAddr(instance, "vkCreateXlibSurfaceKHR");
		__vkGetPhysicalDeviceXlibPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR)vkelGetInstanceProcAddr(instance, "vkGetPhysicalDeviceXlibPresentationSupportKHR");
#endif /* VK_USE_PLATFORM_XLIB_KHR */

		return VK_TRUE;
	}

	VkBool32 vkelDeviceInit(VkDevice device)
	{
		if (!vkelVkLibHandle && !vkelInit())
			return VK_FALSE;


		__vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)vkelGetDeviceProcAddr(device, "vkAcquireNextImageKHR");
		__vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)vkelGetDeviceProcAddr(device, "vkAllocateCommandBuffers");
		__vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)vkelGetDeviceProcAddr(device, "vkAllocateDescriptorSets");
		__vkAllocateMemory = (PFN_vkAllocateMemory)vkelGetDeviceProcAddr(device, "vkAllocateMemory");
		__vkAllocationFunction = (PFN_vkAllocationFunction)vkelGetDeviceProcAddr(device, "vkAllocationFunction");
		__vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)vkelGetDeviceProcAddr(device, "vkBeginCommandBuffer");
		__vkBindBufferMemory = (PFN_vkBindBufferMemory)vkelGetDeviceProcAddr(device, "vkBindBufferMemory");
		__vkBindImageMemory = (PFN_vkBindImageMemory)vkelGetDeviceProcAddr(device, "vkBindImageMemory");
		__vkCmdBeginQuery = (PFN_vkCmdBeginQuery)vkelGetDeviceProcAddr(device, "vkCmdBeginQuery");
		__vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)vkelGetDeviceProcAddr(device, "vkCmdBeginRenderPass");
		__vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)vkelGetDeviceProcAddr(device, "vkCmdBindDescriptorSets");
		__vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)vkelGetDeviceProcAddr(device, "vkCmdBindIndexBuffer");
		__vkCmdBindPipeline = (PFN_vkCmdBindPipeline)vkelGetDeviceProcAddr(device, "vkCmdBindPipeline");
		__vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)vkelGetDeviceProcAddr(device, "vkCmdBindVertexBuffers");
		__vkCmdBlitImage = (PFN_vkCmdBlitImage)vkelGetDeviceProcAddr(device, "vkCmdBlitImage");
		__vkCmdClearAttachments = (PFN_vkCmdClearAttachments)vkelGetDeviceProcAddr(device, "vkCmdClearAttachments");
		__vkCmdClearColorImage = (PFN_vkCmdClearColorImage)vkelGetDeviceProcAddr(device, "vkCmdClearColorImage");
		__vkCmdClearDepthStencilImage = (PFN_vkCmdClearDepthStencilImage)vkelGetDeviceProcAddr(device, "vkCmdClearDepthStencilImage");
		__vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)vkelGetDeviceProcAddr(device, "vkCmdCopyBuffer");
		__vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)vkelGetDeviceProcAddr(device, "vkCmdCopyBufferToImage");
		__vkCmdCopyImage = (PFN_vkCmdCopyImage)vkelGetDeviceProcAddr(device, "vkCmdCopyImage");
		__vkCmdCopyImageToBuffer = (PFN_vkCmdCopyImageToBuffer)vkelGetDeviceProcAddr(device, "vkCmdCopyImageToBuffer");
		__vkCmdCopyQueryPoolResults = (PFN_vkCmdCopyQueryPoolResults)vkelGetDeviceProcAddr(device, "vkCmdCopyQueryPoolResults");
		__vkCmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT)vkelGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
		__vkCmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT)vkelGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
		__vkCmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT)vkelGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");
		__vkCmdDispatch = (PFN_vkCmdDispatch)vkelGetDeviceProcAddr(device, "vkCmdDispatch");
		__vkCmdDispatchIndirect = (PFN_vkCmdDispatchIndirect)vkelGetDeviceProcAddr(device, "vkCmdDispatchIndirect");
		__vkCmdDraw = (PFN_vkCmdDraw)vkelGetDeviceProcAddr(device, "vkCmdDraw");
		__vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)vkelGetDeviceProcAddr(device, "vkCmdDrawIndexed");
		__vkCmdDrawIndexedIndirect = (PFN_vkCmdDrawIndexedIndirect)vkelGetDeviceProcAddr(device, "vkCmdDrawIndexedIndirect");
		__vkCmdDrawIndirect = (PFN_vkCmdDrawIndirect)vkelGetDeviceProcAddr(device, "vkCmdDrawIndirect");
		__vkCmdEndQuery = (PFN_vkCmdEndQuery)vkelGetDeviceProcAddr(device, "vkCmdEndQuery");
		__vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)vkelGetDeviceProcAddr(device, "vkCmdEndRenderPass");
		__vkCmdExecuteCommands = (PFN_vkCmdExecuteCommands)vkelGetDeviceProcAddr(device, "vkCmdExecuteCommands");
		__vkCmdFillBuffer = (PFN_vkCmdFillBuffer)vkelGetDeviceProcAddr(device, "vkCmdFillBuffer");
		__vkCmdNextSubpass = (PFN_vkCmdNextSubpass)vkelGetDeviceProcAddr(device, "vkCmdNextSubpass");
		__vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)vkelGetDeviceProcAddr(device, "vkCmdPipelineBarrier");
		__vkCmdPushConstants = (PFN_vkCmdPushConstants)vkelGetDeviceProcAddr(device, "vkCmdPushConstants");
		__vkCmdResetEvent = (PFN_vkCmdResetEvent)vkelGetDeviceProcAddr(device, "vkCmdResetEvent");
		__vkCmdResetQueryPool = (PFN_vkCmdResetQueryPool)vkelGetDeviceProcAddr(device, "vkCmdResetQueryPool");
		__vkCmdResolveImage = (PFN_vkCmdResolveImage)vkelGetDeviceProcAddr(device, "vkCmdResolveImage");
		__vkCmdSetBlendConstants = (PFN_vkCmdSetBlendConstants)vkelGetDeviceProcAddr(device, "vkCmdSetBlendConstants");
		__vkCmdSetDepthBias = (PFN_vkCmdSetDepthBias)vkelGetDeviceProcAddr(device, "vkCmdSetDepthBias");
		__vkCmdSetDepthBounds = (PFN_vkCmdSetDepthBounds)vkelGetDeviceProcAddr(device, "vkCmdSetDepthBounds");
		__vkCmdSetEvent = (PFN_vkCmdSetEvent)vkelGetDeviceProcAddr(device, "vkCmdSetEvent");
		__vkCmdSetLineWidth = (PFN_vkCmdSetLineWidth)vkelGetDeviceProcAddr(device, "vkCmdSetLineWidth");
		__vkCmdSetScissor = (PFN_vkCmdSetScissor)vkelGetDeviceProcAddr(device, "vkCmdSetScissor");
		__vkCmdSetStencilCompareMask = (PFN_vkCmdSetStencilCompareMask)vkelGetDeviceProcAddr(device, "vkCmdSetStencilCompareMask");
		__vkCmdSetStencilReference = (PFN_vkCmdSetStencilReference)vkelGetDeviceProcAddr(device, "vkCmdSetStencilReference");
		__vkCmdSetStencilWriteMask = (PFN_vkCmdSetStencilWriteMask)vkelGetDeviceProcAddr(device, "vkCmdSetStencilWriteMask");
		__vkCmdSetViewport = (PFN_vkCmdSetViewport)vkelGetDeviceProcAddr(device, "vkCmdSetViewport");
		__vkCmdUpdateBuffer = (PFN_vkCmdUpdateBuffer)vkelGetDeviceProcAddr(device, "vkCmdUpdateBuffer");
		__vkCmdWaitEvents = (PFN_vkCmdWaitEvents)vkelGetDeviceProcAddr(device, "vkCmdWaitEvents");
		__vkCmdWriteTimestamp = (PFN_vkCmdWriteTimestamp)vkelGetDeviceProcAddr(device, "vkCmdWriteTimestamp");
		__vkCreateBuffer = (PFN_vkCreateBuffer)vkelGetDeviceProcAddr(device, "vkCreateBuffer");
		__vkCreateBufferView = (PFN_vkCreateBufferView)vkelGetDeviceProcAddr(device, "vkCreateBufferView");
		__vkCreateCommandPool = (PFN_vkCreateCommandPool)vkelGetDeviceProcAddr(device, "vkCreateCommandPool");
		__vkCreateComputePipelines = (PFN_vkCreateComputePipelines)vkelGetDeviceProcAddr(device, "vkCreateComputePipelines");
		__vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)vkelGetDeviceProcAddr(device, "vkCreateDescriptorPool");
		__vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)vkelGetDeviceProcAddr(device, "vkCreateDescriptorSetLayout");
		__vkCreateDisplayModeKHR = (PFN_vkCreateDisplayModeKHR)vkelGetDeviceProcAddr(device, "vkCreateDisplayModeKHR");
		__vkCreateDisplayPlaneSurfaceKHR = (PFN_vkCreateDisplayPlaneSurfaceKHR)vkelGetDeviceProcAddr(device, "vkCreateDisplayPlaneSurfaceKHR");
		__vkCreateEvent = (PFN_vkCreateEvent)vkelGetDeviceProcAddr(device, "vkCreateEvent");
		__vkCreateFence = (PFN_vkCreateFence)vkelGetDeviceProcAddr(device, "vkCreateFence");
		__vkCreateFramebuffer = (PFN_vkCreateFramebuffer)vkelGetDeviceProcAddr(device, "vkCreateFramebuffer");
		__vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)vkelGetDeviceProcAddr(device, "vkCreateGraphicsPipelines");
		__vkCreateImage = (PFN_vkCreateImage)vkelGetDeviceProcAddr(device, "vkCreateImage");
		__vkCreateImageView = (PFN_vkCreateImageView)vkelGetDeviceProcAddr(device, "vkCreateImageView");
		__vkCreatePipelineCache = (PFN_vkCreatePipelineCache)vkelGetDeviceProcAddr(device, "vkCreatePipelineCache");
		__vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)vkelGetDeviceProcAddr(device, "vkCreatePipelineLayout");
		__vkCreateQueryPool = (PFN_vkCreateQueryPool)vkelGetDeviceProcAddr(device, "vkCreateQueryPool");
		__vkCreateRenderPass = (PFN_vkCreateRenderPass)vkelGetDeviceProcAddr(device, "vkCreateRenderPass");
		__vkCreateSampler = (PFN_vkCreateSampler)vkelGetDeviceProcAddr(device, "vkCreateSampler");
		__vkCreateSemaphore = (PFN_vkCreateSemaphore)vkelGetDeviceProcAddr(device, "vkCreateSemaphore");
		__vkCreateShaderModule = (PFN_vkCreateShaderModule)vkelGetDeviceProcAddr(device, "vkCreateShaderModule");
		__vkCreateSharedSwapchainsKHR = (PFN_vkCreateSharedSwapchainsKHR)vkelGetDeviceProcAddr(device, "vkCreateSharedSwapchainsKHR");
		__vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)vkelGetDeviceProcAddr(device, "vkCreateSwapchainKHR");
		__vkDebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT)vkelGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");
		__vkDebugMarkerSetObjectTagEXT = (PFN_vkDebugMarkerSetObjectTagEXT)vkelGetDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT");
		__vkDebugReportCallbackEXT = (PFN_vkDebugReportCallbackEXT)vkelGetDeviceProcAddr(device, "vkDebugReportCallbackEXT");
		__vkDebugReportMessageEXT = (PFN_vkDebugReportMessageEXT)vkelGetDeviceProcAddr(device, "vkDebugReportMessageEXT");
		__vkDestroyBuffer = (PFN_vkDestroyBuffer)vkelGetDeviceProcAddr(device, "vkDestroyBuffer");
		__vkDestroyBufferView = (PFN_vkDestroyBufferView)vkelGetDeviceProcAddr(device, "vkDestroyBufferView");
		__vkDestroyCommandPool = (PFN_vkDestroyCommandPool)vkelGetDeviceProcAddr(device, "vkDestroyCommandPool");
		__vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)vkelGetDeviceProcAddr(device, "vkDestroyDescriptorPool");
		__vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)vkelGetDeviceProcAddr(device, "vkDestroyDescriptorSetLayout");
		__vkDestroyEvent = (PFN_vkDestroyEvent)vkelGetDeviceProcAddr(device, "vkDestroyEvent");
		__vkDestroyFence = (PFN_vkDestroyFence)vkelGetDeviceProcAddr(device, "vkDestroyFence");
		__vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)vkelGetDeviceProcAddr(device, "vkDestroyFramebuffer");
		__vkDestroyImage = (PFN_vkDestroyImage)vkelGetDeviceProcAddr(device, "vkDestroyImage");
		__vkDestroyImageView = (PFN_vkDestroyImageView)vkelGetDeviceProcAddr(device, "vkDestroyImageView");
		__vkDestroyPipeline = (PFN_vkDestroyPipeline)vkelGetDeviceProcAddr(device, "vkDestroyPipeline");
		__vkDestroyPipelineCache = (PFN_vkDestroyPipelineCache)vkelGetDeviceProcAddr(device, "vkDestroyPipelineCache");
		__vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)vkelGetDeviceProcAddr(device, "vkDestroyPipelineLayout");
		__vkDestroyQueryPool = (PFN_vkDestroyQueryPool)vkelGetDeviceProcAddr(device, "vkDestroyQueryPool");
		__vkDestroyRenderPass = (PFN_vkDestroyRenderPass)vkelGetDeviceProcAddr(device, "vkDestroyRenderPass");
		__vkDestroySampler = (PFN_vkDestroySampler)vkelGetDeviceProcAddr(device, "vkDestroySampler");
		__vkDestroySemaphore = (PFN_vkDestroySemaphore)vkelGetDeviceProcAddr(device, "vkDestroySemaphore");
		__vkDestroyShaderModule = (PFN_vkDestroyShaderModule)vkelGetDeviceProcAddr(device, "vkDestroyShaderModule");
		__vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)vkelGetDeviceProcAddr(device, "vkDestroySwapchainKHR");
		__vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)vkelGetDeviceProcAddr(device, "vkDeviceWaitIdle");
		__vkEndCommandBuffer = (PFN_vkEndCommandBuffer)vkelGetDeviceProcAddr(device, "vkEndCommandBuffer");
		__vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)vkelGetDeviceProcAddr(device, "vkEnumerateDeviceExtensionProperties");
		__vkEnumerateDeviceLayerProperties = (PFN_vkEnumerateDeviceLayerProperties)vkelGetDeviceProcAddr(device, "vkEnumerateDeviceLayerProperties");
		__vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges)vkelGetDeviceProcAddr(device, "vkFlushMappedMemoryRanges");
		__vkFreeCommandBuffers = (PFN_vkFreeCommandBuffers)vkelGetDeviceProcAddr(device, "vkFreeCommandBuffers");
		__vkFreeDescriptorSets = (PFN_vkFreeDescriptorSets)vkelGetDeviceProcAddr(device, "vkFreeDescriptorSets");
		__vkFreeFunction = (PFN_vkFreeFunction)vkelGetDeviceProcAddr(device, "vkFreeFunction");
		__vkFreeMemory = (PFN_vkFreeMemory)vkelGetDeviceProcAddr(device, "vkFreeMemory");
		__vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)vkelGetDeviceProcAddr(device, "vkGetBufferMemoryRequirements");
		__vkGetDeviceMemoryCommitment = (PFN_vkGetDeviceMemoryCommitment)vkelGetDeviceProcAddr(device, "vkGetDeviceMemoryCommitment");
		__vkGetDeviceQueue = (PFN_vkGetDeviceQueue)vkelGetDeviceProcAddr(device, "vkGetDeviceQueue");
		__vkGetDisplayModePropertiesKHR = (PFN_vkGetDisplayModePropertiesKHR)vkelGetDeviceProcAddr(device, "vkGetDisplayModePropertiesKHR");
		__vkGetDisplayPlaneCapabilitiesKHR = (PFN_vkGetDisplayPlaneCapabilitiesKHR)vkelGetDeviceProcAddr(device, "vkGetDisplayPlaneCapabilitiesKHR");
		__vkGetDisplayPlaneSupportedDisplaysKHR = (PFN_vkGetDisplayPlaneSupportedDisplaysKHR)vkelGetDeviceProcAddr(device, "vkGetDisplayPlaneSupportedDisplaysKHR");
		__vkGetEventStatus = (PFN_vkGetEventStatus)vkelGetDeviceProcAddr(device, "vkGetEventStatus");
		__vkGetFenceStatus = (PFN_vkGetFenceStatus)vkelGetDeviceProcAddr(device, "vkGetFenceStatus");
		__vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)vkelGetDeviceProcAddr(device, "vkGetImageMemoryRequirements");
		__vkGetImageSparseMemoryRequirements = (PFN_vkGetImageSparseMemoryRequirements)vkelGetDeviceProcAddr(device, "vkGetImageSparseMemoryRequirements");
		__vkGetImageSubresourceLayout = (PFN_vkGetImageSubresourceLayout)vkelGetDeviceProcAddr(device, "vkGetImageSubresourceLayout");
		__vkGetPipelineCacheData = (PFN_vkGetPipelineCacheData)vkelGetDeviceProcAddr(device, "vkGetPipelineCacheData");
		__vkGetQueryPoolResults = (PFN_vkGetQueryPoolResults)vkelGetDeviceProcAddr(device, "vkGetQueryPoolResults");
		__vkGetRenderAreaGranularity = (PFN_vkGetRenderAreaGranularity)vkelGetDeviceProcAddr(device, "vkGetRenderAreaGranularity");
		__vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)vkelGetDeviceProcAddr(device, "vkGetSwapchainImagesKHR");
		__vkInternalAllocationNotification = (PFN_vkInternalAllocationNotification)vkelGetDeviceProcAddr(device, "vkInternalAllocationNotification");
		__vkInternalFreeNotification = (PFN_vkInternalFreeNotification)vkelGetDeviceProcAddr(device, "vkInternalFreeNotification");
		__vkInvalidateMappedMemoryRanges = (PFN_vkInvalidateMappedMemoryRanges)vkelGetDeviceProcAddr(device, "vkInvalidateMappedMemoryRanges");
		__vkMapMemory = (PFN_vkMapMemory)vkelGetDeviceProcAddr(device, "vkMapMemory");
		__vkMergePipelineCaches = (PFN_vkMergePipelineCaches)vkelGetDeviceProcAddr(device, "vkMergePipelineCaches");
		__vkQueueBindSparse = (PFN_vkQueueBindSparse)vkelGetDeviceProcAddr(device, "vkQueueBindSparse");
		__vkQueuePresentKHR = (PFN_vkQueuePresentKHR)vkelGetDeviceProcAddr(device, "vkQueuePresentKHR");
		__vkQueueSubmit = (PFN_vkQueueSubmit)vkelGetDeviceProcAddr(device, "vkQueueSubmit");
		__vkQueueWaitIdle = (PFN_vkQueueWaitIdle)vkelGetDeviceProcAddr(device, "vkQueueWaitIdle");
		__vkReallocationFunction = (PFN_vkReallocationFunction)vkelGetDeviceProcAddr(device, "vkReallocationFunction");
		__vkResetCommandBuffer = (PFN_vkResetCommandBuffer)vkelGetDeviceProcAddr(device, "vkResetCommandBuffer");
		__vkResetCommandPool = (PFN_vkResetCommandPool)vkelGetDeviceProcAddr(device, "vkResetCommandPool");
		__vkResetDescriptorPool = (PFN_vkResetDescriptorPool)vkelGetDeviceProcAddr(device, "vkResetDescriptorPool");
		__vkResetEvent = (PFN_vkResetEvent)vkelGetDeviceProcAddr(device, "vkResetEvent");
		__vkResetFences = (PFN_vkResetFences)vkelGetDeviceProcAddr(device, "vkResetFences");
		__vkSetEvent = (PFN_vkSetEvent)vkelGetDeviceProcAddr(device, "vkSetEvent");
		__vkUnmapMemory = (PFN_vkUnmapMemory)vkelGetDeviceProcAddr(device, "vkUnmapMemory");
		__vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)vkelGetDeviceProcAddr(device, "vkUpdateDescriptorSets");
		__vkVoidFunction = (PFN_vkVoidFunction)vkelGetDeviceProcAddr(device, "vkVoidFunction");
		__vkWaitForFences = (PFN_vkWaitForFences)vkelGetDeviceProcAddr(device, "vkWaitForFences");

#ifdef VK_USE_PLATFORM_ANDROID_KHR
		__vkCreateAndroidSurfaceKHR = (PFN_vkCreateAndroidSurfaceKHR)vkelGetDeviceProcAddr(device, "vkCreateAndroidSurfaceKHR");
#endif /* VK_USE_PLATFORM_ANDROID_KHR */

#ifdef VK_USE_PLATFORM_MIR_KHR
		__vkCreateMirSurfaceKHR = (PFN_vkCreateMirSurfaceKHR)vkelGetDeviceProcAddr(device, "vkCreateMirSurfaceKHR");
		__vkGetPhysicalDeviceMirPresentationSupportKHR = (PFN_vkGetPhysicalDeviceMirPresentationSupportKHR)vkelGetDeviceProcAddr(device, "vkGetPhysicalDeviceMirPresentationSupportKHR");
#endif /* VK_USE_PLATFORM_MIR_KHR */

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
		__vkCreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR)vkelGetDeviceProcAddr(device, "vkCreateWaylandSurfaceKHR");
		__vkGetPhysicalDeviceWaylandPresentationSupportKHR = (PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR)vkelGetDeviceProcAddr(device, "vkGetPhysicalDeviceWaylandPresentationSupportKHR");
#endif /* VK_USE_PLATFORM_WAYLAND_KHR */


#ifdef VK_USE_PLATFORM_XCB_KHR
		__vkCreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR)vkelGetDeviceProcAddr(device, "vkCreateXcbSurfaceKHR");
		__vkGetPhysicalDeviceXcbPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR)vkelGetDeviceProcAddr(device, "vkGetPhysicalDeviceXcbPresentationSupportKHR");
#endif /* VK_USE_PLATFORM_XCB_KHR */

#ifdef VK_USE_PLATFORM_XLIB_KHR
		__vkCreateXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR)vkelGetDeviceProcAddr(device, "vkCreateXlibSurfaceKHR");
		__vkGetPhysicalDeviceXlibPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR)vkelGetDeviceProcAddr(device, "vkGetPhysicalDeviceXlibPresentationSupportKHR");
#endif /* VK_USE_PLATFORM_XLIB_KHR */

		return VK_TRUE;
	}

	void vkelUninit(void)
	{
		if (vkelVkLibHandle)
		{
			vkelPlatformCloseLibrary(vkelVkLibHandle);
			vkelVkLibHandle = NULL;
		}
	}


#ifdef __cplusplus
}
#endif /* __cplusplus */