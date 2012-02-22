#include "slimimagecache.h"
#include <QtGlobal>
#include <QDebug>
#include <QApplication>

#ifdef SLIMIMAGECACHE_DEBUG
#define DEBUGF(...) qDebug() << this->objectName() << Q_FUNC_INFO << __VA_ARGS__;
#else
#define DEBUGF(...)
#endif


SlimImageCache::SlimImageCache(QObject *parent) :
    QObject(parent)
{
    DEBUGF("");
    SlimServerAddr = "172.0.0.1";
    httpPort=9090;
    imageSizeStr = "cover_200x200";
    retrievingImages = false;
}

SlimImageCache::~SlimImageCache()
{
    DEBUGF("");
    if(imageServer)
        delete imageServer;
}
void SlimImageCache::Init(QString serveraddr, qint16 httpport)
{
    DEBUGF("");
    SlimServerAddr = serveraddr;
    httpPort = httpport;
    imageServer = new QNetworkAccessManager();
    imageCache.clear();
    connect(imageServer,SIGNAL(finished(QNetworkReply*)),
            this,SLOT(ArtworkReqply(QNetworkReply*)));
}

void SlimImageCache::Init(QString serveraddr, qint16 httpport, QString imageDim, QString cliuname, QString clipass)
{
    DEBUGF("");
    SlimServerAddr = serveraddr;
    httpPort = httpport;
    imageSizeStr = imageDim;
    cliUsername = cliuname;
    cliPassword = clipass;
    imageServer = new QNetworkAccessManager();
    imageCache.clear();
    connect(imageServer,SIGNAL(finished(QNetworkReply*)),
            this,SLOT(ArtworkReqply(QNetworkReply*)));
}

bool SlimImageCache::HaveListImages(SqueezePictureFlow *pf, QList<Album> list)
{
    // test to see if we have all of the images for a list of albums
    // if not, get the ones we don't have

    picflow = pf;   // remember who we are working for

    retrievingImages = true;
    QListIterator<Album> i(list);
    bool haveAllImages = true;  // assume we have them all

    while(i.hasNext()) {
        Album a = i.next();
        if(!imageCache.contains(a.artist.trimmed().toUpper()+a.albumtitle.trimmed().toUpper())) {
            if(!a.coverid.isEmpty()) {
                while(httpReplyList.size() > 3 )
                    qApp->processEvents(QEventLoop::AllEvents,4000);
                RequestArtwork(a.coverid,a.artist.trimmed().toUpper()+a.albumtitle.trimmed().toUpper());
            }
            haveAllImages = false;
        }
    }
    DEBUGF( "slimimagecache returning " << haveAllImages);
    retrievingImages = false;

    return haveAllImages;
}


void SlimImageCache::RequestArtwork(QByteArray coverID, QString artist_album)
{
    DEBUGF("");
    QString urlString = QString("http://%1:%2/music/%3/%4")
            .arg(SlimServerAddr)
            .arg(httpPort)
            .arg(QString(coverID))
            .arg(imageSizeStr);
    QNetworkRequest req;
    req.setUrl(QUrl(urlString));
    QNetworkReply *reply = imageServer->get(req);
    httpReplyList.insert(reply,artist_album);
    char str[100];
    sprintf(str, "reply pointer is %p",reply);
}

void SlimImageCache::ArtworkReqply(QNetworkReply *reply)
{
    DEBUGF("");
    QPixmap p;
    //    QImageReader reader(reply);
    QString artist_album = httpReplyList.value(reply);

    QByteArray buff;
    buff = reply->readAll();
    p.loadFromData(buff);

    //    p.fromImageReader(&reader);
    if(p.isNull()) { // oops, no image returned, substitute default image
        p.load(":/img/lib/images/noAlbumImage.png");
        DEBUGF("returned null image for " << artist_album);
    }

    imageCache.insert(artist_album.toUpper(),p);
    httpReplyList.remove(reply);
    char str[100];
    sprintf(str, "reply pointer is %p",reply);
    delete reply;

    if(httpReplyList.isEmpty() && !retrievingImages) {
        DEBUGF("emitting imagesready");
        emit ImagesReady(picflow);
    }
}

QPixmap SlimImageCache::RetrieveCover(QString artist_album)
{
    DEBUGF("");
    if(imageCache.contains(artist_album.toUpper()))
        return imageCache.value(artist_album.toUpper());
    else {
        QPixmap pic;
        pic.load(":/img/lib/images/noAlbumImage.png");
        return pic;
    }
}

//void SlimImageCache::run()
//{
//    connect(imageServer,SIGNAL(finished(QNetworkReply*)),
//            this,SLOT(ArtworkReqply(QNetworkReply*)));
//    exec();
//}

