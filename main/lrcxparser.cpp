#include "lrcxparser.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QFileInfo>
#include <QDir>
#include <QtMath>
#include <algorithm>
#include <QVector>
static int unicodeLength(const QString &s)
{
    return s.toUcs4().size();
}
static QString unicodeLeft( const QString &s, int count)
{
    auto ucs4 = s.toUcs4();
    if(count >= ucs4.size()) return s;
    QVector<uint> out;
    for(int i = 0; i < count; ++i) out.append(ucs4[i]);
    return QString::fromUcs4( out.constData(), out.size() );
}
int LrcLine::highlightCount(qint64 lineOffsetMs) const
{
    if (!hasTiming || chars.isEmpty())
    {
        return unicodeLength(text);
    }
    for (int i = 0; i < chars.size() - 1; ++i)
    {
        const CharTiming &cur = chars[i];
        const CharTiming &next = chars[i + 1];
        qint64 startTime = cur.offsetMs;
        qint64 endTime = next.offsetMs;
        int startIndex = cur.charIndex;
        int endIndex = next.charIndex;
        if (lineOffsetMs >= startTime && lineOffsetMs < endTime)
        {
            int charCount = endIndex - startIndex;
            if (charCount <= 0) return startIndex;
            double t = double(lineOffsetMs - startTime) / double(endTime - startTime);
            t = qBound(0.0, t, 1.0);
            int litChars = qCeil(charCount * t);
            return startIndex + litChars;
        }
    }
    return unicodeLength(text);
}
qint64 LrcxParser::parseTimestamp( const QString &inner)
{
    static const QRegularExpression re( R"((\d+):(\d+)\.(\d+))" );
    auto m = re.match(inner);
    if (!m.hasMatch()) return -1;
    qint64 mins = m.captured(1).toLongLong();
    qint64 secs = m.captured(2).toLongLong();
    QString ms3 = m.captured(3);
    while (ms3.size() < 3) ms3 += '0';
    qint64 ms = ms3.left(3).toLongLong();
    return mins * 60000 + secs * 1000 + ms;
}
void LrcxParser::parseTt( const QString &body, LrcLine &line)
{
    static const QRegularExpression re( R"(<([^>]+)>)" );
    QRegularExpressionMatchIterator it = re.globalMatch(body);
    line.chars.clear();
    while (it.hasNext())
    {
        QString inner = it.next().captured(1);
        QStringList p = inner.split(',');
        if (p.size() == 2)
        {
            CharTiming ct;
            ct.offsetMs = static_cast<qint64>( p[0].toDouble() );
            ct.charIndex = p[1].toInt();
            line.chars.append(ct);
        }
        else if (p.size() == 1)
        {
            CharTiming ct;
            ct.offsetMs = static_cast<qint64>( p[0].toDouble() );
            ct.charIndex = line.text.size();
            line.chars.append(ct);
        }
    }
    line.hasTiming = line.chars.size() >= 2;
}
// Parses lines where every character is prefixed by its own timestamp, e.g.:
//   [00:39.423]電[00:39.745]車[00:40.015]に...
// Returns true if the line contains at least one inter-character timestamp
// (i.e. more than just the leading line-start tag).
bool LrcxParser::parseInlineKaraoke(const QString &raw, LrcLine &line)
{
    static const QRegularExpression tsTagRe( R"(\[(\d+:\d+\.\d+)\])" );

    // Collect all timestamp matches and the text segments between them
    QRegularExpressionMatchIterator mit = tsTagRe.globalMatch(raw);
    QList<QRegularExpressionMatch> matches;
    while (mit.hasNext()) matches.append(mit.next());

    if (matches.size() < 2) return false; // need at least 2 tags to be karaoke

    // The first tag is the line start; after it comes: char, [ts], char, [ts], ...
    // Check that there is non-timestamp content interspersed between the tags.
    // We confirm it's inline-karaoke by verifying text exists between consecutive tags.
    bool hasInterleaved = false;
    for (int i = 0; i < matches.size() - 1; ++i)
    {
        int segStart = matches[i].capturedEnd();
        int segEnd   = matches[i + 1].capturedStart();
        if (segEnd > segStart)
        {
            hasInterleaved = true;
            break;
        }
    }
    if (!hasInterleaved) return false;

    // Build the plain text and CharTiming list.
    // Each [ts] marks the start time of the text that follows it until the next [ts].
    QString text;
    QVector<CharTiming> chars;

    qint64 lineStartMs = parseTimestamp(matches.first().captured(1));
    if (lineStartMs < 0) return false;

    for (int i = 0; i < matches.size(); ++i)
    {
        qint64 tsMs = parseTimestamp(matches[i].captured(1));
        if (tsMs < 0) continue;

        int segStart = matches[i].capturedEnd();
        int segEnd   = (i + 1 < matches.size()) ? matches[i + 1].capturedStart() : raw.length();
        QString seg  = raw.mid(segStart, segEnd - segStart);

        if (seg.isEmpty()) continue; // tag with no following text (e.g. trailing end-tag)

        CharTiming ct;
        ct.offsetMs  = tsMs - lineStartMs; // store as offset from line start
        ct.charIndex = unicodeLength(text); // index of first char of this segment
        chars.append(ct);

        text += seg;
    }

    if (text.isEmpty() || chars.size() < 2) return false;

    // Append a sentinel timing at the end so highlightCount() can interpolate the last segment.
    // Use the last tag's absolute time as the end; if the last match had no following text
    // (pure end-tag), use its timestamp directly, otherwise estimate duration = avg char duration.
    {
        // Check whether the last match in `matches` had no following text (it's a pure end-tag)
        int lastSegStart = matches.last().capturedEnd();
        bool lastIsEndTag = (lastSegStart >= raw.length() ||
                             raw.mid(lastSegStart).trimmed().isEmpty());
        if (lastIsEndTag)
        {
            qint64 endMs = parseTimestamp(matches.last().captured(1));
            if (endMs > lineStartMs)
            {
                CharTiming sentinel;
                sentinel.offsetMs  = endMs - lineStartMs;
                sentinel.charIndex = unicodeLength(text);
                chars.append(sentinel);
            }
        }
        else
        {
            // No explicit end tag: estimate end = last char offset + avg inter-char gap
            if (chars.size() >= 2)
            {
                qint64 gap = (chars.last().offsetMs - chars.first().offsetMs) / (chars.size() - 1);
                CharTiming sentinel;
                sentinel.offsetMs  = chars.last().offsetMs + gap;
                sentinel.charIndex = unicodeLength(text);
                chars.append(sentinel);
            }
        }
    }

    line.startMs  = lineStartMs;
    line.text     = text;
    line.chars    = chars;
    line.hasTiming = (chars.size() >= 2);
    return true;
}

bool LrcxParser::load( const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QTextStream in(&f);
    in.setEncoding( QStringConverter::Utf8 );
    clear();
    static const QRegularExpression tsTagRe( R"(\[(\d+:\d+\.\d+)\])" );
    static const QRegularExpression ttTagRe( R"(\[tt\])" );
    while (!in.atEnd())
    {
        QString raw = in.readLine();
        if (raw.trimmed().isEmpty()) continue;
        // Try inline-karaoke format first (every char has its own leading [ts])
        {
            LrcLine karaokeCandidate;
            if (parseInlineKaraoke(raw, karaokeCandidate))
            {
                m_lines.append(karaokeCandidate);
                continue;
            }
        }

        QList<qint64> timestamps;
        QRegularExpressionMatchIterator mit = tsTagRe.globalMatch(raw);
        QList<QRegularExpressionMatch> matches;
        while (mit.hasNext()) matches.append(mit.next());
        if (matches.isEmpty()) continue;
        qint64 startTs = parseTimestamp( matches.first().captured(1) );
        if (startTs < 0) continue;
        timestamps.append(startTs);
        int textStart = matches.first().capturedEnd();
        int textEnd = raw.length();
        bool hasEndTimestamp = false;
        qint64 endTs = -1;
        if (matches.size() >= 2)
        {
            auto last = matches.last();
            if (last.capturedEnd() == raw.length())
            {
                hasEndTimestamp = true;
                endTs = parseTimestamp( last.captured(1) );
                textEnd = last.capturedStart();
            }
        }
        QString rest = raw.mid( textStart, textEnd - textStart ).trimmed();
        bool hasTt = ttTagRe.match(rest) .hasMatch();
        if (hasTt)
        {
            auto ttM = ttTagRe.match(rest);
            QString body = rest.mid( ttM.capturedEnd() );
            qint64 ts = timestamps.first();
            for (int i = m_lines.size() - 1;
                 i >= 0;
                 --i)
            {
                if (m_lines[i].startMs == ts)
                {
                    parseTt( body, m_lines[i] );
                    break;
                }
            }
        }
        else
        {
            QString text = rest.trimmed();
            if(hasEndTimestamp && !text.isEmpty())
            {
                bool attachedTranslation = false;
                for (int i = m_lines.size() - 1; i >= 0; --i)
                {
                    if (m_lines[i].startMs != startTs) continue;
                    if (m_lines[i].text != text && m_lines[i].translation.isEmpty())
                    {
                        m_lines[i].translation = text;
                        attachedTranslation = true;
                    }
                    break;
                }
                if(attachedTranslation)
                    continue;
            }
            for (qint64 ts : timestamps)
            {
                LrcLine line;
                line.startMs = ts;
                line.text = text;
                line.hasTiming = false;
                if (hasEndTimestamp && endTs > ts)
                {
                    CharTiming c0;
                    c0.offsetMs = 0;
                    c0.charIndex = 0;
                    CharTiming c1;
                    c1.offsetMs = endTs - ts;
                    c1.charIndex = unicodeLength(text);
                    line.chars.append(c0);
                    line.chars.append(c1);
                    line.hasTiming = true;
                }
                m_lines.append(line);
            }
        }
    }
    std::stable_sort( m_lines.begin(), m_lines.end(), [](const LrcLine &a, const LrcLine &b)
                     {
                         return a.startMs < b.startMs;
                     }
                     );
    return !m_lines.isEmpty();
}
void LrcxParser::clear()
{
    m_lines.clear();
}
int LrcxParser::currentLine( qint64 posMs) const
{
    if (m_lines.isEmpty()) return -1;
    int res = -1;
    for (int i = 0;
         i < m_lines.size();
         ++i)
    {
        if (m_lines[i].startMs <= posMs) res = i; else break;
    }
    return res;
}
QString LrcxParser::findLyricFile( const QString &audioPath, const QString &lyricsDir)
{
    QFileInfo afi(audioPath);
    QString base = afi.fileName();
    QString stem = afi.completeBaseName();
    QStringList dirs;
    if (!lyricsDir.isEmpty()) dirs << lyricsDir;
    dirs << afi.absolutePath();
    for (const QString &d : dirs)
    {
        QDir dir(d);
        for (const QString &suf : QStringList
             {
                 " - .lrcx", " - .lrc", ".lrcx", ".lrc"
             }
             )
        {
            QString cand = suf.startsWith(' ') ? dir.filePath(base + suf) : dir.filePath(stem + suf);
            if (QFile::exists(cand)) return cand;
        }
    }
    return
        {
        }
    ;
}
