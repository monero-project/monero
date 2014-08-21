/// @file
/// @author rfree (current maintainer in monero.cc project)
/// @brief main network-throttle (count speed and decide on limit)

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QTimerEvent>
#include <QMessageBox>
#include <QColor>
#include <QShortcut>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QApplication>
#include <QLabel>
#include <QString>
#include <QTextStream>
#include <QColor>


#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_series_data.h>
#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_legend.h>
#include <qwt_text.h>
#include <qwt_math.h>
#include <qwt_slider.h>
#include <qwt_plot_grid.h>
#include <qwt_symbol.h>
#include <qwt_plot_histogram.h>
#include <qwt_column_symbol.h>

#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

#include "utils.h"
#include "plotfile.h"
#include "dialog.h"

#define DBG __FUNCTION__<<":"<< __LINE__<<"\t"
#define TEST
#define XMR

using namespace std;
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool dbg;

public slots:
    void sliding(); // enable sliding
    void plot(const string &filename, const int col, const plotType type); // plots from file
    void init(QwtPlot *&Plot); // prepares plot field
    void refresh(); // called by timer
    void openFile(); // getting files (dialog window)
    void addFile(const string &name, plotType type);
    void reloadColors();
    void optionWindow();

private:
    Ui::MainWindow *ui;
    QStringList fileNames; // names of opened files
    vector <QColor> colors; // colors used to ploting curve
    vector <QColor> transparentColors; // colors used to ploting histogram
    void prepareColors(); // creates vector of colors, TODO: more colors
    vector<string> splitString(string toSplit, string delimiter);
    void plotCurve(const vector<double> x, const vector<double> y, const int col, const string &filename);
    void plotHist(const vector<double> t, const vector<double> b, const int col, const string &filename);
    vector <plotFile> files;
    bool monero;
    int interval; // frequency of refreshing (?)
    int sum;
    int xmax;
    int ymax;
};

#endif // MAINWINDOW_H
