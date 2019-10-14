#ifndef APP_HPP
#define APP_HPP

#include <vector>
#include <memory>
#include <filesystem>
#include <QWidget>
#include <QLabel>
#include <QToolButton>
#include <QLineEdit>
#include "mesh.hpp"
#include "glview.hpp"
namespace fs = std::filesystem;

class App : public QWidget {
	Q_OBJECT
public:
	App(fs::path meshDir = {}, QWidget* parent = NULL);

public slots:
	void browse();
	void readMeshes();

protected:
	// Event handlers
	bool eventFilter(QObject* obj, QEvent* event);
	void keyReleaseEvent(QKeyEvent* e);

private:
	// Some typedefs
	typedef std::vector<std::shared_ptr<Mesh>> vecMesh;
	typedef std::vector<vecMesh> vecVecMesh;

	// Internal state
	fs::path meshDir;
	vecVecMesh meshes;					// Models, grouped by cluster ID
	vecVecMesh::iterator clusterIt;		// Refs a cluster with multiple model versions
	vecMesh::iterator meshIt;			// Refs a single model within a cluster

	// GUI elements
	GLView* glView;					// View cluster objs
	QLineEdit* meshDirLE;			// Directory of meshes to display
	QToolButton* browseBtn;			// Browse for directory
	QLabel* nameLbl;				// Name of the current mesh

	// Methods
	void initGui();		// Initialize GUI widgets
	void updateMesh();	// Set the current mesh and name label
	void meshUp();
	void meshDown();
	void meshRight();
	void meshLeft();
};

#endif
