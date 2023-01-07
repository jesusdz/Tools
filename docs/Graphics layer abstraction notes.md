# Graphics layer abstraction notes

## Vulkan / D3D12 analogues

| Direct3D 12        | Vulkan             |
|--------------------|--------------------|
|                    | Instance           |
| Adapter            | PhysicalDevice     |
| Device             | Device             |
|                    | SurfaceKHR         |
| SwapChain          | SwapchainKHR       |
| DescriptorHeap     |                    |
| Resource           |                    |
|                    | ImageView          |
|                    | Framebuffer        |
| CommandQueue       | Queue              |
| CommandAllocator   | CommandPool        |
| CommandList        | CommandBuffer      |

