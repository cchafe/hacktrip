#include "burgalgorithm.h"
#include <QDebug>
#include <stk/Stk.h>

using namespace std;
using namespace stk;


QString qStringFromLongDouble1(const DBL myLongDouble)
{
    std::stringstream ss;
    ss << myLongDouble;

    return QString::fromStdString(ss.str());
}

bool classify(double d)
{
    bool tmp = false;
    switch (fpclassify(d)) {
    case FP_INFINITE:  qDebug() <<  ("infinite");  tmp = true; break;
    case FP_NAN:       qDebug() <<  ("NaN");  tmp = true;        break;
    case FP_ZERO:      qDebug() <<  ("zero");  tmp = true;       break;
    case FP_SUBNORMAL: qDebug() <<  ("subnormal");  tmp = true;  break;
        //    case FP_NORMAL:    qDebug() <<  ("normal");    break;
    }
    //  if (signbit(d)) qDebug() <<  (" negative\n"); else qDebug() <<  (" positive or unsigned\n");
    return tmp;
}

BurgAlgorithm::BurgAlgorithm(vector<DBL> &coeffs, size_t size)
{
    // GET SIZE FROM INPUT VECTORS
    m = coeffs.size();
    N = size - 1;
    this->size = size;
    if (size < m)
        qDebug() << "time_series should have more elements than the AR order is";
    Ak.resize( m + 1 );
    for (size_t i = 0; i < m + 1; i++)
        Ak[i] = 0.0;
    AkReset.resize( m + 1 );
    AkReset = Ak;
    AkReset[ 0 ] = 1.0;
    f.resize(size);
    b.resize(size);

}

// from .pl
void BurgAlgorithm::train(vector<DBL> &coeffs, const vector<float> &x, int pCnt, size_t size )
{


    ////

    // INITIALIZE Ak
    Ak = AkReset;

    // INITIALIZE f and b
    for ( size_t i = 0; i < size; i++ )
        f[i] = b[i] = x[i];

    // INITIALIZE Dk
    DBL Dk = 0.0; // was double
    for ( size_t j = 0; j <= N; j++ )
    {
        // Dk += 3.0 * f[ j ] * f[ j ]; // needs more damping than orig 2.0
        Dk += 2.00002 * f[ j ] * f[ j ]; // needs more damping than orig 2.0
        // Dk += 2.00003 * f[ j ] * f[ j ]; // needs more damping than orig 2.0
        // eliminate overflow Dk += 2.0001 * f[ j ] * f[ j ]; // needs more damping than orig 2.0

        // JT >>
        // Dk += 2.00001 * f[j] * f[j];  // CC: needs more damping than orig 2.0

        // was >>
        // Dk += 2.0000001 * f[ j ] * f[ j ]; // needs more damping than orig 2.0
    }
    Dk -= f[ 0 ] * f[ 0 ] + b[ N ] * b[ N ];

    //// N is $#x-1 in C++ but $#x in perl
    //    my $Dk = sum map {
    //        2.0 * $f[$_] ** 2
    //    } 0 .. $#f;
    //    $Dk -= $f[0] ** 2 + $B[$#x] ** 2;

    //    qDebug() << "Dk" << qStringFromLongDouble1(Dk);
    if ( classify(Dk) )
    { qDebug() << pCnt << "init";
    }

    // BURG RECURSION
    for ( size_t k = 0; k < m; k++ )
    {
        // COMPUTE MU
        DBL mu = 0.0;
        for ( size_t n = 0; n <= N - k - 1; n++ )
        {
            mu += f[ n + k + 1 ] * b[ n ];
        }

        if ( Dk == 0.0 ) Dk = 0.0000001; // from online testing
        if ( classify(Dk) )
        { qDebug() << pCnt << "run";
        }
        mu *= -2.0 / Dk;
        //            if ( isnan(Dk) )  { qDebug() << "k" << k; }

        //            if (Dk!=0.0) {}
        //        else qDebug() << "k" << k << "Dk==0" << qStringFromLongDouble1(Dk);

        //// N is $#x-1
        //# compute mu
        //my $mu = sum map {
        //    $f[$_ + $k + 1] * $B[$_]
        //} 0 .. $#x - $k - 1;
        //$mu *= -2.0 / $Dk;


        // UPDATE Ak
        for ( size_t n = 0; n <= ( k + 1 ) / 2; n++ )
        {
            DBL t1 = Ak[ n ] + mu * Ak[ k + 1 - n ];
            DBL t2 = Ak[ k + 1 - n ] + mu * Ak[ n ];
            Ak[ n ] = t1;
            Ak[ k + 1 - n ] = t2;
        }

        // UPDATE f and b
        for ( size_t n = 0; n <= N - k - 1; n++ )
        {
            DBL t1 = f[ n + k + 1 ] + mu * b[ n ]; // were double
            DBL t2 = b[ n ] + mu * f[ n + k + 1 ];
            f[ n + k + 1 ] = t1;
            b[ n ] = t2;
        }

        // UPDATE Dk
        Dk = ( 1.0 - mu * mu ) * Dk
             - f[ k + 1 ] * f[ k + 1 ]
             - b[ N - k - 1 ] * b[ N - k - 1 ];

    }
    // ASSIGN COEFFICIENTS
    coeffs.assign( ++Ak.begin(), Ak.end() );

    //    return $self->_set_coefficients([ @Ak[1 .. $#Ak] ]);

}

void BurgAlgorithm::predict( vector<DBL> &coeffs, vector<float> &tail )
{
    //    qDebug() << "tail.at(0)" << tail[0]*32768;
    //    qDebug() << "tail.at(1)" << tail[1]*32768;
    // tail.resize(m+tail.size()); size it in main instead
    //    qDebug() << "tail.at(m)" << tail[m]*32768;
    //    qDebug() << "tail.at(...end...)" << tail[tail.size()-1]*32768;
    //    qDebug() << "m" << m << "tail.size()" << tail.size();
    for ( size_t i = m; i < tail.size(); i++ )
    {
        tail[ i ] = 0.0;
        for ( size_t j = 0; j < m; j++ )
        {
            tail[ i ] -= coeffs[ j ] * tail[ i - 1 - j ];
        }
    }
}
