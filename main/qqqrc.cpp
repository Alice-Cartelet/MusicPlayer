// QQ cloud QRC decoder. The DES variant is adapted from QQMusicDecoder by WXRIW
// (MIT license; see third_party/qqmusicdecoder-LICENSE.txt).
#include "qqqrc.h"
#include "lyricsformat.h"

#include <QByteArray>
#include <QRegularExpression>
#include <QStringList>
#include <array>
#include <cstdint>
#include <zlib.h>

namespace {
using Byte = uint8_t;
using RoundKey = std::array<Byte, 6>;
using Schedule = std::array<RoundKey, 16>;
using Block = std::array<Byte, 8>;

constexpr int leftPermutation[32] = {
    57,49,41,33,25,17,9,1,59,51,43,35,27,19,11,3,
    61,53,45,37,29,21,13,5,63,55,47,39,31,23,15,7
};
constexpr int rightPermutation[32] = {
    56,48,40,32,24,16,8,0,58,50,42,34,26,18,10,2,
    60,52,44,36,28,20,12,4,62,54,46,38,30,22,14,6
};
constexpr int keyPermC[28] = {
    56,48,40,32,24,16,8,0,57,49,41,33,25,17,9,1,
    58,50,42,34,26,18,10,2,59,51,43,35
};
constexpr int keyPermD[28] = {
    62,54,46,38,30,22,14,6,61,53,45,37,29,21,13,5,
    60,52,44,36,28,20,12,4,27,19,11,3
};
constexpr int keyCompression[48] = {
    13,16,10,23,0,4,2,27,14,5,20,9,22,18,11,3,
    25,7,15,6,26,19,12,1,40,51,30,36,46,54,29,39,
    50,44,32,47,43,48,38,55,33,52,45,41,49,35,28,31
};
constexpr int roundShift[16] = {1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1};
constexpr Byte sbox[8][64] = {
    {14,4,13,1,2,15,11,8,3,10,6,12,5,9,0,7,0,15,7,4,14,2,13,1,10,6,12,11,9,5,3,8,
     4,1,14,8,13,6,2,11,15,12,9,7,3,10,5,0,15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13},
    {15,1,8,14,6,11,3,4,9,7,2,13,12,0,5,10,3,13,4,7,15,2,8,15,12,0,1,10,6,9,11,5,
     0,14,7,11,10,4,13,1,5,8,12,6,9,3,2,15,13,8,10,1,3,15,4,2,11,6,7,12,0,5,14,9},
    {10,0,9,14,6,3,15,5,1,13,12,7,11,4,2,8,13,7,0,9,3,4,6,10,2,8,5,14,12,11,15,1,
     13,6,4,9,8,15,3,0,11,1,2,12,5,10,14,7,1,10,13,0,6,9,8,7,4,15,14,3,11,5,2,12},
    {7,13,14,3,0,6,9,10,1,2,8,5,11,12,4,15,13,8,11,5,6,15,0,3,4,7,2,12,1,10,14,9,
     10,6,9,0,12,11,7,13,15,1,3,14,5,2,8,4,3,15,0,6,10,10,13,8,9,4,5,11,12,7,2,14},
    {2,12,4,1,7,10,11,6,8,5,3,15,13,0,14,9,14,11,2,12,4,7,13,1,5,0,15,10,3,9,8,6,
     4,2,1,11,10,13,7,8,15,9,12,5,6,3,0,14,11,8,12,7,1,14,2,13,6,15,0,9,10,4,5,3},
    {12,1,10,15,9,2,6,8,0,13,3,4,14,7,5,11,10,15,4,2,7,12,9,5,6,1,13,14,0,11,3,8,
     9,14,15,5,2,8,12,3,7,0,4,10,1,13,11,6,4,3,2,12,9,5,15,10,11,14,1,7,6,0,8,13},
    {4,11,2,14,15,0,8,13,3,12,9,7,5,10,6,1,13,0,11,7,4,9,1,10,14,3,5,12,2,15,8,6,
     1,4,11,13,12,3,7,14,10,15,6,8,0,5,9,2,6,11,13,8,1,4,10,7,9,5,0,15,14,2,3,12},
    {13,2,8,4,6,15,11,1,10,9,3,14,5,0,12,7,1,15,13,8,10,3,7,4,12,5,6,11,0,14,9,2,
     7,11,4,1,9,12,14,2,0,6,10,13,15,3,5,8,2,1,14,7,4,10,8,13,15,12,9,0,3,5,6,11}
};

uint32_t bitnum(const Byte *data, int bit, int shift)
{
    return uint32_t((data[(bit / 32) * 4 + 3 - (bit % 32) / 8] >> (7 - bit % 8)) & 1) << shift;
}

uint32_t bitnumIntr(uint32_t value, int bit, int shift)
{
    return ((value >> (31 - bit)) & 1u) << shift;
}

uint32_t bitnumIntl(uint32_t value, int bit, int shift)
{
    return ((value << bit) & 0x80000000u) >> shift;
}

void keySchedule(const Byte *key, bool decrypt, Schedule &schedule)
{
    uint32_t c = 0, d = 0;
    for (int i = 0; i < 28; ++i) {
        c |= bitnum(key, keyPermC[i], 31 - i);
        d |= bitnum(key, keyPermD[i], 31 - i);
    }
    for (int i = 0; i < 16; ++i) {
        c = ((c << roundShift[i]) | (c >> (28 - roundShift[i]))) & 0xfffffff0u;
        d = ((d << roundShift[i]) | (d >> (28 - roundShift[i]))) & 0xfffffff0u;
        RoundKey &round = schedule[decrypt ? 15 - i : i];
        round.fill(0);
        for (int j = 0; j < 24; ++j)
            round[j / 8] |= Byte(bitnumIntr(c, keyCompression[j], 7 - j % 8));
        for (int j = 24; j < 48; ++j)
            round[j / 8] |= Byte(bitnumIntr(d, keyCompression[j] - 27, 7 - j % 8));
    }
}

uint32_t roundFunction(uint32_t state, const RoundKey &key)
{
    const uint32_t t1 = bitnumIntl(state,31,0) | ((state & 0xf0000000u) >> 1)
        | bitnumIntl(state,4,5) | bitnumIntl(state,3,6) | ((state & 0x0f000000u) >> 3)
        | bitnumIntl(state,8,11) | bitnumIntl(state,7,12) | ((state & 0x00f00000u) >> 5)
        | bitnumIntl(state,12,17) | bitnumIntl(state,11,18) | ((state & 0x000f0000u) >> 7)
        | bitnumIntl(state,16,23);
    const uint32_t t2 = bitnumIntl(state,15,0) | ((state & 0x0000f000u) << 15)
        | bitnumIntl(state,20,5) | bitnumIntl(state,19,6) | ((state & 0x00000f00u) << 13)
        | bitnumIntl(state,24,11) | bitnumIntl(state,23,12) | ((state & 0x000000f0u) << 11)
        | bitnumIntl(state,28,17) | bitnumIntl(state,27,18) | ((state & 0x0000000fu) << 9)
        | bitnumIntl(state,0,23);
    const Byte x[6] = {
        Byte((t1 >> 24) ^ key[0]), Byte((t1 >> 16) ^ key[1]), Byte((t1 >> 8) ^ key[2]),
        Byte((t2 >> 24) ^ key[3]), Byte((t2 >> 16) ^ key[4]), Byte((t2 >> 8) ^ key[5])
    };
    auto sIndex = [](Byte value) { return (value & 0x20u) | ((value & 0x1fu) >> 1) | ((value & 1u) << 4); };
    state = (uint32_t(sbox[0][sIndex(Byte(x[0] >> 2))]) << 28)
        | (uint32_t(sbox[1][sIndex(Byte(((x[0] & 3) << 4) | (x[1] >> 4)))]) << 24)
        | (uint32_t(sbox[2][sIndex(Byte(((x[1] & 15) << 2) | (x[2] >> 6)))]) << 20)
        | (uint32_t(sbox[3][sIndex(Byte(x[2] & 63))]) << 16)
        | (uint32_t(sbox[4][sIndex(Byte(x[3] >> 2))]) << 12)
        | (uint32_t(sbox[5][sIndex(Byte(((x[3] & 3) << 4) | (x[4] >> 4)))]) << 8)
        | (uint32_t(sbox[6][sIndex(Byte(((x[4] & 15) << 2) | (x[5] >> 6)))]) << 4)
        | uint32_t(sbox[7][sIndex(Byte(x[5] & 63))]);
    constexpr int p[32] = {
        15,6,19,20,28,11,27,16,0,14,22,25,4,17,30,9,
        1,7,23,13,31,26,2,8,18,12,29,5,21,10,3,24
    };
    uint32_t result = 0;
    for (int i = 0; i < 32; ++i) result |= bitnumIntl(state, p[i], i);
    return result;
}

Block cryptBlock(const Block &input, const Schedule &schedule)
{
    uint32_t left = 0, right = 0;
    for (int i = 0; i < 32; ++i) {
        left |= bitnum(input.data(), leftPermutation[i], 31 - i);
        right |= bitnum(input.data(), rightPermutation[i], 31 - i);
    }
    for (int i = 0; i < 15; ++i) {
        const uint32_t oldRight = right;
        right = roundFunction(right, schedule[i]) ^ left;
        left = oldRight;
    }
    left = roundFunction(right, schedule[15]) ^ left;
    Block output{};
    auto setBit = [&output](int bit, uint32_t value) {
        output[(bit / 32) * 4 + 3 - (bit % 32) / 8]
            |= Byte(value << (7 - bit % 8));
    };
    for (int i = 0; i < 32; ++i) {
        setBit(leftPermutation[i], (left >> (31 - i)) & 1u);
        setBit(rightPermutation[i], (right >> (31 - i)) & 1u);
    }
    return output;
}

QByteArray inflateQrc(const QByteArray &compressed)
{
    z_stream stream{};
    stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(compressed.constData()));
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
    return status == Z_STREAM_END ? output : QByteArray();
}

QString stamp(qint64 ms)
{
    ms = qMax<qint64>(0, ms);
    return QStringLiteral("[%1:%2.%3]")
        .arg(ms / 60000, 2, 10, QChar('0'))
        .arg((ms / 1000) % 60, 2, 10, QChar('0'))
        .arg(ms % 1000, 3, 10, QChar('0'));
}

LyricsContent parseQrc(const QString &xml)
{
    LyricsContent content;
    const qsizetype start = xml.indexOf(QStringLiteral("LyricContent=\""));
    QString lyric = xml;
    if (start >= 0) {
        const qsizetype begin = start + qsizetype(sizeof("LyricContent=\"") - 1);
        const qsizetype end = xml.indexOf(QStringLiteral("\"/>"), begin);
        if (end > begin) lyric = xml.mid(begin, end - begin);
    }
    lyric.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
    lyric.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
    static const QRegularExpression lineRe(QStringLiteral(R"(^\[(\d+),(\d+)\](.*)$)"));
    static const QRegularExpression wordRe(QStringLiteral(R"(((?:(?!\(\d+,\d+\)).)*?)\((\d+),(\d+)\))"));
    QStringList lines, words;
    bool hasWords = false;
    for (const QString &raw : lyric.split(QLatin1Char('\n'))) {
        const auto line = lineRe.match(raw.trimmed());
        if (!line.hasMatch()) continue;
        const qint64 startMs = line.captured(1).toLongLong();
        const qint64 endMs = startMs + line.captured(2).toLongLong();
        QString plain;
        QString timed = stamp(startMs);
        int count = 0;
        auto matches = wordRe.globalMatch(line.captured(3));
        while (matches.hasNext()) {
            const auto word = matches.next();
            const QString text = word.captured(1);
            if (text.isEmpty()) continue;
            plain += text;
            timed += stamp(word.captured(2).toLongLong()) + text;
            ++count;
        }
        if (!count) plain = line.captured(3);
        if (plain.trimmed().isEmpty()) continue;
        lines.append(stamp(startMs) + plain);
        words.append(count ? timed + stamp(endMs) : stamp(startMs) + plain);
        hasWords |= count > 0;
    }
    content.lines = lines.isEmpty() ? lyric : lines.join(QLatin1Char('\n'));
    if (hasWords) content.words = words.join(QLatin1Char('\n'));
    return content;
}
}

QString QqQrc::decrypt(const QString &hex)
{
    const QByteArray encoded = hex.toLatin1();
    if (encoded.isEmpty() || encoded.size() % 16 || encoded.size() > 8 * 1024 * 1024) return {};
    const QByteArray encrypted = QByteArray::fromHex(encoded);
    if (encrypted.size() * 2 != encoded.size()) return {};
    static constexpr Byte key[] = "!@#)(*$%123ZXC!@!@#)(NHL";
    Schedule first, middle, last;
    keySchedule(key + 16, true, first);
    keySchedule(key + 8, false, middle);
    keySchedule(key, true, last);
    QByteArray compressed(encrypted.size(), Qt::Uninitialized);
    for (qsizetype i = 0; i < encrypted.size(); i += 8) {
        Block input{};
        for (int j = 0; j < 8; ++j) input[j] = Byte(encrypted.at(i + j));
        const Block output = cryptBlock(cryptBlock(cryptBlock(input, first), middle), last);
        for (int j = 0; j < 8; ++j) compressed[i + j] = char(output[j]);
    }
    return QString::fromUtf8(inflateQrc(compressed));
}

LyricsContent LyricsFormat::decodeQqCloud(const QString &encryptedLyric,
                                          const QString &encryptedTranslation)
{
    LyricsContent content = parseQrc(QqQrc::decrypt(encryptedLyric));
    if (!encryptedTranslation.isEmpty())
        content.translation = parseQrc(QqQrc::decrypt(encryptedTranslation)).lines;
    return content;
}
