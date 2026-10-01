#include "musicwallpaperdata.h"
#include <QBuffer>
#include <algorithm>

QJsonArray MusicWallpaperData::lyricTimeline(const LrcxParser &parser)
{
    QJsonArray lines;
    for (int i = 0; i < parser.count(); ++i) {
        const auto &line = parser.line(i);
        QJsonArray timings;
        for (const auto &timing : line.chars)
            timings.append(QJsonObject{{"offsetMs", timing.offsetMs}, {"charIndex", timing.charIndex}});
        lines.append(QJsonObject{{"startMs", line.startMs}, {"text", line.text},
            {"translation", line.translation}, {"hasTiming", line.hasTiming}, {"chars", timings}});
    }
    return lines;
}

QJsonObject MusicWallpaperData::lyricAt(const LrcxParser &parser, qint64 positionMs, qint64 durationMs)
{
    const int index = parser.currentLine(positionMs);
    QJsonObject result{{"index", index}, {"previous", ""}, {"text", ""}, {"next", ""},
        {"translation", ""}, {"startMs", 0}, {"endMs", 0}, {"progress", 0.0},
        {"highlightedCharacters", 0.0}, {"hasTiming", false}};
    if (index < 0) {
        if (!parser.isEmpty()) result["next"] = parser.line(0).text;
        return result;
    }
    const auto &line = parser.line(index);
    const qint64 endMs = std::max(line.startMs,
        index + 1 < parser.count() ? parser.line(index + 1).startMs : durationMs);
    const qint64 offset = std::max<qint64>(0, positionMs - line.startMs);
    const int count = line.text.toUcs4().size();
    const double progress = endMs > line.startMs
        ? qBound(0.0, double(offset) / double(endMs - line.startMs), 1.0) : 0.0;
    double highlighted = count * progress;
    if (line.hasTiming && !line.chars.isEmpty()) {
        highlighted = offset < line.chars.first().offsetMs ? 0.0 : double(count);
        for (int i = 0; i + 1 < line.chars.size(); ++i) {
            const auto &a = line.chars[i];
            const auto &b = line.chars[i + 1];
            if (offset >= a.offsetMs && offset < b.offsetMs && b.offsetMs > a.offsetMs) {
                highlighted = a.charIndex + (b.charIndex - a.charIndex) *
                    double(offset - a.offsetMs) / double(b.offsetMs - a.offsetMs);
                break;
            }
        }
    }
    result["previous"] = index > 0 ? parser.line(index - 1).text : QString();
    result["text"] = line.text;
    result["next"] = index + 1 < parser.count() ? parser.line(index + 1).text : QString();
    result["translation"] = line.translation;
    result["startMs"] = line.startMs;
    result["endMs"] = endMs;
    result["progress"] = progress;
    result["highlightedCharacters"] = qBound(0.0, highlighted, double(count));
    result["hasTiming"] = line.hasTiming;
    return result;
}

QString MusicWallpaperData::imageDataUrl(const QImage &image)
{
    if (image.isNull()) return {};
    QByteArray png;
    QBuffer buffer(&png);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) return {};
    return QStringLiteral("data:image/png;base64,") + QString::fromLatin1(png.toBase64());
}
