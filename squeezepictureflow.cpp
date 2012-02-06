#include "squeezepictureflow.h"

#ifdef SLIMCLI_DEBUG
#define DEBUGF(...) qDebug() << this->objectName() << Q_FUNC_INFO << __VA_ARGS__;
#else
#define DEBUGF(...)
#endif

SqueezePictureFlow::SqueezePictureFlow(QWidget* parent, QString lmsServerAddr,qint16 httpport, bool autoselect,
                                       QString cliuname, QString clipass)
    :PictureFlow(parent)
{
    DEBUGF(QTime::currentTime());
    albumList.clear();
    titleColor = Qt::white;
    autoSelect = autoselect;
    isReady = false;

    worker = new SlimImageCache();
    worker->Init(lmsServerAddr,httpport);
//    worker->start();
}

bool SqueezePictureFlow::LoadAlbumList(QList<Album> list)
{
    DEBUGF(QTime::currentTime());
    albumList = list;
    if( worker->HaveListImages(this,albumList) ) {
        ImagesReady(this);
        return true;
    }
    else {
        connect(worker, SIGNAL(ImagesReady(SqueezePictureFlow*)),
                this, SLOT(ImagesReady(SqueezePictureFlow*)));
        return false;
    }
}

bool SqueezePictureFlow::LoadAlbumList(QList<TrackData> list)
{
    DEBUGF(QTime::currentTime());
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
    if( worker->HaveListImages(this,albumList) ) {
        ImagesReady(this);
        return true;
    }
    else {
        connect(worker, SIGNAL(ImagesReady(SqueezePictureFlow*)),
                this, SLOT(ImagesReady(SqueezePictureFlow*)));
        return false;
    }
}

void SqueezePictureFlow::ImagesReady(SqueezePictureFlow *pf)
{
    DEBUGF(QTime::currentTime());
    // images are ready in worker to load into coverflow
    // first, make sure that we got a signal that belongs to this coverflow
    if(!(pf==this))
        return;

    disconnect(worker,SIGNAL(ImagesReady(SqueezePictureFlow*)),
               this,SLOT(ImagesReady(SqueezePictureFlow*)));

    QListIterator<Album> i(albumList);
    while(i.hasNext()) {
        Album a = i.next();
        PictureFlow::addSlide(worker->RetrieveCover(a.artist_album));
    }
    isReady = true;
}

void SqueezePictureFlow::setBackgroundColor(const QColor& c)
{
    DEBUGF(QTime::currentTime());
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
    DEBUGF(QTime::currentTime());
    PictureFlow::clear();
    albumList.clear();
}

void SqueezePictureFlow::mousePressEvent(QMouseEvent* event)
{
    DEBUGF(QTime::currentTime());
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

void SqueezePictureFlow::paintEvent (QPaintEvent *e)
{
    DEBUGF(QTime::currentTime());
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
