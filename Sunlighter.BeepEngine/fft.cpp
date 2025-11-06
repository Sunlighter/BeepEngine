#include "pch.h"
#include "fft_internal.h"

std::unique_ptr<std::deque<std::wstring>> g_log = std::make_unique<std::deque<std::wstring>>();

void Log(const std::wstring& message)
{
    g_log->push_back(message);
    if (g_log->size() > 1000)
    {
        g_log->pop_front();
    }
}

void WithLogStream(std::function<void(std::wostream & o)> f)
{
    std::wostringstream o;
    f(o);
    Log(o.str());
}

SomethingToWriteBase::~SomethingToWriteBase()
{
    // empty
}

static Complex root_of_unity(int i, int size, bool isInverse)
{
    float angle = 2.0f * (float)(std::numbers::pi)*i / (float)size;
    float theSin = sinf(angle);
    if (!isInverse) theSin = -theSin;
    return Complex(cosf(angle), theSin);
}

namespace FFTUtils
{
    std::shared_ptr<Sequence<Complex>> AllocateSequence(int size)
    {
        ArraySequence<Complex>* sPtr = new ArraySequence<Complex>(size);
        //sPtr->fill(Complex::zero); // should already be done
        return std::shared_ptr<Sequence<Complex>>(sPtr);
    }

    std::shared_ptr<Sequence<Complex>> AllocateSequenceSquare(int size)
    {
        ArraySequence<Complex>* sPtr = new ArraySequence<Complex>(size);
        int half = size >> 1;
        for (int i = 0; i < size; ++i)
        {
            Complex& c = (*sPtr)[i];
            if (i < half)
            {
                c = Complex(-1.0f, 0.0f);
            }
            else
            {
                c = Complex(1.0f, 0.0f);
            }
        }
        return std::shared_ptr<Sequence<Complex>>(sPtr);
    }

    std::shared_ptr<Sequence<Complex>> AllocateSequenceTriangular(int size)
    {
        ArraySequence<Complex>* sPtr = new ArraySequence<Complex>(size);
        int half = size >> 1;
        for (int i = 0; i < size; ++i)
        {
            Complex& c = (*sPtr)[i];
            if (i < half)
            {
                c = Complex(((float)i / (float)half) * 2.0f - 1.0f, 0.0f);
            }
            else
            {
                c = Complex(((float)(size - i) / (float)half) * 2.0f - 1.0f, 0.0f);
            }
        }
        return std::shared_ptr<Sequence<Complex>>(sPtr);
    }

    std::shared_ptr<Sequence<Complex>> AllocateSequenceSawtooth(int size)
    {
        ArraySequence<Complex>* sPtr = new ArraySequence<Complex>(size);
        for (int i = 0; i < size; ++i)
        {
            Complex& c = (*sPtr)[i];
            c = Complex(((float)i / (float)size) * 2.0f - 1.0f, 0.0f);
        }
        return std::shared_ptr<Sequence<Complex>>(sPtr);
    }

    std::shared_ptr<Sequence<Complex>> AllocateCopyFromMemory(const float* src, int size)
    {
        ArraySequence<Complex>* sPtr = new ArraySequence<Complex>(size);
        for (int i = 0; i < size; ++i)
        {
            (*sPtr)[i] = Complex(src[i * 2], src[i * 2 + 1]);
        }
        return std::shared_ptr<Sequence<Complex>>(sPtr);
    }

    void CopyToMemory(std::shared_ptr<Sequence<Complex>> seq, float* dest, int size)
    {
        assert(seq->Length() == size);
        for (int i = 0; i < size; ++i)
        {
            Complex const& c = (*seq)[i];
            dest[i * 2] = c.real();
            dest[i * 2 + 1] = c.imag();
        }
    }

    void Swizzle(std::shared_ptr<Sequence<Complex>> const& input, std::shared_ptr<Sequence<Complex>> output)
    {
        assert(input->Length() == output->Length());
        assert((input->Length() & 1) == 0);

        int iEnd = input->Length() / 2;
        for (int i = 0; i < iEnd; ++i)
        {
            (*output)[i] = (*input)[i * 2];
            (*output)[iEnd + i] = (*input)[i * 2 + 1];
        }
    }

    std::shared_ptr<Sequence<Complex>> LeftHalf(std::shared_ptr<Sequence<Complex>> input)
    {
        int size = input->Length() / 2;
        return std::shared_ptr<Sequence<Complex>>(new Subsequence<Complex>(input, 0, size, 1));
    }

    std::shared_ptr<Sequence<Complex>> RightHalf(std::shared_ptr<Sequence<Complex>> input)
    {
        int size = input->Length() / 2;
        return std::shared_ptr<Sequence<Complex>>(new Subsequence<Complex>(input, size, size, 1));
    }

    bool doLogging = false;

    void DoFFT(std::shared_ptr<Sequence<Complex>> const& input, std::shared_ptr<Sequence<Complex>> output, bool isInverse)
    {
        if (input->Length() == 2)
        {
            if (doLogging)
            {
                WithLogStream
                (
                    [&](std::wostream& o)
                    {
                        o << L"Input:" << input->Length() << L" " << input;
                    }
                );
            }

            Complex a = (*input)[0];
            Complex b = (*input)[1];
            (*output)[0] = a + b;
            (*output)[1] = a - b;

            if (doLogging)
            {
                WithLogStream
                (
                    [&](std::wostream& o)
                    {
                        o << L"output:" << output;
                    }
                );
            }
        }
        else if (IsPowerOfTwo(input->Length()))
        {
            int size = input->Length();

            if (doLogging)
            {
                WithLogStream
                (
                    [&](std::wostream& o)
                    {
                        o << L"Input:" << input->Length() << L" " << input;
                    }
                );
            }

            std::shared_ptr<Sequence<Complex>> temp0 = AllocateSequence(size);
            Swizzle(input, temp0);

            if (doLogging)
            {
                WithLogStream
                (
                    [&](std::wostream& o)
                    {
                        o << L"temp0:" << temp0;
                    }
                );
            }

            std::shared_ptr<Sequence<Complex>> temp1 = AllocateSequence(size);

            DoFFT(LeftHalf(temp0), LeftHalf(temp1), isInverse);
            DoFFT(RightHalf(temp0), RightHalf(temp1), isInverse);

            if (doLogging)
            {
                WithLogStream
                (
                    [&](std::wostream& o)
                    {
                        o << L"temp1:" << temp1;
                    }
                );
            }

            int halfSize = size / 2;

            for (int i = 1; i < halfSize; ++i)
            {
                Complex twiddle = root_of_unity(i, size, isInverse);
                Complex& c = (*temp1)[halfSize + i];
                c = c * twiddle;
            }

            if (doLogging)
            {
                WithLogStream
                (
                    [&](std::wostream& o)
                    {
                        o << L"temp1 (after twiddle factors):" << temp1;
                    }
                );
            }

            for (int i = 0; i < halfSize; ++i)
            {
                Complex a = (*temp1)[i];
                Complex b = (*temp1)[halfSize + i];
                (*output)[i] = a + b;
                (*output)[halfSize + i] = a - b;
            }

            if (doLogging)
            {
                WithLogStream
                (
                    [&](std::wostream& o)
                    {
                        o << L"output:" << output;
                    }
                );
            }
        }
        else
        {
            assert(false);
        }
    }

}

extern "C" __declspec(dllexport) bool FFT(const float* src, float* dest, int size, bool isInverse)
{
    if (!FFTUtils::IsPowerOfTwo(size)) return false;
    std::shared_ptr<Sequence<Complex>> input = FFTUtils::AllocateCopyFromMemory(src, size);
    std::shared_ptr<Sequence<Complex>> output = FFTUtils::AllocateSequence(size);
    FFTUtils::DoFFT(input, output, isInverse);
    FFTUtils::CopyToMemory(output, dest, size);
    if (isInverse)
    {
        int iEnd = size * 2;
        for (int i = 0; i < iEnd; ++i)
        {
            dest[i] /= (float)size;
        }
    }
    return true;
}

extern "C" __declspec(dllexport) int GetLogSize()
{
    return (int)g_log->size();
}

extern "C" __declspec(dllexport) void GetLogEntry(int index, wchar_t* buffer, int bufferSize)
{
    if (index < 0 || index >= g_log->size()) return;
    wcscpy_s(buffer, bufferSize, g_log->at(index).c_str());
}