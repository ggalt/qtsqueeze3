/* PhotoFlow - animated image viewer for mobile devices
 *
 * Copyright (C) 2008 Ariya Hidayat (ariya.hidayat@gmail.com)
 * Copyright (C) 2007 Ariya Hidayat (ariya.hidayat@gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA
 *
 */

/* Modified by George Galt 2012 to pull images from Logitech Media Server
*/



#ifndef IMAGE_THUMBNAIL_H
#define IMAGE_THUMBNAIL_H

#include <qimage.h>
#include <qobject.h>
#include <qsize.h>

#include <qthread.h>

#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>

#include <QTimer>
#include <QTime>
#include <QUrl>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QImageReader>
#include <QPixmap>

#include "squeezedefines.h"


class ImageLoader : public QObject
{
    Q_OBJECT
public:
    ImageLoader(QString serveraddr, qint16 httpport, QString cliuname = NULL, QString clipass = NULL);

    ~ImageLoader();

    //  void generate(const QByteArray& cover_id);
    void RequestArtwork(QByteArray coverID);
    QPixmap RetrieveCover(QByteArray cover_id);
    SlimImageItem ImageCache(void) { return imageCache; }

signals:
    void ImageReady(QByteArray cover_id);

public slots:
    void ArtworkReply(QNetworkReply*);

protected:
//    void run();

private:
//    QMutex mutex;
//    QWaitCondition condition;

    QSize size;
    SlimImageItem imageCache;

    QString SlimServerAddr;   // server IP address
    quint16 httpPort;          // port to use for cli, usually 9090, but we allow the user to change this
    QString cliUsername;      // username for cli if needed
    QString cliPassword;      // password for cli if needed **NOTE: DANGER, DANGER this is done in clear text, so don't use a password you care about!!
    QString imageSizeStr;  // default to "200X200"

    QNetworkAccessManager *imageServer;
    NewtorkReply2Cover httpReplyList;
};


#endif // IMAGE_THUMBNAIL_H

