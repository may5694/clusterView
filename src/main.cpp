#include <QApplication>
#include "app.hpp"

int main(int argc, char** argv) {
	QApplication app(argc, argv);

	App a;
	a.show();

	return app.exec();
}