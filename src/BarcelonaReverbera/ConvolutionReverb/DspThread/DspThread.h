#pragma once

#include <JuceHeader.h>

///////////////////////////////////////////////////////////////////////////////

class DspThread : public juce::Thread
{
public:
	DspThread(const juce::String& threadName, std::function<void(void)> const & funcInit, std::function<void(void)> const & funcExit, std::function<void(void)> const & funcProcessOnSignal) : juce::Thread(threadName)
	{
		m_funcInit = funcInit;
		m_funcExit = funcExit;
		m_funcProcessOnSignal = funcProcessOnSignal;
	}

	inline bool isInitialized(void)
	{
		return m_initialized;
	}

private:
	void run(void) override
	{
		m_initialized = false;
		
		m_funcInit();
	
		m_initialized = true;

		while (!threadShouldExit())
		{
			if (wait(-1.0))
			{
				if (threadShouldExit())
					break;

				m_funcProcessOnSignal();
			}
		}

		m_funcExit();

		m_initialized = false;
	}

private:
	std::function<void(void)> m_funcInit; 
	std::function<void(void)> m_funcExit; 
	std::function<void(void)> m_funcProcessOnSignal;

	std::atomic<bool> m_initialized = false;
	static_assert(std::atomic<bool>::is_always_lock_free);
};

///////////////////////////////////////////////////////////////////////////////
