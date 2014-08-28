/**
@file
@author rfree (current maintainer in monero.cc project)
@brief mainwindow handling GUI, ploting mechanism
*/
#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    interval(100),
    sum(1000),
    dbg(false),
    monero(true),
    xmax(10),
    ymax(10)
{
    srand(time(NULL));
    ui->setupUi(this);

    init(ui->qwtPlot);

    prepareColors();

    // slider and sliding
    connect(ui->Slider, SIGNAL(valueChanged(double)), this, SLOT(sliding()));
    ui->Slider->setRange(0,sum,10,interval);

    connect(this, SIGNAL(replot()), ui->qwtPlot, SLOT(plotCurve()));

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(refresh()));
    timer->start(1000);

    // functions to buttons
    connect(ui->stopButton, SIGNAL(clicked()), timer, SLOT(stop()));
    connect(ui->startButton, SIGNAL(clicked()), timer, SLOT(start()));
    connect(ui->openButton, SIGNAL(clicked()), this, SLOT(openFile()));
    connect(ui->rcolorsButton, SIGNAL(clicked()),this, SLOT(reloadColors()));
    connect(ui->optionsButton, SIGNAL(clicked()), this, SLOT(optionWindow()));

    ui->lockY->setChecked(true);
    ui->pointsCheckBox->setChecked(true);
    ui->gridCheckBox->setChecked(true);
}

void MainWindow::optionWindow() {
    Dialog *option = new Dialog;
    option->loadFiles(files);
    int result = option->exec();

    if(result == QDialog::Accepted) {
        cout << DBG << "success" << endl;
        option->save(files);
    }
    else if (result == QDialog::Rejected ) cout << DBG << "some errors" << endl;
//    else if (result == QDialog::)
}

void MainWindow::addFile(const string &name, plotType type) {
    plotFile pfile(name,type);
    files.push_back(pfile);
}


void MainWindow::openFile() {

    this->fileNames = QFileDialog::getOpenFileNames(this, tr("Choose file with data to plot"),"",tr("Text File (*.txt);;All Files (*)"));

    if (this->fileNames.empty()) return; // if filename is empty
    else { // not empty
        for(QString fileName : fileNames) { // can open files?
            QFile file(fileName);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox::information(this, tr("Unable to open file"),
                                         file.errorString());
                return;
            }
        }
    }
}

void MainWindow::refresh() {
    cout << DBG <<  "timer " << endl;

    if(dbg || monero) {
        int rtti; bool autoDelete;
        ui->qwtPlot->detachItems(rtti = QwtPlotItem::Rtti_PlotItem, autoDelete = true);

        for(int i=0; i<files.size(); i++)
            plot(files.at(i), i);
    }
// TODO:
//    if(!this->fileNames.empty()) {
//        // clear plot
//        int rtti; bool autoDelete;
//        ui->qwtPlot->detachItems(rtti = QwtPlotItem::Rtti_PlotItem, autoDelete = true);

//        // call plot function for files
//        for(int i=0; i<fileNames.size(); i++)
//            plot(fileNames.at(i).toStdString(),i);
//    }
}


MainWindow::~MainWindow() {
    delete ui;
}
void MainWindow::reloadColors() {
    prepareColors();
}

void MainWindow::sliding() {
    double v = ui->Slider->value();

    // set scale - values from spinbox
    ui->qwtPlot->setAxisScale( QwtPlot::yLeft, 0, ui->spinBox->value());
    this->interval=ui->spinBox_2->value();
    ui->qwtPlot->setAxisScale( QwtPlot::xBottom, v, v+interval);
    sum+=interval;
}

void MainWindow::init(QwtPlot *&Plot) {
    Plot->setCanvasBackground(QColor(Qt::white));
    Plot->setAutoReplot(true);
    ui->qwtPlot->setAxisScale( QwtPlot::xBottom, 0, interval);
}

void MainWindow::prepareColors() {
//    http://qt-project.org/doc/qt-4.8/qcolor.html#colorNames
//    nice color to plots

    transparentColors.clear();
    colors.clear();
    cout << DBG << "Preparing colors" << endl;
    for(int i=0; i<100; i++) { // TODO Better randomize...
        QColor color(rand()%255,  rand()%255, rand()%255);
        srand(time(NULL)*(i+1));
        colors.push_back(color.darker(100));
        srand(time(NULL)*(i+10));

        QColor transparentColor(rand()%240,  rand()%240, rand()%240, 50);
        transparentColors.push_back(transparentColor.darker(100));
    }
    cout << endl;
    cout << DBG << "Colors ready" << endl;
}



void MainWindow::plot(plotFile pfile, const int col) {
     // enable sliding
    sliding();
    const string filename = pfile.getName();

    // grid: connected with checkbox
    QwtPlotGrid *grid = new QwtPlotGrid();
    if(ui->gridCheckBox->isChecked()) grid->attach(ui->qwtPlot);
    else grid->detach();

    fstream file;
    file.open(filename.c_str(), ios::in);

    if(!file.is_open())  {
        cout << DBG << "Can't open file " << filename << endl;
        return;
    }
    cout << DBG << filename << " is open" << endl;
    int i=0;

    pfile.clearVectors();

    // getting data
    while(!file.eof()) {
        // geting x and y values from file, format: "x y" per line
        double x,y;
        file >> x;
        file >> y;

        // saving this information to vectors
        pfile.addX(x);
        pfile.addY(y);

        ui->qwtPlot->replot();
        i++;
    }
    file.close();

    // curve or histogram
    if(pfile.getType()==curve) plotCurve(pfile.Xs(), pfile.Ys(), col, filename);
    else if (pfile.getType()==histogram || pfile.getType()==histogram_avg)  {
        utils test; vector <double> h = test.prepareHistogramData(pfile.Xs(), pfile.Ys(), 1);
        plotHist(pfile.Xs(), h, col, filename);

        // histogram with avg
        if(pfile.getType()==histogram_avg && ui->avgSpinBox->value()) {
            pfile.calculateAvg(ui->avgSpinBox->value(),h);
            plotCurve(pfile.avgXs(),pfile.avgYs(),col,pfile.getName());
        }
    }
    // auto adjustment y axis
    if(ui->lockY->isChecked()) {
        ui->spinBox->setValue(ymax);
        ui->spinBox->setDisabled(true);
    }
    else ui->spinBox->setEnabled(true);

    ui->qwtPlot->replot();
}

void MainWindow::plotCurve(const vector<double> x, const vector<double> y, const int col, const string &filename) {
    // preparing curve
    QColor color = colors.at(col);
    QwtPlotCurve *curve1 = new QwtPlotCurve(QString::fromStdString(splitString(filename,"/").back()));
    curve1->setPen( QPen( color, 2 ) );
    QwtPointSeriesData* myData = new QwtPointSeriesData;
    QVector<QPointF>* samples = new QVector<QPointF>;
    myData->setSamples(*samples);
    curve1->setData(myData);
    curve1->attach(ui->qwtPlot);

    // points
    if(ui->pointsCheckBox->isChecked()) {
        QwtSymbol *symbol = new QwtSymbol( QwtSymbol::Ellipse, QBrush( color ), QPen( color, 2 ), QSize( 8, 8 ) );
        curve1->setSymbol( symbol );
    }

    ui->qwtPlot->insertLegend(new QwtLegend());

    // smoothing here
    if(ui->smoothSpinBox->value() != 0.) {
        utils data;
        vector <double> final_Y = data.simpleSmooth(y,ui->smoothSpinBox->value());

        for(int i=0; i<x.size(); i++) {
            samples->push_back(QPointF(x.at(i), final_Y.at(i)));
            if(final_Y.at(i)>ymax) ymax = final_Y.at(i)+1;

            if(xmax < i) { // moving slider
                ui->Slider->setValue(i-interval);
                xmax=i;

                // slider configuration
                ui->Slider->setRange(0,xmax+1000,(ui->spinBox_2->value())/10,  0);
            }
        }
    } // or no smoothing
    else {
        for(int i=0; i<x.size(); i++) {
            samples->push_back(QPointF(x.at(i), y.at(i)));
            if(y.at(i)>ymax) ymax = y.at(i)+1;

            if(xmax < i) { // moving slider
                ui->Slider->setValue(i-interval);
                xmax=i;

                // slider configuration
                ui->Slider->setRange(0,xmax+1000,(ui->spinBox_2->value())/10,  0);
            }
        }
    }

    // plot
    myData->setSamples(*samples);
    curve1->setData(myData);
    curve1->attach(ui->qwtPlot);
}

void MainWindow::plotHist(const vector<double> t, const vector<double> h, const int col, const string &filename) {
    const double frame=1.; // TODO: frame value from spinbox
    QColor color = transparentColors.at(col);
    QwtPlotHistogram *histogram = new QwtPlotHistogram(QString::fromStdString(splitString(filename,"/").back()));
    histogram->setStyle(QwtPlotHistogram::Columns);
    histogram->setPen(QPen(color, 5));
    histogram->setBrush(QBrush(QColor(color)));
    QVector<QwtIntervalSample> samples2(t.size());

    double pos = 0.0;

    for ( int i=0; i<h.size(); i++ )    {
        const int width = frame;
        double value = h.at(i);
        if(h.at(i)>ymax) ymax = h.at(i)+1;

        samples2[i].interval = QwtInterval(pos, pos + double(width));
        samples2[i].value = value;
        pos += width;

        if(xmax < i) { // moving slider
            ui->Slider->setValue(i-interval+2);
            xmax=i;

            // slider configuration
            ui->Slider->setRange(0,xmax+1000,(ui->spinBox_2->value())/10,  0);
        }
    }

    ui->qwtPlot->insertLegend(new QwtLegend());

    histogram->setSamples(samples2);
    histogram->attach(ui->qwtPlot);
}

vector<string> MainWindow::splitString(string toSplit, string delimiter) {
    toSplit+=delimiter;
    string token;
    size_t pos = 0;
    vector<string> data;

    while ((pos = toSplit.find(delimiter)) != string::npos) {
        token = toSplit.substr(0, pos);
        toSplit.erase(0, pos + delimiter.length());
        data.push_back(token);
    }

    return data;
}
