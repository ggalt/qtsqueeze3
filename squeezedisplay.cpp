#include <QtDebug>

#include "squeezedisplay.h"

#define DEBUGMSG(...) qDebug() << this->objectName() << Q_FUNC_INFO << QTime::currentTime().toString() << __VA_ARGS__;
//#define DEBUGMSG(...)
#define PADDING 20

SqueezeDisplay::SqueezeDisplay(QWidget *parent) :
    QLabel(parent)
{
    setObjectName("SqueezeDisplay");
    vertTransTimer = new QTimeLine( 350, this );
    horzTransTimer = new QTimeLine( 700, this );
    bumpTransTimer = new QTimeLine( 100, this );
    vertTransTimer->setFrameRange( 0, 0 );
    horzTransTimer->setFrameRange( 0, 0 );
    bumpTransTimer->setFrameRange( 0, 0 );
    isTransition = false;
    transitionDirection = transNONE;
    xOffsetOld = xOffsetNew = 0;
    yOffsetOld = yOffsetNew = 0;
    ScrollOffset = 0;
    scrollState = NOSCROLL;
    Brightness = 255;
    line1Alpha = 0;
}

SqueezeDisplay::~SqueezeDisplay(void)
{
    if(line1fm!=NULL)
        delete line1fm;
    if(vertTransTimer!=NULL)
        delete vertTransTimer;
    if(horzTransTimer!=NULL)
        delete horzTransTimer;
    if(bumpTransTimer!=NULL)
        delete bumpTransTimer;
    if(!displayImage->isNull())
        delete displayImage;
}

void SqueezeDisplay::Init(QColor txtcolGen, QColor dispBgrdColor)
{
    m_textcolorGeneral = txtcolGen;
    m_displayBackgroundColor = dispBgrdColor;
    Init();
}

void SqueezeDisplay::Init(void)
{
    resetDimensions();
    connect( &scrollTimer, SIGNAL( timeout() ), this, SLOT(slotUpdateScrollOffset()) );
    connect( vertTransTimer, SIGNAL(frameChanged(int)), this, SLOT(slotUpdateTransition(int)));
    connect( horzTransTimer, SIGNAL(frameChanged(int)), this, SLOT(slotUpdateTransition(int)));
    connect( bumpTransTimer, SIGNAL(frameChanged(int)), this, SLOT(slotUpdateTransition(int)));
    connect( vertTransTimer, SIGNAL(finished()), this, SLOT(slotTransitionFinished()));
    connect( horzTransTimer, SIGNAL(finished()), this, SLOT(slotTransitionFinished()));
    connect( bumpTransTimer, SIGNAL(finished()), this, SLOT(slotTransitionFinished()));
}

void SqueezeDisplay::resizeEvent(QResizeEvent *e)
{
    DEBUGMSG("Resizing to " << e->size());
    QLabel::resizeEvent(e);
    resetDimensions();
}

void SqueezeDisplay::resetDimensions(void)
{
    /*  Display is divided into a small top line (line0) and a large bottom line (line1)
        If Line0 = A, Line1 = 3A, with the gap at the top of Line0 and the bottom of Line1
        being 0.25A and the gap between Line0 and Line1 being .5A.  A = total height of the
        display rectangle divided by 5.  The display rectangle is always a ratio of 1X10, which
        corresponds to the original SqueezeBox 3 display of 32X320.
      */

    // first establish display parameters for entire display area
    // Leave a blank area of 10 pixels all around

    int drawwidth = width()-PADDING;
    int drawheight = height();

    if(drawwidth/drawheight >= 10) {  // too wide
        int newWidth = drawheight*10;
        m_displayRect = QRect(PADDING+((drawwidth-newWidth)/2),PADDING,newWidth,drawheight);
    } else {
        int newHeight = drawwidth/10;
        m_displayRect = QRect(PADDING,PADDING+((drawheight-newHeight)/2),drawwidth,newHeight);
    }

    DEBUGMSG(m_displayRect << drawwidth << drawheight << width() << height());

    fullDisplayClipping = QRegion(m_displayRect);

    if(m_displayRect.height() < 32) {   // probably too small to use, so for a size of 320x32
        DEBUGMSG(QString("original display rectangle is too small, with height of %1, main rect is %2 high").arg(m_displayRect.height()).arg(height()));
        m_displayRect.setHeight(32);
        m_displayRect.setWidth(320);
    }

    // establish size of each display line
    int line0height = m_displayRect.height()/5;
    line0Bounds = QRect(m_displayRect.left(),m_displayRect.top()+line0height/4,m_displayRect.width(),line0height);
    line1Bounds = QRect(m_displayRect.left(),m_displayRect.top()+((line0height*7)/4),m_displayRect.width(),line0height*3);

    // establish font
    small.setFamily( "Helvetica" );
    medium.setFamily( "Helvetica" );
    large.setFamily( "Helvetica" );

    QFontMetrics fm = QFontMetrics(small);
    int i = 4;
    do {
        small.setPixelSize(i);
        fm = QFontMetrics(small);
        i++;
    } while(fm.height() < line0Bounds.height());


    fm = QFontMetrics(medium);
    i = 4;
    do {
        medium.setPixelSize( i );
        fm = QFontMetrics(medium);
        i++;
    } while(fm.height() < line0Bounds.height());

    fm = QFontMetrics(large);
    i = 4;
    do {
        large.setPixelSize( i );
        fm = QFontMetrics(large);
        i++;
    } while(fm.height() < line0Bounds.height());

    // establish information for scrolling
    if(line1fm!=NULL)
        delete line1fm;
    line1fm = new QFontMetrics(large);
    Line1FontWidth = line1fm->width('W');
    scrollStep = (Line1FontWidth < 40 ? 1 : Line1FontWidth / 40);

    vertTransTimer->setFrameRange( 0, line1Bounds.height() );
    horzTransTimer->setFrameRange( 0, m_displayRect.width() );
    bumpTransTimer->setFrameRange( 0, Line1FontWidth );

    // establish volume and time progress bars
    volFillRect = volRect = QRect( m_displayRect.left(), height()/2 - line0Bounds.height()/2, m_displayRect.width(),line0Bounds.height());
    progFillRect = progRect = QRect(0,line0Bounds.top()+ line0Bounds.height()/4,line0Bounds.width(),line0Bounds.height()/2);    // note, "left" and "width" are irrelevant here, only "top" and "height" will remain constant
    radius = volRect.height()/4;

    // establish display image
    if(displayImage)
        delete displayImage;
    displayImage = new QImage(width(),height(),QImage::Format_ARGB32 );
    displayImage->fill((uint)m_displayBackgroundColor.rgb());
}

void SqueezeDisplay::slotResetSlimDisplay(void)
{
    DEBUGMSG("");
//    scrollTimer.stop();
//    ScrollOffset = 0;
//    line1Alpha = 0;
//    boundingRect = ln1FM->boundingRect( Line1Rect, Qt::AlignLeft | Qt::AlignHCenter, activeDevice->getDisplayBuffer().line1 );
//    if( boundingRect.width() > Line1Rect.width() ) {
//        scrollState = PAUSE_SCROLL;
//        scrollTextLen = boundingRect.width() - Line1Rect.width();
//        scrollTimer.setInterval( 5000 );
//        scrollTimer.start();
//    }
//    else {
//        scrollState = NOSCROLL;
//    }
    PaintSqueezeDisplay(&activeDevice->getDisplayBuffer());
}

void SqueezeDisplay::slotUpdateSlimDisplay( void )
{
    DEBUGMSG("");
    //    boundingRect = ln1FM->boundingRect( Line1Rect, Qt::AlignLeft | Qt::AlignHCenter, activeDevice->getDisplayBuffer().line1 );
//    lineWidth = ln1FM->width( activeDevice->getDisplayBuffer().line1 );
//    pointLine1_2 = QPoint( lineWidth + ( ui->lblSlimDisplay->width() - Line1Rect.width()), ( 9 * displayRect.height() ) / 10 );

//    if(  lineWidth > Line1Rect.width() ) {
//        if( scrollState == NOSCROLL ) {
//            StopScroll();
//            scrollTextLen = lineWidth - Line1Rect.width();
//            scrollTimer.setInterval( 5000 );
//            scrollTimer.start();
//        }
//    }
//    else {
//        ScrollOffset = 0;
//        scrollState = NOSCROLL;
//        scrollTimer.stop();
//    }
    PaintSqueezeDisplay(&activeDevice->getDisplayBuffer());
}

void SqueezeDisplay::PaintSqueezeDisplay(DisplayBuffer *buf)
{
    int playedCount = 0;
    int totalCount = 1; // this way we never get a divide by zero error
    QString timeText = "";

    QPainter p( displayImage );
    QBrush b( m_displayBackgroundColor );
    m_textcolorGeneral.setAlpha( Brightness );
    textcolorLine1.setAlpha( Brightness - line1Alpha );

    QBrush c( m_textcolorGeneral );
    QBrush e( c ); // non-filling brush
    e.setStyle( Qt::NoBrush );
    p.setBackground( b );
    p.setBrush( c );
    p.setPen( m_textcolorGeneral );
    p.setFont( large );
    p.eraseRect( displayImage->rect() );

    // draw Line 0  NOTE: Only transitions left or right, but NOT up and down
    if( buf->line0.length() > 0 ) {
        p.setFont( small );
//        if( isTransition ) {
//            p.drawText(line0Bounds.left()+xOffsetOld, line0Bounds.top(), transBuffer.line0);
//            p.drawText(line0Bounds.left()+xOffsetNew, line0Bounds.top(), buf->line0);
//        }
//        else
            p.drawText(line0Bounds.left(), line0Bounds.top(), buf->line0);
    }

    // draw Line 1
    if( buf->line1.length() > 0 ) {
        if( buf->line0.left( 8 ) == "Volume (" ) {   // it's the volume, so emulate a progress bar
            qreal volume = buf->line0.mid( 8, buf->line0.indexOf( ')' ) - 8 ).toInt();
            volFillRect.setWidth( (qreal)volRect.width() * ( volume / (qreal)100 ) );
            p.setBrush( e );  // non-filling brush so we end up with an outline of a rounded rectangle
            p.drawRoundedRect( volRect, radius, radius );
            p.setBrush( c );
            if( volume > 1 ) // if it's too small, we get a funny line at the start of the progress bar
                p.drawRoundedRect( volFillRect, radius, radius );
        }
        else {
            QBrush cLine1( textcolorLine1 );
            p.setBrush( cLine1 );
            p.setPen( textcolorLine1 );
            p.setFont( large );
            p.setClipRegion( line1Clipping );
//            if( isTransition ) {
//                p.drawText( pointLine1.x() + xOffsetOld, pointLine1.y() + yOffsetOld, transBuffer.line1);
//                p.drawText( pointLine1.x() + xOffsetNew, pointLine1.y() + yOffsetNew, buf->line1 );
//            } else {
            p.drawText( line1Bounds.x(), line1Bounds.y(), buf->line1 );
//                p.drawText( pointLine1.x() - ScrollOffset, pointLine1.y(), buf->line1 );
//                if( scrollState != NOSCROLL )
//                    p.drawText(pointLine1_2.x() - ScrollOffset, pointLine1.y(), buf->line1 );
//            }
            p.setClipRegion( fullDisplayClipping );
            p.setBrush( c );
            p.setPen( m_textcolorGeneral );
        }
    }

    // deal with "overlay0" (the right-hand portion of the display) this can be time (e.g., "-3:08") or number of items (e.g., "(2 of 7)")
    if( buf->overlay0.length() > 0 ) {
        if( Slimp3Display( buf->overlay0 ) ) {
            QRegExp rx( "\\W(\\w+)\\W");
            //            QRegExp rx( "\037(\\w+)\037" );
            QStringList el;
            int pos = 0;

            while ((pos = rx.indexIn(buf->overlay0, pos)) != -1) {
                el << rx.cap(1);
                pos += rx.matchedLength();
            }

            rx.indexIn( buf->overlay0 );
            QStringList::iterator it = el.begin();
            while( it != el.end() ) {
                QString s = *it;
                if( s.left( 12 ) == "leftprogress" ) { // first element
                    int inc = s.at( 12 ).digitValue();
                    playedCount += inc;
                    totalCount += 4;
                }
                else if( s.left( 14 ) == "middleprogress" ) { // standard element
                    int inc = s.at( 14 ).digitValue();
                    playedCount += inc;
                    totalCount += 4;
                }
                else if( s.left( 10 ) == "solidblock" ) { // full block
                    playedCount += 4;
                    totalCount += 4;
                }
                else if( s.left( 13 ) == "rightprogress" ) { // end element
                    int inc = s.at( 13 ).digitValue();
                    playedCount += inc;
                    totalCount += 4;
                }
                ++it;
            }
            QChar *data = buf->overlay0.data();
            for( int i = ( buf->overlay0.length() - 8 ); i < buf->overlay0.length(); i++ ) {
                if( *( data + i ) == ' ' ) {
                    timeText = buf->overlay0.mid( i + 1 );
                }
            }
        }
        else if( buf->overlay0.contains( QChar( 8 ) ) ) {
            QChar elapsed = QChar(8);
            QChar remaining = QChar(5);
            QChar *data = buf->overlay0.data();
            for( int i = 0; i < buf->overlay0.length(); i++, data++ ) {
                if( *data == elapsed ) {
                    playedCount++;
                    totalCount++;
                }
                else if( *data == remaining )
                    totalCount++;
                else if( *data == ' ' ) {
                    timeText = buf->overlay0.mid( i + 1 );
                }
            }
        }
        else {
            timeText = buf->overlay0;
        }
        p.setFont( small );
        QFontMetrics fm = p.fontMetrics();
        p.setClipping( false );
//        if( isTransition ) {
//            p.drawText( ( pointLine0Right.x() + xOffsetNew ) - fm.width(timeText), pointLine0Right.y(), timeText );
//        }
//        else {
            p.drawText( line0Bounds.right() - fm.width(timeText), line0Bounds.y(), timeText );
//        }
        if( totalCount > 1 ) {  // make sure we received data on a progress bar, otherwise, don't draw
            progRect.setLeft( ( qreal )( line0Bounds.x() + fm.width( buf->line0.toUpper() ) ) );
            progRect.setRight( ( qreal )( line0Bounds.right() - ( qreal )( 3 * fm.width( timeText ) / 2 ) ) );
            progFillRect.setLeft( progRect.left() );
            progFillRect.setWidth( ( playedCount * progRect.width() ) / totalCount );
            p.setBrush( e );  // non-filling brush so we end up with an outline of a rounded rectangle
            p.drawRoundedRect( progRect, radius, radius );
            p.setBrush( c );
            if( playedCount > 1 ) // if it's too small, we get a funny line at the start of the progress bar
                p.drawRoundedRect( progFillRect, radius, radius );
        }
    }

    // deal with "overlay1" (the right-hand portion of the display)
    /*
    if( buf->overlay1.length() > 0 ) {
        DEBUGMSG( "Don't know what to do with overlay1 yet" );
    }
*/
    // if we've received a "center" display, it means we're powered down, so draw them
//    if( buf->center0.length() > 0 ) {
//        p.setFont( medium );
//        QFontMetrics fm = p.fontMetrics();
//        QPoint start = QPoint( upperMiddle.x() - ( fm.width( buf->center0 )/2 ), upperMiddle.y() );
//        p.drawText( start, buf->center0 );
//    }

//    if( buf->center1.length() > 0 ) {
//        p.setFont( medium );
//        QFontMetrics fm = p.fontMetrics();
//        QPoint start = QPoint( lowerMiddle.x() - ( fm.width( buf->center1 )/2 ), lowerMiddle.y() );
//        p.drawText( start, buf->center1 );
//    }
    setPixmap(QPixmap::fromImage( *displayImage) );

}

void SqueezeDisplay::slotUpdateScrollOffset(void)
{

}

void SqueezeDisplay::slotUpdateTransition(int frame)
{

}

void SqueezeDisplay::slotTransitionFinished(void)
{

}

bool SqueezeDisplay::Slimp3Display( QString txt )
{
    DEBUGMSG(QTime::currentTime());
    QRegExp rx( "\037" );      // the CLI overlay for the Slimp3 display uses 0x1F (037 octal) to delimit the segments of the counter
    if( rx.indexIn( txt ) != -1 )
        return true;
    else
        return false;

}

/*


*/
