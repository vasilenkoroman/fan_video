#include "main_window.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QtFunEmulatorApplication w;
    w.show();
    return a.exec();
}
