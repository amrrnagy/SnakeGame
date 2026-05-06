#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    // Initialize the Qt Application
    QApplication app(argc, argv);

    // Create and show your main window
    MainWindow window;
    window.show();

    // Start the application loop
    return app.exec();
}