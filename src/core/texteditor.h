#pragma once

#include <QPlainTextEdit>

namespace Core {

class Gutter;

class TextEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit TextEditor(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *) override;

private:
    void updateGutterWidth(int);
    void updateGutter(const QRect &rect, int dy);

private:
    friend class Gutter;

    Gutter *m_gutter;
};

}
