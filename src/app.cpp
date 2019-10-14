#include <iostream>
#include <sstream>
#include <map>
#include <QApplication>
#include <QKeyEvent>
#include <QBoxLayout>
#include <QStyle>
#include <QFileDialog>
#include <QProgressDialog>
#include "app.hpp"
using namespace std;

// Constructor
App::App(fs::path meshDir, QWidget* parent) : QWidget(parent) {
	initGui();
	meshDirLE->setText(QString::fromStdString(meshDir));
}

// Browse for a directory
void App::browse() {
	// Select an existing directory
	QString dirname = QFileDialog::getExistingDirectory(this,
		"Select directory", "",
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	// Do nothing if user cancelled
	if (dirname.isNull()) return;

	// Set the lineEdit text
	meshDirLE->setText(dirname);
	meshDirLE->editingFinished();
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

	cout << "Reading meshes..." << endl;

	// Gather any .obj files
	vector<fs::path> objPaths;
	fs::directory_iterator di(meshDir), dend;
	for (; di != dend; ++di) {
		if (!fs::is_regular_file(di->path())) continue;
		if (di->path().extension().string() == ".obj")
			objPaths.push_back(di->path());
	}

	// Show progress pop-up
	QProgressDialog progress("Reading meshes...", "", 0, objPaths.size(), this);
	progress.setWindowModality(Qt::WindowModal);
	progress.setCancelButton(NULL);

	// Read each mesh
	for (int i = 0; i < objPaths.size(); i++) {
		progress.setValue(i);
		fs::path p = objPaths[i];

		// Get cluster ID
		string modelName = p.filename().string();
		string cluster = modelName.substr(0, modelName.find_first_of("_"));

		// If we already have this cluster, add this model to its vector
		if (clusterMap.find(cluster) != clusterMap.end()) {
			meshes[clusterMap.at(cluster)].push_back(
				shared_ptr<Mesh>(new Mesh(glView, p)));

		// If we don't have this cluster, add a new cluster vec
		} else {
			clusterMap[cluster] = meshes.size();
			meshes.push_back({ shared_ptr<Mesh>(new Mesh(glView, p)) });
		}

	}

	// Close the progress dialog
	progress.setValue(objPaths.size());

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
	QLabel* meshDirLbl = new QLabel("Cluster directory:", this);
	ctrlLayout->addWidget(meshDirLbl);
	QHBoxLayout* dirLayout = new QHBoxLayout;
	ctrlLayout->addLayout(dirLayout);
	meshDirLE = new QLineEdit(this);
	meshDirLE->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	dirLayout->addWidget(meshDirLE);
	// Browse button
	browseBtn = new QToolButton(this);
	browseBtn->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
	dirLayout->addWidget(browseBtn);

	// Current mesh name
	nameLbl = new QLabel(this);
	nameLbl->setMinimumWidth(200);
	ctrlLayout->addWidget(nameLbl);

	ctrlLayout->addSpacing(40);

	// Instructions
	stringstream ss;
	ss << "↑, ↓: Switch models (projtex, seg, synth)" << endl;
	ss << "←, →: Switch clusters" << endl;
	ss << "Left click + drag:  Rotate" << endl;
	ss << "Right click + drag: Zoom" << endl;
	QLabel* instrLbl = new QLabel(QString::fromStdString(ss.str()), this);
	ctrlLayout->addWidget(instrLbl);


	ctrlLayout->addStretch();

	// 3D view widget
	glView = new GLView(this);
	glView->installEventFilter(this);
	glView->setFocus();
	topLayout->addWidget(glView);

	// Connect signals and slots
	connect(meshDirLE, &QLineEdit::editingFinished, this, &App::readMeshes);
	connect(meshDirLE, &QLineEdit::editingFinished, [=](){ glView->setFocus(); });
	connect(browseBtn, &QToolButton::clicked, this, &App::browse);
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
