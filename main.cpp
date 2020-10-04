#include "mainwindow.h"
#include <QApplication>
#include "trackswidget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    TracksWidget dialog;
    dialog.show();

    return a.exec();
}
