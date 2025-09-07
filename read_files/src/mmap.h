#pragma once
#include <iostream>
#include <fcntl.h>
#include <string_view>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>
#include <chrono>

inline void read_with_mmap(const std::string_view filename)
{
    int fd = open(filename.data(), O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    auto data = (uint8_t*)mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        return;
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (off_t i = 0; i < file_size; ++i) {
        uint8_t byte = data[i];
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time taken to read file with mmap: " << duration.count() << " ms" << std::endl;

    munmap(data, file_size);
    close(fd);
}
