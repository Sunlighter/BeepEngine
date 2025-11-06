#pragma once

extern "C" __declspec(dllexport) bool FFT(const float * src, float * dest, int size, bool isInverse);
extern "C" __declspec(dllexport) int GetLogSize();
extern "C" __declspec(dllexport) void GetLogEntry(int index, wchar_t* buffer, int bufferSize);
