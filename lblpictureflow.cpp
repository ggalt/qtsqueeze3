#include <QtGlobal>
#include <QDebug>

#include "lblpictureflow.h"

LblPictureFlow::LblPictureFlow(QWidget* parent, QString lmsServerAddr,qint16 httpport, bool autoselect,
                               QString cliuname, QString clipass)
    :PictureFlow( parent )
{
    albumList.clear();
    titleColor = Qt::white;
    worker = new ImageLoader(lmsServerAddr,httpport,cliuname,clipass);
    autoSelect = autoselect;
}

void LblPictureFlow::addSlide(Album &album )
{
//    PictureFlow::addSlide( image );
    albumList.append( album );
}

void LblPictureFlow::setSlide(int index, Album &album)
{
    if(slideCount()<index+1)  // we don't have enough slides to put one where we want it
        for(int i = slideCount(); i < index; i++) {
            QPixmap p(":/img/lib/images/noAlbumImage.png");
            PictureFlow::addSlide(p);
            Album a;
            albumList.append(a);
        }

//    PictureFlow::setSlide(index, image);
    albumList.replace(index, album);
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
    albumList.clear();
}

void LblPictureFlow::mousePressEvent(QMouseEvent* event)
{
    if(event->type()== QEvent::MouseButtonDblClick) {  // selected current center image
        emit SelectSlide(centerIndex());
        return;
    }

    if(autoSelect) { // clicking on the next image means load that item
        if(event->x() > width()/2)
            emit NextSlide();
        else
            emit PrevSlide();
    }
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

    if( centerIndex() < albumList.count() && centerIndex() >= 0 ) {
        QString title = albumList.at( centerIndex() ).title;

        // Draw Title
        p.setFont(QFont(p.font().family(), p.font().pointSize() + 1, QFont::Bold));
        p.drawText(cw - (QFontMetrics(p.font()).width( title ) / 2), wh - 30, title );
    }
    p.end();
}

/* vim: set expandtab tabstop=4 shiftwidth=4: */
