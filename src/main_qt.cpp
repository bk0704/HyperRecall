#include "qt/main_window.h"
#include "qt/qt_platform.h"

#include <QApplication>
#include <QDir>
#include <cstdio>

extern "C" {
#include "app.h"
#include "platform.h"
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application metadata
    QApplication::setApplicationName("HyperRecall");
    QApplication::setApplicationVersion("0.1.0");
    QApplication::setOrganizationName("HyperRecall");
    
    // Ensure working directory is set to the binary location
    // so that assets/ can be found relative to the executable
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    
    // Initialize the application core
    AppContext *appContext = app_create();
    if (appContext == nullptr) {
        fprintf(stderr, "Failed to initialise application context.\n");
        return 1;
    }
    
    // Create and show the main window
    MainWindow mainWindow;
    mainWindow.setAppContext(appContext);
    
    // Replace the platform handle with a Qt-specific one
    // The platform_create in qt_platform.cpp creates a placeholder
    // Here we create the actual Qt handle and replace it
    if (appContext->platform) {
        auto *qtPlatform = new QtPlatformHandle(&mainWindow, nullptr);
        // Store the Qt platform handle in place of the placeholder
        auto *oldHandle = appContext->platform;
        appContext->platform = reinterpret_cast<struct PlatformHandle *>(qtPlatform);
        free(oldHandle);
    }
    
    mainWindow.show();
    mainWindow.startEventLoop();
    
    // Run Qt event loop
    int result = app.exec();
    
    // Clean up
    app_destroy(appContext);
    
    return result;
}
