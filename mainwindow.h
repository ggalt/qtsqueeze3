#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt declarations
#include <QtGui/QMainWindow>
#include <QtWebKit/QtWebKit>
#include <QObject>
#include <QtGlobal>
#include <QApplication>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QBrush>
#include <QPalette>
#include <QColor>
#include <QFont>
#include <QFontInfo>
#include <QPoint>
#include <QProcess>
#include <QPixmap>
#include <QHttp>
#include <QKeyEvent>
#include <QTimer>
#include <QNetworkInterface>
#include <QMainWindow>
#include <QLabel>
#include <QUrl>
#include <QSplashScreen>
#include <QSettings>
#include <QColorDialog>
#include <QIcon>

// Local App declarations
#include "squeezedefines.h"
#include "slimcli.h"
#include "slimdevice.h"
#include "squeezepictureflow.h"
#include "slimserverinfo.h"

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    bool Create(void);

public slots:
    void SqueezePlayerError( void );
    void SqueezePlayerOutput( void );

    void slotResetSlimDisplay( void );
    void slotUpdateSlimDisplay( void );
    void slotUpdateScrollOffset( void );
    void slotUpdateTransition( int frame );
    void slotTransitionFinished ( void );

    void slotUpdateCoverFlow( int trackIndex );
    void slotCreateCoverFlow( void );

    void slotDisablePlayer( void );
    void slotEnablePlayer( void );
    void slotSetActivePlayer( void );
    void slotSetActivePlayer( SlimDevice *d );

    void updateDisplayConfig(void);
    void updateConnectionConfig(void);
    void setupConfig(void);
    void setConfigDisplay2Defaults(void);
    void setConfigConnection2Defaults(void);

    void setCoverFlowColor(void);
    void setDisplayBackgroundColor(void);
    void setDisplayTextColor(void);

    // button commands from Default.map file
    void slotRewind( void ) { activeDevice->SendDeviceCommand( QString( "button rew.single\n" ) ); }
    void slotPrev( void ) { activeDevice->SendDeviceCommand( QString( "button rew\n" ) ); }
    void slotStop( void ) { activeDevice->SendDeviceCommand( QString( "button pause.hold\n" ) ); }
    void slotPlay( void ) { activeDevice->SendDeviceCommand( QString( "button play.single\n" ) ); }
    void slotPause( void ) { activeDevice->SendDeviceCommand( QString( "button pause\n" ) ); }
    void slotFForward( void ) { activeDevice->SendDeviceCommand( QString( "button fwd.single\n" ) ); }
    void slotAdd( void ) { activeDevice->SendDeviceCommand( QString( "button add.single\n" ) ); }
    //  void slotPower( void ) { activeDevice->SendDeviceCommand( QString( "button power\n" ) ); close(); }
    void slotPower( void ) { close(); }
    void slotVolUp( void ) { activeDevice->SendDeviceCommand( QString( "button volup\n" ) ); }
    void slotVolDown( void ) { activeDevice->SendDeviceCommand( QString( "button voldown\n" ) ); }
    void slotMute( void ) { activeDevice->SendDeviceCommand( QString( "button muting\n" ) ); }
    void slotShuffle( void ) { activeDevice->SendDeviceCommand( QString( "button shuffle\n" ) ); }
    void slotRepeat( void ) { activeDevice->SendDeviceCommand( QString( "button repeat\n" ) ); }
    void slotBright( void ) { activeDevice->SendDeviceCommand( QString( "button brightness\n" ) ); }
    void slotSize( void ) { activeDevice->SendDeviceCommand( QString( "button size\n" ) ); }
    void slotSearch( void ) { activeDevice->SendDeviceCommand( QString( "button search\n" ) ); }
    void slotSleep( void ) { activeDevice->SendDeviceCommand( QString( "button sleep\n" ) ); }
    void slotLeftArrow( void );
    void slotRightArrow( void );
    void slotUpArrow( void );
    void slotDownArrow( void );
    void slotNowPlaying( void ) { activeDevice->SendDeviceCommand( QString( "button now_playing\n" ) ); }
    void slot0PAD( void ) { activeDevice->SendDeviceCommand( QString( "button 0\n" ) ); }
    void slot1PAD( void ) { activeDevice->SendDeviceCommand( QString( "button 1\n" ) ); }
    void slot2PAD( void ) { activeDevice->SendDeviceCommand( QString( "button 2\n" ) ); }
    void slot3PAD( void ) { activeDevice->SendDeviceCommand( QString( "button 3\n" ) ); }
    void slot4PAD( void ) { activeDevice->SendDeviceCommand( QString( "button 4\n" ) ); }
    void slot5PAD( void ) { activeDevice->SendDeviceCommand( QString( "button 5\n" ) ); }
    void slot6PAD( void ) { activeDevice->SendDeviceCommand( QString( "button 6\n" ) ); }
    void slot7PAD( void ) { activeDevice->SendDeviceCommand( QString( "button 7\n" ) ); }
    void slot8PAD( void ) { activeDevice->SendDeviceCommand( QString( "button 8\n" ) ); }
    void slot9PAD( void ) { activeDevice->SendDeviceCommand( QString( "button 9\n" ) ); }

private:
    void getplayerMACAddress( void );
    void SetUpDisplay( void );
    bool Slimp3Display( QString txt );
    void PaintTextDisplay( void );
    void StopScroll( void );

    void loadDisplayConfig(void);
    void loadConnectionConfig(void);


    Ui::MainWindow *ui;
    QProcess *squeezePlayer;
    SlimCLI *slimCLI;
    SlimDevice *activeDevice;
    SlimServerInfo *serverInfo;

    QByteArray MacAddress;      // MAC address of this machine (which will become the MAC address for our player)
    QString SlimServerAddr;   // server IP address
    QString SlimServerAudioPort;  // address for audio connection, default 3483
    QString SlimServerCLIPort;    // address for CLI interfact, default 9090
    QString SlimServerHttpPort;       // address for http connection, default 9000
    QString PortAudioDevice;    // device to use for PortAudio -- leave blank for default device

    // for display of the slim device interface
    QImage *displayImage;  // use a QImage not a QPixmap so we can use alpha blends
    bool isTransition;    // are we currently transitioning?
    transitionType transitionDirection;  // -1 = left, -2 = down, +1 = right, +2 = down
    int scrollSpeed;
    int scrollInterval;

    QFont small;
    QFont medium;
    QFont large;
    QColor textcolorGeneral;
    QColor textcolorLine1;
    QColor displayBackgroundColor;
    QColor temptextcolorGeneral;
    QColor tempdisplayBackgroundColor;
    int Brightness;
    int line1Alpha;   // alpha blending figure for menu fade-in and fade-out

    QRect displayRect;
    QRectF timeRect;
    QRectF timeFillRect;
    QRectF volRect;
    QRectF volFillRect;
    QRect line1Bounds;
    qreal radius;
    int xOffsetOld;  // horizontal offset for menu transitions
    int yOffsetOld;  // vertical offset for menu transitions
    int xOffsetNew;
    int yOffsetNew;
    QRegion line1Clipping;
    QRegion noClipping;
    QPoint pointLine0;  // starting display point for line0 text
    QPoint pointLine1;  // starting display point for line1 text
    QPoint pointLine1_2;  // follow-on text for scrolling display
    QRect Line0Rect;
    QRect Line1Rect;  // rectangle for displaying Line1 text (used in scrolling text)
    QRect boundingRect; // the rectangle occupied by the full Line1 text (which may be larger that the display rect, requiring scrolling)
    int lineWidth;
    QFontMetrics *ln1FM;   // font metrics of line 1 (in "Large" text)
    int Line1FontWidth;   // width of the letter "W" (used in scrolling text)
    int ScrollOffset;   // used in scrolling
    int scrollTextLen;  // used in scrolling
    scrollStatus scrollState;
    QTimeLine *transitionTimer;
    QTimeLine *vertTransTimer;
    QTimeLine *horzTransTimer;
    QTimeLine *bumpTransTimer;
    DisplayBuffer transBuffer;

    QPoint pointLine0Right; // right edge for text (i.e., the right side of text)
    QPoint pointLine1Right; // right edge for text (i.e., the right side of text)
    QPoint upperMiddle;     // center point for Center0
    QPoint lowerMiddle;     // center point for Center1
    QPoint centerPoint; // center of display

    QTimer scrollTimer; // timer for scrolling of text too long to fit in display

    QRect flowRect;
    SqueezePictureFlow *CoverFlow;
    QColor coverflowBackground;
    QColor tempcoverflowBackground;

    QSplashScreen *waitWindow;
    bool isStartUp;

    QSettings *mySettings;
};


#endif // MAINWINDOW_H

