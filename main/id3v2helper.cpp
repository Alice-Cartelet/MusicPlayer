#include "id3v2helper.h"
#include <QFile>
#include <QByteArray>
#include <QDebug>
namespace
{
quint32 readSynchsafe(const uchar *b)
{
    return ((quint32)b[0] << 21) | ((quint32)b[1] << 14) | ((quint32)b[2] << 7) | (quint32)b[3];
}
void writeSynchsafe(uchar *b, quint32 v)
{
    b[3] = v & 0x7F; v >>= 7;
    b[2] = v & 0x7F; v >>= 7;
    b[1] = v & 0x7F; v >>= 7;
    b[0] = v & 0x7F;
}
quint32 readBE32(const uchar *b)
{
    return ((quint32)b[0] << 24) | ((quint32)b[1] << 16) | ((quint32)b[2] << 8) | (quint32)b[3];
}
void writeBE32(uchar *b, quint32 v)
{
    b[0] = (v >> 24) & 0xFF;
    b[1] = (v >> 16) & 0xFF;
    b[2] = (v >> 8) & 0xFF;
    b[3] = v & 0xFF;
}
QByteArray buildApicFrame(const QByteArray &imgData, const QByteArray &mime)
{
    QByteArray payload;
    payload += '\x00';
    payload += mime;
    payload += '\x00';
    payload += '\x03';
    payload += '\x00';
    payload += imgData;
    QByteArray frame;
    frame += "APIC";
    uchar sz[4];
    writeBE32(sz, (quint32)payload.size());
    frame += QByteArray(reinterpret_cast<char*>(sz), 4);
    frame += '\x00';
    frame += '\x00';
    frame += payload;
    return frame;
}
struct Id3Info
{
    bool valid = false;
    int version = 0;
    quint32 tagSize = 0;
    bool hasExtHeader = false;
}
;
Id3Info parseHeader(const QByteArray &header10)
{
    Id3Info info;
    if (header10.size() < 10) return info;
    const uchar *h = reinterpret_cast<const uchar*>(header10.constData());
    if (h[0] != 'I' || h[1] != 'D' || h[2] != '3') return info;
    info.version = h[3];
    if (info.version < 3) return info;
    info.hasExtHeader = (h[5] & 0x40) != 0;
    info.tagSize = readSynchsafe(h + 6);
    info.valid = true;
    return info;
}
}
QByteArray Id3v2Helper::readCover(const QString &mp3Path)
{
    QFile f(mp3Path);
    if (!f.open(QIODevice::ReadOnly)) return
            {
            }
        ;
    QByteArray header = f.read(10);
    Id3Info info = parseHeader(header);
    if (!info.valid) return
            {
            }
        ;
    QByteArray tagBody = f.read(info.tagSize);
    f.close();
    int pos = 0;
    if (info.hasExtHeader && tagBody.size() >= 4)
    {
        const uchar *eb = reinterpret_cast<const uchar*>(tagBody.constData());
        quint32 extSize = (info.version == 4) ? readSynchsafe(eb) : readBE32(eb);
        pos += extSize;
    }
    const uchar *data = reinterpret_cast<const uchar*>(tagBody.constData());
    int len = tagBody.size();
    while (pos + 10 <= len)
    {
        QByteArray frameId(reinterpret_cast<const char*>(data + pos), 4);
        pos += 4;
        quint32 frameSize;
        if (info.version == 4) frameSize = readSynchsafe(data + pos);
        else frameSize = readBE32(data + pos);
        pos += 4;
        pos += 2;
        if (frameSize == 0 || pos + (int)frameSize > len) break;
        if (frameId == "APIC")
        {
            const uchar *p = data + pos;
            int remaining = (int)frameSize;
            if (remaining < 1) break;
            int pi = 1;
            while (pi < remaining && p[pi] != 0) ++pi;
            ++pi;
            if (pi >= remaining) break;
            ++pi;
            uchar enc = p[0];
            if (enc == 0x01 || enc == 0x02)
            {
                while (pi + 1 < remaining && (p[pi] != 0 || p[pi+1] != 0)) pi += 2;
                pi += 2;
            }
            else
            {
                while (pi < remaining && p[pi] != 0) ++pi;
                ++pi;
            }
            if (pi >= remaining) break;
            return QByteArray(reinterpret_cast<const char*>(p + pi), remaining - pi);
        }
        pos += (int)frameSize;
    }
    return
        {
        }
    ;
}
bool Id3v2Helper::writeCover(const QString &mp3Path, const QByteArray &imgData, const QString &mime)
{
    QFile f(mp3Path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray header = f.read(10);
    Id3Info info = parseHeader(header);
    QByteArray audioData;
    if (info.valid)
    {
        f.read(info.tagSize);
        audioData = f.readAll();
    }
    else
    {
        f.seek(0);
        audioData = f.readAll();
    }
    f.close();
    QByteArray apicFrame = buildApicFrame(imgData, mime.toLatin1());
    QByteArray keptFrames;
    if (info.valid)
    {
        QFile f2(mp3Path);
        f2.open(QIODevice::ReadOnly);
        f2.seek(10);
        QByteArray tagBody = f2.read(info.tagSize);
        f2.close();
        int pos = 0;
        if (info.hasExtHeader && tagBody.size() >= 4)
        {
            const uchar *eb = reinterpret_cast<const uchar*>(tagBody.constData());
            quint32 extSize = (info.version == 4) ? readSynchsafe(eb) : readBE32(eb);
            pos += extSize;
        }
        const uchar *data = reinterpret_cast<const uchar*>(tagBody.constData());
        int len = tagBody.size();
        while (pos + 10 <= len)
        {
            QByteArray frameId(reinterpret_cast<const char*>(data + pos), 4);
            quint32 frameSize;
            if (info.version == 4) frameSize = readSynchsafe(data + pos + 4);
            else frameSize = readBE32(data + pos + 4);
            int totalFrame = 10 + (int)frameSize;
            if (pos + totalFrame > len) break;
            if (frameId == "APIC")
            {
                pos += totalFrame;
                continue;
            }
            if (frameId[0] == '\0') break;
            keptFrames += tagBody.mid(pos, totalFrame);
            pos += totalFrame;
        }
    }
    QByteArray newTagBody = keptFrames + apicFrame;
    int padTo = ((newTagBody.size() / 128) + 1) * 128;
    int padding = padTo - newTagBody.size();
    newTagBody += QByteArray(padding, '\0');
    QByteArray newHeader(10, '\0');
    newHeader[0] = 'I'; newHeader[1] = 'D'; newHeader[2] = '3';
    newHeader[3] = 0x03;
    newHeader[4] = 0x00;
    newHeader[5] = 0x00;
    writeSynchsafe(reinterpret_cast<uchar*>(newHeader.data()) + 6, (quint32)newTagBody.size());
    QString tmpPath = mp3Path + ".id3tmp";
    QFile out(tmpPath);
    if (!out.open(QIODevice::WriteOnly)) return false;
    out.write(newHeader);
    out.write(newTagBody);
    out.write(audioData);
    out.close();
    QFile::remove(mp3Path);
    if (!QFile::rename(tmpPath, mp3Path))
    {
        QFile src(tmpPath);
        QFile dst(mp3Path);
        if (src.open(QIODevice::ReadOnly) && dst.open(QIODevice::WriteOnly))
        {
            dst.write(src.readAll());
            src.close(); dst.close();
        }
        QFile::remove(tmpPath);
    }
    return true;
}