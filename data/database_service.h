#pragma once

#include <QSqlDatabase>
#include <QString>

class DatabaseManager {
public:
    static bool initialize(QString *errorMessage = nullptr);
    static QSqlDatabase database();
    static QString databaseFilePath();

private:
    static bool ensureSchema(QSqlDatabase &db, QString *errorMessage);
    static bool execSql(QSqlDatabase &db, const QString &sql, QString *errorMessage);
    static int userVersion(QSqlDatabase &db, QString *errorMessage);
    static bool setUserVersion(QSqlDatabase &db, int version, QString *errorMessage);

    static constexpr int kSchemaVersion = 1;
    static constexpr const char *kConnectionName = "linksdash";
};
