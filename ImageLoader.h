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

#include <qmutex.h>
#include <qwaitcondition.h>

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

#include "squeezedefines.h"


class ImageLoader : public QThread
{
public:
  ImageLoader(QString serveraddr, qint16 httpport, QString cliuname = NULL, QString clipass = NULL);

  ~ImageLoader();

  // returns FALSE if worker is still busy and can't take the task
  bool busy() const;

  void generate(int index, const QByteArray& coverID, QSize size);

  int index() const { return idx; }

  QImage result() const { return img; }

protected:
  void run();

private:
  QMutex mutex;
  QWaitCondition condition;

  bool restart;
  bool working;
  int idx;
  QString fileName;
  QSize size;
  QImage img;

  QString SlimServerAddr;   // server IP address
  quint16 httpPort;          // port to use for cli, usually 9090, but we allow the user to change this
  QString cliUsername;      // username for cli if needed
  QString cliPassword;      // password for cli if needed **NOTE: DANGER, DANGER this is done in clear text, so don't use a password you care about!!

  QNetworkAccessManager *imageServer;
};


#endif // IMAGE_THUMBNAIL_H

