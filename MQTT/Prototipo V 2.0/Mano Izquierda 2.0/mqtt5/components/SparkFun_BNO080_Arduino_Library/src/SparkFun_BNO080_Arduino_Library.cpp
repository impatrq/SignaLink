/*
  Minimal shim implementation for BNO080 used by main.cpp.
  - Detects possible I2C addresses for BNO08x modules and returns success
    from begin() if a device ACKs.
  - Provides enableRotationVector(), dataAvailable() and getQuat() with a
    simple behavior so the code using the SparkFun API compiles and runs.

  Limitations: this does NOT implement the SH-2 binary protocol nor the
  full feature set of the SparkFun library. Replace with the official
  SparkFun_BNO080_Arduino_Library for real sensor fusion data.
*/

#include "SparkFun_BNO080_Arduino_Library.h"
#include <Wire.h>

// Common addresses that BNO08x (SparkFun) breakout may use (7-bit)
static const uint8_t possible_addrs[] = {0x4A, 0x4B, 0x28};

BNO080::BNO080()
{
    _present = false;
    _rot_enabled = false;
    _addr = 0;
}

bool BNO080::begin()
{
    // Use Wire to probe addresses
    for (size_t i = 0; i < sizeof(possible_addrs); ++i)
    {
        uint8_t a = possible_addrs[i];
        Wire.beginTransmission(a);
        int r = Wire.endTransmission();
        if (r == 0)
        {
            _present = true;
            _addr = a;
            return true;
        }
    }
    _present = false;
    return false;
}

void BNO080::enableRotationVector()
{
    // Shim: remember user requested rotation vector. No real configuration
    // of the device is performed here. For full support use the official
    // SparkFun library.
    _rot_enabled = true;
}

bool BNO080::dataAvailable()
{
    // Shim policy: if device was detected and rotation vector was requested
    // return true so getQuat() is called. getQuat will provide a stable
    // identity quaternion if no real data available.
    return _present && _rot_enabled;
}

void BNO080::getQuat(float *qw, float *qx, float *qy, float *qz)
{
    if (!qw || !qx || !qy || !qz)
        return;

    // Identity quaternion as fallback
    *qw = 1.0f;
    *qx = 0.0f;
    *qy = 0.0f;
    *qz = 0.0f;

    // NOTE: a real implementation must parse SH-2 messages coming from the
    // sensor and fill the quaternion with measured values. This shim keeps
    // the data stable so the rest of the app runs. Replace with the real
    // SparkFun library for meaningful orientation data.
}