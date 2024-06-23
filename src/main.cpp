#include "monitor/monitoring.h"
#include <QtWidgets/QApplication>

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	monitoring w;
	w.show();
	return a.exec();
}
