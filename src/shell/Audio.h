#pragma once

#include <miniaudio.h>
#include <string>

namespace jpog::shell
{

// Audio output through miniaudio (replaces FMOD 3). Plays silence until the sound system is
// reimplemented; the device and backend show in the developer menu.
class Audio
{
public:
	Audio() = default;
	Audio(const Audio&) = delete;
	Audio& operator=(const Audio&) = delete;
	~Audio();

	// `nullBackend` uses miniaudio's null device (headless runs). False if no device opens.
	bool Init(bool nullBackend);
	void Shutdown();

	bool IsRunning() const { return m_bDevice; }
	std::string Describe() const;

private:
	ma_context m_oContext{};
	ma_device m_oDevice{};
	bool m_bContext = false;
	bool m_bDevice = false;
};

} // namespace jpog::shell
