#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#if defined(__EMSCRIPTEN__)
#  include <emscripten.h>
#endif // __EMSCRIPTEN__
#include <webgpu/webgpu_cpp.h>
#if defined(WGPU_BACKEND_DAWN)
#include <dawn/webgpu_cpp_print.h>
#endif

namespace TriangleApp {

wgpu::Adapter gAdapter;
wgpu::Device gDevice;

}


void printLimits(wgpu::Limits limits, std::string_view prefix);

int main(int, char**) {

#if defined(WGPU_BACKEND_DAWN)
    std::cout << "Using Dawn backend." << std::endl;
#elif defined(WGPU_BACKEND_WGPU_NATIVE)
    std::cout << "Using wgpu-native backend." << std::endl;
#elif defined(WGPU_BACKEND_EMSCRIPTEN)
    std::cout << "Using Emscripten backend." << std::endl;
#endif

    // 1. Create a WebGPU instance
#if defined(__EMSCRIPTEN__)
    wgpu::Instance instance = wgpu::CreateInstance(nullptr);
#else
    wgpu::InstanceFeatureName features[] = { wgpu::InstanceFeatureName::TimedWaitAny };
    wgpu::InstanceDescriptor instanceDesc {
        .requiredFeatureCount = 1,
        .requiredFeatures = features,
    };
    const char* toggleName = "enable_immediate_error_handling";
    wgpu::DawnTogglesDescriptor togglesDesc(wgpu::DawnTogglesDescriptor::Init {
        .nextInChain = nullptr,
        .enabledToggleCount = 1,
        .enabledToggles = &toggleName,
        .disabledToggleCount = 0,
    });
    instanceDesc.nextInChain = &togglesDesc;
    wgpu::Instance instance = wgpu::CreateInstance(&instanceDesc);
#endif
    if (!instance) {
        std::cerr << "Failed to create WebGPU instance.\n";
        return 1;
    }

    // 2. Request an adapter from the instance
    wgpu::Future f1 = instance.RequestAdapter(
        nullptr, wgpu::CallbackMode::WaitAnyOnly,
        [](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message) {
            if (status != wgpu::RequestAdapterStatus::Success)
            {
                std::cerr << "Failed to request adapter: " << message << "\n";
                exit(0);
            }
            ::TriangleApp::gAdapter = std::move(adapter);
        });
    instance.WaitAny(f1, UINT64_MAX);

    wgpu::DawnAdapterPropertiesPowerPreference power_props {};
    // get info
    wgpu::AdapterInfo info {};
    info.nextInChain = &power_props;
    ::TriangleApp::gAdapter.GetInfo(&info);
    std::cout << "VendorID: " << std::hex << info.vendorID << std::dec << "\n";
    std::cout << "Vendor: " << info.vendor << "\n";
    std::cout << "Architecture: " << info.architecture << "\n";
    std::cout << "DeviceID: " << std::hex << info.deviceID << std::dec << "\n";
    std::cout << "Name: " << info.device << "\n";
    std::cout << "Driver description: " << info.description << "\n";
    std::cout << "Adapter Type: " << info.adapterType << "\n";
    std::cout << "Backend Type: " << info.backendType << "\n";
    std::cout << "Power Preference: " << power_props.powerPreference << "\n";

    // get limits
    // TODO: test wgpuAdapterGetLimits on Google Chrome (April 1st, 2024)
#if !defined(__EMSCRIPTEN__)
    wgpu::Limits supportedLimits = {};
    if (::TriangleApp::gAdapter.GetLimits(&supportedLimits)) {
        printLimits(supportedLimits, "Adapter");
    } else {
        std::cerr << "Failed to get adapter limits.\n";
        return 1;
    }
#endif

    // get features
    wgpu::SupportedFeatures supportedFeatures {};
    ::TriangleApp::gAdapter.GetFeatures(&supportedFeatures);
    std::cout << "\nAdapter supports features:\n================================\n";
    for (size_t i = 0; i < supportedFeatures.featureCount; i++) {
        std::cout << supportedFeatures.features[i] << "\n";
    }

    // 3. Request a device from the adapter
    wgpu::DeviceDescriptor deviceDesc = {};
    deviceDesc.SetUncapturedErrorCallback([](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message) {
        std::cerr << "[Device] Uncaptured error: " << type << " - message: " << message << "\n";
    });
    deviceDesc.SetDeviceLostCallback(wgpu::CallbackMode::WaitAnyOnly, [](const wgpu::Device&, wgpu::DeviceLostReason reason, wgpu::StringView message) {
        std::cerr << "[Device] Device lost: " << reason << "\n - message: " << message << "\n";
    });

    wgpu::Future f2 = ::TriangleApp::gAdapter.RequestDevice(
        &deviceDesc, wgpu::CallbackMode::WaitAnyOnly,
        [](wgpu::RequestDeviceStatus status, wgpu::Device device, wgpu::StringView message) {
            if (status != wgpu::RequestDeviceStatus::Success)
            {
                std::cerr << "Failed to request device: " << message << "\n";
                exit(0);
            }
            ::TriangleApp::gDevice = std::move(device);
        });
    instance.WaitAny(f2, UINT64_MAX);

    wgpu::Limits deviceLimits = {};
    ::TriangleApp::gDevice.GetLimits(&deviceLimits);
    printLimits(deviceLimits, "Device");

    return 0;
}

void printLimits(wgpu::Limits limits, std::string_view prefix) {
    std::cout << "\n" << prefix << " limits:\n=========================\n" << std::endl;
    std::cout << " - maxTextureDimension1D: " << limits.maxTextureDimension1D << std::endl;
    std::cout << " - maxTextureDimension2D: " << limits.maxTextureDimension2D << std::endl;
    std::cout << " - maxTextureDimension3D: " << limits.maxTextureDimension3D << std::endl;
    std::cout << " - maxTextureArrayLayers: " << limits.maxTextureArrayLayers << std::endl;
    std::cout << " - maxBindGroups: " << limits.maxBindGroups << std::endl;
    std::cout << " - maxBindGroupsPlusVertexBuffers: " << limits.maxBindGroupsPlusVertexBuffers << std::endl;
    std::cout << " - maxBindingsPerBindGroup: " << limits.maxBindingsPerBindGroup << std::endl;
    std::cout << " - maxDynamicUniformBuffersPerPipelineLayout: "
              << limits.maxDynamicUniformBuffersPerPipelineLayout << std::endl;
    std::cout << " - maxDynamicStorageBuffersPerPipelineLayout: "
              << limits.maxDynamicStorageBuffersPerPipelineLayout << std::endl;
    std::cout << " - maxSampledTexturesPerShaderStage: " << limits.maxSampledTexturesPerShaderStage
              << std::endl;
    std::cout << " - maxSamplersPerShaderStage: " << limits.maxSamplersPerShaderStage << std::endl;
    std::cout << " - maxStorageBuffersPerShaderStage: " << limits.maxStorageBuffersPerShaderStage << std::endl;
    std::cout << " - maxStorageTexturesPerShaderStage: " << limits.maxStorageTexturesPerShaderStage
              << std::endl;
    std::cout << " - maxUniformBuffersPerShaderStage: " << limits.maxUniformBuffersPerShaderStage << std::endl;
    std::cout << " - maxUniformBufferBindingSize: " << limits.maxUniformBufferBindingSize << std::endl;
    std::cout << " - maxStorageBufferBindingSize: " << limits.maxStorageBufferBindingSize << std::endl;
    std::cout << " - minUniformBufferOffsetAlignment: " << limits.minUniformBufferOffsetAlignment << std::endl;
    std::cout << " - minStorageBufferOffsetAlignment: " << limits.minStorageBufferOffsetAlignment << std::endl;
    std::cout << " - maxVertexBuffers: " << limits.maxVertexBuffers << std::endl;
    std::cout << " - maxBufferSize: " << limits.maxBufferSize << std::endl;
    std::cout << " - maxVertexAttributes: " << limits.maxVertexAttributes << std::endl;
    std::cout << " - maxVertexBufferArrayStride: " << limits.maxVertexBufferArrayStride << std::endl;
    std::cout << " - maxInterStageShaderVariables: " << limits.maxInterStageShaderVariables << std::endl;
    std::cout << " - maxColorAttachments: " << limits.maxColorAttachments << std::endl;
    std::cout << " - maxColorAttachmentBytesPerSample: " << limits.maxColorAttachmentBytesPerSample
              << std::endl;
    std::cout << " - maxComputeWorkgroupStorageSize: " << limits.maxComputeWorkgroupStorageSize << std::endl;
    std::cout << " - maxComputeInvocationsPerWorkgroup: " << limits.maxComputeInvocationsPerWorkgroup
              << std::endl;
    std::cout << " - maxComputeWorkgroupSizeX: " << limits.maxComputeWorkgroupSizeX << std::endl;
    std::cout << " - maxComputeWorkgroupSizeY: " << limits.maxComputeWorkgroupSizeY << std::endl;
    std::cout << " - maxComputeWorkgroupSizeZ: " << limits.maxComputeWorkgroupSizeZ << std::endl;
    std::cout << " - maxComputeWorkgroupsPerDimension: " << limits.maxComputeWorkgroupsPerDimension
              << std::endl;
    std::cout << " - maxImmediateSize: " << limits.maxImmediateSize << std::endl;
}
