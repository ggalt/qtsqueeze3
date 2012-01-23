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

#include "ImageLoader.h"
#include "slimcli.h"
#include "slimserverinfo.h"


#include <qimage.h>


ImageLoader::ImageLoader(QString serveraddr, qint16 httpport, QString cliuname = NULL, QString clipass = NULL)
    : QThread(),
      restart(false), working(false), idx(-1),
      SlimServerAddr(serveraddr), httpPort(httpport), cliUsername(cliuname), cliPassword(clipass)
{
}

ImageLoader::~ImageLoader()
{
  mutex.lock();
  condition.wakeOne();
  mutex.unlock();
  wait();
}



// load and resize image
static QImage loadAndResize(const QString& fileName, QSize size)
{
/*
    QEventLoop q;
    QTimer tT;

    tT.setSingleShot(true);
    connect(&tT, SIGNAL(timeout()), &q, SLOT(quit()),Qt::DirectConnection);

    m_request.setUrl( m_url );
    VERBOSE( VB_GENERAL, LOC + "Audio Request made to: " + m_url.toString());

    m_reply = (audioReply*)get(m_request);
    connect(m_reply, SIGNAL(readyRead()),
            &q, SLOT(quit()),Qt::DirectConnection);

    tT.start(5000); // 5s timeout
    q.exec();

    if(tT.isActive()){
        // download complete
        tT.stop();
        return m_reply;
    } else {
        VERBOSE( VB_IMPORTANT, LOC + "Audio Stream Request timed out");
        VERBOSE( VB_IMPORTANT, LOC + QString("reply contains %1 bytes").arg(m_reply->bytesAvailable()));
        return NULL; // returning NULL will indicate no stream file retrieved
    }

  */
  QImage image;
  bool result = image.load(fileName);

  if(!result)
    return QImage();

  image = image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

  return image;
}

bool ImageLoader::busy() const
{
  return isRunning() ? working : false;
}  

void ImageLoader::generate(int index, const QString& fileName, QSize size)
{
  mutex.lock();
  this->idx = index;
  this->fileName = fileName;
  this->size = size;
  mutex.unlock();

  if (!isRunning())
    start();
  else
  {
    // already running, wake up whenever ready
    restart = true;
    condition.wakeOne();
   }
 }

void ImageLoader::run()
{
  for(;;)
  {
    // copy necessary data
    mutex.lock();
    this->working = true;
    QString fileName = this->fileName;
    QSize size = this->size;
    mutex.unlock();

    QImage image = loadAndResize(fileName, size);

      // let everyone knows it is ready
    mutex.lock();
    this->working = false;
    this->img = image;
    mutex.unlock();

    // put to sleep
    mutex.lock();
    if (!this->restart)
      condition.wait(&mutex);
    restart = false;
    mutex.unlock();
  }
}

/* vim: set expandtab tabstop=4 shiftwidth=4: */
