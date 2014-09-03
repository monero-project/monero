/**
@file
@author rfree (current maintainer in monero.cc project)
@brief utils smoothing and converting data to histogram
*/

#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>

#define DBG __FUNCTION__<<":"<< __LINE__<<"\t"

using namespace std;

class utils
{
public:
    utils();
    vector<long double>  simpleSmooth(vector<long double> data, double alpha);
    vector<long double> prepareHistogramData(vector<long double> t, vector<long double> b, int frame);
private:
    void display(vector <double> data);
};

#endif // UTILS_H
