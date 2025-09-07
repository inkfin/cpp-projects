#pragma once
#include <future>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>

inline void read_with_ifstream(const std::string_view filename)
{
    std::ifstream file(filename.data(), std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file with ifstream" << std::endl;
        return;
    }

    auto start = std::chrono::high_resolution_clock::now();

    file.seekg(0, std::ios::end);
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    char* buffer = new char[file_size];
    file.read(buffer, file_size);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "ifstream read time: " << duration.count() << " ms" << std::endl;

    delete[] buffer;
    file.close();
}

inline void read_chunk_in_thread(const std::string_view filename, size_t offset, size_t chunk_size, char* buffer)
{
    std::ifstream file(filename.data(), std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file with ifstream" << std::endl;
        return;
    }
    // Set file pointer to specified position
    file.seekg(offset);
    // Read file chunk
    file.read(buffer, chunk_size);
    file.close();
}

inline void read_in_parallel(const std::string_view filename, size_t chunk_size, size_t num_threads)
{
    std::ifstream file(filename.data(), std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file with ifstream" << std::endl;
        return;
    }

    file.seekg(0, std::ios::end);
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    file.close();

    size_t num_chunks = file_size / chunk_size;
    size_t remaining_bytes = file_size % chunk_size;

    // Allocate buffers for each thread
    std::vector<char*> buffers(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        buffers[i] = new char[chunk_size + remaining_bytes]; // Extra space for potential remaining bytes
    }

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::future<void>> futures;

    // Process chunks in batches of num_threads
    for (size_t chunk_start = 0; chunk_start < num_chunks; chunk_start += num_threads) {
        // Launch up to num_threads threads for this batch
        size_t chunks_in_batch = std::min(num_threads, num_chunks - chunk_start);

        for (size_t i = 0; i < chunks_in_batch; ++i) {
            size_t chunk_index = chunk_start + i;
            size_t offset = chunk_index * chunk_size;
            futures.push_back(std::async(std::launch::async, read_chunk_in_thread, filename, offset, chunk_size, buffers[i]));
        }

        for (auto& fut : futures) {
            fut.get();
        }
        futures.clear();

        // Process the data in buffers here if needed
        // For example: write to output, process data, etc.
    }

    // Handle remaining bytes with a single thread if necessary
    if (remaining_bytes > 0) {
        futures.push_back(std::async(std::launch::async, read_chunk_in_thread, filename, num_chunks * chunk_size, remaining_bytes, buffers[0]));
        for (auto& fut : futures) {
            fut.get();
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Multi-thread chunk read time: " << duration.count() << " ms" << std::endl;

    // Clean up buffers
    for (int i = 0; i < num_threads; ++i) {
        delete[] buffers[i];
    }
}
