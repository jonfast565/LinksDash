#include "utilities.h"

#include <QDir>
#include <QStandardPaths>

namespace AppPaths {
    QString appDataPath(const QString &fileName) {
        const auto baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        const auto dir = baseDir.isEmpty() ? QDir::home().filePath(".linksdash") : baseDir;
        const auto finalDir = QDir(dir).filePath(fileName);
        return finalDir;
    }
} // namespace AppPaths
