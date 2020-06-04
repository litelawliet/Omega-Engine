/*#include <optix.h>
#include <optix_stubs.h>
#include <optix_function_table_definition.h>
#include <cuda.h>

namespace Denoiser
{

    static void CUDA_CHECK(CUresult result)
    {
        if (result != CUDA_SUCCESS) {
            const char* errorName = nullptr;
            if (CUDA_ERROR_INVALID_VALUE != cuGetErrorName(result, &errorName)) {
                Q_CHECK_PTR(errorName);
                const char* errorString = nullptr;
                if (CUDA_ERROR_INVALID_VALUE != cuGetErrorString(result, &errorString)) {
                    Q_CHECK_PTR(errorString);
                    qFatal("CUDA runtime error (%s): %s", errorName, errorString);
                }
            }
        }
    }

    static void OPTIX_CHECK(OptixResult error)
    {
        if (error != OPTIX_SUCCESS) {
            qFatal("CUDA runtime error (%s): %s", optixGetErrorName(error), optixGetErrorString(error));
        }
    }

    struct MemoryType
    {
        vk::PhysicalDevice physicalDevice;
        vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();

        vk::Bool32 getMemoryType(uint32_t typeBits, vk::MemoryPropertyFlags properties, uint32_t* typeIndex) const
        {
            for (*typeIndex = 0; *typeIndex < memoryProperties.memoryTypeCount; ++ * typeIndex) {
                uint32_t typeBit = (uint32_t(1) << *typeIndex);
                if ((typeBits & typeBit) == typeBit) {
                    if ((memoryProperties.memoryTypes[*typeIndex].propertyFlags & properties) == properties) {
                        return VK_TRUE;
                    }
                }
            }
            return VK_FALSE;
        }

        uint32_t getMemoryType(uint32_t typeBits, vk::MemoryPropertyFlags properties) const
        {
            uint32_t result = 0;
            if (VK_FALSE == getMemoryType(typeBits, properties, &result)) {
                qFatal("Unable to find memory type %s", vk::to_string(properties).c_str());
            }
            return result;
        }
    };

#ifdef VK_USE_PLATFORM_WIN32_KHR
    static constexpr auto defaultExternalMemoryHandleType = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;
#else
    static constexpr auto defaultExternalMemoryHandleType = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
#endif

    struct BufferCuda
    {
        vk::UniqueBuffer buffer;
        vk::UniqueDeviceMemory allocation;

#ifdef VK_USE_PLATFORM_WIN32_KHR
        struct Win32Handle
        {
            void reset(HANDLE newWin32Handle = {})
            {
                if (win32Handle) {
                    CloseHandle(win32Handle);
                }
                win32Handle = newWin32Handle;
            }

            ~Win32Handle()
            {
                reset();
            }

            HANDLE get() const
            {
                return win32Handle;
            }

        private:
            HANDLE win32Handle = {};
        } win32Handle;
#else
        int fd = 0;
#endif
        CUdeviceptr p = 0;

        BufferCuda() = default;

        void create(vk::Device device, vk::PhysicalDevice physicalDevice, const vk::BufferCreateInfo& bufferCreateInfo, vk::MemoryPropertyFlags properties = vk::MemoryPropertyFlagBits::eDeviceLocal)
        {
            buffer = device.createBufferUnique(bufferCreateInfo);

            vk::BufferMemoryRequirementsInfo2 bufferMemoryRequirementsInfo{ *buffer };
            auto memoryRequirementsStructureChain = device.getBufferMemoryRequirements2<vk::MemoryRequirements2, vk::MemoryDedicatedRequirements>(bufferMemoryRequirementsInfo);
            const auto& memoryRequirements = memoryRequirementsStructureChain.get<vk::MemoryRequirements2>().memoryRequirements;
            const auto& memoryDedicatedRequirement = memoryRequirementsStructureChain.get<vk::MemoryDedicatedRequirements>();
            (void)memoryDedicatedRequirement;

            vk::StructureChain<vk::MemoryAllocateInfo, vk::ExportMemoryAllocateInfo> memoryAllocInfoStructureChain;
            auto& memoryAllocInfo = memoryAllocInfoStructureChain.get<vk::MemoryAllocateInfo>();
            memoryAllocInfo.allocationSize = memoryRequirements.size;
            memoryAllocInfo.memoryTypeIndex = MemoryType{ physicalDevice }.getMemoryType(memoryRequirements.memoryTypeBits, properties);
            auto& exportMemoryAllocateInfo = memoryAllocInfoStructureChain.get<vk::ExportMemoryAllocateInfo>();
            exportMemoryAllocateInfo.handleTypes = defaultExternalMemoryHandleType;
            allocation = device.allocateMemoryUnique(memoryAllocInfo);

            device.bindBufferMemory(*buffer, *allocation, 0);
#ifdef VK_USE_PLATFORM_WIN32_KHR
            vk::MemoryGetWin32HandleInfoKHR memoryWin32HandleInfo;
            memoryWin32HandleInfo.handleType = defaultExternalMemoryHandleType;
            memoryWin32HandleInfo.memory = *allocation;
            win32Handle.reset(device.getMemoryWin32HandleKHR(memoryWin32HandleInfo));
#else
            vk::MemoryGetFdInfoKHR memoryFdInfo;
            memoryFdInfo.handleType = defaultExternalMemoryHandleType;
            memoryFdInfo.memory = *allocation;
            fd = device.getMemoryFdKHR(memoryFdInfo);
#endif

            CUDA_EXTERNAL_MEMORY_HANDLE_DESC externalMemoryHandle = {};
#ifdef VK_USE_PLATFORM_WIN32_KHR
            externalMemoryHandle.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32;
            externalMemoryHandle.handle.win32.handle = win32Handle.get();
#else
            externalMemoryHandle.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD;
            externalMemoryHandle.handle.fd = fd;
#endif
            externalMemoryHandle.size = memoryRequirements.size;

            CUexternalMemory cuExternalMemory = nullptr;
            CUDA_CHECK(cuImportExternalMemory(&cuExternalMemory, &externalMemoryHandle));

            CUDA_EXTERNAL_MEMORY_BUFFER_DESC cuExternalMemoryBufferDesc = {};
            cuExternalMemoryBufferDesc.offset = 0;
            cuExternalMemoryBufferDesc.size = memoryRequirements.size;
            cuExternalMemoryBufferDesc.flags = 0;
            CUDA_CHECK(cuExternalMemoryGetMappedBuffer(&p, cuExternalMemory, &cuExternalMemoryBufferDesc));
        }
    };

    static vk::AccessFlags accessFlagsForLayout(vk::ImageLayout layout)
    {
        switch (layout) {
        case vk::ImageLayout::ePreinitialized:
            return vk::AccessFlagBits::eHostWrite;

        case vk::ImageLayout::eTransferDstOptimal:
            return vk::AccessFlagBits::eTransferWrite;

        case vk::ImageLayout::eTransferSrcOptimal:
            return vk::AccessFlagBits::eTransferRead;

        case vk::ImageLayout::eColorAttachmentOptimal:
            return vk::AccessFlagBits::eColorAttachmentWrite;

        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            return vk::AccessFlagBits::eDepthStencilAttachmentWrite;

        case vk::ImageLayout::eShaderReadOnlyOptimal:
            return vk::AccessFlagBits::eShaderRead;

        default:
            return vk::AccessFlags();
        }
    }

    static vk::PipelineStageFlags pipelineStageForLayout(vk::ImageLayout layout)
    {
        switch (layout) {
        case vk::ImageLayout::eTransferDstOptimal:
        case vk::ImageLayout::eTransferSrcOptimal:
            return vk::PipelineStageFlagBits::eTransfer;

        case vk::ImageLayout::eColorAttachmentOptimal:
            return vk::PipelineStageFlagBits::eColorAttachmentOutput;

        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            return vk::PipelineStageFlagBits::eEarlyFragmentTests;

        case vk::ImageLayout::eShaderReadOnlyOptimal:
            return vk::PipelineStageFlagBits::eFragmentShader;

        case vk::ImageLayout::ePreinitialized:
            return vk::PipelineStageFlagBits::eHost;

        case vk::ImageLayout::eUndefined:
            return vk::PipelineStageFlagBits::eTopOfPipe;

        default:
            return vk::PipelineStageFlagBits::eBottomOfPipe;
        }
    }

    static void setImageLayout(vk::CommandBuffer commandBuffer,
        vk::Image image,
        vk::ImageLayout oldImageLayout,
        vk::ImageLayout newImageLayout,
        const vk::ImageSubresourceRange& subresourceRange)
    {
        vk::ImageMemoryBarrier imageMemoryBarrier;
        imageMemoryBarrier.oldLayout = oldImageLayout;
        imageMemoryBarrier.newLayout = newImageLayout;
        imageMemoryBarrier.image = image;
        imageMemoryBarrier.subresourceRange = subresourceRange;
        imageMemoryBarrier.srcAccessMask = accessFlagsForLayout(oldImageLayout);
        imageMemoryBarrier.dstAccessMask = accessFlagsForLayout(newImageLayout);
        vk::PipelineStageFlags srcStageMask = pipelineStageForLayout(oldImageLayout);
        vk::PipelineStageFlags destStageMask = pipelineStageForLayout(newImageLayout);
        commandBuffer.pipelineBarrier(srcStageMask, destStageMask, vk::DependencyFlags(), nullptr, nullptr, imageMemoryBarrier);
    }

    struct Denoiser
    {
        void logCallback(unsigned int level, const char* tag, const char* message) const
        {
            switch (level) {
            case 0:
                return;
            case 1: {
                qFatal("[%s]: %s", tag, message);
#ifdef _MSC_VER
                break;
#endif
            }
            case 2: {
                qCritical("[%s]: %s", tag, message);
                break;
            }
            case 3: {
                qWarning("[%s]: %s", tag, message);
                break;
            }
            case 4: {
                qInfo("[%s]: %s", tag, message);
                break;
            }
            default:
                return;
            }
        }

        static void optixLogCallback(unsigned int level, const char* tag, const char* message, void* cbdata)
        {
            return static_cast<const Denoiser*>(cbdata)->logCallback(level, tag, message);
        }

        vk::Device device;

        vk::PhysicalDevice physicalDevice;
        uint32_t queueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vk::Queue queue;

        static constexpr OptixPixelFormat optixPixelFormat = OPTIX_PIXEL_FORMAT_FLOAT4;
        static constexpr OptixDenoiserModelKind optixDenoiserModel = OPTIX_DENOISER_MODEL_KIND_HDR; // OPTIX_DENOISER_MODEL_KIND_LDR

        vk::UniqueCommandPool commandPool;

        CUdevice dev = -1;
        struct CudaContext
        {
            void set(int dev)
            {
                CUDA_CHECK(cuCtxCreate(&ctx, 0, dev));
            }

            ~CudaContext()
            {
                if (ctx) {
                    cuCtxDestroy(ctx);
                }
            }

            CUcontext ctx = nullptr;
        } cuContext;
        OptixDeviceContext optixDeviceContext = nullptr;
        OptixDenoiser denoiser = nullptr;

        vk::Extent2D imageSize;

        BufferCuda pixelBufferIn, pixelBufferOut, albedoBufferIn;

        CUstream cuStream = nullptr;

        OptixDenoiserSizes sizes = {};
        CUdeviceptr intensity = 0;
        CUdeviceptr denoiserData = 0;
        CUdeviceptr scratch = 0;

        Denoiser(vk::Device device, vk::PhysicalDevice physicalDevice, uint32_t queueFamilyIndex, vk::Queue queue)
            : device{ device }
            , physicalDevice{ physicalDevice }
            , queueFamilyIndex{ queueFamilyIndex }
            , queue{ queue }
        {
            vk::CommandPoolCreateInfo commandPoolCreateInfo;
            commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
            commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
            commandPool = device.createCommandPoolUnique(commandPoolCreateInfo);

            CUDA_CHECK(cuInit(0));

            int devCount = 0;
            CUDA_CHECK(cuDeviceGetCount(&devCount));
            if (devCount == 0) {
                qFatal("");
            }

            CUDA_CHECK(cuDeviceGet(&dev, 0));

            char name[256];
            CUDA_CHECK(cuDeviceGetName(name, sizeof name, dev));
            qInfo().noquote() << QStringLiteral("CUDA device name: %1").arg(name);

            int major, minor;
            CUDA_CHECK(cuDeviceGetAttribute(&major, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, dev));
            CUDA_CHECK(cuDeviceGetAttribute(&minor, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, dev));
            qInfo().noquote() << QStringLiteral("CUDA version: %1.%2").arg(dev).arg(dev);

            size_t mem = 0;
            CUDA_CHECK(cuDeviceTotalMem(&mem, dev));
            qInfo().noquote() << QStringLiteral("CUDA total global memory: %1").arg(QLocale::c().formattedDataSize(qint64(mem)));

            cuContext.set(dev);

            OPTIX_CHECK(optixInit());

            OptixDeviceContextOptions options = {};
            options.logCallbackFunction = &optixLogCallback;
            options.logCallbackLevel = 4;
            OPTIX_CHECK(optixDeviceContextCreate(cuContext.ctx, &options, &optixDeviceContext));
            //OPTIX_CHECK(optixDeviceContextSetLogCallback(m_optixDevice, context_log_cb, nullptr, 4));

            OptixDenoiserOptions denoiserOptions = { OPTIX_DENOISER_INPUT_RGB_ALBEDO, optixPixelFormat };
            OPTIX_CHECK(optixDenoiserCreate(optixDeviceContext, &denoiserOptions, &denoiser));
            OPTIX_CHECK(optixDenoiserSetModel(denoiser, optixDenoiserModel, nullptr, 0));
        }

    private:

        void clear()
        {
            CUDA_CHECK(cuMemFree(scratch));
            scratch = 0;

            CUDA_CHECK(cuMemFree(denoiserData));
            denoiserData = 0;

            CUDA_CHECK(cuMemFree(intensity));
            intensity = 0;
        }

    public:

        ~Denoiser()
        {
            clear();
            OPTIX_CHECK(optixDenoiserDestroy(denoiser));
            OPTIX_CHECK(optixDeviceContextDestroy(optixDeviceContext));
        }

    private:

        void allocateBuffers()
        {
            clear();

            vk::DeviceSize bufferSize = imageSize.width * imageSize.height * vk::DeviceSize(4 * sizeof(float));

            vk::BufferCreateInfo bufferCreateInfo;
            bufferCreateInfo.size = bufferSize;
            bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

            bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst;
            pixelBufferIn.create(device, physicalDevice, bufferCreateInfo);
            albedoBufferIn.create(device, physicalDevice, bufferCreateInfo);

            bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferSrc;
            pixelBufferOut.create(device, physicalDevice, bufferCreateInfo);

            OPTIX_CHECK(optixDenoiserComputeMemoryResources(denoiser, imageSize.width, imageSize.height, &sizes));

            CUDA_CHECK(cuMemAlloc(&denoiserData, sizes.stateSizeInBytes));
            CUDA_CHECK(cuMemAlloc(&scratch, sizes.recommendedScratchSizeInBytes));
            CUDA_CHECK(cuMemAlloc(&intensity, sizeof(float)));

            OPTIX_CHECK(optixDenoiserSetup(
                denoiser, cuStream,
                imageSize.width, imageSize.height,
                denoiserData, sizes.stateSizeInBytes,
                scratch, sizes.recommendedScratchSizeInBytes));
        }

        template<typename F, typename ...Args>
        void submit(F&& f, Args&&... args) const
        {
            vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
            commandBufferAllocateInfo.commandPool = *commandPool;
            commandBufferAllocateInfo.commandBufferCount = 1;
            commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
            auto commandBuffer = std::move(device.allocateCommandBuffersUnique(commandBufferAllocateInfo).back());

            vk::CommandBufferBeginInfo beginInfo;
            beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
            commandBuffer->begin(beginInfo);
            std::forward<F>(f)(*commandBuffer, std::forward<Args>(args)...);
            commandBuffer->end();

            vk::SubmitInfo submitInfo;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &*commandBuffer;
            queue.submit(submitInfo, vk::Fence{});
            queue.waitIdle();
        }

        void denoise()
        {
            using U = unsigned int;
            U rowStrideInBytes = U(4 * sizeof(float)) * imageSize.width;

            OptixImage2D inputLayers[2] = {};

            OptixImage2D& inputLayer = inputLayers[0];
            inputLayer.data = pixelBufferIn.p;
            inputLayer.width = imageSize.width;
            inputLayer.height = imageSize.height;
            inputLayer.rowStrideInBytes = rowStrideInBytes;
            inputLayer.pixelStrideInBytes = 0;
            inputLayer.format = optixPixelFormat;

            OPTIX_CHECK(optixDenoiserComputeIntensity(
                denoiser, cuStream, &inputLayer, intensity,
                scratch, sizes.recommendedScratchSizeInBytes));

            OptixImage2D& inputLayerAlbedo = inputLayers[1];
            inputLayerAlbedo = inputLayer;
            inputLayerAlbedo.data = albedoBufferIn.p;

            OptixImage2D outputLayer = inputLayer;
            outputLayer.data = pixelBufferOut.p;

            OptixDenoiserParams params = {};
            params.denoiseAlpha = 0;
            params.hdrIntensity = intensity;

            OPTIX_CHECK(optixDenoiserInvoke(
                denoiser, cuStream, &params,
                denoiserData, sizes.stateSizeInBytes,
                &inputLayer, 2,
                0, 0,
                &outputLayer,
                scratch, sizes.recommendedScratchSizeInBytes));

            CUDA_CHECK(cuStreamSynchronize(cuStream));
        }

    public:

        vk::Buffer denoiseImage(const vk::Extent2D& newImageSize, vk::Image image, vk::Image albedo, vk::Image filteredImage = {})
        {
            if (newImageSize != imageSize) {
                imageSize = newImageSize;
                allocateBuffers();
            }

            struct Timer : QElapsedTimer
            {
                Timer() { start(); }
                ~Timer() { qInfo().noquote() << QStringLiteral("(OptiX) HDR image denoising time: %1").arg(nsecsElapsed() * 1E-6); }
            } denoiseTimer;

            vk::ImageSubresourceRange subresourceRange;
            subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = 1;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.layerCount = 1;

            vk::BufferImageCopy copyRegion;
            vk::ImageSubresourceLayers& imageSubresourceLayers = copyRegion.imageSubresource;
            imageSubresourceLayers.aspectMask = vk::ImageAspectFlagBits::eColor;
            imageSubresourceLayers.mipLevel = 0;
            imageSubresourceLayers.baseArrayLayer = 0;
            imageSubresourceLayers.layerCount = 1;
            copyRegion.imageExtent = vk::Extent3D{ imageSize, 1 };

            auto copyImageInToInputBuffer = [&](vk::CommandBuffer commandBuffer)
            {
                setImageLayout(commandBuffer, image, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal, subresourceRange);
                commandBuffer.copyImageToBuffer(image, vk::ImageLayout::eTransferSrcOptimal, *pixelBufferIn.buffer, copyRegion);

                setImageLayout(commandBuffer, albedo, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal, subresourceRange);
                commandBuffer.copyImageToBuffer(albedo, vk::ImageLayout::eTransferSrcOptimal, *albedoBufferIn.buffer, copyRegion);
            };
            submit(copyImageInToInputBuffer);

            denoise();

            if (filteredImage) {
                auto copyOutputBufferToImageOut = [&](vk::CommandBuffer commandBuffer)
                {
                    setImageLayout(commandBuffer, filteredImage, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eTransferDstOptimal, subresourceRange);
                    commandBuffer.copyBufferToImage(*pixelBufferOut.buffer, filteredImage, vk::ImageLayout::eTransferDstOptimal, copyRegion);
                    setImageLayout(commandBuffer, filteredImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral, subresourceRange);
                };
                submit(copyOutputBufferToImageOut);
            }
            return *pixelBufferOut.buffer;
        }
    };

}*/