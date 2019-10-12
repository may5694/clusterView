#ifndef APP_HPP
#define APP_HPP

#include <QWidget>
#include <QPushButton>
#include "glview.hpp"

class App : public QWidget {
	Q_OBJECT
public:
	App(QWidget* parent = NULL);

public slots:

protected:
	// Event handlers
	void keyReleaseEvent(QKeyEvent* e);

private:
	// GUI elements
	GLView* glView;					// View cluster objs
	QPushButton* resetViewBtn;		// Reset the view

	// Methods
	void initGui();		// Initialize GUI widgets
};

#endif
