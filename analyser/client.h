#pragma once


class Client
{
public:
    Client();

    virtual void updateHeater(double vh, double ih) = 0;
    virtual void testProgress(int progress) = 0;
    virtual void testFinished() = 0;
    virtual void testAborted() = 0;

    virtual void hvCalibrationSampleReady(int hv1Adc,
                                          int iaHi1Adc,
                                          int iaLo1Adc,
                                          int hv2Adc,
                                          int iaHi2Adc,
                                          int iaLo2Adc)
    {
        (void)hv1Adc;
        (void)iaHi1Adc;
        (void)iaLo1Adc;
        (void)hv2Adc;
        (void)iaHi2Adc;
        (void)iaLo2Adc;
    }
};

