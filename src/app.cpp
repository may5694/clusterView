#include <map>
#include <QApplication>
#include <QKeyEvent>
#include <QBoxLayout>
#include "app.hpp"
using namespace std;

// Constructor
App::App(fs::path meshDir, QWidget* parent) : QWidget(parent) {
	initGui();
	meshDirLE->setText(QString::fromStdString(meshDir));
}

// Read meshes from directory
void App::readMeshes() {
	if (!glView->initialized()) return;

	// Get the new directory
	fs::path newMeshDir = meshDirLE->text().toStdString();

	// If it doesn't exist or isn't a directory, do nothing
	if (!fs::exists(newMeshDir) || !fs::is_directory(newMeshDir)) return;
	// If it's not different from the current dir, do nothing
	if (fs::exists(meshDir) && fs::equivalent(meshDir, newMeshDir)) {
		meshDir = newMeshDir;
		return;
	}

	// Set the new directory and clear any existing meshes
	meshDir = newMeshDir;
	meshes.clear();

	// Group meshes according to cluster ID
	map<string, int> clusterMap;

	// Look for any obj files
	fs::directory_iterator di(meshDir), dend;
	for (; di != dend; ++di) {
		// Skip non-files
		if (!fs::is_regular_file(di->path())) continue;
		if (di->path().extension().string() == ".obj") {
			// Get cluster ID
			string modelName = di->path().filename().string();
			string cluster = modelName.substr(0, modelName.find_first_of("_"));

			// If we already have this cluster, add this model to its vector
			if (clusterMap.find(cluster) != clusterMap.end()) {
				meshes[clusterMap.at(cluster)].push_back(
					shared_ptr<Mesh>(new Mesh(glView, di->path())));

			// If we don't have this cluster, add a new cluster vec
			} else {
				clusterMap[cluster] = meshes.size();
				meshes.push_back({ shared_ptr<Mesh>(new Mesh(glView, di->path())) });
			}
		}
	}

	// Initialize iterators
	clusterIt = meshes.begin();
	if (clusterIt != meshes.end())
		meshIt = clusterIt->begin();
	// Update current mesh
	updateMesh();
}

// Keyboard event filter for GLView
bool App::eventFilter(QObject* object, QEvent* event) {
	if (object == glView && event->type() == QEvent::KeyRelease) {
		QKeyEvent* e = static_cast<QKeyEvent*>(event);

		// Switch viewed mesh with arrow keys
		switch (e->key()) {
		case Qt::Key_Up:
			meshUp();
			break;
		case Qt::Key_Down:
			meshDown();
			break;
		case Qt::Key_Right:
			meshRight();
			break;
		case Qt::Key_Left:
			meshLeft();
			break;
		}
	}

	// Allow further processing
	return false;
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

	// Directory of meshes to view
	meshDirLE = new QLineEdit(this);
	meshDirLE->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	ctrlLayout->addWidget(meshDirLE);

	// Current mesh name
	nameLbl = new QLabel(this);
	nameLbl->setMinimumWidth(200);
	ctrlLayout->addWidget(nameLbl);

	// Reset view
	resetViewBtn = new QPushButton("Reset view", this);
	ctrlLayout->addWidget(resetViewBtn);

	ctrlLayout->addStretch();

	// 3D view widget
	glView = new GLView(this);
	glView->installEventFilter(this);
	glView->setFocus();
	topLayout->addWidget(glView);

	// Connect signals and slots
	connect(meshDirLE, &QLineEdit::editingFinished, this, &App::readMeshes);
	connect(resetViewBtn, &QPushButton::clicked, glView, &GLView::resetView);
	connect(glView, &GLView::glInitialized, this, &App::readMeshes);
}

// Set the current mesh and name label
void App::updateMesh() {
	// Clear mesh and name if invalid iterator
	if (clusterIt == meshes.end()) {
		glView->setMesh({});
		nameLbl->setText("");

	// Otherwise set the display mesh and mesh name
	} else {
		glView->setMesh(*meshIt);
		nameLbl->setText(QString::fromStdString((*meshIt)->name));
	}
}

// Scroll through model version
void App::meshUp() {
	if (clusterIt == meshes.end()) return;

	// Detect loops
	if (meshIt == clusterIt->begin())
		meshIt = clusterIt->end();
	// Decrement iterator
	meshIt--;

	// Update mesh viewer
	updateMesh();
}
void App::meshDown() {
	if (clusterIt == meshes.end()) return;

	// Increment iterator
	meshIt++;
	// Detect loops
	if (meshIt == clusterIt->end())
		meshIt = clusterIt->begin();

	// Update mesh viewer
	updateMesh();
}

// Scroll through clusters
void App::meshRight() {
	if (clusterIt == meshes.end()) return;
	if (meshes.size() == 1) return;

	// Increment iterator
	clusterIt++;
	// Detect loops
	if (clusterIt == meshes.end())
		clusterIt = meshes.begin();

	// Reset mesh iterator
	meshIt = clusterIt->begin();

	// Update mesh viewer
	updateMesh();
}
void App::meshLeft() {
	if (clusterIt == meshes.end()) return;
	if (meshes.size() == 1) return;

	// Detect loops
	if (clusterIt == meshes.begin())
		clusterIt = meshes.end();
	// Decrement iterator
	clusterIt--;

	// Reset mesh iterator
	meshIt = clusterIt->begin();

	// Update mesh viewer
	updateMesh();
}
