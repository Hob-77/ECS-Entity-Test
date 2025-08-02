#pragma once
#include <cstdio>
#include <cstring>  // for memcpy
#include <utility>  // for std::move
#include <new>      // for std::nothrow
#include <algorithm> // for std::min/max
#include <SDL3/SDL.h>

template<class Datatype>
class Array
{
private:
    Datatype* m_array;
    int m_size;
    int m_capacity;  // Add capacity for better resize performance

public:
    // Constructor
    explicit Array(int p_size) : m_array(nullptr), m_size(p_size), m_capacity(p_size)
    {
        if (p_size > 0)
        {
            m_array = new Datatype[p_size];
            // Initialize to default values
            for (int i = 0; i < p_size; i++)
            {
                m_array[i] = Datatype{};
            }
        }
        else
        {
            m_size = 0;
            m_capacity = 0;
        }
    }

    // Default constructor
    Array() : m_array(nullptr), m_size(0), m_capacity(0) {}

    // Copy constructor
    Array(const Array& other) : m_array(nullptr), m_size(other.m_size), m_capacity(other.m_capacity)
    {
        if (other.m_size > 0)
        {
            m_array = new Datatype[other.m_capacity];
            for (int i = 0; i < m_size; i++)
            {
                m_array[i] = other.m_array[i];
            }
        }
    }

    // Move constructor
    Array(Array&& other) noexcept
        : m_array(other.m_array), m_size(other.m_size), m_capacity(other.m_capacity)
    {
        other.m_array = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
    }

    // Copy assignment operator
    Array& operator=(const Array& other)
    {
        if (this != &other)
        {
            delete[] m_array;

            m_size = other.m_size;
            m_capacity = other.m_capacity;

            if (other.m_size > 0)
            {
                m_array = new Datatype[other.m_capacity];
                for (int i = 0; i < m_size; i++)
                {
                    m_array[i] = other.m_array[i];
                }
            }
            else
            {
                m_array = nullptr;
            }
        }
        return *this;
    }

    // Move assignment operator
    Array& operator=(Array&& other) noexcept
    {
        if (this != &other)
        {
            delete[] m_array;
            m_array = other.m_array;
            m_size = other.m_size;
            m_capacity = other.m_capacity;
            other.m_array = nullptr;
            other.m_size = 0;
            other.m_capacity = 0;
        }
        return *this;
    }

    // Destructor
    ~Array()
    {
        delete[] m_array;
    }

    // Size and capacity
    int Size() const { return m_size; }
    int Capacity() const { return m_capacity; }
    bool Empty() const { return m_size == 0; }

    // Resize with better growth strategy
    void Resize(int p_size)
    {
        if (p_size < 0)
        {
            SDL_Log("Array::Resize - Invalid size: %d", p_size);
            return;
        }

        // If shrinking within capacity, just update size
        if (p_size <= m_capacity)
        {
            m_size = p_size;
            return;
        }

        // Need to grow - use growth factor of 1.5
        int newCapacity = m_capacity;
        while (newCapacity < p_size)
        {
            newCapacity = newCapacity + newCapacity / 2 + 1;
        }

        // Allocate new array
        Datatype* newarray = new(std::nothrow) Datatype[newCapacity];
        if (!newarray)
        {
            SDL_Log("Array::Resize - Failed to allocate %d elements", newCapacity);
            return;
        }

        // Copy old data
        int copyCount = (p_size < m_size) ? p_size : m_size;
        for (int i = 0; i < copyCount; i++)
        {
            newarray[i] = std::move(m_array[i]);
        }

        // Initialize new elements
        for (int i = copyCount; i < p_size; i++)
        {
            newarray[i] = Datatype{};
        }

        // Replace array
        delete[] m_array;
        m_array = newarray;
        m_size = p_size;
        m_capacity = newCapacity;
    }

    // Element access with bounds checking
    const Datatype& operator[](int p_index) const
    {
#ifdef _DEBUG
        if (p_index < 0 || p_index >= m_size)
        {
            SDL_Log("Array::operator[] - Out of bounds: %d (size: %d)", p_index, m_size);
            SDL_assert(false);
        }
#endif
        return m_array[p_index];
    }

    Datatype& operator[](int p_index)
    {
#ifdef _DEBUG
        if (p_index < 0 || p_index >= m_size)
        {
            SDL_Log("Array::operator[] - Out of bounds: %d (size: %d)", p_index, m_size);
            SDL_assert(false);
        }
#endif
        return m_array[p_index];
    }

    // Safe access methods
    Datatype* Data() { return m_array; }
    const Datatype* Data() const { return m_array; }

    // Insert with bounds checking
    void Insert(const Datatype& p_item, int p_index)
    {
        if (p_index < 0 || p_index > m_size)
        {
            SDL_Log("Array::Insert - Invalid index: %d (size: %d)", p_index, m_size);
            return;
        }

        // Ensure we have space
        if (m_size >= m_capacity)
        {
            Resize(m_size + 1);
        }
        else
        {
            m_size++;
        }

        // Shift elements
        for (int i = m_size - 1; i > p_index; i--)
        {
            m_array[i] = std::move(m_array[i - 1]);
        }

        m_array[p_index] = p_item;
    }

    // Remove with bounds checking
    void Remove(int p_index)
    {
        if (p_index < 0 || p_index >= m_size)
        {
            SDL_Log("Array::Remove - Invalid index: %d (size: %d)", p_index, m_size);
            return;
        }

        // Shift elements
        for (int i = p_index + 1; i < m_size; i++)
        {
            m_array[i - 1] = std::move(m_array[i]);
        }

        m_size--;
    }

    // Clear array
    void Clear()
    {
        m_size = 0;
        // Don't deallocate - keep capacity
    }

    // File I/O using SDL
    bool WriteFile(const char* p_filename)
    {
        SDL_IOStream* file = SDL_IOFromFile(p_filename, "wb");
        if (!file)
        {
            SDL_Log("Array::WriteFile - Failed to open %s", p_filename);
            return false;
        }

        size_t written = SDL_WriteIO(file, m_array, sizeof(Datatype) * m_size);
        SDL_CloseIO(file);

        if (written != sizeof(Datatype) * m_size)
        {
            SDL_Log("Array::WriteFile - Write error: expected %zu, wrote %zu",
                sizeof(Datatype) * m_size, written);
            return false;
        }

        return true;
    }

    bool ReadFile(const char* p_filename)
    {
        SDL_IOStream* file = SDL_IOFromFile(p_filename, "rb");
        if (!file)
        {
            SDL_Log("Array::ReadFile - Failed to open %s", p_filename);
            return false;
        }

        size_t read = SDL_ReadIO(file, m_array, sizeof(Datatype) * m_size);
        SDL_CloseIO(file);

        if (read != sizeof(Datatype) * m_size)
        {
            SDL_Log("Array::ReadFile - Read error: expected %zu, read %zu",
                sizeof(Datatype) * m_size, read);
            return false;
        }

        return true;
    }

    // Legacy operator for compatibility
    operator Datatype* () { return m_array; }
};

template<class Datatype>
class Array2D
{
private:
    Datatype* m_array;
    int m_width;
    int m_height;

public:
    // Constructor
    Array2D(int p_width, int p_height) : m_array(nullptr), m_width(p_width), m_height(p_height)
    {
        if (p_width > 0 && p_height > 0)
        {
            m_array = new Datatype[p_width * p_height];
            // Initialize all elements to zero
            for (int i = 0; i < p_width * p_height; i++)
            {
                m_array[i] = Datatype{};  // Better than = 0, works for all types
            }
        }
        else
        {
            m_width = 0;
            m_height = 0;
        }
    }

    // Default constructor
    Array2D() : m_array(nullptr), m_width(0), m_height(0) {}

    // Copy constructor - CRITICAL FOR SAFETY
    Array2D(const Array2D& other) : m_array(nullptr), m_width(other.m_width), m_height(other.m_height)
    {
        if (other.m_array && m_width > 0 && m_height > 0)
        {
            m_array = new Datatype[m_width * m_height];
            for (int i = 0; i < m_width * m_height; i++)
            {
                m_array[i] = other.m_array[i];
            }
        }
    }

    // Move constructor
    Array2D(Array2D&& other) noexcept : m_array(other.m_array), m_width(other.m_width), m_height(other.m_height)
    {
        other.m_array = nullptr;
        other.m_width = 0;
        other.m_height = 0;
    }

    // Copy assignment operator
    Array2D& operator=(const Array2D& other)
    {
        if (this != &other)
        {
            // Delete old data
            delete[] m_array;

            // Copy new data
            m_width = other.m_width;
            m_height = other.m_height;

            if (other.m_array && m_width > 0 && m_height > 0)
            {
                m_array = new Datatype[m_width * m_height];
                for (int i = 0; i < m_width * m_height; i++)
                {
                    m_array[i] = other.m_array[i];
                }
            }
            else
            {
                m_array = nullptr;
            }
        }
        return *this;
    }

    // Move assignment operator
    Array2D& operator=(Array2D&& other) noexcept
    {
        if (this != &other)
        {
            delete[] m_array;
            m_array = other.m_array;
            m_width = other.m_width;
            m_height = other.m_height;
            other.m_array = nullptr;
            other.m_width = 0;
            other.m_height = 0;
        }
        return *this;
    }

    // Destructor
    ~Array2D()
    {
        delete[] m_array;
    }

    // Resize with better error handling
    void Resize(int p_width, int p_height)
    {
        if (p_width < 0 || p_height < 0)
        {
            SDL_Log("Array2D::Resize - Invalid dimensions: %dx%d", p_width, p_height);
            return;
        }

        // Handle empty array case
        if (p_width == 0 || p_height == 0)
        {
            delete[] m_array;
            m_array = nullptr;
            m_width = 0;
            m_height = 0;
            return;
        }

        // Allocate new array
        Datatype* newarray = new(std::nothrow) Datatype[p_width * p_height];
        if (!newarray)
        {
            SDL_Log("Array2D::Resize - Failed to allocate %dx%d array", p_width, p_height);
            return;
        }

        // Initialize new array to default values
        for (int i = 0; i < p_width * p_height; i++)
        {
            newarray[i] = Datatype{};
        }

        // Copy old data if exists
        if (m_array)
        {
            int minx = (p_width < m_width) ? p_width : m_width;
            int miny = (p_height < m_height) ? p_height : m_height;

            for (int y = 0; y < miny; y++)
            {
                for (int x = 0; x < minx; x++)
                {
                    newarray[y * p_width + x] = m_array[y * m_width + x];
                }
            }
        }

        // Replace old array
        delete[] m_array;
        m_array = newarray;
        m_width = p_width;
        m_height = p_height;
    }

    void Clear(Datatype value = Datatype{})
    {
        for (int i = 0; i < m_width * m_height; i++)
        {
            m_array[i] = value;
        }
    }

    // Getters
    int Width() const { return m_width; }
    int Height() const { return m_height; }
    int Size() const { return m_width * m_height; }

    // Safe element access with bounds checking
    Datatype& Get(int p_x, int p_y)
    {
#ifdef _DEBUG
        if (p_x < 0 || p_x >= m_width || p_y < 0 || p_y >= m_height)
        {
            SDL_Log("Array2D::Get - Out of bounds access: (%d,%d) in %dx%d array", p_x, p_y, m_width, m_height);
            // In debug, we'll assert. In release, we'll clamp.
            SDL_assert(false);
        }
#endif

        // Clamp to valid range in release
        p_x = SDL_clamp(p_x, 0, m_width - 1);
        p_y = SDL_clamp(p_y, 0, m_height - 1);

        return m_array[p_y * m_width + p_x];
    }

    const Datatype& Get(int p_x, int p_y) const
    {
#ifdef _DEBUG
        if (p_x < 0 || p_x >= m_width || p_y < 0 || p_y >= m_height)
        {
            SDL_Log("Array2D::Get - Out of bounds access: (%d,%d) in %dx%d array", p_x, p_y, m_width, m_height);
            SDL_assert(false);
        }
#endif

        // Cast away const to use SDL_clamp (it's safe since we're just reading)
        int x = SDL_clamp(p_x, 0, m_width - 1);
        int y = SDL_clamp(p_y, 0, m_height - 1);

        return m_array[y * m_width + x];
    }

    // Check if coordinates are valid
    bool IsValidCoord(int x, int y) const
    {
        return x >= 0 && x < m_width && y >= 0 && y < m_height;
    }
};

// Array3D stays mostly the same - just needs the critical fixes
template<class Datatype>
class Array3D
{
private:
    Datatype* m_array;
    int m_width;
    int m_height;
    int m_depth;

public:
    // Constructor
    Array3D(int p_width, int p_height, int p_depth)
        : m_array(nullptr), m_width(p_width), m_height(p_height), m_depth(p_depth)
    {
        if (p_width > 0 && p_height > 0 && p_depth > 0)
        {
            m_array = new Datatype[p_width * p_height * p_depth];
            // Initialize to default
            int size = p_width * p_height * p_depth;
            for (int i = 0; i < size; i++)
            {
                m_array[i] = Datatype{};
            }
        }
        else
        {
            m_width = 0;
            m_height = 0;
            m_depth = 0;
        }
    }

    // Copy constructor
    Array3D(const Array3D& other)
        : m_array(nullptr), m_width(other.m_width), m_height(other.m_height), m_depth(other.m_depth)
    {
        int size = m_width * m_height * m_depth;
        if (other.m_array && size > 0)
        {
            m_array = new Datatype[size];
            for (int i = 0; i < size; i++)
            {
                m_array[i] = other.m_array[i];
            }
        }
    }

    // Move constructor
    Array3D(Array3D&& other) noexcept
        : m_array(other.m_array), m_width(other.m_width), m_height(other.m_height), m_depth(other.m_depth)
    {
        other.m_array = nullptr;
        other.m_width = 0;
        other.m_height = 0;
        other.m_depth = 0;
    }

    // Assignment operators
    Array3D& operator=(const Array3D& other)
    {
        if (this != &other)
        {
            delete[] m_array;

            m_width = other.m_width;
            m_height = other.m_height;
            m_depth = other.m_depth;

            int size = m_width * m_height * m_depth;
            if (other.m_array && size > 0)
            {
                m_array = new Datatype[size];
                for (int i = 0; i < size; i++)
                {
                    m_array[i] = other.m_array[i];
                }
            }
            else
            {
                m_array = nullptr;
            }
        }
        return *this;
    }

    Array3D& operator=(Array3D&& other) noexcept
    {
        if (this != &other)
        {
            delete[] m_array;
            m_array = other.m_array;
            m_width = other.m_width;
            m_height = other.m_height;
            m_depth = other.m_depth;
            other.m_array = nullptr;
            other.m_width = 0;
            other.m_height = 0;
            other.m_depth = 0;
        }
        return *this;
    }

    // Destructor
    ~Array3D()
    {
        delete[] m_array;
    }

    Datatype& Get(int p_x, int p_y, int p_z)
    {
        return m_array[(p_z * m_width * m_height) + (p_y * m_width) + p_x];
    }

    void Resize(int p_width, int p_height, int p_depth)
    {
        // Create a new array
        Datatype* newarray = new Datatype[p_width * p_height * p_depth];
        if (newarray == 0)
        {
            return;
        }

        // Initialize new array
        int newSize = p_width * p_height * p_depth;
        for (int i = 0; i < newSize; i++)
        {
            newarray[i] = Datatype{};
        }

        // Copy old data if exists
        if (m_array)
        {
            int minx = (p_width < m_width ? p_width : m_width);
            int miny = (p_height < m_height ? p_height : m_height);
            int minz = (p_depth < m_depth ? p_depth : m_depth);

            for (int z = 0; z < minz; z++)
            {
                for (int y = 0; y < miny; y++)
                {
                    for (int x = 0; x < minx; x++)
                    {
                        int oldIdx = (z * m_width * m_height) + (y * m_width) + x;
                        int newIdx = (z * p_width * p_height) + (y * p_width) + x;
                        newarray[newIdx] = m_array[oldIdx];
                    }
                }
            }
        }

        // Delete the old array
        delete[] m_array;

        // Set the new array and dimensions
        m_array = newarray;
        m_width = p_width;
        m_height = p_height;
        m_depth = p_depth;
    }

    int Size() const { return m_width * m_height * m_depth; }
    int Width() const { return m_width; }
    int Height() const { return m_height; }
    int Depth() const { return m_depth; }
};