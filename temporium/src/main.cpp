#include <QApplication>
#include <QStyleFactory>
#include <QLoggingCategory>
#include <QIcon>
#include <iostream>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    // Подавление некритичных предупреждений Wayland
    QLoggingCategory::setFilterRules(
        "qt.qpa.wayland.warning=false\n"
        "qt.qpa.wayland=false"
    );
    
    QApplication app(argc, argv);
    
    // Устанавливаем стиль Fusion для консистентного вида
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Устанавливаем иконку приложения
    QIcon appIcon(":/icons/app");
    app.setWindowIcon(appIcon);
    
    QApplication::setApplicationName("Temporium");
    QApplication::setApplicationVersion("2.0");
    QApplication::setOrganizationName("NSTU");
    QApplication::setOrganizationDomain("nstu.ru");
    
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
