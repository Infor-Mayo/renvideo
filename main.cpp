#include "remvideo.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    RemVideo w;
    w.show();
    return a.exec();
}
