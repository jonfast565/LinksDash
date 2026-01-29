#include "main_window.h"
#include "ui_main_window.h"

#include "../data/database_service.h"
#include "../dialogs/link_dialog.h"
#include "../models/link_item.h"

#include <algorithm>

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QHeaderView>
#include <QList>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QPair>
#include <QPushButton>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QStatusBar>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTableView>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();

    QString errorMessage;
    if (!DatabaseManager::initialize(&errorMessage)) {
        showError("Database Error", errorMessage);
        tableView_->setEnabled(false);
        editButton_->setEnabled(false);
        deleteButton_->setEnabled(false);
        saveButton_->setEnabled(false);
        return;
    }

    setupModel();
    setupTray();
    refreshTrayMenu();
}

MainWindow::~MainWindow()
{
    delete ui_;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!trayAvailable_ || QCoreApplication::closingDown()) {
        QMainWindow::closeEvent(event);
        return;
    }

    hide();
    event->ignore();

    if (toggleWindowAction_) {
        toggleWindowAction_->setText("Configure");
    }

    if (trayNoticeShown_) {
        return;
    }

    trayIcon_->showMessage("LinksDash", "LinksDash is still running in the tray.");
    trayNoticeShown_ = true;
}

void MainWindow::setupUi()
{
    ui_ = new Ui::MainWindow;
    ui_->setupUi(this);

    tableView_ = ui_->tableView;
    addButton_ = ui_->addButton;
    editButton_ = ui_->editButton;
    deleteButton_ = ui_->deleteButton;
    saveButton_ = ui_->saveButton;

    tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView_->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableView_->setAlternatingRowColors(true);

    connect(addButton_, &QPushButton::clicked, this, &MainWindow::handleAdd);
    editButton_->setToolTip("Edit the selected link.");
    connect(editButton_, &QPushButton::clicked, this, &MainWindow::handleEdit);
    connect(deleteButton_, &QPushButton::clicked, this, &MainWindow::handleDelete);
    connect(saveButton_, &QPushButton::clicked, this, &MainWindow::handleSave);

    statusBar()->showMessage("Ready.");
}

void MainWindow::setupModel()
{
    auto db = DatabaseManager::database();
    if (!db.isValid()) {
        showError("Database Error", "Database connection is invalid.");
        return;
    }

    model_ = new QSqlTableModel(this, db);
    model_->setTable("links");
    model_->setEditStrategy(QSqlTableModel::OnManualSubmit);

    if (!model_->select()) {
        showError("Database Error", model_->lastError().text());
        return;
    }

    tableView_->setModel(model_);

    const int idColumn = model_->fieldIndex("id");
    if (idColumn >= 0) {
        tableView_->hideColumn(idColumn);
    }

    const int titleColumn = model_->fieldIndex("title");
    const int categoryColumn = model_->fieldIndex("category");
    const int urlColumn = model_->fieldIndex("url");

    if (titleColumn >= 0) {
        model_->setHeaderData(titleColumn, Qt::Horizontal, "Title");
    }
    if (categoryColumn >= 0) {
        model_->setHeaderData(categoryColumn, Qt::Horizontal, "Category");
    }
    if (urlColumn >= 0) {
        model_->setHeaderData(urlColumn, Qt::Horizontal, "URL");
    }
    tableView_->horizontalHeader()->setStretchLastSection(true);
    tableView_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    connect(tableView_->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::updateButtonStates);
    connect(model_, &QSqlTableModel::dataChanged, this,
            [this](const auto &, const auto &, const auto &) { markPendingChanges(); });
    connect(model_, &QSqlTableModel::rowsInserted, this,
            [this](const QModelIndex &, int, int) { markPendingChanges(); });
    connect(model_, &QSqlTableModel::rowsRemoved, this,
            [this](const QModelIndex &, int, int) { markPendingChanges(); });

    updateButtonStates();
}

void MainWindow::setupTray()
{
    trayAvailable_ = QSystemTrayIcon::isSystemTrayAvailable();
    if (!trayAvailable_) {
        return;
    }

    trayIcon_ = new QSystemTrayIcon(this);
    QIcon trayIcon = windowIcon();
    if (trayIcon.isNull()) {
        trayIcon = QIcon::fromTheme("link");
    }
    if (trayIcon.isNull()) {
        trayIcon = QApplication::style()->standardIcon(QStyle::SP_DirLinkIcon);
    }
    trayIcon_->setIcon(trayIcon);
    if (!trayIcon.isNull()) {
        setWindowIcon(trayIcon);
    }
    trayIcon_->setToolTip("LinksDash");

    trayMenu_ = new QMenu(this);

    trayIcon_->setContextMenu(trayMenu_);
    trayIcon_->show();
}

void MainWindow::refreshTrayMenu()
{
    if (!trayMenu_) {
        return;
    }

    trayMenu_->clear();
    toggleWindowAction_ = nullptr;
    addLinkAction_ = nullptr;
    quitAction_ = nullptr;

    if (!model_) {
        QAction *disabled = trayMenu_->addAction("Database unavailable");
        disabled->setEnabled(false);
    } else {
        const int titleColumn = model_->fieldIndex("title");
        const int categoryColumn = model_->fieldIndex("category");
        const int urlColumn = model_->fieldIndex("url");

        if (titleColumn < 0 || categoryColumn < 0 || urlColumn < 0) {
            QAction *disabled = trayMenu_->addAction("Schema error");
            disabled->setEnabled(false);
        } else {
            QMap<QString, QList<QPair<QString, QString>>> byCategory;
            for (int row = 0; row < model_->rowCount(); ++row) {
                const auto title = model_->data(model_->index(row, titleColumn)).toString().trimmed();
                const auto rawCategory = model_->data(model_->index(row, categoryColumn)).toString().trimmed();
                const auto url = model_->data(model_->index(row, urlColumn)).toString().trimmed();

                if (title.isEmpty() || url.isEmpty()) {
                    continue;
                }

                const auto category = rawCategory.isEmpty() ? QString("Uncategorized") : rawCategory;
                byCategory[category].append({title, url});
            }

            if (byCategory.isEmpty()) {
                QAction *emptyAction = trayMenu_->addAction("No links yet");
                emptyAction->setEnabled(false);
            } else {
                bool firstCategory = true;
                for (auto it = byCategory.cbegin(); it != byCategory.cend(); ++it) {
                    if (!firstCategory) {
                        trayMenu_->addSeparator();
                    }
                    firstCategory = false;

                    QAction *categoryHeader = trayMenu_->addAction(it.key());
                    categoryHeader->setEnabled(false);

                    auto entries = it.value();
                    std::sort(entries.begin(), entries.end(), [](const auto &left, const auto &right) {
                        return left.first.toLower() < right.first.toLower();
                    });

                    for (const auto &entry : entries) {
                        QAction *action = trayMenu_->addAction(entry.first);
                        const auto url = entry.second;
                        connect(action, &QAction::triggered, this, [this, url]() { openUrl(url); });
                    }
                }
            }
        }
    }

    trayMenu_->addSeparator();

    toggleWindowAction_ = trayMenu_->addAction(isVisible() ? "Hide LinksDash" : "Configure");
    connect(toggleWindowAction_, &QAction::triggered, this, [this]() {
        if (isVisible()) {
            hide();
            toggleWindowAction_->setText("Configure");
            return;
        }
        show();
        raise();
        activateWindow();
        toggleWindowAction_->setText("Hide LinksDash");
    });

    addLinkAction_ = trayMenu_->addAction("Add Link...");
    connect(addLinkAction_, &QAction::triggered, this, &MainWindow::handleAddFromTray);

    quitAction_ = trayMenu_->addAction("Quit");
    connect(quitAction_, &QAction::triggered, this, [this]() { qApp->quit(); });
}

void MainWindow::updateButtonStates()
{
    const bool hasSelection = selectedRow() >= 0;
    editButton_->setEnabled(hasSelection);
    deleteButton_->setEnabled(hasSelection);
}

void MainWindow::handleEdit()
{
    if (!model_) {
        return;
    }

    const int row = selectedRow();
    if (row < 0) {
        return;
    }

    openLinkDialog(row);
}

void MainWindow::handleAdd()
{
    if (!model_) {
        return;
    }

    openLinkDialog(-1);
}

void MainWindow::handleDelete()
{
    if (!model_) {
        return;
    }

    const int row = selectedRow();
    if (row < 0) {
        return;
    }

    const auto response = QMessageBox::question(this, "Delete Link", "Delete the selected link?");
    if (response != QMessageBox::Yes) {
        return;
    }

    if (!model_->removeRow(row)) {
        showError("Database Error", model_->lastError().text());
        return;
    }

    markPendingChanges("Link removed. Click Save to commit.");
}

void MainWindow::handleSave()
{
    if (!model_) {
        return;
    }

    auto db = model_->database();
    if (!db.isValid()) {
        showError("Database Error", "Database connection is invalid.");
        return;
    }

    if (!db.transaction()) {
        showError("Database Error", db.lastError().text());
        return;
    }

    if (!model_->submitAll()) {
        db.rollback();
        showError("Save Failed", model_->lastError().text());
        return;
    }

    if (!db.commit()) {
        db.rollback();
        showError("Save Failed", db.lastError().text());
        return;
    }

    model_->select();
    refreshTrayMenu();
    statusBar()->showMessage("Saved.", 3000);
}

void MainWindow::handleAddFromTray()
{
    if (!isVisible()) {
        show();
    }
    raise();
    activateWindow();
    handleAdd();
}

int MainWindow::selectedRow() const
{
    if (!tableView_ || !tableView_->selectionModel()) {
        return -1;
    }

    const auto rows = tableView_->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return -1;
    }

    return rows.first().row();
}

void MainWindow::openLinkDialog(int row)
{
    if (!model_) {
        return;
    }

    const int titleColumn = model_->fieldIndex("title");
    const int categoryColumn = model_->fieldIndex("category");
    const int urlColumn = model_->fieldIndex("url");

    if (titleColumn < 0 || categoryColumn < 0 || urlColumn < 0) {
        showError("Schema Error", "The links table is missing required columns.");
        return;
    }

    LinkDialog dialog(this);
    if (row >= 0) {
        dialog.setMode(LinkDialog::Mode::Edit);
        const auto record = model_->record(row);
        dialog.setLink({
            record.value("title").toString(),
            record.value("category").toString(),
            record.value("url").toString()
        });
    } else {
        dialog.setMode(LinkDialog::Mode::Create);
    }

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const auto link = dialog.link();

    if (row < 0) {
        QSqlRecord record = model_->record();
        record.setValue("title", link.title);
        record.setValue("category", link.category);
        record.setValue("url", link.url);

        if (!model_->insertRecord(-1, record)) {
            showError("Database Error", model_->lastError().text());
            return;
        }

        markPendingChanges("Link added. Click Save to commit.");
        return;
    }

    model_->setData(model_->index(row, titleColumn), link.title);
    model_->setData(model_->index(row, categoryColumn), link.category);
    model_->setData(model_->index(row, urlColumn), link.url);
    markPendingChanges();
}

void MainWindow::markPendingChanges(const QString &message)
{
    statusBar()->showMessage(message);
}

void MainWindow::showError(const QString &title, const QString &message)
{
    QMessageBox::critical(this, title, message);
}

void MainWindow::openUrl(const QString &urlText)
{
    const auto url = QUrl::fromUserInput(urlText);
    if (!url.isValid()) {
        showError("Invalid URL", "The selected link has an invalid URL.");
        return;
    }

    if (!QDesktopServices::openUrl(url)) {
        showError("Open Failed", "Unable to open the link in the default browser.");
    }
}
