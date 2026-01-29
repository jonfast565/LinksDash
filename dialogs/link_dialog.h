#pragma once

#include <QDialog>

#include "../models/link_item.h"

namespace Ui {
class LinkDialog;
}

class LinkDialog : public QDialog {
    Q_OBJECT

public:
    enum class Mode {
        Create,
        Edit
    };

    explicit LinkDialog(QWidget *parent = nullptr);
    ~LinkDialog();

    void setMode(Mode mode);
    void setLink(const LinkItem &link);
    LinkItem link() const;

protected:
    void accept() override;

private:
    void setupUi();

    Mode mode_ = Mode::Create;
    Ui::LinkDialog *ui_ = nullptr;
};
