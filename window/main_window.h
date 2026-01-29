#pragma once

#include <QMainWindow>
#include <QSystemTrayIcon>

class QAction;
class QMenu;
class QPushButton;
class QSqlTableModel;
class QTableView;
class QCloseEvent;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUi();
    void setupModel();
    void setupTray();
    void refreshTrayMenu();
    void updateButtonStates();

    void handleEdit();
    void handleAdd();
    void handleDelete();
    void handleSave();
    void handleAddFromTray();

    void openLinkDialog(int row);
    int selectedRow() const;
    void markPendingChanges(const QString &message = "Changes pending. Click Save to commit.");
    void showError(const QString &title, const QString &message);
    void openUrl(const QString &urlText);

    Ui::MainWindow *ui_ = nullptr;
    QTableView *tableView_ = nullptr;
    QPushButton *addButton_ = nullptr;
    QPushButton *editButton_ = nullptr;
    QPushButton *deleteButton_ = nullptr;
    QPushButton *saveButton_ = nullptr;

    QSystemTrayIcon *trayIcon_ = nullptr;
    QMenu *trayMenu_ = nullptr;
    QAction *toggleWindowAction_ = nullptr;
    QAction *addLinkAction_ = nullptr;
    QAction *quitAction_ = nullptr;

    QSqlTableModel *model_ = nullptr;
    bool trayAvailable_ = false;
    bool trayNoticeShown_ = false;
};
