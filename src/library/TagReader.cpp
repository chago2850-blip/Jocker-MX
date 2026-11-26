// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "TagReader.h"

#include <QFile>
#include <QFileInfo>
#include <QStringDecoder>
#include <cstdint>

namespace jx {

namespace {

uint32_t be32(const uchar* p) { return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3]; }
uint32_t le32(const uchar* p) { return (uint32_t(p[3]) << 24) | (uint32_t(p[2]) << 16) | (uint32_t(p[1]) << 8) | p[0]; }
uint32_t syncsafe(const uchar* p) { return (uint32_t(p[0] & 0x7f) << 21) | (uint32_t(p[1] & 0x7f) << 14) | (uint32_t(p[2] & 0x7f) << 7) | (p[3] & 0x7f); }

QString latin1(const QByteArray& b) { return QString::fromLatin1(b).trimmed(); }

// Texto de un marco ID3 (primer byte = codificación).
QString id3Text(const QByteArray& data) {
    if (data.isEmpty()) return {};
    const char enc = data[0];
    QByteArray b = data.mid(1);
    QString s;
    switch (enc) {
    case 0: s = QString::fromLatin1(b); break;
    case 3: s = QString::fromUtf8(b); break;
    case 1: {   // UTF-16 con BOM
        QStringDecoder d(QStringDecoder::Utf16);
        s = d.decode(b);
        break;
    }
    case 2: {
        QStringDecoder d(QStringDecoder::Utf16BE);
        s = d.decode(b);
        break;
    }
    default: s = QString::fromLatin1(b);
    }
    // Varios valores separados por NUL: tomar el primero no vacío
    const QStringList parts = s.split(QChar(0), Qt::SkipEmptyParts);
    return parts.isEmpty() ? QString() : parts.first().trimmed();
}

QString genreFromId3(QString g) {
    // "(17)" o "17" → nombre ID3v1 (lista parcial de los más comunes)
    static const char* names[] = {"Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge", "Hip-Hop",
                                  "Jazz", "Metal", "New Age", "Oldies", "Other", "Pop", "R&B", "Rap", "Reggae",
                                  "Rock", "Techno", "Industrial", "Alternative", "Ska", "Death Metal", "Pranks",
                                  "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk", "Fusion",
                                  "Trance", "Classical", "Instrumental", "Acid", "House", "Game", "Sound Clip",
                                  "Gospel", "Noise", "Alternative Rock", "Bass", "Soul", "Punk", "Space",
                                  "Meditative", "Instrumental Pop", "Instrumental Rock", "Ethnic", "Gothic",
                                  "Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk", "Eurodance", "Dream",
                                  "Southern Rock", "Comedy", "Cult", "Gangsta", "Top 40", "Christian Rap", "Pop/Funk",
                                  "Jungle", "Native American", "Cabaret", "New Wave", "Psychedelic", "Rave",
                                  "Showtunes", "Trailer", "Lo-Fi", "Tribal", "Acid Punk", "Acid Jazz", "Polka",
                                  "Retro", "Musical", "Rock & Roll", "Hard Rock"};
    QString t = g;
    if (t.startsWith('(') && t.contains(')')) t = t.mid(1, t.indexOf(')') - 1);
    bool ok = false;
    const int n = t.toInt(&ok);
    if (ok && n >= 0 && n < int(sizeof(names) / sizeof(names[0]))) return QString::fromLatin1(names[n]);
    return g;
}

void applyField(TrackTags& t, const QString& keyUpper, const QString& v) {
    if (v.isEmpty()) return;
    if (keyUpper == "TITLE" && t.title.isEmpty()) t.title = v;
    else if (keyUpper == "ARTIST" && t.artist.isEmpty()) t.artist = v;
    else if (keyUpper == "ALBUM" && t.album.isEmpty()) t.album = v;
    else if (keyUpper == "GENRE" && t.genre.isEmpty()) t.genre = genreFromId3(v);
    else if ((keyUpper == "BPM" || keyUpper == "TEMPO") && t.bpm <= 0) t.bpm = v.toDouble();
    else if ((keyUpper == "KEY" || keyUpper == "INITIALKEY") && t.key.isEmpty()) t.key = v;
    else if ((keyUpper == "DATE" || keyUpper == "YEAR") && t.year.isEmpty()) t.year = v.left(4);
    else if (keyUpper == "COMMENT" && t.comment.isEmpty()) t.comment = v;
}

// Analiza una etiqueta ID3v2 completa (bytes desde "ID3").
void parseId3v2(const QByteArray& tag, TrackTags& t) {
    if (tag.size() < 10 || !tag.startsWith("ID3")) return;
    const auto* h = reinterpret_cast<const uchar*>(tag.constData());
    const int ver = h[3];
    const uint32_t size = syncsafe(h + 6);
    int pos = 10;
    const int end = std::min<int>(tag.size(), 10 + static_cast<int>(size));
    if (h[5] & 0x40) {   // encabezado extendido
        if (end < 14) return;
        const uint32_t ext = ver == 4 ? syncsafe(h + 10) : be32(h + 10) + 4;
        pos += static_cast<int>(ext);
    }
    const int idLen = ver == 2 ? 3 : 4;
    const int hdrLen = ver == 2 ? 6 : 10;
    while (pos + hdrLen <= end) {
        const QByteArray id = tag.mid(pos, idLen);
        if (id.isEmpty() || id[0] == '\0') break;
        const auto* p = reinterpret_cast<const uchar*>(tag.constData()) + pos;
        uint32_t fs;
        if (ver == 2) fs = (uint32_t(p[3]) << 16) | (uint32_t(p[4]) << 8) | p[5];
        else if (ver == 4) fs = syncsafe(p + 4);
        else fs = be32(p + 4);
        pos += hdrLen;
        if (fs == 0 || pos + static_cast<int>(fs) > end) break;
        const QByteArray data = tag.mid(pos, static_cast<int>(fs));
        pos += static_cast<int>(fs);

        static const QHash<QByteArray, QString> map = {
            {"TIT2", "TITLE"}, {"TPE1", "ARTIST"}, {"TALB", "ALBUM"}, {"TCON", "GENRE"}, {"TBPM", "BPM"},
            {"TKEY", "KEY"},   {"TDRC", "DATE"},   {"TYER", "DATE"},  {"TT2", "TITLE"},  {"TP1", "ARTIST"},
            {"TAL", "ALBUM"},  {"TCO", "GENRE"},   {"TBP", "BPM"},    {"TKE", "KEY"},    {"TYE", "DATE"}};
        const auto it = map.find(id);
        if (it != map.end()) applyField(t, it.value(), id3Text(data));
    }
}

void parseId3v1(QFile& f, TrackTags& t) {
    if (f.size() < 128) return;
    f.seek(f.size() - 128);
    const QByteArray b = f.read(128);
    if (!b.startsWith("TAG")) return;
    auto field = [&](int off, int len) { return latin1(b.mid(off, len).split('\0').first()); };
    applyField(t, "TITLE", field(3, 30));
    applyField(t, "ARTIST", field(33, 30));
    applyField(t, "ALBUM", field(63, 30));
    applyField(t, "DATE", field(93, 4));
    const int g = static_cast<uchar>(b[127]);
    if (g < 80) applyField(t, "GENRE", QString::number(g));
}

// Duración de MP3: cabecera Xing/Info si existe; si no, estimación CBR.
void mp3Duration(QFile& f, qint64 audioStart, TrackTags& t) {
    f.seek(audioStart);
    const QByteArray b = f.read(4096);
    const auto* p = reinterpret_cast<const uchar*>(b.constData());
    for (int i = 0; i + 4 < b.size(); ++i) {
        if (p[i] != 0xFF || (p[i + 1] & 0xE0) != 0xE0) continue;
        const int verBits = (p[i + 1] >> 3) & 3;      // 3 = MPEG1, 2 = MPEG2, 0 = MPEG2.5
        const int layer = (p[i + 1] >> 1) & 3;        // 1 = Layer III
        const int brIdx = (p[i + 2] >> 4) & 15;
        const int srIdx = (p[i + 2] >> 2) & 3;
        if (layer != 1 || brIdx == 0 || brIdx == 15 || srIdx == 3 || verBits == 1) continue;
        static const int br1[] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320};
        static const int br2[] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160};
        static const int sr1[] = {44100, 48000, 32000};
        const bool mpeg1 = verBits == 3;
        const int sr = sr1[srIdx] / (mpeg1 ? 1 : (verBits == 2 ? 2 : 4));
        const int kbps = mpeg1 ? br1[brIdx] : br2[brIdx];
        const int spf = mpeg1 ? 1152 : 576;
        const bool mono = ((p[i + 3] >> 6) & 3) == 3;
        const int xingOff = i + 4 + (mpeg1 ? (mono ? 17 : 32) : (mono ? 9 : 17));
        if (xingOff + 12 <= b.size()) {
            const QByteArray tag = b.mid(xingOff, 4);
            if ((tag == "Xing" || tag == "Info") && (p[xingOff + 7] & 1)) {
                const uint32_t frames = be32(p + xingOff + 8);
                t.duration = double(frames) * spf / sr;
                return;
            }
        }
        if (kbps > 0) t.duration = double(f.size() - audioStart) * 8.0 / (kbps * 1000.0);
        return;
    }
}

void readMp3(QFile& f, TrackTags& t) {
    const QByteArray head = f.read(10);
    qint64 audioStart = 0;
    if (head.startsWith("ID3") && head.size() == 10) {
        const uint32_t size = syncsafe(reinterpret_cast<const uchar*>(head.constData()) + 6);
        f.seek(0);
        parseId3v2(f.read(10 + size), t);
        audioStart = 10 + size + ((static_cast<uchar>(head[5]) & 0x10) ? 10 : 0);
    }
    parseId3v1(f, t);
    mp3Duration(f, audioStart, t);
}

void readFlac(QFile& f, TrackTags& t) {
    if (f.read(4) != "fLaC") return;
    for (int guard = 0; guard < 128; ++guard) {
        const QByteArray h = f.read(4);
        if (h.size() < 4) break;
        const auto* p = reinterpret_cast<const uchar*>(h.constData());
        const bool last = p[0] & 0x80;
        const int type = p[0] & 0x7f;
        const uint32_t len = (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
        if (type == 0 && len >= 18) {
            const QByteArray si = f.read(len);
            const auto* s = reinterpret_cast<const uchar*>(si.constData());
            const uint32_t sr = (uint32_t(s[10]) << 12) | (uint32_t(s[11]) << 4) | (s[12] >> 4);
            const uint64_t total = (uint64_t(s[13] & 0x0F) << 32) | be32(s + 14);
            if (sr > 0) t.duration = double(total) / sr;
        } else if (type == 4) {
            const QByteArray vc = f.read(len);
            const auto* v = reinterpret_cast<const uchar*>(vc.constData());
            int pos = 0;
            if (vc.size() < 8) break;
            const uint32_t vendorLen = le32(v);
            pos = 4 + static_cast<int>(vendorLen);
            if (pos + 4 > vc.size()) break;
            const uint32_t count = le32(v + pos);
            pos += 4;
            for (uint32_t c = 0; c < count && pos + 4 <= vc.size(); ++c) {
                const uint32_t l = le32(v + pos);
                pos += 4;
                if (pos + static_cast<int>(l) > vc.size()) break;
                const QString kv = QString::fromUtf8(vc.mid(pos, static_cast<int>(l)));
                pos += static_cast<int>(l);
                const int eq = kv.indexOf('=');
                if (eq > 0) applyField(t, kv.left(eq).toUpper(), kv.mid(eq + 1).trimmed());
            }
        } else {
            f.seek(f.pos() + len);
        }
        if (last) break;
    }
}

void readWav(QFile& f, TrackTags& t) {
    const QByteArray riff = f.read(12);
    if (riff.size() < 12 || !riff.startsWith("RIFF") || riff.mid(8, 4) != "WAVE") return;
    uint32_t byteRate = 0;
    while (!f.atEnd()) {
        const QByteArray ch = f.read(8);
        if (ch.size() < 8) break;
        const QByteArray id = ch.left(4);
        const uint32_t len = le32(reinterpret_cast<const uchar*>(ch.constData()) + 4);
        const qint64 next = f.pos() + len + (len & 1);
        if (id == "fmt " && len >= 16) {
            const QByteArray fmt = f.read(16);
            byteRate = le32(reinterpret_cast<const uchar*>(fmt.constData()) + 8);
        } else if (id == "data") {
            if (byteRate > 0) t.duration = double(len) / byteRate;
        } else if (id == "LIST" && len > 4 && len < 1 << 20) {
            const QByteArray list = f.read(len);
            if (list.startsWith("INFO")) {
                int pos = 4;
                while (pos + 8 <= list.size()) {
                    const QByteArray sid = list.mid(pos, 4);
                    const uint32_t sl = le32(reinterpret_cast<const uchar*>(list.constData()) + pos + 4);
                    const QString val = QString::fromUtf8(list.mid(pos + 8, static_cast<int>(sl)).split('\0').first()).trimmed();
                    static const QHash<QByteArray, QString> m = {{"INAM", "TITLE"}, {"IART", "ARTIST"}, {"IPRD", "ALBUM"},
                                                                 {"IGNR", "GENRE"}, {"ICRD", "DATE"}, {"ICMT", "COMMENT"}};
                    if (m.contains(sid)) applyField(t, m.value(sid), val);
                    pos += 8 + static_cast<int>(sl) + (sl & 1);
                }
            }
        } else if ((id == "id3 " || id == "ID3 ") && len < 1 << 24) {
            parseId3v2(f.read(len), t);
        }
        if (!f.seek(next)) break;
    }
}

// Comentarios Vorbis (mismo formato que FLAC pero dentro de páginas Ogg)
void parseVorbisComments(const QByteArray& vc, TrackTags& t) {
    const auto* v = reinterpret_cast<const uchar*>(vc.constData());
    if (vc.size() < 8) return;
    int pos = 4 + static_cast<int>(le32(v));
    if (pos + 4 > vc.size()) return;
    const uint32_t count = le32(v + pos);
    pos += 4;
    for (uint32_t c = 0; c < count && pos + 4 <= vc.size(); ++c) {
        const uint32_t l = le32(v + pos);
        pos += 4;
        if (l > static_cast<uint32_t>(vc.size() - pos)) break;
        const QString kv = QString::fromUtf8(vc.mid(pos, static_cast<int>(l)));
        pos += static_cast<int>(l);
        const int eq = kv.indexOf('=');
        if (eq > 0) applyField(t, kv.left(eq).toUpper(), kv.mid(eq + 1).trimmed());
    }
}

void readOgg(QFile& f, TrackTags& t) {
    // Reunir la carga útil de las primeras páginas (cabeceras de identificación y comentarios)
    QByteArray payload;
    for (int page = 0; page < 64 && payload.size() < (1 << 20); ++page) {
        const QByteArray h = f.read(27);
        if (h.size() < 27 || !h.startsWith("OggS")) break;
        const int nseg = static_cast<uchar>(h[26]);
        const QByteArray segs = f.read(nseg);
        int len = 0;
        for (char c : segs) len += static_cast<uchar>(c);
        payload += f.read(len);
        if (payload.indexOf("\x05vorbis") >= 0) break;     // ya pasamos la cabecera de comentarios
    }
    uint32_t rate = 0;
    const int idh = payload.indexOf("\x01vorbis");
    if (idh >= 0 && idh + 16 <= payload.size())
        rate = le32(reinterpret_cast<const uchar*>(payload.constData()) + idh + 12);
    const int ch = payload.indexOf("\x03vorbis");
    if (ch >= 0) parseVorbisComments(payload.mid(ch + 7), t);
    // Duración: posición de gránulo de la última página
    if (rate > 0 && f.size() > 27) {
        const qint64 tail = std::min<qint64>(f.size(), 65536);
        f.seek(f.size() - tail);
        const QByteArray end = f.read(tail);
        const int last = end.lastIndexOf("OggS");
        if (last >= 0 && last + 14 <= end.size()) {
            const auto* g = reinterpret_cast<const uchar*>(end.constData()) + last + 6;
            uint64_t gran = 0;
            for (int i = 7; i >= 0; --i) gran = (gran << 8) | g[i];
            if (gran > 0 && gran < (uint64_t(1) << 50)) t.duration = double(gran) / rate;
        }
    }
}

} // namespace

TrackTags readTags(const QString& path) {
    TrackTags t;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return t;
    t.ok = true;
    const QByteArray magic = f.peek(12);
    if (magic.startsWith("fLaC")) readFlac(f, t);
    else if (magic.startsWith("RIFF")) readWav(f, t);
    else if (magic.startsWith("OggS")) readOgg(f, t);
    else readMp3(f, t);
    if (t.bpm > 0 && (t.bpm < 30 || t.bpm > 300)) t.bpm = 0;
    return t;
}

} // namespace jx
