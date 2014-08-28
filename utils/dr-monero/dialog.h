/**
@file
@author rfree (current maintainer in monero.cc project)
@brief dialog window with options, not used for now
*/

#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

#include <vector>
#include <string>

#include "mainwindow.h"
#include "plotfile.h"

#define DBG __FUNCTION__<<":"<< __LINE__<<"\t"


using namespace std;

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT
    friend class MainWindow;
public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

public slots:
    void settings(int i);

private:
    Ui::Dialog *ui;
    void loadFiles(vector<plotFile> &files);
    void set(plotFile pfile);
    void applyChanges(vector<plotFile> &files);
    vector <plotFile> copyOfFiles;
    void save(vector<plotFile> &files);
    bool reallyApply;
};

#endif // DIALOG_H
