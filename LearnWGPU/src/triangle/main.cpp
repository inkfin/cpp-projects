#include <webgpu/webgpu_cpp.h>
#include <iostream>

int main() {
    wgpu::InstanceDescriptor instanceDesc = {};

    wgpu::Instance instance = wgpu::CreateInstance(&instanceDesc);

    std::cout << "WebGPU instance created successfully." << std::endl;
    return 0;
}
