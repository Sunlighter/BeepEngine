#pragma once

#include "fft.h"

extern std::unique_ptr<std::deque<std::wstring>> g_log;

void Log(const std::wstring& message);

void WithLogStream(std::function<void(std::wostream& o)> f);

class SomethingToWriteBase
{
public:
    virtual ~SomethingToWriteBase();
    virtual void WriteTo(std::wostream& o) const = 0;
};

template<typename T>
class SomethingToWrite : public SomethingToWriteBase
{
public:
    SomethingToWrite(const std::wstring& name, T value)
        : name(name)
        , value(value)
    {
    }

    void WriteTo(std::wostream& o) const override
    {
        o << name << L" = [" << value << L"]";
    }
private:
    const std::wstring name;
    const T value;
};

class WriteList
{
public:
    template<typename T>
    WriteList& Add(const std::wstring& name, T value)
    {
        list.push_back(std::move(std::unique_ptr<SomethingToWriteBase>(new SomethingToWrite<T>(name, value))));
        return *this;
    }

    void WriteToLog()
    {
        WithLogStream
        (
            [&](std::wostream& o)
            {
                bool needDelim = false;
                for (const auto& item : list)
                {
                    if (needDelim) o << L", ";
                    item->WriteTo(o);
                    needDelim = true;
                }
            }
        );

        list.clear();
    }

private:
    std::vector<std::unique_ptr<SomethingToWriteBase>> list;
};

typedef std::complex<float> Complex;

static Complex root_of_unity(int i, int size, bool isInverse);

template<typename T>
class Sequence
{
public:
    virtual ~Sequence() {}
    virtual int Length() const = 0;
    virtual T operator[](int i) const = 0;
    virtual T& operator[](int i) = 0;
};

template<typename T>
std::wostream& operator<<(std::wostream& o, const Sequence<T>& c)
{
    o << L"[ ";
    bool needDelim = false;
    for (int i = 0; i < c.Length(); ++i)
    {
        if (needDelim) o << L", ";
        needDelim = true;
        o << c[i];
    }
    if (needDelim) o << L" ";
    o << L"]";
    return o;
}

template<typename T>
std::wostream& operator<<(std::wostream& o, std::shared_ptr<Sequence<T>> const& c)
{
    o << (*c.get());
    return o;
}

template<typename T>
class ArraySequence : public Sequence<T>
{
public:
    ArraySequence(int length)
        : m_data(nullptr)
        , m_length(length)
    {
        m_data = new T[length];
    }

    ~ArraySequence()
    {
        delete[] m_data;
    }

    int Length() const override { return m_length; }

    T operator[](int i) const override
    {
        return m_data[i];
    }

    T& operator[](int i) override
    {
        return m_data[i];
    }

    void fill(T item)
    {
        std::fill(m_data, m_data + m_length, item);
    }

    void copy_from_memory(const T* src)
    {
        std::copy(src, src + m_length, m_data);
    }

    void copy_to_memory(T* dest)
    {
        std::copy(m_data, m_data + m_length, dest);
    }
private:
    T* m_data;
    int m_length;
};

template<typename T>
class Subsequence : public Sequence<T>
{
public:
    Subsequence(std::shared_ptr<Sequence<T>> parent, int offset, int length, int stride)
        : m_parent(parent)
        , m_offset(offset)
        , m_length(length)
        , m_stride(stride)
    {
    }

    int Length() const { return m_length; }

    T operator[](int i) const
    {
        return (*m_parent)[m_offset + i * m_stride];
    }

    T& operator[](int i)
    {
        return (*m_parent)[m_offset + i * m_stride];
    }

private:
    std::shared_ptr<Sequence<T>> m_parent;
    int m_offset;
    int m_length;
    int m_stride;
};

namespace FFTUtils
{
    constexpr bool IsPowerOfTwo(int i)
    {
        return i != 0 && (i & (i - 1)) == 0;
    }

    std::shared_ptr<Sequence<Complex>> AllocateSequence(int size);
    std::shared_ptr<Sequence<Complex>> AllocateSequenceSquare(int size);
    std::shared_ptr<Sequence<Complex>> AllocateSequenceTriangular(int size);
    std::shared_ptr<Sequence<Complex>> AllocateSequenceSawtooth(int size);
    std::shared_ptr<Sequence<Complex>> AllocateCopyFromMemory(const float* src, int size);
    void CopyToMemory(std::shared_ptr<Sequence<Complex>> seq, float* dest, int size);
    void Swizzle(std::shared_ptr<Sequence<Complex>> const& input, std::shared_ptr<Sequence<Complex>> output);
    std::shared_ptr<Sequence<Complex>> LeftHalf(std::shared_ptr<Sequence<Complex>> input);
    std::shared_ptr<Sequence<Complex>> RightHalf(std::shared_ptr<Sequence<Complex>> input);
    void DoFFT(std::shared_ptr<Sequence<Complex>> const& input, std::shared_ptr<Sequence<Complex>> output, bool isInverse);
}
