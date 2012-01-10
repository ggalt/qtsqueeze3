#include <QtGlobal>
#include <QDebug>

#include "lblpictureflow.h"

LblPictureFlow::LblPictureFlow(QWidget* parent )
    :PictureFlow( parent )
{
  titles.clear();
  titleColor = Qt::white;
}


void LblPictureFlow::addSlide(const QImage& image, QString &title )
{
  PictureFlow::addSlide( image );
  titles.append( title );
}

void LblPictureFlow::addSlide(const QPixmap& pixmap, QString &title )
{
  PictureFlow::addSlide( pixmap );
  titles.append( title );
}

void LblPictureFlow::setBackgroundColor(const QColor& c)
{
    PictureFlow::setBackgroundColor(c);
    int hue, saturation, value;
    c.getHsv(&hue,&saturation,&value);
//    if(value < 127)
//        titleColor.setHsv(abs(180-hue),saturation,255);
//    else
//        titleColor.setHsv(abs(180-hue),saturation,0);
    if(value < 128)
        value += 128;
    else
        value -= 128;
    if(saturation < 128)
        saturation += 128;
    else
        saturation -= 128;

    titleColor.setHsv(abs(180-hue),saturation,value);
//    titleColor.setHsv(hue,saturation,value);
}

void LblPictureFlow::clear()
{
  PictureFlow::clear();
  titles.clear();
}
/*
void LblPictureFlow::keyPressEvent(QKeyEvent* event)
{
}
*/
void LblPictureFlow::mousePressEvent(QMouseEvent* event)
{
  if(event->x() > width()/2)
    emit NextSlide();
  else
    emit PrevSlide();
  PictureFlow::mousePressEvent(event);
}

void LblPictureFlow::paintEvent (QPaintEvent *e)
{
  PictureFlow::paintEvent(e);

  if (slideCount() < 1)
    return;

  QPainter p(this);

  // White Pen for Title Info
  p.setPen(titleColor);

  int cw = width() / 2;
  int wh = height();

  if( centerIndex() < titles.count() && centerIndex() >= 0 ) {
    QString title = this->titles.at( centerIndex() );

    // Draw Title
    p.setFont(QFont(p.font().family(), p.font().pointSize() + 1, QFont::Bold));
    p.drawText(cw - (QFontMetrics(p.font()).width( title ) / 2), wh - 30, title );
  }
  p.end();
}

/* vim: set expandtab tabstop=4 shiftwidth=4: */
