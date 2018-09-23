#pragma once
#include <chrono>

class Timer
{
public:
	void StartTimer()
	{
		m_StartTime = std::chrono::system_clock::now();
		m_bIsRunning = true;
	}

	void StopTimer()
	{
		m_EndTime = std::chrono::system_clock::now();
		m_bIsRunning = false;
	}

	float GetElapsedTime()
	{
		std::chrono::time_point<std::chrono::system_clock> EndTime;

		if (m_bIsRunning)
			EndTime = std::chrono::system_clock::now();
		else
			EndTime = m_EndTime;

		return std::chrono::duration_cast<std::chrono::milliseconds>(EndTime - m_StartTime).count();
	}

private:
	std::chrono::time_point<std::chrono::system_clock> m_StartTime;
	std::chrono::time_point<std::chrono::system_clock> m_EndTime;
	bool                                               m_bIsRunning = false;
};