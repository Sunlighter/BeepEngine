#pragma once

extern "C" __declspec(dllimport) bool FFT(const float* src, float* dest, int size, bool isInverse);
extern "C" __declspec(dllimport) int GetLogSize();
extern "C" __declspec(dllimport) void GetLogEntry(int index, wchar_t* buffer, int bufferSize);
