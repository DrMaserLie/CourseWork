#include <QApplication>
#include <QStyleFactory>
#include <QLoggingCategory>
#include <QIcon>
#include <iostream>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QLoggingCategory::setFilterRules(
        "qt.qpa.wayland.warning=false\n"
        "qt.qpa.wayland=false"
    );
    
    QApplication app(argc, argv);
    
    app.setStyle(QStyleFactory::create("Fusion"));
    
    QApplication::setApplicationName("Temporium");
    QApplication::setApplicationVersion("4.1.0");
    QApplication::setOrganizationName("NSTU");
    QApplication::setOrganizationDomain("nstu.ru");
    
    QApplication::setDesktopFileName("temporium");
    
    QIcon appIcon(":/icons/app");
    app.setWindowIcon(appIcon);
    
    try {
        Temporium::MainWindow mainWindow;
        mainWindow.setWindowIcon(appIcon);
        mainWindow.show();
        
        return app.exec();
    } catch (const std::exception& e) {
        std::cerr << "Critical error: " << e.what() << std::endl;
        return 1;
    }
}
