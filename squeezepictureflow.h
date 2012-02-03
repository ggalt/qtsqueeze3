#ifndef SQUEEZEPICTUREFLOW_H
#define SQUEEZEPICTUREFLOW_H

#include <QPaintEvent>
#include <QPainter>
#include <QList>
#include <QHash>
#include <QMultiHash>
#include <QStringList>
#include <QColor>

#include "pictureflow.h"
#include "squeezedefines.h"
#include "slimimagecache.h"

class SqueezePictureFlow : public PictureFlow
{
    Q_OBJECT
public:
    explicit SqueezePictureFlow(QWidget* parent,
                                QString lmsServerAddr, qint16 httpport,
                                bool autoselect = true, QString cliuname = NULL, QString clipass = NULL);

    bool LoadAlbumList(QList<Album> list);
    bool LoadAlbumList(QList<TrackData> list);
    QList<Album> *GetAlbumList(void) {return &albumList; }
    
    void setBackgroundColor(const QColor& c);
    void clear();

    bool IsReady(void) { return isReady; }

signals:
    void NextSlide();
    void PrevSlide();
    void SelectSlide(int);

public slots:
    void ImagesReady(SqueezePictureFlow *pf);


protected:
    //  void keyPressEvent(QKeyEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void paintEvent (QPaintEvent *e);

private:
    QList<Album> albumList;
    QMultiHash<QByteArray,imageIndexCheck> imageIndexer;
    QColor titleColor;
    SlimImageCache *worker;
    bool autoSelect;
    bool isReady;
};

#endif // SQUEEZEPICTUREFLOW_H
