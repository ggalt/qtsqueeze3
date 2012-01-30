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

#include "squeezedefines.h"


class SlimImageCache : public QThread
{
    Q_OBJECT
public:
    explicit SlimImageCache(QObject *parent = 0);
    ~SlimImageCache();

    void Init(QString serveraddr, qint16 httpport);
    void Init(QString serveraddr, qint16 httpport, QString imageDim, QString cliuname, QString clipass);
    void RequestArtwork(QByteArray coverID);
    QPixmap RetrieveCover(QByteArray cover_id);
    SlimImageItem ImageCache(void) { return imageCache; }

signals:
    void ImageReady(QByteArray cover_id);

public slots:
    void ArtworkReqply(QNetworkReply *reply);

protected:
    void run();

private:
    QMutex mutex;
    QWaitCondition condition;

    SlimImageItem imageCache;

    QString SlimServerAddr;   // server IP address
    quint16 httpPort;          // port to use for cli, usually 9090, but we allow the user to change this
    QString cliUsername;      // username for cli if needed
    QString cliPassword;      // password for cli if needed **NOTE: DANGER, DANGER this is done in clear text, so don't use a password you care about!!
    QString imageSizeStr;  // default to "200X200"

    QNetworkAccessManager *imageServer;
    QHash< QNetworkReply*,QByteArray > httpReplyList; // associate image request with coverid
};

#endif // SLIMIMAGECACHE_H
