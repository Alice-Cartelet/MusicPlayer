#pragma once

#include <QAudioBuffer>

// Detect onsets in the decoded low-frequency signal. The output is visual
// metadata only; the audio buffer is never changed or retained.
class AudioBeatDetector
{
public:
    struct Result {
        float level = 0;
        float bass = 0;
        bool beat = false;
        float strength = 0;
    };

    void reset();
    Result consume(const QAudioBuffer &buffer);

private:
    double m_lowPass = 0;
    double m_bassEnergy = 0;
    double m_totalEnergy = 0;
    double m_baseline = 0;
    int m_windowFrames = 0;
    int m_sinceBeatFrames = 0;
    bool m_armed = true;
};
