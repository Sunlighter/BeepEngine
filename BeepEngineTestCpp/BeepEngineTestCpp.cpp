// BeepEngineTestCpp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <iostream>
#include <numbers>
#include "beepengine.h"
#include "fft.h"

class FloatBuffer
{
public:
	FloatBuffer(int size)
		: m_size(size)
		, m_buffer(new float[size])
	{
	}
	~FloatBuffer()
	{
		delete[] m_buffer;
	}
	float* GetBuffer() { return m_buffer; }
	int GetSize() { return m_size; }
private:
	int m_size;
	float* m_buffer;
};

void WriteBuffer(std::unique_ptr<FloatBuffer> const& buf)
{
	int size = buf->GetSize() / 2;
	const float* buffer = buf->GetBuffer();

	for (int i = 0; i < size; ++i)
	{
		float x = buffer[i * 2];
		float y = buffer[i * 2 + 1];
		float mag = sqrt(x * x + y * y);
		float phase = atan2f(y, x) / (float)std::numbers::pi;

		std::wcout << i << L": (" << x << L", " << y << L") = (mag " << mag << L", phase " << phase << L" pi)\n";
	}
}

int main()
{
#if 0
    bool b = StartBeepEngine();
    if (b)
    {
        std::wcout << L"Beep engine started.\n";
        Sleep(500);

		BeepEngineBeep(110.0f, 1.0f);
        BeepEngineBeep(220.0f, 1.0f);
        BeepEngineBeep(440.0f, 1.0f);
        BeepEngineBeep(880.0f, 1.0f);
        BeepEngineBeep(1760.0f, 1.0f);
        BeepEngineBeep(12000.0f, 1.0f);

        Sleep(500);

        BeepEngineClearBuffer();
        BeepEngineAddNoteToBuffer(0.0, 220.0, 0.125, 1.0);
        BeepEngineAddNoteToBuffer(0.5, 330.0, 0.125, 1.0);
        BeepEngineAddEventToBuffer(1.5, 0x378c);
        BeepEngineStartPlayBuffer();

		BeepEngineWaitForEvent(0x378c);

        Sleep(500);

        StopBeepEngine();
        std::wcout << L"Beep engine stopped.\n";
    }
    else
    {
        std::wcout << L"Beep engine did not start.\n";
    }
#endif

	int size = 16;
	std::unique_ptr<FloatBuffer> buf = std::make_unique<FloatBuffer>(size * 2);
	std::fill(buf->GetBuffer(), buf->GetBuffer() + buf->GetSize(), 0.0f);
	buf->GetBuffer()[2] = 1.0f;

	std::wcout << L"Input:\n";

	WriteBuffer(buf);

	std::unique_ptr <FloatBuffer> destBuf = std::make_unique<FloatBuffer>(size * 2);
	FFT(buf->GetBuffer(), destBuf->GetBuffer(), size, false);

	std::wcout << L"Output:\n";

	WriteBuffer(destBuf);

	std::wcout << L"\n";

	std::wcout << "Log Size: " << GetLogSize() << L"\n";

	std::wcout << "Log Entries:\n";

	wchar_t buffer[4097];
	for (int i = 0; i < GetLogSize(); ++i)
	{
		GetLogEntry(i, buffer, 4096);
		buffer[4096] = 0;
		std::wcout << buffer << L"\n";
	}

	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
