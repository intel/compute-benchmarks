# Vulkan SDK

Fallback used by `source/framework/vk/CMakeLists.txt` when `find_package(Vulkan)` finds nothing,
or when `USE_SYSTEM_VULKAN=OFF`. It only needs to make the build work - running the benchmarks
additionally requires a Vulkan driver (ICD) installed on the system.

| Path | Origin |
|------|--------|
| `include/vulkan`, `include/vk_video` | LunarG Vulkan SDK 1.3.290.0 (`VK_HEADER_VERSION 290`), C headers only |
| `lib/x64/vulkan-1.lib` | LunarG Vulkan SDK 1.3.290.0, import library for `vulkan-1.dll` |
| `lib/x64/libvulkan.so` | Vulkan loader 1.3.275, `SONAME libvulkan.so.1` |

The headers are Apache-2.0 / MIT, see [LICENSE.md](LICENSE.md).
