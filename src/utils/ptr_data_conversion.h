#pragma once

template <typename T>
const T &GetPtrData(const void* ptr)
{
    if (ptr == nullptr) {
        throw std::runtime_error("Null Pointer Exception.");
    }
    const T &data = *(static_cast<const T*>(ptr));
    
    return data;
}

template <typename T>
const void* GetDataPtr(const T &data)
{
    const void* ptr = static_cast<const void*>(&data);
    if (ptr == nullptr) {
        throw std::runtime_error("Null Pointer Exception.");
    }
    
    return ptr;
}
