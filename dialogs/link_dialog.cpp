#include "link_dialog.h"
#include "ui_link_dialog.h"

#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMessageBox>

LinkDialog::LinkDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
}

LinkDialog::~LinkDialog()
{
    delete ui_;
}

void LinkDialog::setMode(Mode mode)
{
    mode_ = mode;
    setWindowTitle(mode_ == Mode::Create ? "Add Link" : "Edit Link");
}

void LinkDialog::setLink(const LinkItem &link)
{
    ui_->titleLineEdit->setText(link.title);
    ui_->categoryLineEdit->setText(link.category);
    ui_->urlLineEdit->setText(link.url);
}

LinkItem LinkDialog::link() const
{
    return {
        ui_->titleLineEdit->text().trimmed(),
        ui_->categoryLineEdit->text().trimmed(),
        ui_->urlLineEdit->text().trimmed()
    };
}

void LinkDialog::accept()
{
    const auto current = link();

    if (current.title.isEmpty()) {
        QMessageBox::warning(this, "Missing Title", "Please enter a link title.");
        return;
    }

    if (current.category.isEmpty()) {
        QMessageBox::warning(this, "Missing Category", "Please enter a category.");
        return;
    }

    if (current.url.isEmpty()) {
        QMessageBox::warning(this, "Missing URL", "Please enter a URL.");
        return;
    }

    QDialog::accept();
}

void LinkDialog::setupUi()
{
    setModal(true);

    ui_ = new Ui::LinkDialog;
    ui_->setupUi(this);

    ui_->titleLineEdit->setPlaceholderText("Example: Dashboard");
    ui_->categoryLineEdit->setPlaceholderText("Example: Work");
    ui_->urlLineEdit->setPlaceholderText("https://example.com");

    connect(ui_->buttonBox, &QDialogButtonBox::accepted, this, &LinkDialog::accept);
    connect(ui_->buttonBox, &QDialogButtonBox::rejected, this, &LinkDialog::reject);
}
