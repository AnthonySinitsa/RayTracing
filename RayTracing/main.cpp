#include <windows.h>
#include <vulkan/vulkan.h>
#include <stdio.h>

#pragma comment(lib, "vulkan-1.lib")

// helper function/debug log function
// just in case window environment closes before we see error
// could also redirect error to a file
int dprintf(const char* format, ...) {
	static char s_printf_buf[1024];
	va_list args;
	va_start(args, format);
	_vsnprintf_s(s_printf_buf, sizeof(s_printf_buf), format, args);
	va_end(args);
	OutputDebugStringA(s_printf_buf);
	return 0;
}

int main() {
	VkApplicationInfo applicationInfo;
	VkInstanceCreateInfo instanceInfo;
	VkInstance instance;

	// Filling out application desc.
	// sType is mandatory
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	// pNext is mandatory
	applicationInfo.pNext = NULL;
	// application name
	applicationInfo.pApplicationName = "Hello World";
	// engine name
	applicationInfo.pEngineName = NULL;
	// engine version
	applicationInfo.engineVersion = 1;
	// version of vulkane
	applicationInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);

	// Filling out instance info
	// sType is mandatory
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	// pNext is mandatory
	instanceInfo.pNext = NULL;
	// flags is mandatory
	instanceInfo.flags = 0;
	// application infor structure is then passed through instance
	instanceInfo.pApplicationInfo = &applicationInfo;
	// don't enable and layer
	instanceInfo.enabledLayerCount = 0;
	instanceInfo.ppEnabledLayerNames = NULL;
	// don't enable extensions
	instanceInfo.enabledExtensionCount = 0;
	instanceInfo.ppEnabledExtensionNames = NULL;

	// Now create vulkane instance
	VkResult result = vkCreateInstance(&instanceInfo, NULL, &instance);
	if (result != VK_SUCCESS) {
		dprintf("Failed to create vulkane instance. Error number:%d", result);
		return 0; // can't continue
	}

	dprintf("Vulkan is ready!.");

	// code using Vulkan HERE

	// cleanup and goodbye
	vkDestroyInstance(instance, NULL);

}