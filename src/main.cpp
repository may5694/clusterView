#include <string>
#include <QApplication>
#include "app.hpp"
using namespace std;

int main(int argc, char** argv) {
	QApplication app(argc, argv);

	string modelDir;
	if (argc > 1) modelDir = argv[1];

	App a(modelDir);
	a.show();

	return app.exec();
}
