#ifndef LBLPICTUREFLOW_H
#define LBLPICTUREFLOW_H

#include <QPaintEvent>
#include <QPainter>
#include <QList>
#include <QStringList>
#include <QColor>

#include "pictureflow.h"
#include "squeezedefines.h"

class LblPictureFlow : public PictureFlow
{
    Q_OBJECT

public:
  LblPictureFlow(QWidget* parent = 0);
  void addSlide(const QImage& image, Album &album );
  void addSlide(const QPixmap& pixmap, Album &album );
  void setSlide(int index, const QImage& image, Album &album);
  void setSlide(int index, const QPixmap& pixmap, Album &album);

  void setBackgroundColor(const QColor& c);
  void clear();

signals:
  void NextSlide();
  void PrevSlide();


protected:
  //  void keyPressEvent(QKeyEvent* event);
  void mousePressEvent(QMouseEvent* event);
  void paintEvent (QPaintEvent *e);

private:
  QList<Album> albumList;
  QColor titleColor;
};


#endif // LBLPICTUREFLOW_H

