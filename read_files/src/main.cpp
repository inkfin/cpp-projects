#include <iostream>
#include "mmap.h"
#include "fstream.h"

int main(int argc, char** argv)
{
    const char* filename = "assets/test_file.bin"; // 1GB 的文件

    std::cout << "Testing mmap..." << std::endl;
    read_with_mmap(filename);

    std::cout << "Testing ifstream..." << std::endl;
    read_with_ifstream(filename);

    size_t chunk_size = 16 * 1024 * 1024; // 16MB 分块读取
    int num_threads = 12;
    std::cout << "Testing multi-threaded chunked reading..." << std::endl;
    read_in_parallel(filename, chunk_size, num_threads);

    return 0;
}
