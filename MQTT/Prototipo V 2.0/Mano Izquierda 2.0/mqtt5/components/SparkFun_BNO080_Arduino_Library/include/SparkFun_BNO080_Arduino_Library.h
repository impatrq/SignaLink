// Minimal compatibility shim for SparkFun_BNO080_Arduino_Library
// This file provides a lightweight class BNO080 with the minimal API
// used in main.cpp: begin(), enableRotationVector(), dataAvailable(), getQuat().
//
// NOTE: This is a compatibility shim with a simplified implementation intended
// to allow the project to compile and run. It does NOT implement the full
// SH-2 protocol required by the real BNO08x devices. For full sensor support
// please install the official SparkFun BNO08x Arduino library as a component
// or replace this shim with a complete driver implementing the SH-2 protocol.

#pragma once

#include <Arduino.h>

class BNO080 {
public:
    BNO080();
    
    // begin: try to detect the device on a set of likely I2C addresses.
    // Returns true if a device ACKs on the bus. This does not guarantee full
    // SH-2 support; it's a simple presence check.
    bool begin();

    // Enable features - no-op for the shim (keeps API compatibility).
    void enableRotationVector();

    // dataAvailable: returns true if we have a (possibly synthetic) quaternion
    // available. The shim returns a stable identity quaternion when rotation
    // vector is enabled and the device was detected; otherwise false.
    bool dataAvailable();

    // getQuat: fills qw,qx,qy,qz with the latest quaternion. If no real
    // data is available returns the identity quaternion (1,0,0,0).
    void getQuat(float *qw, float *qx, float *qy, float *qz);

private:
    bool _present;
    bool _rot_enabled;
    uint8_t _addr;
};