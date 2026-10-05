#include "lyricsformat.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>
#include <QStringList>
#include <QtMath>
#include <zlib.h>

namespace {
qint64 parseTime(const QString &minutes, const QString &seconds, const QString &fraction)
{
    QString milliseconds = fraction;
    while (milliseconds.size() < 3) milliseconds += QLatin1Char('0');
    return minutes.toLongLong() * 60000 + seconds.toLongLong() * 1000
        + milliseconds.left(3).toLongLong();
}

QString stamp(qint64 ms)
{
    ms = qMax<qint64>(0, ms);
    return QStringLiteral("[%1:%2.%3]")
        .arg(ms / 60000, 2, 10, QChar('0'))
        .arg((ms / 1000) % 60, 2, 10, QChar('0'))
        .arg(ms % 1000, 3, 10, QChar('0'));
}

QString shiftStamps(const QString &line, int offsetMs, double speed)
{
    static const QRegularExpression re(QStringLiteral(R"(\[(\d+):(\d+)\.(\d{1,3})\])"));
    QString result;
    int previousEnd = 0;
    auto matches = re.globalMatch(line);
    while (matches.hasNext()) {
        const auto match = matches.next();
        result += line.mid(previousEnd, match.capturedStart() - previousEnd);
        const qint64 original = parseTime(match.captured(1), match.captured(2), match.captured(3));
        result += stamp(qRound64(original / speed) + offsetMs);
        previousEnd = match.capturedEnd();
    }
    result += line.mid(previousEnd);
    return result;
}

struct TimedLine {
    qint64 start = 0;
    qint64 end = 0;
    QString raw;
    QString text;
};

QList<TimedLine> timedLines(const QString &lrc)
{
    static const QRegularExpression startRe(QStringLiteral(R"(^\[(\d+):(\d+)\.(\d{1,3})\](.*)$)"));
    static const QRegularExpression stampRe(QStringLiteral(R"(\[(\d+):(\d+)\.(\d{1,3})\])"));
    QList<TimedLine> lines;
    for (const QString &raw : lrc.split(QLatin1Char('\n'))) {
        const QString trimmed = raw.trimmed();
        const auto match = startRe.match(trimmed);
        if (!match.hasMatch()) continue;
        TimedLine line;
        line.start = parseTime(match.captured(1), match.captured(2), match.captured(3));
        line.raw = trimmed;
        line.text = match.captured(4);
        auto stamps = stampRe.globalMatch(trimmed);
        while (stamps.hasNext()) {
            const auto timestamp = stamps.next();
            if (!stamps.hasNext() && timestamp.capturedEnd() == trimmed.size())
                line.end = parseTime(timestamp.captured(1), timestamp.captured(2), timestamp.captured(3));
        }
        lines.append(line);
    }
    return lines;
}

QString inflateKrc(const QByteArray &data)
{
    if (!data.startsWith("krc1") || data.size() <= 4) return {};
    static const unsigned char key[] = {
        0x40, 0x47, 0x61, 0x77, 0x5e, 0x32, 0x74, 0x47,
        0x51, 0x36, 0x31, 0x2d, 0xce, 0xd2, 0x6e, 0x69
    };
    QByteArray compressed = data.mid(4);
    for (qsizetype i = 0; i < compressed.size(); ++i)
        compressed[i] = char(uchar(compressed[i]) ^ key[i % sizeof(key)]);

    z_stream stream{};
    stream.next_in = reinterpret_cast<Bytef *>(compressed.data());
    stream.avail_in = uInt(compressed.size());
    if (inflateInit(&stream) != Z_OK) return {};
    QByteArray output;
    char chunk[16384];
    int status = Z_OK;
    while (status == Z_OK && output.size() < 4 * 1024 * 1024) {
        stream.next_out = reinterpret_cast<Bytef *>(chunk);
        stream.avail_out = sizeof(chunk);
        status = inflate(&stream, Z_NO_FLUSH);
        if (status == Z_OK || status == Z_STREAM_END)
            output.append(chunk, int(sizeof(chunk) - stream.avail_out));
    }
    inflateEnd(&stream);
    return status == Z_STREAM_END ? QString::fromUtf8(output) : QString();
}
}

LyricsContent LyricsFormat::decodeKrc(const QByteArray &data)
{
    LyricsContent content;
    const QString decoded = inflateKrc(data);
    if (decoded.isEmpty()) return content;
    static const QRegularExpression lineRe(QStringLiteral(R"(^\[(\d+),(\d+)\](.*)$)"));
    static const QRegularExpression wordRe(QStringLiteral(R"((?:\[\d+,\d+\])?<(\d+),(\d+),\d+>(.*?)(?=(?:\[\d+,\d+\])?<\d+,\d+,\d+>|$))"));
    static const QRegularExpression languageRe(QStringLiteral(R"(^\[language:([^\]]+)\]$)"));
    QStringList lineOutput, wordOutput;
    QList<qint64> starts;
    QString language;
    bool hasWordTiming = false;
    for (const QString &raw : decoded.split(QLatin1Char('\n'))) {
        const QString line = raw.trimmed();
        const auto languageMatch = languageRe.match(line);
        if (languageMatch.hasMatch()) {
            language = languageMatch.captured(1);
            continue;
        }
        const auto match = lineRe.match(line);
        if (!match.hasMatch()) continue;
        const qint64 start = match.captured(1).toLongLong();
        const qint64 end = start + match.captured(2).toLongLong();
        QString plain;
        QString timed = stamp(start);
        int wordCount = 0;
        auto words = wordRe.globalMatch(match.captured(3));
        while (words.hasNext()) {
            const auto word = words.next();
            const QString text = word.captured(3);
            plain += text;
            timed += stamp(start + word.captured(1).toLongLong()) + text;
            ++wordCount;
        }
        if (!wordCount) plain = match.captured(3);
        if (plain.trimmed().isEmpty()) continue;
        starts.append(start);
        lineOutput.append(stamp(start) + plain);
        if (wordCount) {
            wordOutput.append(timed + stamp(end));
            hasWordTiming = true;
        } else {
            wordOutput.append(stamp(start) + plain);
        }
    }
    content.lines = lineOutput.join(QLatin1Char('\n'));
    if (hasWordTiming) content.words = wordOutput.join(QLatin1Char('\n'));

    const QJsonObject translations = QJsonDocument::fromJson(QByteArray::fromBase64(language.toLatin1())).object();
    for (const QJsonValue &entry : translations.value(QStringLiteral("content")).toArray()) {
        const QJsonObject group = entry.toObject();
        if (group.value(QStringLiteral("type")).toInt(-1) != 1) continue;
        const QJsonArray lines = group.value(QStringLiteral("lyricContent")).toArray();
        QStringList translated;
        for (int i = 0; i < starts.size() && i < lines.size(); ++i) {
            const QJsonArray translatedWords = lines.at(i).toArray();
            const QString text = translatedWords.isEmpty() ? QString() : translatedWords.at(0).toString();
            if (!text.trimmed().isEmpty()) translated.append(stamp(starts.at(i)) + text);
        }
        content.translation = translated.join(QLatin1Char('\n'));
        break;
    }
    return content;
}

LyricsContent LyricsFormat::decodeNetease(const QString &yrc, const QString &lrc,
                                          const QString &translation)
{
    LyricsContent content;
    auto normalizeLines = [](const QString &input) {
        QStringList output;
        for (const QString &raw : input.split(QLatin1Char('\n'))) {
            const QString trimmed = raw.trimmed();
            if (!trimmed.startsWith(QLatin1Char('{'))) {
                if (!trimmed.isEmpty()) output.append(trimmed);
                continue;
            }
            const QJsonObject object = QJsonDocument::fromJson(trimmed.toUtf8()).object();
            if (!object.contains(QStringLiteral("t"))) continue;
            QString text;
            for (const QJsonValue &part : object.value(QStringLiteral("c")).toArray())
                text += part.toObject().value(QStringLiteral("tx")).toString();
            if (!text.trimmed().isEmpty()) output.append(stamp(object.value(QStringLiteral("t")).toInteger()) + text);
        }
        return output.join(QLatin1Char('\n'));
    };
    content.translation = normalizeLines(translation);
    // NetEase's YRC includes metadata as JSON lines. Lyric lines carry absolute
    // millisecond timings in [start,duration] and (wordStart,duration,flag).
    static const QRegularExpression lineRe(QStringLiteral(R"(^\[(\d+),(\d+)\](.*)$)"));
    static const QRegularExpression wordRe(QStringLiteral(R"(\((\d+),(\d+),\d+\)(.*?)(?=\(\d+,\d+,\d+\)|$))"));
    QStringList lines, words;
    for (const QString &raw : yrc.split(QLatin1Char('\n'))) {
        const auto line = lineRe.match(raw.trimmed());
        if (!line.hasMatch()) continue;
        const qint64 start = line.captured(1).toLongLong();
        const qint64 end = start + line.captured(2).toLongLong();
        QString plain, timed = stamp(start);
        int count = 0;
        auto matches = wordRe.globalMatch(line.captured(3));
        while (matches.hasNext()) {
            const auto word = matches.next();
            if (word.captured(3).isEmpty()) continue;
            plain += word.captured(3);
            timed += stamp(word.captured(1).toLongLong()) + word.captured(3);
            ++count;
        }
        if (plain.trimmed().isEmpty()) continue;
        lines.append(stamp(start) + plain);
        if (count) words.append(timed + stamp(end));
    }
    content.lines = lines.isEmpty() ? normalizeLines(lrc) : lines.join(QLatin1Char('\n'));
    content.words = words.join(QLatin1Char('\n'));
    return content;
}

QString LyricsFormat::render(const LyricsContent &content, bool wordByWord, bool includeTranslation,
                             int offsetMs, double speed, QString *error)
{
    if (error) error->clear();
    if (wordByWord && content.words.isEmpty()) {
        if (error) *error = QStringLiteral("这条结果没有逐字时间数据，请选逐句歌词或换一条结果。 ");
        return {};
    }
    const QList<TimedLine> originals = timedLines(wordByWord ? content.words : content.lines);
    if (originals.isEmpty()) {
        // Plain lyrics remain editable, but they cannot be synchronized.
        if (!wordByWord && !content.lines.trimmed().isEmpty()) return content.lines;
        if (error) *error = QStringLiteral("没有可用的时间轴歌词。 ");
        return {};
    }
    QMap<qint64, QString> translations;
    if (includeTranslation) {
        for (const TimedLine &line : timedLines(content.translation))
            if (!line.text.trimmed().isEmpty()) translations.insert(line.start, line.text);
    }
    QStringList output;
    for (int i = 0; i < originals.size(); ++i) {
        const TimedLine &line = originals.at(i);
        output.append(shiftStamps(line.raw, offsetMs, speed));
        if (!translations.contains(line.start)) continue;
        qint64 end = line.end > line.start ? line.end
            : i + 1 < originals.size() ? originals.at(i + 1).start - 1 : line.start + 3000;
        end = qMax(end, line.start + 1);
        output.append(shiftStamps(stamp(line.start) + translations.value(line.start) + stamp(end), offsetMs, speed));
    }
    return output.join(QLatin1Char('\n'));
}
