#ifndef SQUEEZEDISPLAY_H
#define SQUEEZEDISPLAY_H

#include <QLabel>
#include <QFont>
#include <QFontMetrics>
#include <QRect>
#include <QSize>
#include <QPoint>
#include <QRegion>
#include <QTimeLine>
#include <QPainter>
#include <QBrush>
#include <QColor>
#include <QTimer>
#include <QResizeEvent>

#include "slimdevice.h"
#include "squeezedefines.h"

class SqueezeDisplay : public QLabel
{
    Q_OBJECT

    Q_PROPERTY(QColor textcolorGeneral READ getTextColor WRITE setTextColor)
    Q_PROPERTY(QColor displayBackgroundColor READ getDisplayBackgroundColor WRITE setDisplayBackgroundColor)
    Q_PROPERTY(int scrollSpeed READ getScrollSpeed WRITE setScrollSpeed)
    Q_PROPERTY(int scrollInterval READ getScrollInterval WRITE setScrollInterval)

public:
    explicit SqueezeDisplay(QWidget *parent = 0);
    ~SqueezeDisplay(void);

    void Init(QColor txtcolGen, QColor dispBgrdColor);
    void Init(void);

    void SetActiveDevice(SlimDevice *ad) {activeDevice=ad;}

    void setTextColor(QColor c) {m_textcolorGeneral = c;}
    QColor getTextColor() const {return m_textcolorGeneral;}
    void setDisplayBackgroundColor(QColor c) { m_displayBackgroundColor=c; }
    QColor getDisplayBackgroundColor() const {return m_displayBackgroundColor; }
    void setScrollSpeed(int s) {m_scrollSpeed=s;}
    int getScrollSpeed() const {return m_scrollSpeed;}
    void setScrollInterval(int s) {m_scrollInterval=s;}
    int getScrollInterval() const {return m_scrollInterval;}

    bool Slimp3Display( QString txt );

signals:
    void ErrorMsg(QString err);

public slots:
    void PaintSqueezeDisplay(DisplayBuffer *buf);
    void slotUpdateSlimDisplay(void);
    void slotResetSlimDisplay(void);

    void slotUpdateScrollOffset(void);
    void slotUpdateTransition(int frame);
    void slotTransitionFinished(void);

protected:
    void resizeEvent(QResizeEvent *);

private:
    void resetDimensions(void);

    // for display of the slim device interface
    QImage *displayImage;  // use a QImage not a QPixmap so we can use alpha blends
    SlimDevice *activeDevice;   // reference to active device, must be set manually
    DisplayBuffer *d;       // pointer to display buffer received from main program
    bool isTransition;    // are we currently transitioning?
    transitionType transitionDirection;  // -1 = left, -2 = down, +1 = right, +2 = down
    int m_scrollSpeed;
    int m_scrollInterval;

    QFont small;
    QFont medium;
    QFont large;
    int lineWidth;
    QFontMetrics *line1fm;        // font metrics of line 1 (in "Large" text) NOTE: must be pointer because it has to be intialized
    int Line1FontWidth;     // width of the letter "W" (used in scrolling text)
    int scrollStep;         // distance we travel during each scroll step == to 1/40 of the width of the letter "W" (or 1, if less than 1)

    QColor m_textcolorGeneral;
    QColor textcolorLine1;
    QColor m_displayBackgroundColor;
    int Brightness;
    int line1Alpha;   // alpha blending figure for menu fade-in and fade-out

    QRect m_displayRect;          // area in which to display squeeze display text
    QRect line0Bounds;          // Rectangle in which line0 gets displayed
    QRect line1Bounds;          // Rectangle in which line1 gets displayed
    QRegion line0Clipping;      // Clipping region equal to line0Bounds
    QRegion line1Clipping;      // Clipping region equal to line1Bounds
    QRegion fullDisplayClipping;         // Clipping region equal to the full size of the display
//    QRegion noClipping;
//    QPoint pointLine0;          // starting display point for line0 text
//    QPoint pointLine1;          // starting display point for line1 text
//    QPoint pointLine1_2;        // follow-on text for scrolling display

    QRect progRect;         // the rounded and outlined displayed rectangle that shows progress
    QRect progFillRect;     // the rounded and filled portion of the displayed rectangle that shows progress
    QRect volRect;          // the rounded and outlined displayed rectangle for showing the volume
    QRect volFillRect;      // the rounded and filled portion of the displayed rectangle for showing the volume
    qreal radius;           // the radius of the rounded display of the above rectangles

    int xOffsetOld;  // horizontal offset for menu transitions
    int yOffsetOld;  // vertical offset for menu transitions
    int xOffsetNew;
    int yOffsetNew;
    int ScrollOffset;   // used in scrolling
    int scrollTextLen;  // used in scrolling
    QRect boundingRect; // the rectangle occupied by the full Line1 text (which may be larger that the display rect, requiring scrolling)

    scrollStatus scrollState;
    QTimeLine *transitionTimer;
    QTimeLine *vertTransTimer;
    QTimeLine *horzTransTimer;
    QTimeLine *bumpTransTimer;
    DisplayBuffer transBuffer;

//    QPoint pointLine0Right; // right edge for text (i.e., the right side of text)
//    QPoint pointLine1Right; // right edge for text (i.e., the right side of text)
//    QPoint upperMiddle;     // center point for Center0
//    QPoint lowerMiddle;     // center point for Center1
//    QPoint centerPoint; // center of display

    QTimer scrollTimer; // timer for scrolling of text too long to fit in display
    
};

#endif // SQUEEZEDISPLAY_H
