#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    interval(100),
    sum(1000),
    dbg(true),
    xmax(10),
    ymax(10)
{
    ui->setupUi(this);

    init(ui->qwtPlot);

    prepareColors();

    if(dbg)
        prepareTestData();

    // slider and sliding
    connect(ui->Slider, SIGNAL(valueChanged(double)), this, SLOT(sliding()));
    ui->Slider->setRange(0,sum,10,interval);
//    ui->Slider->setRange(0,ui->spinBox_2->value(),10,interval);

    connect(this, SIGNAL(replot()), ui->qwtPlot, SLOT(plotCurve()));

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(refresh()));
    timer->start(1000);

    // functions to buttons
    connect(ui->stopButton, SIGNAL(clicked()), timer, SLOT(stop()));
    connect(ui->startButton, SIGNAL(clicked()), timer, SLOT(start()));
    connect(ui->openButton, SIGNAL(clicked()), this, SLOT(openFile()));

    utils test;

}
TEST void MainWindow::prepareTestData() {
    testFiles.push_back("../example.txt");
//    testFiles.push_back("../example2.txt");
//    testFiles.push_back("../example3.txt");
//    testFiles.push_back("../example4.txt");
//    testFiles.push_back("../example5.txt");

//     HistogramItem *histogram = new HistogramItem();



}

void MainWindow::openFile() {

    this->fileNames = QFileDialog::getOpenFileNames(this, tr("Choose file with data to plot"),"",tr("Text File (*.txt);;All Files (*)"));

    //                         |
    // is this really needed?  v
    if (this->fileNames.empty()) return; // if filename is empty
    else { // not empty
        for(QString fileName : fileNames) { // con open files?
            QFile file(fileName);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox::information(this, tr("Unable to open file"),
                                         file.errorString());
                return;
            }

            // list opened files
//            ui->filesLabel->setText(ui->filesLabel->text() + fileName + "\n");
        }
    }
}

void MainWindow::refresh() {
    cout << DBG <<  "timer " << endl;

    if(dbg) {
        int rtti; bool autoDelete;
        ui->qwtPlot->detachItems(rtti = QwtPlotItem::Rtti_PlotItem, autoDelete = true);
        for(int i=0; i<testFiles.size(); i++) plot(testFiles.at(i),i);
    }

    if(!this->fileNames.empty()) {
        // clear plot
        int rtti; bool autoDelete;
        ui->qwtPlot->detachItems(rtti = QwtPlotItem::Rtti_PlotItem, autoDelete = true);

        // call plot function for files
        for(int i=0; i<fileNames.size(); i++)
            plot(fileNames.at(i).toStdString(),i);
    }

    //else cout << DBG << "Nothing to do; " << endl;
}


MainWindow::~MainWindow()
{
    delete ui;
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
    colors.push_back(QColor(Qt::red));
    colors.push_back(QColor(Qt::green));
    colors.push_back(QColor(Qt::blue));
    colors.push_back(QColor(Qt::black));
    colors.push_back(QColor(Qt::darkBlue));
    colors.push_back(QColor(Qt::darkGreen));
    colors.push_back(QColor(Qt::darkRed));
    colors.push_back(QColor(Qt::darkYellow));


    ui->colorsLabel->setText("red \ngreen \nblue \nblack \ndarkBlue \ndarkGreen \ndarkRed \ndarkYellow");

}



void MainWindow::plot(const string &filename, const int col) {
    // clear current plot
//    int rtti; bool autoDelete;
//    ui->qwtPlot->detachItems(rtti = QwtPlotItem::Rtti_PlotItem, autoDelete = true);


     // enable sliding
    sliding();






    // grid: connected with checkbox
    QwtPlotGrid *grid = new QwtPlotGrid();
    if(ui->gridCheckBox->isChecked()) grid->attach(ui->qwtPlot);
    else grid->detach();

    fstream file;
    cout << DBG << filename << endl;
    file.open(filename.c_str(), ios::in | ios::out);

    if(!file.is_open())  {
        cout << DBG << "Can't open file " << filename << endl;
        return;
    }


    static int si; // static - determinates position of slider (following right plot)

    int i=0;

    vector <double> xval;
    vector <double> yval;


    // getting data
    while(!file.eof()) {
        // geting x and y values from file, format: "x y" per line
        double x,y;
        file >> x;
        file >> y;

        if(x > xmax) xmax = x;
        if(y > ymax) ymax = y;


        // saving this information to vectors
        xval.push_back(x);
        yval.push_back(y);

        // samples->push_back(QPointF(x, y));
        ui->qwtPlot->replot();
        i++;

        if(si < i) { // moving slider
            ui->Slider->setValue(i-interval);
            si=i;

            // slider configuration
            ui->Slider->setRange(0,si+1000,(ui->spinBox_2->value())/10,  0);
        }

    }

    //ui->spinBox_2->setValue(xmax);
    ui->spinBox->setValue(ymax);

   plotCurve(xval,yval,col,filename);
    plotHist(xval,yval,col,filename);





    file.close();

    // TODO: some smooth
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
    if(ui->pointsCheckBox->isChecked()) {
        QwtSymbol *symbol = new QwtSymbol( QwtSymbol::Ellipse, QBrush( Qt::yellow ), QPen( color, 2 ), QSize( 8, 8 ) );
        curve1->setSymbol( symbol );
    }


    ui->qwtPlot->insertLegend(new QwtLegend());

    // smoothing here
    if(ui->smoothSpinBox->value() != 0.) {
        utils data;
        vector <double> final_Y = data.simpleSmooth(y,ui->smoothSpinBox->value());

        for(int i=0; i<x.size(); i++) samples->push_back(QPointF(x.at(i), final_Y.at(i)));

        if(dbg) {
            for (int i=0; i<final_Y.size(); i++)
                cout << y.at(i) << " -> " << final_Y.at(i) << endl;

        }
    } // end of smoothing
    else {
        for(int i=0; i<x.size(); i++) samples->push_back(QPointF(x.at(i), y.at(i)));
    }

    // plot
    myData->setSamples(*samples);
    curve1->setData(myData);
    curve1->attach(ui->qwtPlot);
    ui->qwtPlot->replot();
}

void MainWindow::plotHist(const vector<double> t, const vector<double> b, const int col, const string &filename) {
    const double frame=1.; // 1s


    QwtPlotHistogram *histogram = new QwtPlotHistogram(QString::fromStdString(splitString(filename,"/").back()));
    histogram->setStyle(QwtPlotHistogram::Columns);
    histogram->setPen(QPen(Qt::black, 3));
    histogram->setBrush(QBrush(QColor(17, 0, 255, 60)));
    QVector<QwtIntervalSample> samples2(t.size());

    double pos = 0.0;
    cout <<DBG << endl;
    utils test; vector <double> h = test.prepareHistogramData(t,b,frame);
//    for (double tmp :b) cout << DBG << tmp << ";";
//    for (double tmp :t) cout << DBG << tmp << ";";


    for ( int i=0; i<h.size(); i++ )    {
        const int width = frame;

        double value = h.at(i);




        samples2[i].interval = QwtInterval(pos, pos + double(width));
        samples2[i].value = value;
        pos += width;

    }
    ui->qwtPlot->insertLegend(new QwtLegend());

    histogram->setSamples(samples2);
    histogram->attach(ui->qwtPlot);
    ui->qwtPlot->replot();
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
