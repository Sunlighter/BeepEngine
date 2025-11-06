#pragma once

extern "C" __declspec(dllexport) bool StartBeepEngine();

extern "C" __declspec(dllexport) void StopBeepEngine();

extern "C" __declspec(dllexport) bool IsBeepEngineRunning();

extern "C" __declspec(dllexport) void BeepEngineBeep(float frequency, float duration);

extern "C" __declspec(dllexport) void BeepEngineClearBuffer();

extern "C" __declspec(dllexport) void BeepEngineAddNoteToBuffer(float startTime, float frequency, float amplitude, float duration);

extern "C" __declspec(dllexport) void BeepEngineAddEventToBuffer(float time, UINT32 eventId);

extern "C" __declspec(dllexport) void BeepEngineStartPlayBuffer();

extern "C" __declspec(dllexport) bool BeepEngineWaitForEvent(UINT32 eventId);
