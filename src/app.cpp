#include <QApplication>
#include <QKeyEvent>
#include <QBoxLayout>
#include "app.hpp"
using namespace std;

// Constructor
App::App(QWidget* parent) : QWidget(parent) {
	initGui();
}

// Key release event
void App::keyReleaseEvent(QKeyEvent* e) {
	switch (e->key()) {

	// Quit on ESC
	case Qt::Key_Escape:
		QApplication::quit();
		break;

	// Parent class handles all others
	default:
		QWidget::keyReleaseEvent(e);
		break;
	}
}

// Create the graphical user interface
void App::initGui() {
	setWindowTitle("Cluster viewer");

	// Top-level horizontal layout
	QHBoxLayout* topLayout = new QHBoxLayout;
	topLayout->setContentsMargins(0, 0, 0, 0);
	setLayout(topLayout);

	// Control panel layout
	QVBoxLayout* ctrlLayout = new QVBoxLayout;
	ctrlLayout->setContentsMargins(11, 11, 11, 11);
	topLayout->addLayout(ctrlLayout);

	// Reset view
	resetViewBtn = new QPushButton("Reset view", this);
	ctrlLayout->addWidget(resetViewBtn);

	ctrlLayout->addStretch();

	// 3D view widget
	glView = new GLView(this);
	topLayout->addWidget(glView);

	// Connect signals and slots
	connect(resetViewBtn, &QPushButton::clicked, glView, &GLView::resetView);
	connect(glView, &GLView::glInitialized, [=]() {
		glView->setMesh(shared_ptr<Mesh>(new Mesh(glView, "0003_synth__group_opt.obj")));
	});
}
