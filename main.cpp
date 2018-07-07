#include "behaviorlabel.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    BehaviorLabel w;
    w.show();

    return a.exec();
}
