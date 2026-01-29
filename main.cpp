#include "main.h"
#include "utilities.h"
#include "window/main_window.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QLockFile>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setApplicationName(AppConfig::kName);
    QApplication::setOrganizationName(AppConfig::kOrganization);
    QApplication::setOrganizationDomain(AppConfig::kDomain);
    QApplication::setQuitOnLastWindowClosed(false);

    const auto lockPath = AppPaths::appDataPath("linksdash.lock");
    QDir().mkpath(QFileInfo(lockPath).dir().path());
    QLockFile lockFile(lockPath);
    lockFile.setStaleLockTime(0);
    if (!lockFile.tryLock()) {
        const auto message = lockFile.error() == QLockFile::LockFailedError
            ? "LinksDash is already running."
            : "Unable to start LinksDash due to a lock file error.";
        QMessageBox::warning(nullptr, "LinksDash", message);
        return 0;
    }

    const QIcon appIcon(":/icons/linksdash_icon.svg");
    if (!appIcon.isNull()) {
        QApplication::setWindowIcon(appIcon);
    }
    MainWindow w;
    return a.exec();
}
