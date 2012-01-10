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

#include "ImageLoader.h"


#include <qimage.h>
// load and resize image
static QImage loadAndResize(const QString& fileName, QSize size)
{
  QImage image;
  bool result = image.load(fileName);

  if(!result)
    return QImage();

  image = image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

  return image;
}

ImageLoader::ImageLoader(): QThread(), 
restart(false), working(false), idx(-1)
{
}

ImageLoader::~ImageLoader()
{
  mutex.lock();
  condition.wakeOne();
  mutex.unlock();
  wait();
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
