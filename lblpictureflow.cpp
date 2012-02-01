#include <QtGlobal>
#include <QDebug>
#include <QSize>

#include "lblpictureflow.h"

LblPictureFlow::LblPictureFlow(QWidget* parent, QString lmsServerAddr,qint16 httpport, bool autoselect,
                               QString cliuname, QString clipass)
    :PictureFlow( parent )
{
    albumList.clear();
    titleColor = Qt::white;
    autoSelect = autoselect;

    worker = new SlimImageCache();
    worker->Init(lmsServerAddr,httpport);

    connect(worker, SIGNAL(ImageReady(QByteArray)),
            this, SLOT(ImageReady(QByteArray)));
    worker->start();
}

void LblPictureFlow::addSlide(Album &album )
{
    QPixmap pic;
    imageIndexCheck c;
    c.index=slideCount();

    if(!album.coverid.isEmpty() && worker->ImageCache().contains(album.coverid)) {
        pic = worker->RetrieveCover(album.coverid);
        c.loaded = true;
    }
    else {
        pic.load(":/img/lib/images/noAlbumImage.png","PNG");
        c.loaded=false;
        worker->RequestArtwork(album.coverid);
    }

    PictureFlow::addSlide(pic);
    albumList.append( album );
    imageIndexer.insert(album.coverid,c);
//    setSlide(slideCount(),album);
}

void LblPictureFlow::setSlide(int index, Album &album)
{
    qDebug() << "setting slide " << index << " with cover id: " << album.coverid << " for album " << album.albumtitle;
    if(slideCount()<=index)  // we don't have enough slides to put one where we want it
        for(int i = slideCount()-1; i < index; i++) {
            QPixmap p(":/img/lib/images/noAlbumImage.png","PNG");
            qDebug() << "adding slide";
            PictureFlow::addSlide(p);
            qDebug() << "slide added";
            Album a;
            albumList.append(a);
        }

    QPixmap pic;
    imageIndexCheck c;
    c.index=index;

    if(!album.coverid.isEmpty() && worker->ImageCache().contains(album.coverid)) {
        pic = worker->RetrieveCover(album.coverid);
        c.loaded = true;
    }
    else {
        pic.load(":/img/lib/images/noAlbumImage.png","PNG");
        c.loaded=false;
        if(!imageIndexer.contains(album.coverid))   // skip requesting image if we've already requested it
            worker->RequestArtwork(album.coverid);
    }

//    if(pic.isNull())    // check
//        pic.load(":/img/lib/images/noAlbumImage.png","PNG");

    qDebug() << "index is:" << index << " and slide count is:" << slideCount();
    PictureFlow::setSlide(index, pic);
    albumList.replace(index, album);
    imageIndexer.insert(album.coverid,c);
}

void LblPictureFlow::ImageReady(QByteArray coverID)
{
    qDebug() << "Image ready for " << coverID;
    int idx = imageIndexer.value(coverID).index;
    qDebug() << "Image going to index:" << idx << "slide count:" << slideCount();
    imageIndexCheck c;
    c.index=idx;
    c.loaded=true;
    qDebug() << "inserting in image indexer";
    imageIndexer.insert(coverID,c);
    qDebug() << "adding to pictureflow";
    PictureFlow::setSlide(idx,worker->RetrieveCover(coverID));
    qDebug() << "done adding to picture flow picture";
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
        const Album *a = &albumList.at( centerIndex() );
        QString title = QString(a->songtitle) + " - " + QString(a->albumtitle);

        // Draw Title
        p.setFont(QFont(p.font().family(), p.font().pointSize() + 1, QFont::Bold));
        p.drawText(cw - (QFontMetrics(p.font()).width( title ) / 2), wh - 30, title );
    }
    p.end();
}

/* vim: set expandtab tabstop=4 shiftwidth=4: */
