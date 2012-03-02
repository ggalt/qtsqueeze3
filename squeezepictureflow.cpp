#include "squeezepictureflow.h"

#ifdef SQUEEZEPICFLOW_DEBUG
#define DEBUGF(...) qDebug() << this->objectName() << Q_FUNC_INFO << __VA_ARGS__;
#else
#define DEBUGF(...)
#endif

SqueezePictureFlow::SqueezePictureFlow(QWidget* parent, bool autoselect)
    :PictureFlow(parent)
{
    DEBUGF("");
    albumList.clear();
    titleColor = Qt::white;
    autoSelect = autoselect;
    isReady = false;
}

SqueezePictureFlow::~SqueezePictureFlow()
{
}

void SqueezePictureFlow::LoadAlbumList(QList<Album> list)
{
    DEBUGF("");
    albumList.clear();
    albumList = list;
    connect(imageCache,SIGNAL(ImagesReady()),
            this,SLOT(FetchCovers()));
    imageCache->CheckImages(albumList);
}

void SqueezePictureFlow::LoadAlbumList(QList<TrackData> list)
{
    DEBUGF("");
    albumList.clear();
    QListIterator< TrackData > i(list);
    while( i.hasNext() ) {
        TrackData j = i.next();
        Album a;
        a.songtitle = j.title;
        a.album_id = j.album_id;
        a.albumtitle = j.album;
        a.artist = j.artist;
        // a.artist_id =
        a.coverid = j.coverid;
        a.year = j.year;
        a.artist_album = j.artist.trimmed().toUpper()+j.album.trimmed().toUpper();
        albumList.append(a);
    }
    connect(imageCache,SIGNAL(ImagesReady()),
            this,SLOT(FetchCovers()));
    imageCache->CheckImages(albumList);
}

void SqueezePictureFlow::FetchCovers(void)
{
    DEBUGF("");
    disconnect(imageCache,SIGNAL(ImagesReady()),
               this,SLOT(FetchCovers()));

    QListIterator<Album> i(albumList);
    imageCache->StartRequestingImages();    // let image cache know we've started
    PictureFlow::clear();   // make sure we don't have more slides than images
    while(i.hasNext()) {
        QPixmap p;
        p.loadFromData(imageCache->RetrieveCover(i.next()));
        if(p.isNull())
            p.load(":/img/lib/images/noAlbumImage.png");
        addSlide(p);
    }
    imageCache->DoneRequestingImages();     // let image cache know we're done
    emit CoverFlowReady();
}

void SqueezePictureFlow::setBackgroundColor(const QColor& c)
{
    DEBUGF("");
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

void SqueezePictureFlow::clear()
{
    DEBUGF("");
    PictureFlow::clear();
    albumList.clear();
}

void SqueezePictureFlow::mousePressEvent(QMouseEvent* event)
{
    DEBUGF("");
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

//void mouseReleaseEvent(QMouseEvent *event)
//{
//    PictureFlow::mouseReleaseEvent(event);
//}

//void SqueezePictureFlow::resizeEvent(QResizeEvent *event)
//{
//    QSize newSize = event->size();
//    DEBUGF(event->size());
//    if(newSize.width() < 760)
//        newSize.setWidth(760);
//    if(newSize.height() < 270)
//        newSize.setHeight(270);
//    DEBUGF(newSize);
//    resize(newSize);
//}

void SqueezePictureFlow::resetDimensions(QWidget *win)
{
    DEBUGF("");
    int width = win->width();
    int height = win->height();
    if(width<760)
        width = 760;
    if(height<270)
        height=270;
    resize(width,height);
}

void SqueezePictureFlow::paintEvent (QPaintEvent *e)
{
//    DEBUGF("");
    PictureFlow::paintEvent(e);

    if (slideCount() < 1)
        return;

    QPainter p(this);

    // White Pen for Title Info
    int cw = width() / 2;
    int wh = height();

    if( centerIndex() < albumList.count() && centerIndex() >= 0 ) {
        const Album *a = &albumList.at( centerIndex() );
        QString title = QString(a->songtitle) + " - " + QString(a->albumtitle);

        // Draw Title
        QFont fnt;
        fnt.setFamily("Helvetica");
        fnt.setBold(true);
        p.setPen(titleColor);
        p.setFont(fnt);
//        p.setFont(QFont(p.font().family(), p.font().pointSize() + 1, QFont::Bold));
        p.drawText(cw - (QFontMetrics(p.font()).width( title ) / 2), wh - 30, title );
    }
    p.end();
}

/* vim: set expandtab tabstop=4 shiftwidth=4: */
