#include "pch.h"

#include "beepengine.h"
#include "fft_internal.h"

class BufferData
{
public:
    BufferData()
        : m_bufferEvent(nullptr)
        , m_didCreateBufferEvent(false)
        , m_buffer(nullptr)
        , m_bufferSize(0)
        , m_lastError(0u)
        , m_xBuffer(nullptr)
    {
    }

    bool Initialize(int bufferSize)
    {
        m_bufferEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_bufferEvent == nullptr)
        {
            m_lastError = ::GetLastError();
            return false;
        }

        m_didCreateBufferEvent = true;

		m_buffer = new float[bufferSize];
		if (m_buffer == nullptr)
		{
			return false;
		}

        m_bufferSize = bufferSize;

		m_xBuffer = new XAUDIO2_BUFFER();
        if (m_xBuffer == nullptr)
        {
            return false;
        }

		memset(m_xBuffer, 0, sizeof(XAUDIO2_BUFFER));

		m_xBuffer->pAudioData = reinterpret_cast<BYTE*>(m_buffer);
		m_xBuffer->AudioBytes = bufferSize * sizeof(float);
		m_xBuffer->pContext = this;

        return true;
    }

	DWORD GetLastError() const { return m_lastError; }

	HANDLE GetEventHandle() const { return m_bufferEvent; }

	void SetEvent() const
	{
		::SetEvent(m_bufferEvent);
	}

	XAUDIO2_BUFFER* GetXBuffer() const { return m_xBuffer; }

	float* GetBuffer() const { return m_buffer; }

    int GetBufferSize() const { return m_bufferSize; }

    ~BufferData()
    {
        if (m_xBuffer != nullptr)
        {
            delete m_xBuffer;
            m_xBuffer = nullptr;
        }

        if (m_buffer != nullptr)
        {
            delete[] m_buffer;
			m_buffer = nullptr;
        }

        if (m_didCreateBufferEvent)
        {
            CloseHandle(m_bufferEvent);
            m_bufferEvent = nullptr;
            m_didCreateBufferEvent = false;
        }
    }
private:
    HANDLE m_bufferEvent;
    bool m_didCreateBufferEvent;
    float* m_buffer;
    int m_bufferSize;
    DWORD m_lastError;
    XAUDIO2_BUFFER * m_xBuffer;
};

class VoiceCallback2 : public IXAudio2VoiceCallback {
public:
    VoiceCallback2() {}
    void STDMETHODCALLTYPE OnBufferEnd(void* pBufferContext) override
    {
		BufferData* bufData = reinterpret_cast<BufferData*>(pBufferContext);
        bufData->SetEvent();
    }
    void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override {}
    void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
    void STDMETHODCALLTYPE OnStreamEnd() override {}
    void STDMETHODCALLTYPE OnBufferStart(void*) override {}
    void STDMETHODCALLTYPE OnLoopEnd(void*) override {}
    void STDMETHODCALLTYPE OnVoiceError(void*, HRESULT) override {}
};

class BeepCommand
{
public:
	virtual ~BeepCommand() {}
    virtual UINT32 EventStartTimeSamples() const = 0;
};

class BeepCommand_Beep : public BeepCommand
{
public:
	BeepCommand_Beep(UINT32 eventStartTimeSamples, float frequencyRadiansPerSample, float amplitude, UINT32 durationSamples)
		: m_eventStartTimeSamples(eventStartTimeSamples)
		, m_frequencyRadiansPerSample(frequencyRadiansPerSample)
		, m_amplitude(amplitude)
		, m_durationSamples(durationSamples)
	{
	}

	UINT32 EventStartTimeSamples() const override { return m_eventStartTimeSamples; }
	float FrequencyRadiansPerSample() const { return m_frequencyRadiansPerSample; }
	float Amplitude() const { return m_amplitude; }
    UINT32 DurationSamples() const { return m_durationSamples; }
private:
	const UINT32 m_eventStartTimeSamples;
	const float m_frequencyRadiansPerSample;
	const float m_amplitude;
	const UINT32 m_durationSamples;
};

class BeepCommand_Event : public BeepCommand
{
public:
	BeepCommand_Event(UINT32 eventStartTimeSamples, UINT32 eventId)
		: m_eventStartTimeSamples(eventStartTimeSamples)
		, m_eventId(eventId)
	{
	}

	UINT32 EventStartTimeSamples() const override { return m_eventStartTimeSamples; }
	UINT32 EventId() const { return m_eventId; }
private:
	const UINT32 m_eventStartTimeSamples;
	const UINT32 m_eventId;
};

class BeepCommandCompare
{
public:
    bool operator()(std::shared_ptr<BeepCommand> const & lhs, std::shared_ptr<BeepCommand> const & rhs) const
	{
		return lhs->EventStartTimeSamples() > rhs->EventStartTimeSamples();
	}
};

typedef std::set<UINT32> EventSet;
typedef std::priority_queue<std::shared_ptr<BeepCommand>, std::vector<std::shared_ptr<BeepCommand>>, BeepCommandCompare> BeepCommandQueue;

class AudioBeepCommand
{
public:
    virtual ~AudioBeepCommand() {}
    virtual std::shared_ptr<BeepCommand> CreateCommand(UINT32 sampleRate, UINT32 offsetTime, bool * pPostWrap) const = 0;
};

class AudioBeepCommand_Beep : public AudioBeepCommand
{
public:
    AudioBeepCommand_Beep(float eventStartTimeSeconds, float frequencyHz, float amplitude, float durationSeconds)
        : m_eventStartTimeSeconds(eventStartTimeSeconds)
        , m_frequencyHz(frequencyHz)
        , m_amplitude(amplitude)
        , m_durationSeconds(durationSeconds)
    {
    }

    float EventStartTimeSeconds() const { return m_eventStartTimeSeconds; }
    float FrequencyHz() const { return m_frequencyHz; }
    float Amplitude() const { return m_amplitude; }
    float DurationSeconds() const { return m_durationSeconds; }

    virtual std::shared_ptr<BeepCommand> CreateCommand(UINT32 sampleRate, UINT32 offsetTime, bool * pPostWrap) const override
    {
		UINT32 eventStartTimeSamples = static_cast<UINT32>(m_eventStartTimeSeconds * sampleRate);
		UINT32 offsetEventStartTimeSamples = eventStartTimeSamples + offsetTime;
		if (pPostWrap != nullptr)
		{
			*pPostWrap = offsetEventStartTimeSamples < offsetTime;
		}
		float frequencyRadiansPerSample = 2.0f * (float)(std::numbers::pi) * m_frequencyHz / sampleRate;
		UINT32 durationSamples = static_cast<UINT32>(m_durationSeconds * sampleRate);
		return std::shared_ptr<BeepCommand>(new BeepCommand_Beep(offsetEventStartTimeSamples, frequencyRadiansPerSample, m_amplitude, durationSamples));
    }
private:
    const float m_eventStartTimeSeconds;
    const float m_frequencyHz;
    const float m_amplitude;
    const float m_durationSeconds;
};

class AudioBeepCommand_Event : public AudioBeepCommand
{
public:
    AudioBeepCommand_Event(float eventStartTimeSeconds, UINT32 eventId)
        : m_eventStartTimeSeconds(eventStartTimeSeconds)
        , m_eventId(eventId)
    {
    }

    float EventStartTimeSeconds() const { return m_eventStartTimeSeconds; }
    UINT32 EventId() const { return m_eventId; }

    virtual std::shared_ptr<BeepCommand> CreateCommand(UINT32 sampleRate, UINT32 offsetTime, bool * pPostWrap) const override
    {
        UINT32 eventStartTimeSamples = static_cast<UINT32>(m_eventStartTimeSeconds * sampleRate);
		UINT32 offsetEventStartTimeSamples = eventStartTimeSamples + offsetTime;
        if (pPostWrap != nullptr)
        {
			*pPostWrap = offsetEventStartTimeSamples < offsetTime;
        }
        return std::shared_ptr<BeepCommand>(new BeepCommand_Event(offsetEventStartTimeSamples, m_eventId));
    }

private:
	const float m_eventStartTimeSeconds;
	const UINT32 m_eventId;
};

class AudioThreadCommand
{
public:
    virtual ~AudioThreadCommand() {}
};

class AudioThreadCommand_ScheduleBeeps : public AudioThreadCommand
{
public:
    AudioThreadCommand_ScheduleBeeps(std::vector<std::unique_ptr<AudioBeepCommand>> && commands)
        : commands(std::move(commands))
    {
    }

    std::vector<std::unique_ptr<AudioBeepCommand>> const & Commands() const { return commands; }
private:
    std::vector<std::unique_ptr<AudioBeepCommand>> commands;
};

class AudioThreadCommand_WaitForEvent : public AudioThreadCommand
{
public:
	AudioThreadCommand_WaitForEvent(UINT32 eventId, HANDLE hResponseEvent, bool * pEventOccurred)
		: m_eventId(eventId)
		, m_hResponseEvent(hResponseEvent)
        , m_pEventOccurred(pEventOccurred)
	{
	}
	UINT32 EventId() const { return m_eventId; }
	HANDLE ResponseEvent() const { return m_hResponseEvent; }
	void SetEventOccurred(bool eventOccurred) const
    {
        if (m_pEventOccurred != nullptr)
        {
            *m_pEventOccurred = eventOccurred;
        }
    }
private:
	const UINT32 m_eventId;
	const HANDLE m_hResponseEvent;
	bool * const m_pEventOccurred;
};

typedef std::map<UINT32, std::unique_ptr<AudioThreadCommand_WaitForEvent>> EventMap;

class BeepInProgress
{
public:
	virtual std::optional<std::unique_ptr<BeepInProgress>> AddToBuffer(float* buf, UINT32 size) = 0;
    virtual ~BeepInProgress() {}
};

class BeepInProgress_SineWave : public BeepInProgress
{
public:
    BeepInProgress_SineWave(float frequencyRadiansPerSample, float amplitude, UINT32 delayStart, UINT32 totalDuration, UINT32 alreadyPlayed = 0)
        : m_frequencyRadiansPerSample(frequencyRadiansPerSample)
        , m_amplitude(amplitude)
        , m_delayStart(delayStart)
        , m_totalDuration(totalDuration)
        , m_alreadyPlayed(alreadyPlayed)
    {
    }

    virtual std::optional<std::unique_ptr<BeepInProgress>> AddToBuffer(float* buf, UINT32 bufSize) override
	{
        if (m_delayStart > bufSize)
        {
            // this should never happen, it should still be in the queued beeps
            OutputDebugString(L"Delayed more than one buffer\n");
			return std::optional<std::unique_ptr<BeepInProgress>>(new BeepInProgress_SineWave(m_frequencyRadiansPerSample, m_amplitude, m_delayStart - bufSize, m_totalDuration, m_alreadyPlayed));
        }
        else
        {
            float* start = buf + m_delayStart;
			UINT32 sizeThisTime = min(m_totalDuration, bufSize - m_delayStart);
            float* end = start + sizeThisTime;
			for (float* it = start; it != end; ++it)
			{
				*it += m_amplitude * sinf(m_frequencyRadiansPerSample * (m_alreadyPlayed + (it - start)));
			}
            if (m_totalDuration > sizeThisTime)
            {
                return std::optional<std::unique_ptr<BeepInProgress>>
                (
                    std::unique_ptr<BeepInProgress>
                    (
                        new BeepInProgress_SineWave
                        (
                            m_frequencyRadiansPerSample,
                            m_amplitude,
                            0,
                            m_totalDuration - sizeThisTime,
                            m_alreadyPlayed + sizeThisTime
                        )
                    )
                );
            }
            else
            {
                return std::nullopt;
            }
        }
	}

private:
    const float m_frequencyRadiansPerSample;
    const float m_amplitude;
    const UINT32 m_delayStart;
    const UINT32 m_totalDuration;
    const UINT32 m_alreadyPlayed;
};

typedef std::vector<std::unique_ptr<BeepInProgress>> BeepInProgressVector;

HANDLE hStopEvent = nullptr;

class AudioThreadData
{
public:
    AudioThreadData(int bufferSize)
		: BUFFER_SIZE(bufferSize)
        , m_didCoInitialize(false)
        , m_pXAudio2(nullptr)
        , m_didCreateXAudio2(false)
        , m_pMasterVoice(nullptr)
        , m_didCreateMasteringVoice(false)
        , m_sampleRate(0)
        , m_lastError(0u)
		, m_buffer1(nullptr)
		, m_buffer2(nullptr)
        , m_callback(nullptr)
        , m_pSourceVoice(nullptr)
        , m_didCreateSourceVoice(false)
        , m_queueLock(nullptr)
        , m_currentTime(0u)
        , m_hQueueEvent(nullptr)
        , m_commandQueue(nullptr)
        , m_queuedBeeps(nullptr)
        , m_queuedBeepsPostWrap(nullptr)
        , m_possibleFutureEvents(nullptr)
        , m_waitingEvents(nullptr)
    {
    }

	UINT32 GetSampleRate() const { return m_sampleRate; }

	DWORD GetLastError() const { return m_lastError; }

    bool Initialize()
    {
        assert(!m_didCoInitialize);

        HRESULT coInitiailzeResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(coInitiailzeResult)) return false;
        m_didCoInitialize = true;

        HRESULT hr = XAudio2Create(&m_pXAudio2);
        if (FAILED(hr)) return false;
        m_didCreateXAudio2 = true;

        hr = m_pXAudio2->CreateMasteringVoice(&m_pMasterVoice);
        if (FAILED(hr)) return false;
        m_didCreateMasteringVoice = true;

        XAUDIO2_VOICE_DETAILS masterDetails;
        m_pMasterVoice->GetVoiceDetails(&masterDetails);
        m_sampleRate = masterDetails.InputSampleRate;

        m_buffer1 = std::unique_ptr<BufferData>(new BufferData());
        if (m_buffer1 == nullptr) return false;
        bool isInitialized = m_buffer1->Initialize(BUFFER_SIZE);
        if (!isInitialized) { m_lastError = m_buffer1->GetLastError(); return false; }

		m_buffer2 = std::unique_ptr<BufferData>(new BufferData());
        if (m_buffer2 == nullptr) return false;
        isInitialized = m_buffer2->Initialize(BUFFER_SIZE);
		if (!isInitialized) { m_lastError = m_buffer2->GetLastError(); return false; }

        m_callback = new VoiceCallback2();
		if (m_callback == nullptr) return false;

        WAVEFORMATEX wfx = { WAVE_FORMAT_IEEE_FLOAT, 1, m_sampleRate, m_sampleRate * 4, 4, 32, 0 };
        hr = m_pXAudio2->CreateSourceVoice(&m_pSourceVoice, &wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, m_callback);
        if (FAILED(hr)) return false;
        m_didCreateSourceVoice = true;

		m_queueLock = std::unique_ptr<std::mutex>(new std::mutex());
        if (m_queueLock == nullptr) return false;

		m_hQueueEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_hQueueEvent == nullptr) { m_lastError = ::GetLastError(); return false; }

		m_commandQueue = std::unique_ptr<std::queue<std::unique_ptr<AudioThreadCommand>>>(new std::queue<std::unique_ptr<AudioThreadCommand>>());
        if (m_commandQueue == nullptr) { return false; }

        m_queuedBeeps = std::unique_ptr<BeepCommandQueue>(new BeepCommandQueue());
        if (m_queuedBeeps == nullptr) { return false; }

		m_queuedBeepsPostWrap = std::unique_ptr<BeepCommandQueue>(new BeepCommandQueue());
        if (m_queuedBeepsPostWrap == nullptr) { return false; }
        
		m_possibleFutureEvents = std::unique_ptr<EventSet>(new EventSet());
		if (m_possibleFutureEvents == nullptr) { return false; }

        m_waitingEvents = std::unique_ptr<EventMap>(new EventMap());
		if (m_waitingEvents == nullptr) { return false; }

		m_beepInProgressVector = std::unique_ptr<BeepInProgressVector>(new BeepInProgressVector());
		if (m_beepInProgressVector == nullptr) { return false; }

        return true;
    }

    void ScheduleBeeps(std::vector<std::unique_ptr<AudioBeepCommand>>&& commands)
    {
		std::lock_guard<std::mutex> lock(*m_queueLock);
		m_commandQueue->push(std::unique_ptr<AudioThreadCommand>(new AudioThreadCommand_ScheduleBeeps(std::move(commands))));
		::SetEvent(m_hQueueEvent);
    }

    bool WaitForEvent(UINT32 eventId)
    {
		HANDLE hResponseEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (hResponseEvent == nullptr) return false;
		bool eventOccurred = false;
        {
			std::lock_guard<std::mutex> lock(*m_queueLock);
			m_commandQueue->push(std::unique_ptr<AudioThreadCommand>(new AudioThreadCommand_WaitForEvent(eventId, hResponseEvent, &eventOccurred)));
			::SetEvent(m_hQueueEvent);
        }
		WaitForSingleObject(hResponseEvent, INFINITE);
		CloseHandle(hResponseEvent);
        return eventOccurred;
    }

    void RunLoop()
    {
        HRESULT hr = SubmitBuffer(m_buffer1.get());
        if (FAILED(hr))
        {
            OutputDebugString(L"Failed to submit first buffer\n");
            Stop();
            return;
        }
        hr = SubmitBuffer(m_buffer2.get());
		if (FAILED(hr))
		{
			OutputDebugString(L"Failed to submit second buffer\n");
			Stop();
			return;
		}
        Start();
        HANDLE events[4] = { hStopEvent, m_hQueueEvent, m_buffer1->GetEventHandle(), m_buffer2->GetEventHandle() };
        while (true)
        {
            DWORD waitResult = WaitForMultipleObjects(4, events, FALSE, INFINITE);
            switch (waitResult)
            {
            case WAIT_OBJECT_0:
                Stop();
                return;

			case WAIT_OBJECT_0 + 1:
                ProcessQueue();
				break;

            case WAIT_OBJECT_0 + 2:
                RenderToBuffer(m_buffer1.get());
				hr = SubmitBuffer(m_buffer1.get());
                if (FAILED(hr))
                {
					OutputDebugString(L"Failed to submit buffer\n");
                    Stop();
                    return;
                }
				break;

			case WAIT_OBJECT_0 + 3:
                RenderToBuffer(m_buffer2.get());
				hr = SubmitBuffer(m_buffer2.get());
                if (FAILED(hr))
                {
					OutputDebugString(L"Failed to submit buffer\n");
					Stop();
					return;
                }
                break;
            }
        }
    }

    ~AudioThreadData()
    {
		if (m_hQueueEvent != nullptr)
		{
			CloseHandle(m_hQueueEvent);
			m_hQueueEvent = nullptr;
		}

        if (m_didCreateSourceVoice)
        {
			m_pSourceVoice->DestroyVoice();
			m_pSourceVoice = nullptr;
            m_didCreateSourceVoice = false;
        }

        if (m_callback != nullptr)
        {
            delete m_callback;
            m_callback = nullptr;
        }
        
        if (m_didCreateMasteringVoice)
        {
            m_pMasterVoice->DestroyVoice();
			m_didCreateMasteringVoice = false;
        }

        if (m_didCreateXAudio2)
        {
            m_pXAudio2->Release();
			m_didCreateXAudio2 = false;
        }

        if (m_didCoInitialize)
        {
            CoUninitialize();
			m_didCoInitialize = false;
        }
    }
private:
    const int BUFFER_SIZE;

    bool m_didCoInitialize;
    IXAudio2* m_pXAudio2;
	bool m_didCreateXAudio2;
    IXAudio2MasteringVoice* m_pMasterVoice;
	bool m_didCreateMasteringVoice;
    UINT32 m_sampleRate;
    DWORD m_lastError;
    std::unique_ptr<BufferData> m_buffer1;
    std::unique_ptr<BufferData> m_buffer2;
	VoiceCallback2 * m_callback;
    IXAudio2SourceVoice* m_pSourceVoice;
	bool m_didCreateSourceVoice;
    UINT32 m_currentTime;

	std::unique_ptr<std::mutex> m_queueLock;
    HANDLE m_hQueueEvent;
	std::unique_ptr<std::queue<std::unique_ptr<AudioThreadCommand>>> m_commandQueue;
    std::unique_ptr<BeepCommandQueue> m_queuedBeeps;
    std::unique_ptr<BeepCommandQueue> m_queuedBeepsPostWrap;
    std::unique_ptr<EventSet> m_possibleFutureEvents;
    std::unique_ptr<EventMap> m_waitingEvents;
    std::unique_ptr<BeepInProgressVector> m_beepInProgressVector;

	void Start()
	{
		m_pSourceVoice->Start(0);
	}

	void Stop()
	{
		m_pSourceVoice->Stop();
	}

    HRESULT SubmitBuffer(BufferData* bufferData)
    {
        HRESULT hr = m_pSourceVoice->SubmitSourceBuffer(bufferData->GetXBuffer());
		if (FAILED(hr))
		{
			OutputDebugString(L"Failed to submit buffer\n");
		}
        return hr;
    }

    void ProcessQueue()
    {
        std::lock_guard<std::mutex> lock(*m_queueLock);
        while (!m_commandQueue->empty())
        {
            std::unique_ptr<AudioThreadCommand> command = std::move(m_commandQueue->front());
            m_commandQueue->pop();

            AudioThreadCommand_ScheduleBeeps* sb = dynamic_cast<AudioThreadCommand_ScheduleBeeps*>(command.get());
            if (sb != nullptr)
            {
                std::vector<std::unique_ptr<AudioBeepCommand>> const& commands = sb->Commands();
                for (std::vector<std::unique_ptr<AudioBeepCommand>>::const_iterator it = commands.cbegin(); it != commands.cend(); ++it)
                {
                    bool postWrap = false;
                    std::shared_ptr<BeepCommand> beepCommand = (*it)->CreateCommand(m_sampleRate, m_currentTime, &postWrap);

                    BeepCommand_Event* eventCommand = dynamic_cast<BeepCommand_Event*>(beepCommand.get());
                    if (eventCommand != nullptr)
                    {
                        m_possibleFutureEvents->insert(eventCommand->EventId());
                    }

                    if (postWrap)
                    {
                        m_queuedBeepsPostWrap->push(beepCommand);
                    }
                    else
                    {
                        m_queuedBeeps->push(beepCommand);
                    }
                }
            }
            else
            {
                AudioThreadCommand_WaitForEvent* wfe = dynamic_cast<AudioThreadCommand_WaitForEvent*>(command.get());
                if (wfe != nullptr)
                {
                    if (m_possibleFutureEvents->find(wfe->EventId()) == m_possibleFutureEvents->end())
                    {
                        // this event cannot possibly happen (or has already happened), so we return immediately
                        wfe->SetEventOccurred(false);
                        ::SetEvent(wfe->ResponseEvent());
                    }
                    else
                    {
                        command.release();
						std::unique_ptr<AudioThreadCommand_WaitForEvent> wfe2 = std::unique_ptr<AudioThreadCommand_WaitForEvent>(wfe);
                        m_waitingEvents->insert(std::move(std::make_pair(wfe->EventId(), std::move(wfe2))));
                    }
                }
                else
                {
                    OutputDebugString(L"Unknown command type\n");
                }
            }
        }
    }

    void RenderToBuffer(BufferData* bufferData)
    {
        float* buffer = bufferData->GetBuffer();
        std::fill(buffer, buffer + bufferData->GetBufferSize(), 0.0f);

        UINT32 endTime = m_currentTime + bufferData->GetBufferSize();

        auto processQueuedBeeps = [=](bool all)
        {
            while (!m_queuedBeeps->empty() && (all || m_queuedBeeps->top()->EventStartTimeSamples() < endTime))
            {
                BeepCommand_Beep* beepCommand = dynamic_cast<BeepCommand_Beep*>(m_queuedBeeps->top().get());
                if (beepCommand != nullptr)
                {
                    m_beepInProgressVector->push_back
                    (
                        std::unique_ptr<BeepInProgress>
                        (
                            new BeepInProgress_SineWave
                            (
                                beepCommand->FrequencyRadiansPerSample(),
                                beepCommand->Amplitude(),
                                beepCommand->EventStartTimeSamples() - m_currentTime,
                                beepCommand->DurationSamples()
                            )
                        )
                    );
                }
                else
                {
                    BeepCommand_Event* eventCommand = dynamic_cast<BeepCommand_Event*>(m_queuedBeeps->top().get());
                    if (eventCommand != nullptr)
                    {
                        auto it = m_waitingEvents->find(eventCommand->EventId());
                        if (it != m_waitingEvents->end())
                        {
                            OutputDebugString(L"Waiting event found.\n");
                            it->second->SetEventOccurred(true);
                            ::SetEvent(it->second->ResponseEvent());
                            m_possibleFutureEvents->erase(eventCommand->EventId());
                            m_waitingEvents->erase(it);
                        }
                    }
                    else
                    {
                        OutputDebugString(L"Unknown command type\n");
                    }
                }
                m_queuedBeeps->pop();
            }
        };

        if (endTime < m_currentTime)
        {
            processQueuedBeeps(true);
            std::swap(m_queuedBeeps, m_queuedBeepsPostWrap);
        }
        processQueuedBeeps(false);

        BeepInProgressVector newBeepsInProgress;
        for (BeepInProgressVector::const_iterator it = m_beepInProgressVector->cbegin(); it != m_beepInProgressVector->cend(); ++it)
        {
            std::optional<std::unique_ptr<BeepInProgress>> newBeep = (*it)->AddToBuffer(buffer, bufferData->GetBufferSize());
            if (newBeep.has_value())
            {
                newBeepsInProgress.push_back(std::move(newBeep.value()));
            }
        }

		std::swap(*m_beepInProgressVector, newBeepsInProgress);
        //*m_beepInProgressVector = std::move(newBeepsInProgress);

        m_currentTime = endTime;
    }
};

HANDLE hAudioThread = nullptr;
HANDLE hAudioThreadInitialized = nullptr;
AudioThreadData* pAudioThreadData = nullptr;

DWORD WINAPI AudioThreadProc(LPVOID arg)
{
    AudioThreadData a(2048);

    if (a.Initialize())
    {
        pAudioThreadData = &a;
		SetEvent(hAudioThreadInitialized);
        a.RunLoop();
        pAudioThreadData = nullptr;
        return 0;
    }
    else
	{
		return 1;
	}
}

extern "C" __declspec(dllexport) bool StartBeepEngine()
{
	if (hAudioThread != nullptr) return true;

	hAudioThreadInitialized = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (hAudioThreadInitialized == nullptr)
    {
        DWORD lastError = GetLastError();
        OutputDebugString(L"Failed to create hAudioThreadInitialized event\n");
        return false;
    }

	hStopEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (hStopEvent == nullptr)
    {
        DWORD lastError = GetLastError();
        OutputDebugString(L"Failed to create hStopEvent\n");
        CloseHandle(hAudioThreadInitialized);
        return false;
    }

	hAudioThread = CreateThread(nullptr, 0, AudioThreadProc, nullptr, 0, nullptr);
	if (hAudioThread == nullptr)
    {
        DWORD lastError = GetLastError();
        OutputDebugString(L"Failed to create hAudioThread\n");
        CloseHandle(hStopEvent);
        CloseHandle(hAudioThreadInitialized);
        return false;
    }

	HANDLE events[2] = { hAudioThread, hAudioThreadInitialized };
	DWORD waitResult = WaitForMultipleObjects(2, events, FALSE, INFINITE);

    if (waitResult == WAIT_OBJECT_0)
    {
        OutputDebugString(L"Audio thread failed to initialize\n");
        CloseHandle(hAudioThread);
        CloseHandle(hStopEvent);
        CloseHandle(hAudioThreadInitialized);
        return false;
    }

    return true;
}

extern "C" __declspec(dllexport) void StopBeepEngine()
{
	if (hAudioThread == nullptr) return;
	SetEvent(hStopEvent);
	WaitForSingleObject(hAudioThread, INFINITE);
	CloseHandle(hAudioThread);
	CloseHandle(hStopEvent);
	CloseHandle(hAudioThreadInitialized);
    pAudioThreadData = nullptr;
	hAudioThread = nullptr;
}

extern "C" __declspec(dllexport) bool IsBeepEngineRunning()
{
    return (hAudioThread != nullptr);
}

extern "C" __declspec(dllexport) void BeepEngineBeep(float frequency, float duration)
{
    const UINT32 eventId = 0xFFFFEA8Bu;
	if (pAudioThreadData == nullptr) return;
	std::vector<std::unique_ptr<AudioBeepCommand>> commands;
	commands.push_back(std::unique_ptr<AudioBeepCommand>(new AudioBeepCommand_Beep(0.0f, frequency, 0.125f, duration)));
    commands.push_back(std::unique_ptr<AudioBeepCommand>(new AudioBeepCommand_Event(duration, eventId)));

	pAudioThreadData->ScheduleBeeps(std::move(commands));
    pAudioThreadData->WaitForEvent(eventId);
}

std::unique_ptr<std::vector<std::unique_ptr<AudioBeepCommand>>> g_beepCommands;

extern "C" __declspec(dllexport) void BeepEngineClearBuffer()
{
    g_beepCommands = std::unique_ptr<std::vector<std::unique_ptr<AudioBeepCommand>>>(new std::vector<std::unique_ptr<AudioBeepCommand>>());
}

extern "C" __declspec(dllexport) void BeepEngineAddNoteToBuffer(float startTime, float frequency, float amplitude, float duration)
{
	if (g_beepCommands == nullptr) BeepEngineClearBuffer();
    g_beepCommands->push_back
    (
        std::unique_ptr<AudioBeepCommand>
        (
            new AudioBeepCommand_Beep(startTime, frequency, amplitude, duration)
        )
    );
}

extern "C" __declspec(dllexport) void BeepEngineAddEventToBuffer(float time, UINT32 eventId)
{
	if (g_beepCommands == nullptr) BeepEngineClearBuffer();
	g_beepCommands->push_back
	(
		std::unique_ptr<AudioBeepCommand>
		(
			new AudioBeepCommand_Event(time, eventId)
		)
	);
}

extern "C" __declspec(dllexport) void BeepEngineStartPlayBuffer()
{
	if (pAudioThreadData == nullptr) return;
	if (g_beepCommands == nullptr) return;
    if (g_beepCommands->empty()) return;

	pAudioThreadData->ScheduleBeeps(std::move(*g_beepCommands));
	g_beepCommands = nullptr;
}

extern "C" __declspec(dllexport) bool BeepEngineWaitForEvent(UINT32 eventId)
{
    if (pAudioThreadData == nullptr) return false;
	return pAudioThreadData->WaitForEvent(eventId);
}
