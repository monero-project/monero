/**
@file
@author rfree (current maintainer in monero.cc project)
@brief plotfile information about files
*/

#include "plotfile.h"

plotFile::plotFile(const string &_name, const plotType _type) :
    name(_name),
    type(_type),
    enabled(true),
    sum(0.)
{
    this->qname=QString::fromStdString(name);
}

plotFile::plotFile(const plotFile &pFile) {
    this->name = pFile.name;
    this->qname = pFile.qname;
    this->type = pFile.type;
    this->enabled = pFile.enabled;
    this->sum = pFile.sum;
}

void plotFile::setEnabled(bool b) {
    this->enabled = b;
}

void plotFile::changeType(plotType _type) {
    this->type = _type;
}

void plotFile::clearVectors() {
    this->Xval.clear();
    this->Yval.clear();
}

void plotFile::calculateAvg(int window, vector<long double> h) {
    double a=0.;
    for(size_t i=0; i<h.size(); i+=window) {
        if(i+window > h.size()) return;
        a=0.;
        for(size_t j=i; j<window+i; j++) a += h.at(j);
        a /=double(window);
        avgX.push_back(i+window/2);
        avgY.push_back(a);
        cout << DBG << i << " - " << a << endl;
    }
}

bool plotFile::getDataFromFile() {
    // open file
    fstream file;
    const string filename = this->getName();
    file.open(filename.c_str(), ios::in);
    if(!file.is_open())  {
        cout << DBG << "[warn] Can't open file " << filename << endl;
        return false;
    }
    cout << DBG << "[ok] " << filename << " is open" << endl;

    clearVectors();
    sum=0.;

    // getting data
    while(!file.eof()) {
        // geting x and y values from file, format: "x y" per line
        long double x,y;
        file >> x;
        file >> y;

        // saving this information to vectors
        Xval.push_back(x);
        Yval.push_back(y);

        sum+=y;
    }
    file.close();
    return true;
}


