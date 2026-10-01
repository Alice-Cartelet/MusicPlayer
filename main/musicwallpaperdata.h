#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QImage>
#include "lrcxparser.h"

namespace MusicWallpaperData {
QJsonArray lyricTimeline(const LrcxParser &parser);
QJsonObject lyricAt(const LrcxParser &parser, qint64 positionMs, qint64 durationMs);
QString imageDataUrl(const QImage &image);
}
