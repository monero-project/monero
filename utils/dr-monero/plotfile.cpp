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

void plotFile::calculateAvg(int window, vector <double> h) {
    double sum=0.;
    for(int i=0; i<h.size(); i+=window) {
        if(i+window > h.size()) return;
        sum=0.;
        for(int j=i; j<window+i; j++) sum += h.at(j);
        sum /=double(window);
        avgX.push_back(i+window/2);
        avgY.push_back(sum);
        cout << DBG << i << " - " << sum << endl;
    }
}
