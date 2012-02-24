#ifndef SQUEEZEPICTUREFLOW_H
#define SQUEEZEPICTUREFLOW_H

#include <QPaintEvent>
#include <QPainter>
#include <QList>
#include <QListIterator>
#include <QHash>
#include <QHashIterator>
#include <QStringList>
#include <QColor>
#include <QWidget>

#include "pictureflow.h"
#include "squeezedefines.h"
#include "slimimagecache.h"

class SqueezePictureFlow : public PictureFlow
{
    Q_OBJECT
public:
    explicit SqueezePictureFlow(QWidget* parent);
    ~SqueezePictureFlow();


    void Init(QString lmsServerAddr, qint16 httpport, bool autoselect = true);
    void Init(QString lmsServerAddr, qint16 httpport,
              bool autoselect, QString cliuname, QString clipass);

    void LoadAlbumList(QList<Album> list);
    void LoadAlbumList(QList<TrackData> list);
    QList<Album> *GetAlbumList(void) {return &albumList; }

    void setBackgroundColor(const QColor& c);
    void clear();
    void resetDimensions(QWidget *win);

    bool IsReady(void) { return isReady; }

signals:
    void NextSlide();
    void PrevSlide();
    void SelectSlide(int);
    void CoverFlowReady(void);

protected:
    //  void keyPressEvent(QKeyEvent* event);
    void mousePressEvent(QMouseEvent* event);
//    void mouseReleaseEvent(QMouseEvent *event);
    void paintEvent (QPaintEvent *e);
//    void resizeEvent(QResizeEvent *event);

private:
    void FetchCovers(void);

    QList<Album> albumList;
    QColor titleColor;
    SlimImageCache *worker;
    bool autoSelect;
    bool isReady;
};

#endif // SQUEEZEPICTUREFLOW_H
