#include "MainWindow.h"

#include <QApplication>
#include <QString>
#include <QSurfaceFormat>

int main(int argc, char* argv[]) {
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Vectorial"));
    QApplication::setOrganizationName(QStringLiteral("Vectorial"));
    QApplication::setStyle(QStringLiteral("Fusion"));

    MainWindow window;
    window.resize(1200, 800);
    window.show();

    return app.exec();
}
