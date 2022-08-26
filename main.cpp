#include "valveworkbench.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ValveWorkbench w;
    w.show();
    return a.exec();
}
