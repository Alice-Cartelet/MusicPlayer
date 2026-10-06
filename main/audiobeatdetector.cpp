#include "audiobeatdetector.h"

#include <QAudioFormat>
#include <algorithm>
#include <cmath>

void AudioBeatDetector::reset()
{
    *this = AudioBeatDetector();
}

AudioBeatDetector::Result AudioBeatDetector::consume(const QAudioBuffer &buffer)
{
    Result result;
    if (!buffer.isValid()) return result;
    const QAudioFormat format = buffer.format();
    const int channels = format.channelCount();
    const int rate = format.sampleRate();
    const int bytesPerSample = format.bytesPerSample();
    if (channels <= 0 || rate <= 0 || bytesPerSample <= 0 || buffer.frameCount() <= 0)
        return result;

    const auto *samples = buffer.constData<quint8>();
    const int windowSize = std::max(1, rate / 50); // 20 ms onset windows
    const double lowPassAlpha = 1.0 - std::exp(-2.0 * 3.141592653589793 * 150.0 / rate);
    double lastLevel = 0;
    double lastBass = 0;
    for (qsizetype frame = 0; frame < buffer.frameCount(); ++frame) {
        double mono = 0;
        const auto *frameData = samples + frame * channels * bytesPerSample;
        for (int channel = 0; channel < channels; ++channel)
            mono += format.normalizedSampleValue(frameData + channel * bytesPerSample);
        mono /= channels;
        m_lowPass += lowPassAlpha * (mono - m_lowPass);
        m_totalEnergy += mono * mono;
        m_bassEnergy += m_lowPass * m_lowPass;
        ++m_windowFrames;
        ++m_sinceBeatFrames;
        if (m_windowFrames < windowSize) continue;

        const double levelEnergy = m_totalEnergy / m_windowFrames;
        const double bassEnergy = m_bassEnergy / m_windowFrames;
        lastLevel = std::sqrt(levelEnergy);
        lastBass = std::sqrt(bassEnergy);
        if (m_baseline > 0) {
            const double ratio = bassEnergy / std::max(m_baseline, 0.000001);
            if (!m_armed && ratio < 1.25) m_armed = true;
            if (m_armed && m_sinceBeatFrames >= rate * 0.18 &&
                bassEnergy > 0.00008 && ratio > 1.7) {
                result.beat = true;
                result.strength = std::max(result.strength,
                    float(std::clamp((ratio - 1.7) / 3.0, 0.15, 1.0)));
                m_armed = false;
                m_sinceBeatFrames = 0;
            }
            m_baseline += 0.035 * (bassEnergy - m_baseline);
        } else {
            m_baseline = std::max(bassEnergy, 0.000001);
        }
        m_windowFrames = 0;
        m_totalEnergy = 0;
        m_bassEnergy = 0;
    }
    result.level = float(std::clamp(lastLevel * 3.0, 0.0, 1.0));
    result.bass = float(std::clamp(lastBass * 4.0, 0.0, 1.0));
    return result;
}
