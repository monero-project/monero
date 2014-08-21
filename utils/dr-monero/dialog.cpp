/// @file
/// @author rfree (current maintainer in monero.cc project)
/// @brief main network-throttle (count speed and decide on limit)

#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

//    connect(ui->FileListComboBox, SIGNAL(currentIndexChanged(int)) ,this, SLOT(settings(int)));

    if(ui->fileCheckBox->isChecked()) {
        ui->CurveRadioButton->setDisabled(true);
        ui->HistogramRadioButton->setDisabled(true);
    }
    else {
        ui->CurveRadioButton->setEnabled(true);
        ui->HistogramRadioButton->setEnabled(true);
    }


//   connect(ui->okButton, SIGNAL(clicked()), this, SLOT(done();

}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::loadFiles(vector<plotFile> &files) {
    for (int i=0; i<files.size(); i++) {
        ui->FileListComboBox->insertItem(i,files.at(i).getQName());
        copyOfFiles.push_back(files.at(i));
    }
    set(files.at(0));
    connect(ui->FileListComboBox, SIGNAL(currentIndexChanged(int)) ,this, SLOT(settings(int)));


}

// @file
/// @author rfree (current maintainer in monero.cc project)
/// @brief main network-throttle (count speed and decide on limit)

void Dialog::settings(int i) {
    int index = ui->FileListComboBox->currentIndex();
    plotFile checked =  copyOfFiles.at(i);
    set(checked);
}

void Dialog::set(plotFile pfile) {
    if(pfile.getType()==histogram) ui->HistogramRadioButton->setChecked(true);
    else if(pfile.getType()==curve) ui->CurveRadioButton->setChecked(true);

    ui->fileCheckBox->setChecked(!pfile.isEnabled());
}

void Dialog::save(vector<plotFile> &files) {
    files.clear();
    for (int i=0; i<copyOfFiles.size(); i++)  {
        plotFile tmp(copyOfFiles.at(i));
        files.push_back(tmp);
    }
}
