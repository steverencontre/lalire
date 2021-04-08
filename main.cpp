#include "Window.h"

#include <QApplication>

int main (int argc, char *argv[])
{
	QCoreApplication::setOrganizationDomain ("rsn-tech.co.uk");
	QCoreApplication::setOrganizationName ("RSN Technology");
	QCoreApplication::setApplicationName ("Sipper");

	QApplication a {argc, argv};

	Window w;
	w.show();
	return a.exec();
}
