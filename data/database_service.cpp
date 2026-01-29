#include "database_service.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include "../utilities.h"

namespace {
const char *kInitSql = R"SQL(
CREATE TABLE IF NOT EXISTS links (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL,
    category TEXT NOT NULL,
    url TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_links_category ON links(category);
)SQL";

QString formatError(const QString &context, const QSqlError &error)
{
    if (error.text().isEmpty()) {
        return context;
    }
    return context + ": " + error.text();
}
}

bool DatabaseManager::initialize(QString *errorMessage)
{
    if (QSqlDatabase::contains(kConnectionName)) {
        auto existing = QSqlDatabase::database(kConnectionName);
        if (existing.isOpen()) {
            return true;
        }

        if (existing.databaseName().isEmpty()) {
            existing.setDatabaseName(databaseFilePath());
        }

        if (!existing.open()) {
            if (errorMessage) {
                *errorMessage = formatError("Failed to open database", existing.lastError());
            }
            return false;
        }

        return ensureSchema(existing, errorMessage);
    }

    auto db = QSqlDatabase::addDatabase("QSQLITE", kConnectionName);
    if (!db.isValid()) {
        if (errorMessage) {
            *errorMessage = "Failed to load SQLite driver.";
        }
        return false;
    }

    const auto dbPath = databaseFilePath();
    if (dbPath.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Failed to resolve database path.";
        }
        return false;
    }

    const QFileInfo fileInfo(dbPath);
    QDir dir = fileInfo.dir();
    if (!dir.exists() && !dir.mkpath(".")) {
        if (errorMessage) {
            *errorMessage = "Failed to create database directory.";
        }
        return false;
    }

    db.setDatabaseName(dbPath);
    if (!db.open()) {
        if (errorMessage) {
            *errorMessage = formatError("Failed to open database", db.lastError());
        }
        return false;
    }

    return ensureSchema(db, errorMessage);
}

QSqlDatabase DatabaseManager::database()
{
    if (!QSqlDatabase::contains(kConnectionName)) {
        return {};
    }
    return QSqlDatabase::database(kConnectionName);
}

QString DatabaseManager::databaseFilePath()
{
    return AppPaths::appDataPath("linksdash.sqlite");
}

bool DatabaseManager::ensureSchema(QSqlDatabase &db, QString *errorMessage)
{
    const int version = userVersion(db, errorMessage);
    if (version < 0) {
        return false;
    }

    if (version > kSchemaVersion) {
        if (errorMessage) {
            *errorMessage = "Database version is newer than this application supports.";
        }
        return false;
    }

    if (version == 0) {
        if (!execSql(db, kInitSql, errorMessage)) {
            return false;
        }
        return setUserVersion(db, kSchemaVersion, errorMessage);
    }

    return true;
}

bool DatabaseManager::execSql(QSqlDatabase &db, const QString &sql, QString *errorMessage)
{
    const auto statements = sql.split(';', Qt::SkipEmptyParts);
    for (const auto &statement : statements) {
        const auto trimmed = statement.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        QSqlQuery query(db);
        if (!query.exec(trimmed)) {
            if (errorMessage) {
                *errorMessage = formatError("Failed to run init script", query.lastError());
            }
            return false;
        }
    }
    return true;
}

int DatabaseManager::userVersion(QSqlDatabase &db, QString *errorMessage)
{
    QSqlQuery query(db);
    if (!query.exec("PRAGMA user_version;")) {
        if (errorMessage) {
            *errorMessage = formatError("Failed to read schema version", query.lastError());
        }
        return -1;
    }

    if (!query.next()) {
        if (errorMessage) {
            *errorMessage = "Failed to read schema version.";
        }
        return -1;
    }

    return query.value(0).toInt();
}

bool DatabaseManager::setUserVersion(QSqlDatabase &db, int version, QString *errorMessage)
{
    QSqlQuery query(db);
    if (!query.exec(QString("PRAGMA user_version = %1;").arg(version))) {
        if (errorMessage) {
            *errorMessage = formatError("Failed to set schema version", query.lastError());
        }
        return false;
    }
    return true;
}
