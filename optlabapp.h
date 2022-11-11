#ifndef OPTLABAPP_H
#define OPTLABAPP_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class OptLabApp; }
QT_END_NAMESPACE

class OptLabApp : public QMainWindow
{
    Q_OBJECT

public:
    OptLabApp(QWidget *parent = nullptr);
    ~OptLabApp();

private:
    Ui::OptLabApp *ui;
};
#endif // OPTLABAPP_H
