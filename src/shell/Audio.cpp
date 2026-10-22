#include "shell/Audio.h"

namespace jpog::shell
{

namespace
{

// miniaudio silences the output buffer before each callback, so writing nothing plays silence.
void PlaySilence(ma_device*, void*, const void*, ma_uint32)
{
}

} // namespace

Audio::~Audio()
{
	Shutdown();
}

bool Audio::Init(bool nullBackend)
{
	const ma_backend nullOnly[] = {ma_backend_null};
	const ma_context_config contextConfig = ma_context_config_init();
	if (ma_context_init(nullBackend ? nullOnly : nullptr, nullBackend ? 1 : 0, &contextConfig, &m_oContext) !=
	    MA_SUCCESS)
		return false;
	m_bContext = true;

	ma_device_config config = ma_device_config_init(ma_device_type_playback);
	config.playback.format = ma_format_f32;
	config.playback.channels = 2;
	config.sampleRate = 44100;
	config.dataCallback = PlaySilence;
	if (ma_device_init(&m_oContext, &config, &m_oDevice) != MA_SUCCESS)
		return false;
	if (ma_device_start(&m_oDevice) != MA_SUCCESS)
	{
		ma_device_uninit(&m_oDevice);
		return false;
	}
	m_bDevice = true;
	return true;
}

void Audio::Shutdown()
{
	if (m_bDevice)
		ma_device_uninit(&m_oDevice);
	if (m_bContext)
		ma_context_uninit(&m_oContext);
	m_bDevice = false;
	m_bContext = false;
}

std::string Audio::Describe() const
{
	if (!m_bDevice)
		return "no audio device";
	return std::string(m_oDevice.playback.name) + " (" + ma_get_backend_name(m_oContext.backend) + ", " +
	       std::to_string(m_oDevice.sampleRate) + " Hz)";
}

} // namespace jpog::shell
