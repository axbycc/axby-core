#pragma once

#include <cuda.h>
#include <cuda_runtime.h>

#include <span>
#include <string>

#include "debug/log.h"
#include "debug/check.h"
#include "fast_resizable_vector/fast_resizable_vector.h"

namespace axby {

#define CUDACHECK(err)                         \
    do {                                       \
        ::axby::cuda_check((err), __FILE__, __LINE__);  \
    } while (false)

inline void cuda_check(cudaError_t error_code, const char* file, int line) {
    if (error_code != cudaSuccess) {
        LOG(FATAL) << "CUDA Error " << cudaGetErrorString(error_code) << " on "
                   << file << " line " << line;
    }
}

template <typename T>
class CudaBuffer {
   public:
    CudaBuffer(size_t size, cudaStream_t stream = 0) { allocate(size, stream); }

    CudaBuffer(const char* name = nullptr) { name_ = name; };

    ~CudaBuffer() { free(/*cudaStream_t=*/0); }

    CudaBuffer(CudaBuffer&& other) noexcept
        : data_(other.data_),
          size_(other.size_),
          capacity_(other.capacity_),
          name_(other.name_) {
        other.name_ = "";
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    CudaBuffer(const CudaBuffer& other) noexcept { *this = other; }

    CudaBuffer& operator=(CudaBuffer&& other) noexcept {
        if (this != &other) {
            free(/*cudaStream_t=*/0);  // Free current memory

            data_ = other.data_;  // Transfer ownership
            size_ = other.size_;
            capacity_ = other.capacity_;
            name_ = other.name_;

            other.data_ = nullptr;
            other.capacity_ = 0;
            other.size_ = 0;
            other.name_ = "";
        }
        return *this;
    }

    CudaBuffer& operator=(const CudaBuffer& other) noexcept {
        reinit(other.size_);
        cudaMemcpy(/*dst=*/data_, /*src=*/other.data_, sizeof(T) * other.size_,
                   cudaMemcpyDeviceToDevice);
        size_ = other.size_;
        return *this;
    }

    void upload(std::span<const T> data, cudaStream_t stream = 0) {
        reinit(data.size(), stream);
        cudaMemcpyAsync(/*dst=*/data_, /*src=*/data.data(),
                        sizeof(T) * data.size(), cudaMemcpyHostToDevice,
                        stream);
    }

    void download(FastResizableVector<T>& data, cudaStream_t stream = 0) {
        data.resize(size_);
        download(std::span<T>(data), stream);
    }

    void download(std::span<T> data, cudaStream_t stream = 0) {
        CHECK(data.size() == size_);
        cudaMemcpyAsync(/*dst=*/data.data(), /*src=*/data_,
                        sizeof(T) * data.size(), cudaMemcpyDeviceToHost,
                        stream);
        cudaStreamSynchronize(stream);
    }

    T* data() { return data_; }
    size_t size() const { return size_; }

    T back(cudaStream_t cuda_stream = 0) const {
        if (size_ == 0) {
            throw std::out_of_range(
                "CudaBuffer is empty, cannot access the last element");
        }

        T host_value;  // Variable to hold the copied value
        cudaError_t err =
            cudaMemcpyAsync(&host_value, data_ + size_ - 1, sizeof(T),
                            cudaMemcpyDeviceToHost, cuda_stream);
        CHECK(err == cudaSuccess)
            << "CUDA memcpy failed: " << std::string(cudaGetErrorString(err));

        // not sure this is needed. host_value is not pinned memory so therefore
        // an implicit sync is required.
        cudaStreamSynchronize(cuda_stream);

        return host_value;
    }

    void reinit(size_t size, cudaStream_t cuda_stream = 0) {
        if (size <= capacity_) {
            size_ = size;
            return;
        }

        free(cuda_stream);
        allocate(size, cuda_stream);
    }

    T* begin() { return data_; }
    T* end() { return data_ + size_; }

    void free(cudaStream_t cuda_stream = 0) {
        if (data_) {
            cudaFreeAsync(data_, cuda_stream);
            data_ = nullptr;
        }
        size_ = 0;
        capacity_ = 0;
    }

    void allocate(size_t size, cudaStream_t cuda_stream = 0) {
        cudaError_t err =
            cudaMallocAsync(&data_, size * sizeof(T), cuda_stream);
        CUDACHECK(err);
        size_ = size;
        capacity_ = size;
    }

    T* data_ = nullptr;
    size_t size_ = 0;
    size_t capacity_ = 0;

    const char* name_ = nullptr;  // debug name
};
}  // namespace axby
