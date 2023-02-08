#ifndef GUI_TRANSFORMPREVIEWDIALOG_H
#define GUI_TRANSFORMPREVIEWDIALOG_H

#include "core/lspdocument.h"

#include <QDialog>

namespace Gui {

namespace Ui {
    class TransformPreviewDialog;
}

class TransformPreviewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TransformPreviewDialog(Core::LspDocument *document, QString resultText, int numberReplacements,
                                    QWidget *parent = nullptr);
    ~TransformPreviewDialog();

private:
    Ui::TransformPreviewDialog *ui;
};

} // namespace Gui
#endif // GUI_TRANSFORMPREVIEWDIALOG_H