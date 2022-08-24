#ifndef REGULATOR_H
#define REGULATOR_H

#include "regulator_global.h"
#include <math.h>
#include <QDebug>
#include <QElapsedTimer>
//#include "AudioInterface.h"
//#include "RingBuffer.h"
//#include "../hapitrip/hapitrip.h"
//#include <QMutex>
//#include <QMutexLocker>
typedef float sample_t; // from JackTrip

class BurgAlgorithm
{
public:
    bool classify(double d);
    void train(std::vector<long double>& coeffs, const std::vector<float>& x);
    void predict(std::vector<long double>& coeffs, std::vector<float>& tail);

private:
    // the following are class members to minimize heap memory allocations
    std::vector<long double> Ak;
    std::vector<long double> f;
    std::vector<long double> b;
};

class ChanData
{
public:
    ChanData(int i, int FPP, int hist);
    int ch;
    int trainSamps;
    std::vector<sample_t> mTruth;
    std::vector<sample_t> mTrain;
    std::vector<sample_t> mTail;
    std::vector<sample_t> mPrediction;  // ORDER
    std::vector<long double> mCoeffs;
    std::vector<sample_t> mXfadedPred;
    std::vector<sample_t> mLastPred;
    std::vector<std::vector<sample_t>> mLastPackets;
    std::vector<sample_t> mCrossFadeDown;
    std::vector<sample_t> mCrossFadeUp;
    std::vector<sample_t> mCrossfade;
};

class StdDev
{
public:
    StdDev(int id, QElapsedTimer* timer, int w);
    void tick();
    double calcAuto(double autoHeadroom, double localFPPdur);
    int mId;
    int plcOverruns;
    int plcUnderruns;
    double lastTime;
    double lastMean;
    double lastMin;
    double lastMax;
    int lastPlcOverruns;
    int lastPlcUnderruns;
    double lastPLCdspElapsed;
    double lastStdDev;
    double longTermStdDev;
    double longTermStdDevAcc;
    double longTermMax;
    double longTermMaxAcc;

private:
    void reset();
    QElapsedTimer* mTimer;
    std::vector<double> data;
    double mean;
    int window;
    double acc;
    double min;
    double max;
    int ctr;
    int longTermCnt;
};


class REGULATOR_EXPORT Regulator  {
public:
    Regulator(int rcvChannels, int bit_res, int FPP, int qLen);
    // wasThisWayInJackTrip     virtual
    ~Regulator();

    void shimFPP(int8_t* buf, int len, int seq_num);
    void pushPacket(int8_t* buf, int seq_num);
    // can hijack unused2 to propagate incoming seq num if needed
    // option is in UdpDataProtocol
    // if (!mJackTrip->writeAudioBuffer(src, host_buf_size, last_seq_num))
    // instead of
    // if (!mJackTrip->writeAudioBuffer(src, host_buf_size, gap_size))
    // wasThisWayInJackTrip virtual
    /*
    bool insertSlotNonBlockingRegulator(const int8_t* ptrToSlot,
                                        [[maybe_unused]] int len,
    [[maybe_unused]] int seq_num,
    [[maybe_unused]] int lostLen)
    {
        shimFPP(ptrToSlot, len, seq_num);
        return (true);
    }
    */

    void pullPacket(int8_t* buf);

    // wasThisWayInJackTrip    virtual
    void readSlotNonBlocking(int8_t* ptrToReadSlot) { pullPacket(ptrToReadSlot); }

    //    virtual QString getStats(uint32_t statCount, uint32_t lostCount);
    // wasThisWayInJackTrip    virtual bool getStats(IOStat* stat, bool reset);

private:
    void setFPPratio();
    bool mFPPratioIsSet;
    void processPacket(bool glitch);
    void processChannel(int ch, bool glitch, int packetCnt, bool lastWasGlitch);
    int mNumChannels;
    int mAudioBitRes;
    int mFPP;
    int mPeerFPP;
    uint32_t mLastLostCount;
    int mNumSlots;
    int mHist;
    // wasThisWayInJackTrip     AudioInterface::audioBitResolutionT mBitResolutionMode;
    BurgAlgorithm ba;
// wasThisWayInJackTrip      int mBytes;
    int mBytesPeerPacket;
    int8_t* mXfrBuffer;
    int8_t* mAssembledPacket;
    int mPacketCnt;
    sample_t bitsToSample(int ch, int frame);
    void sampleToBits(sample_t sample, int ch, int frame);
    std::vector<sample_t> mFadeUp;
    std::vector<sample_t> mFadeDown;
    bool mLastWasGlitch;
    std::vector<int8_t*> mSlots;
    int8_t* mZeros;
    double mMsecTolerance;
    std::vector<ChanData*> mChanData;
    StdDev* pushStat;
    StdDev* pullStat;
    QElapsedTimer mIncomingTimer;
    int mLastSeqNumIn;
    int mLastSeqNumOut;
    std::vector<double> mPhasor;
    std::vector<double> mIncomingTiming;
    int mModSeqNum;
    int mLostWindow;
    int mSkip;
    int mFPPratioNumerator;
    int mFPPratioDenominator;
    int mAssemblyCnt;
    int mModCycle;
    bool mAuto;
    int mModSeqNumPeer;
    double mAutoHeadroom;
    double mFPPdurMsec;
    void changeGlobal(double);
    void changeGlobal_2(int);
    void changeGlobal_3(int);
    void printParams();
    /// Pointer for the Receive RingBuffer
    // wasThisWayInJackTrip     RingBuffer* m_b_ReceiveRingBuffer;
    // wasThisWayInJackTrip     int m_b_BroadcastQueueLength;
//    QMutex mMutex;                     ///< Mutex to protect read and write operations
};

#endif // REGULATOR_H
