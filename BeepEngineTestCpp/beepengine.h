#pragma once

extern "C" __declspec(dllimport) bool StartBeepEngine();

extern "C" __declspec(dllimport) void StopBeepEngine();

extern "C" __declspec(dllimport) bool IsBeepEngineRunning();

extern "C" __declspec(dllimport) void BeepEngineBeep(float frequency, float duration);

extern "C" __declspec(dllimport) void BeepEngineClearBuffer();

extern "C" __declspec(dllimport) void BeepEngineAddNoteToBuffer(float startTime, float frequency, float amplitude, float duration);

extern "C" __declspec(dllimport) void BeepEngineAddEventToBuffer(float time, UINT32 eventId);

extern "C" __declspec(dllimport) void BeepEngineStartPlayBuffer();

extern "C" __declspec(dllimport) bool BeepEngineWaitForEvent(UINT32 eventId);
