#pragma once

namespace daisy {
class DaisyPatch;
}

// Runs a calibration mode.
void RunCalibration(daisy::DaisyPatch* patch);

// Read the CV input while applying calibration map. Returns a voltage in the
// range of 0-5V
float ReadCalibratedCV(daisy::DaisyPatch* patch, int cv);