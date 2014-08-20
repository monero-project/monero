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

    vector<double>  simpleSmooth(vector<double> data, double alpha);
    void display(vector <double> data);

    vector<double> prepareHistogramData(vector<double> t, vector<double> b, int frame);

private:

    double calc(vector<double> t, vector<double> b, int i, int &jump) ;

};

#endif // UTILS_H
