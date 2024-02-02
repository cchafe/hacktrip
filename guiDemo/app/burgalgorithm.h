#ifndef BURGALGORITHM_H
#define BURGALGORITHM_H

#include <math.h>
#include <vector>
using namespace std;
#define DBL  float
class BurgAlgorithm
{
public:
    BurgAlgorithm(vector<DBL> &coeffs, int size);
    void train(vector<DBL> &coeffs, const vector<float> &x, int pCnt, int size);
    void predict( vector<DBL> &coeffs, vector<float> &predicted );
private:
    size_t m;
    size_t N;
    int size;
    vector<DBL> Ak;
    vector<DBL> AkReset;
    vector<DBL> f;
    vector<DBL> b;
};

#endif // BURGALGORITHM_H
