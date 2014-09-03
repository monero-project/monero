/**
@file
@author rfree (current maintainer in monero.cc project)
@brief plotfile information about files
*/

#ifndef PLOTFILE_H
#define PLOTFILE_H

#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>

#include <QString>

#define DBG __FUNCTION__<<":"<< __LINE__<<"\t"

using namespace std;

enum  plotType {histogram, curve, histogram_avg} ;

class plotFile
{
public:
    plotFile(const string &_name,  const plotType _type);
    plotFile(const plotFile &pFile);

    void setEnabled(bool b);
    void changeType(plotType _type);
    void addX(double x) { Xval.push_back(x);};
    void addY(double y) { Yval.push_back(y);};
    void clearVectors();
    void calculateAvg(int window, vector<long double> h);

    // getters
    string getName() { return name; } ;
    QString getQName() { return qname; } ;
    plotType getType() { return type; } ;
    bool isEnabled() { return enabled; } ;
    vector <long double> Xs() { return Xval; } ;
    vector <long double> Ys() { return Yval; } ;
    vector <long double> avgXs() { return avgX; } ;
    vector <long double> avgYs() { return avgY; } ;
    bool getDataFromFile();
    long double getSum() { return sum; } ;

private:
    string name;
    QString qname;
    plotType type;
    bool enabled;
    long double sum;
    vector <long double> Xval;
    vector <long double> Yval;

    vector <long double> avgX;
    vector <long double> avgY;


};

#endif // PLOTFILE_H
