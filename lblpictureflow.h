#ifndef LBLPICTUREFLOW_H
#define LBLPICTUREFLOW_H

#include <QPaintEvent>
#include <QPainter>
#include <QStringList>
#include <QColor>

#include "pictureflow.h"

class LblPictureFlow : public PictureFlow
{
    Q_OBJECT

public:
  LblPictureFlow(QWidget* parent = 0);
  void addSlide(const QImage& image, QString &title );
  void addSlide(const QPixmap& pixmap, QString &title);
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
  QStringList titles;
  QColor titleColor;
};


#endif // LBLPICTUREFLOW_H

