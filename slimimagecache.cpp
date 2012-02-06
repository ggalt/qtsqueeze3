#include "slimimagecache.h"
#include <QtGlobal>
#include <QDebug>
#include <QApplication>

#ifdef SLIMCLI_DEBUG
#define DEBUGF(...) qDebug() << this->objectName() << Q_FUNC_INFO << __VA_ARGS__;
#else
#define DEBUGF(...)
#endif


SlimImageCache::SlimImageCache(QObject *parent) :
    QObject(parent)
{
    DEBUGF(QTime::currentTime());
    SlimServerAddr = "172.0.0.1";
    httpPort=9090;
    imageSizeStr = "cover_200x200";
    retrievingImages = false;
}

SlimImageCache::~SlimImageCache()
{
    DEBUGF(QTime::currentTime());
    if(imageServer)
        delete imageServer;
}
void SlimImageCache::Init(QString serveraddr, qint16 httpport)
{
    DEBUGF(QTime::currentTime());
    SlimServerAddr = serveraddr;
    httpPort = httpport;
    imageServer = new QNetworkAccessManager();
    imageCache.clear();
    connect(imageServer,SIGNAL(finished(QNetworkReply*)),
            this,SLOT(ArtworkReqply(QNetworkReply*)));
}

void SlimImageCache::Init(QString serveraddr, qint16 httpport, QString imageDim, QString cliuname, QString clipass)
{
    DEBUGF(QTime::currentTime());
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
    DEBUGF(QTime::currentTime());
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
    qDebug() << "slimimagecache returning " << haveAllImages;
    retrievingImages = false;

    return haveAllImages;
}


void SlimImageCache::RequestArtwork(QByteArray coverID, QString artist_album)
{
    DEBUGF(QTime::currentTime());
    QString urlString = QString("http://%1:%2/music/%3/%4")
            .arg(SlimServerAddr)
            .arg(httpPort)
            .arg(QString(coverID))
            .arg(imageSizeStr);
    QNetworkRequest req;
    req.setUrl(QUrl(urlString));
    QNetworkReply *reply = imageServer->get(req);
    qDebug() << "http request: " << urlString;
    httpReplyList.insert(reply,artist_album);
    qDebug() << "requesting artwork with id: " << coverID << " for artist:" << artist_album;
    char str[100];
    sprintf(str, "reply pointer is %p",reply);
    qDebug() << "request id is:" << str;
}

void SlimImageCache::ArtworkReqply(QNetworkReply *reply)
{
    DEBUGF(QTime::currentTime());
    QPixmap p;
    //    QImageReader reader(reply);
    QString artist_album = httpReplyList.value(reply);

    qDebug() << "retrieved image for id: " << artist_album;
    qDebug() << "reply has data of size:" << reply->bytesAvailable();

    QByteArray buff;
    buff = reply->readAll();
    p.loadFromData(buff);

    //    p.fromImageReader(&reader);
    if(p.isNull()) { // oops, no image returned, substitute default image
        p.load(":/img/lib/images/noAlbumImage.png");
        qDebug() << "returned null image for " << artist_album;
    }

    imageCache.insert(artist_album.toUpper(),p);
    httpReplyList.remove(reply);
    char str[100];
    sprintf(str, "reply pointer is %p",reply);
    qDebug() << "request id is:" << str;
    delete reply;

    qDebug() << "httpRequest list size is:" << httpReplyList.size() << " and is empty? " << httpReplyList.isEmpty();
    if(httpReplyList.isEmpty() && !retrievingImages) {
        qDebug() << "emitting imagesready";
        emit ImagesReady(picflow);
    }
}

QPixmap SlimImageCache::RetrieveCover(QString artist_album)
{
    DEBUGF(QTime::currentTime());
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

