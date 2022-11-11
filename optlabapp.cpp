#include "optlabapp.h"
#include "./ui_optlabapp.h"

OptLabApp::OptLabApp(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::OptLabApp)
{
    ui->setupUi(this);
}

OptLabApp::~OptLabApp()
{
    delete ui;
}

