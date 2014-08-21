/// @file
/// @author rfree (current maintainer in monero.cc project)
/// @brief main network-throttle (count speed and decide on limit)

#include <QApplication>
#include "mainwindow.h"
#include "plotfile.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
//    w.addFile("../../log/net/in/all-size.data", histogram);
    w.addFile("../../log/net/out/all-size.data", histogram);
    w.addFile("../../log/net/inreq/all-size.data", histogram);


    w.addFile("../../log/net/ping.var",curve);
    w.addFile("../../log/net/ping-p0.var" ,curve);
    w.addFile("../../log/net/ping-p1.var" ,curve);
    w.addFile("../../log/net/ping-p2.var",curve);
    w.addFile("../../log/net/ping-p3.var",curve);
    w.addFile("../../log/net/ping-p4.var",curve);
    w.addFile("../../log/net/ping-p5.var",curve);

/*

    w.addFile("example.txt",histogram);
    w.addFile("example2.txt",histogram);
    w.addFile("example3.txt",histogram);


    w.addFile("example.txt",curve);
    w.addFile("example2.txt",curve);
    w.addFile("example3.txt",curve);*/

    w.show();
    





   return a.exec();
}
