/// @file
/// @author rfree (current maintainer in monero.cc project)
/// @brief main network-throttle (count speed and decide on limit)

#include "utils.h"

utils::utils() {

}

vector<double> utils::simpleSmooth(const vector<double> data, const double alpha) {
    vector <double> smoothed;
    smoothed.clear();
    smoothed.push_back(data.at(0));
    for (int t=1; t<data.size(); t++) {
        double tmp = ( (1.-alpha) * smoothed.at(t-1) )   +   ( alpha*data.at(t-1) )  ;
        smoothed.push_back(tmp);
    }
    return smoothed;

}

void utils::display(vector <double> data) {
    for(double tmp : data)
        cout << tmp << "; ";
    cout << endl;
}

vector<double> utils::prepareHistogramData(vector<double> t, vector<double> b, int frame) {
//   example:
//    1	1.0 0.5
//    2	1.3 4
//    3	1.7 0.1
//   ================== 4.6
//    4	2.0 5
//    5	2.1 2
//   ==================
//    6	3.5 4

    vector <double> h;
    frame--; //TODO frame!=1

    double value=0;
    for (int i=0; i<t.size()-1; i++) {
        value+=b.at(i);
        if( int(t.at(i)) != int(t.at(i+1))  )  { // eg. 1.2 => 1
            h.push_back(value);
            value=0;
        }
    }
    return h;
}

