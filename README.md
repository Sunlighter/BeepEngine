<!-- -*- coding: utf-8; fill-column: 118 -*- -->

# BeepEngine

A C++ DLL that generates beeps

## Purpose and Features

This project centers around a DLL that generates beeps.

It has a simple API that can be called from other languages.

```cpp
extern "C" __declspec(dllexport) bool StartBeepEngine();

extern "C" __declspec(dllexport) void StopBeepEngine();

extern "C" __declspec(dllexport) bool IsBeepEngineRunning();

extern "C" __declspec(dllexport) void BeepEngineBeep(float frequency, float duration);

extern "C" __declspec(dllexport) void BeepEngineClearBuffer();

extern "C" __declspec(dllexport) void BeepEngineAddNoteToBuffer(float startTime, float frequency, float amplitude, float duration);

extern "C" __declspec(dllexport) void BeepEngineAddEventToBuffer(float time, UINT32 eventId);

extern "C" __declspec(dllexport) void BeepEngineStartPlayBuffer();

extern "C" __declspec(dllexport) bool BeepEngineWaitForEvent(UINT32 eventId);
```

The beep engine generates audio the whole time it is running. If there are no beeps going on, it generates silence.

The `BeepEngineBeep` function queues a beep and returns immediately.

The buffering capability allows you to build a combination of beeps and &ldquo;events.&rdquo; Once the buffer is
created, you can play it. Usually you would create an event at the end of the buffer, and wait for that event, so that
you would know that the buffer had finished playing. However, it is possible to put events anywhere in the buffer.

I was also working on Fast Fourier Transforms. I intended to support different waveforms such as square waves,
sawtooth, triangular, etc., and FFTs allow that to be done without aliasing. The FFTs are implemented and work, but
the rest of the work (creating, allocating, initializing, filtering waveforms) has not yet been done.
