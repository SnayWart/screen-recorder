#include <QApplication>
#include <csignal>
#include "gui_window.h"

static void handle_signal(int) {
    QCoreApplication::quit();
}

int main(int argc, char *argv[]) {
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    QApplication app(argc, argv);
    app.setApplicationName("ScreenRecorder");
    app.setApplicationDisplayName("Экранный рекордер");

    MainWindow window;
    window.show();

    return app.exec();
}
