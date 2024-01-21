#ifndef BURGALGORITHM_H
#define BURGALGORITHM_H

#include <math.h>
#include <vector>
using namespace std;
#define DBL  double
class BurgAlgorithm
{
public:
    BurgAlgorithm();
    void compute( vector<DBL> &coeffs, const vector<double> &x );
    void train(vector<DBL> &coeffs, const vector<float> &x, int pCnt, int size);
    void predict( vector<DBL> &coeffs, vector<float> &predicted );
};

#endif // BURGALGORITHM_H
