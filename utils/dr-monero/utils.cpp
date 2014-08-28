/**
@file
@author rfree (current maintainer in monero.cc project)
@brief utils smoothing and converting data to histogram
*/
#include "utils.h"

utils::utils() {

}

vector<long double> utils::simpleSmooth(const vector<long double> data, const double alpha) {
    vector <long double> smoothed;
    smoothed.clear();
    smoothed.push_back(data.at(0));
    for (size_t t=1; t<data.size(); t++) {
        double tmp = ( (1.-alpha) * smoothed.at(t-1) )   +   ( alpha*data.at(t-1) )  ;
        smoothed.push_back(tmp);
    }
    return smoothed;

}

void utils::display(vector <double> data) {
    for(auto tmp : data)
        cout << tmp << "; ";
    cout << endl;
}

vector<long double> utils::prepareHistogramData(vector<long double> t, vector<long double> b, int frame) {
//   example:
//    1	1.0 0.5
//    2	1.3 4
//    3	1.7 0.1
//   ================== 4.6
//    4	2.0 5
//    5	2.1 2
//   ==================
//    6	3.5 4

    vector <long double> h;
    frame--; //TODO frame!=1
    double value=0;

    int size = t.size()-1;

    for (int i=0; i<size; i++) {
        value+=b.at(i);

        if( int(t.at(i+1)) - int(t.at(i))  == 1  )  {
            h.push_back(value);
            value=0;
        }
        else if( int(t.at(i+1)) - int(t.at(i)) >= 2) {
            int control=int(t.at(i));
            h.push_back(value);
            value=0;
            while(t.at(i+1) - control >= 2) {
                h.push_back(0);
                control++;
            }
        }
    }
//    cout << DBG << endl; for (int i=0; i<h.size(); i++) cout << i+1 << "; " << h.at(i) << endl;
//    cout << DBG << endl; for (int i=0; i<t.size(); i++) cout << t.at(i) << "; " << b.at(i) << endl;

    return h;
}

