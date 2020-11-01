#include "calibration.h"

#include <string>

#include "daisy_patch.h"
#include "settings.h"

using namespace daisy;

void RunCalibration(daisy::DaisyPatch* patch) {
  // Run in a loop where we read the controls, process and update the oled

  // The amount of time each loop should take
  uint32_t update_span_ms =
      static_cast<uint32_t>((1.0f / patch->AudioSampleRate()) * 1000);

  // Wait for things to settle
  for (int settle = 0; settle < 1000; settle++) {
    uint32_t start = dsy_system_getnow();

    patch->DebounceControls();
    patch->UpdateAnalogControls();

    uint32_t end       = dsy_system_getnow();
    uint32_t span_used = end - start;
    if (span_used <= update_span_ms) {
      dsy_system_delay(update_span_ms - span_used);
    }
  }

  // Only enter calibration mode if the encoder is pressed.  If not, early exit.
  if (!patch->encoder.Pressed()) {
    return;
  }

  // Memory to hold the current value
  float ctrl_values[4];

  // initialize calibration
  for (int i = 0; i < 4; i++) {
    ctrl_values[0]        = 0.5;
    gSettings.cals[i].min = 0.5;
    gSettings.cals[i].max = 0.5;
  }

  while (true) {
    uint32_t start = dsy_system_getnow();

    patch->DebounceControls();
    patch->UpdateAnalogControls();

    if (patch->encoder.RisingEdge()) {
      break;
    }

    // Read all of the values and track min/max
    for (int i = 0; i < 4; i++) {
      float val = ctrl_values[i] = patch->GetCtrlValue((DaisyPatch::Ctrl)i);
      if (val < gSettings.cals[i].min) {
        gSettings.cals[i].min = val;
      }
      if (val > gSettings.cals[i].max) {
        gSettings.cals[i].max = val;
      }
    }

    // Update OLED
    char line[100];
    patch->display.Fill(false);
    patch->display.SetCursor(0, 0);
    patch->display.WriteString("0xBEDA Calibration", Font_6x8, true);

    for (int i = 0; i < 4; i++) {
      patch->display.SetCursor(0, 10 + i * 10);
      snprintf(line, 100, "%0.3f %0.3f %0.3f", ctrl_values[i],
               gSettings.cals[i].min, gSettings.cals[i].max);
      patch->display.WriteString(line, Font_6x8, true);
    }
    
    patch->display.Update();

    uint32_t end       = dsy_system_getnow();
    uint32_t span_used = end - start;
    if (span_used <= update_span_ms) {
      dsy_system_delay(update_span_ms - span_used);
    }
  }

  // Write the calibration settings out
  SaveSettings(&patch->seed);
}
