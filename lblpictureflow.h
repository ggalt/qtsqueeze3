#ifndef LBLPICTUREFLOW_H
#define LBLPICTUREFLOW_H

#include <QPaintEvent>
#include <QPainter>
#include <QList>
#include <QStringList>
#include <QColor>

#include "pictureflow.h"
#include "squeezedefines.h"
#include "ImageLoader.h"

class LblPictureFlow : public PictureFlow
{
    Q_OBJECT

public:
  LblPictureFlow(QWidget* parent, QString lmsServerAddr, qint16 httpport, bool autoselect = true, QString cliuname = NULL, QString clipass = NULL);
  void addSlide(Album &album );
  void setSlide(int index, Album &album);

  void setBackgroundColor(const QColor& c);
  void clear();

signals:
  void NextSlide();
  void PrevSlide();
  void SelectSlide(int);


protected:
  //  void keyPressEvent(QKeyEvent* event);
  void mousePressEvent(QMouseEvent* event);
  void paintEvent (QPaintEvent *e);

private:
  QList<Album> albumList;
  QColor titleColor;
  ImageLoader *worker;
  bool autoSelect;
};


#endif // LBLPICTUREFLOW_H

