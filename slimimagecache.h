#ifndef SLIMIMAGECACHE_H
#define SLIMIMAGECACHE_H

#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>

#include <QTimer>
#include <QTime>
#include <QUrl>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QList>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QImageReader>
#include <QPixmap>
#include <QFile>
#include <QDir>

#include "squeezedefines.h"


class SlimImageCache : public QObject
{
    Q_OBJECT
public:
    explicit SlimImageCache(QObject *parent = 0);
    ~SlimImageCache();

    void Init(QString serveraddr, qint16 httpport);
    void Init(QString serveraddr, qint16 httpport, QString imageSize, QString cliuname, QString clipass);
//    bool HaveListImages(SqueezePictureFlow *picflow, QList<Album> list);
    QPixmap RetrieveCover(const Album &a);

    void StartRequestingImages(void) { requestingImages = true; }
    void DoneRequestingImages(void) { requestingImages = false; }
    bool RequestingImages(void) { return requestingImages; }


signals:
    void ImagesReady(void);

public slots:
    void ArtworkReply(QNetworkReply *reply);

//protected:
//    void run();

private:
//    QMutex mutex;
//    QWaitCondition condition;

    void RequestArtwork(QByteArray cover_id, QString artist_album);

//    SqueezePictureFlow *picflow;    // reference to the object we are working for

    QString SlimServerAddr;   // server IP address
    quint16 httpPort;          // port to use for cli, usually 9090, but we allow the user to change this
    QString cliUsername;      // username for cli if needed
    QString cliPassword;      // password for cli if needed **NOTE: DANGER, DANGER this is done in clear text, so don't use a password you care about!!
    QString imageSizeStr;  // default to "200X200"

    QString cachePath;
    bool requestingImages;

    QNetworkAccessManager *imageServer;
    QHash< QNetworkReply*,QString > httpReplyList; // associate image request to artist+album name
};

#endif // SLIMIMAGECACHE_H
