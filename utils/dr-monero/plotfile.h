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

#include <QString>

#define DBG __FUNCTION__<<":"<< __LINE__<<"\t"

using namespace std;

enum  plotType {histogram, curve, histogram_avg} ;

class plotFile
{
public:
    plotFile(string _name,  plotType _type);
    plotFile(const plotFile &pFile);

    void setEnabled(bool b);
    void changeType(plotType _type);
    void addX(double x) { Xval.push_back(x);};
    void addY(double y) { Yval.push_back(y);};
    void clearVectors();
    void calculateAvg(int window, vector<double> h);

    // getters
    string getName() { return name; } ;
    QString getQName() { return qname; } ;
    plotType getType() { return type; } ;
    bool isEnabled() { return enabled; } ;
    vector <double> Xs() { return Xval; } ;
    vector <double> Ys() { return Yval; } ;
    vector <double> avgXs() { return avgX; } ;
    vector <double> avgYs() { return avgY; } ;

private:
    string name;
    QString qname;
    plotType type;
    bool enabled;
    vector <double> Xval;
    vector <double> Yval;

    vector <double> avgX;
    vector <double> avgY;
};

#endif // PLOTFILE_H
