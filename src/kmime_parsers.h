/*
    kmime_parsers.h

    KMime, the KDE Internet mail/usenet news message library.
    Copyright (c) 2001 the KMime authors.
    See file AUTHORS for details

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#ifndef __KMIME_PARSERS__
#define __KMIME_PARSERS__

#include<QByteArray>
#include<QList>
#include <QVector>

namespace KMime
{

namespace Parser
{

/** Helper-class: splits a multipart-message into single
    parts as described in RFC 2046
    @internal
*/
class MultiPart
{
public:
    MultiPart(const QByteArray &src, const QByteArray &boundary);
    ~MultiPart() {}

    bool parse();
    QVector<QByteArray> parts() const
    {
        return m_parts;
    }
    QByteArray preamble() const
    {
        return m_preamble;
    }
    QByteArray epilouge() const
    {
        return m_epilouge;
    }

private:
    QByteArray m_src, m_boundary, m_preamble, m_epilouge;
    QVector<QByteArray> m_parts;
};

/** Helper-class: abstract base class of all parsers for
    non-mime binary data (uuencoded, yenc)
    @internal
*/
class NonMimeParser
{
public:
    explicit NonMimeParser(const QByteArray &src);
    virtual ~NonMimeParser();
    virtual bool parse() = 0;
    bool isPartial()
    {
        return (p_artNr > -1 && t_otalNr > -1 && t_otalNr != 1);
    }
    int partialNumber()
    {
        return p_artNr;
    }
    int partialCount()
    {
        return t_otalNr;
    }
    bool hasTextPart()
    {
        return (t_ext.length() > 1);
    }
    QByteArray textPart()
    {
        return t_ext;
    }
    QList<QByteArray> binaryParts()
    {
        return b_ins;
    }
    QList<QByteArray> filenames()
    {
        return f_ilenames;
    }
    QList<QByteArray> mimeTypes()
    {
        return m_imeTypes;
    }

protected:
    static QByteArray guessMimeType(const QByteArray &fileName);

    QByteArray s_rc, t_ext;
    QList<QByteArray> b_ins, f_ilenames, m_imeTypes;
    int p_artNr, t_otalNr;
};

/** Helper-class: tries to extract the data from a possibly
    uuencoded message
    @internal
*/
class UUEncoded : public NonMimeParser
{
public:
    UUEncoded(const QByteArray &src, const QByteArray &subject);

    bool parse() Q_DECL_OVERRIDE;

protected:
    QByteArray s_ubject;
};

/** Helper-class: tries to extract the data from a possibly
    yenc encoded message
    @internal
*/
class YENCEncoded : public NonMimeParser
{
public:
    explicit YENCEncoded(const QByteArray &src);

    bool parse() Q_DECL_OVERRIDE;
    QList<QByteArray> binaryParts()
    {
        return b_ins;
    }

protected:
    QList<QByteArray> b_ins;
    static bool yencMeta(QByteArray &src, const QByteArray &name, int *value);
};

} // namespace Parser

} // namespace KMime

#endif // __KMIME_PARSERS__
