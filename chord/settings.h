#pragma once

#include <cstdint>


struct ControlCalibration {
  // Values at 0 through 5V at 1V increments
  float map[6];
};

struct Settings {
  uint32_t signature;
  uint32_t version;
  ControlCalibration cals[4];
};

extern Settings gSettings;

namespace daisy{ class DaisySeed; }
void SaveSettings(daisy::DaisySeed* seed);
void LoadSettings(daisy::DaisySeed* seed);