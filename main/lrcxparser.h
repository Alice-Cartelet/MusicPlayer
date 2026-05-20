#pragma once
#include <QString>
#include <QList>
#include <QVector>
struct CharTiming
{
    qint64 offsetMs;
    int charIndex;
}
;
struct LrcLine
{
    qint64 startMs;
    QString text;
    QVector<CharTiming> chars;
    bool hasTiming = false;
    int highlightCount(qint64 lineOffsetMs) const;
}
;
class LrcxParser
{
public: bool load(const QString &filePath);
    void clear();
    bool isEmpty() const
    {
        return m_lines.isEmpty();
    }
    int count() const
    {
        return m_lines.size();
    }
    int currentLine(qint64 posMs) const;
    const LrcLine &line(int i) const
    {
        return m_lines[i];
    }
    static QString findLyricFile(const QString &audioFilePath, const QString &lyricsDir =
    {}
);
private: QList<LrcLine> m_lines;
    static qint64 parseTimestamp(const QString &inner);
    static void parseTt(const QString &ttBody, LrcLine &line);
}
;