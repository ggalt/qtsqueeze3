#include <QtGui/QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    if( !w.Create() )
      return 0;
    w.show();
    return a.exec();
}
/* vim: set expandtab tabstop=4 shiftwidth=4: */
