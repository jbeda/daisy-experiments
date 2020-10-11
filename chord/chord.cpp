// This is based on the DaisyExamples PolyOsc example.

#include <string>

#include "daisy_patch.h"
#include "daisysp.h"
#include "notes.h"

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;

Oscillator osc[3];

std::string waveNames[5];

int waveform;
int final_wave;

const int   base_note      = 10;  // A0
const float voltage_offset = semitone_to_e1vo(base_note);

float analog_to_voltage(float analog) { return analog * 5.0f; }

void UpdateControls();

static void AudioCallback(float **in, float **out, size_t size) {
  UpdateControls();
  for (size_t i = 0; i < size; i++) {
    float mix = 0;
    // Process and output the three oscillators
    for (size_t chn = 0; chn < 3; chn++) {
      float sig = osc[chn].Process();
      mix += sig * .33f;
      out[chn][i] = sig;
    }

    // output the mixed oscillators
    out[3][i] = mix;
  }
}

void SetupOsc(float samplerate) {
  for (int i = 0; i < 3; i++) {
    osc[i].Init(samplerate);
    osc[i].SetAmp(.7);
  }
}

void SetupWaveNames() {
  waveNames[0] = "sine";
  waveNames[1] = "triangle";
  waveNames[2] = "saw";
  waveNames[3] = "ramp";
  waveNames[4] = "square";
}

void UpdateOled();

int main(void) {
  float samplerate;

  patch.Init();  // Initialize hardware (daisy seed, and patch)
  samplerate = patch.AudioSampleRate();

  waveform   = 0;
  final_wave = Oscillator::WAVE_POLYBLEP_TRI;

  SetupOsc(samplerate);
  SetupWaveNames();

  patch.StartAdc();
  patch.StartAudio(AudioCallback);
  while (1) {
    UpdateOled();
  }
}

void UpdateOled() {
  patch.display.Fill(false);

  patch.display.SetCursor(0, 0);

  patch.display.WriteString("0xBEDA Chord", Font_7x10, true);

  patch.display.SetCursor(0, 20);
  patch.display.WriteString("waveform: ", Font_7x10, true);
  patch.display.WriteString(waveNames[waveform].c_str(), Font_7x10, true);

  patch.display.SetCursor(0, 40);
  char  val[50];
  float fundamental =
      analog_to_voltage(patch.GetCtrlValue(DaisyPatch::CTRL_4)) +
      voltage_offset;

  float       hz        = e1vo_to_hz(fundamental);
  std::string note_name = semitone_name(e1vo_to_semitone(fundamental));

  snprintf(val, 20, "Fund: %0.0f Hz %s", hz, note_name.c_str());
  patch.display.WriteString(val, Font_7x10, true);

  patch.display.Update();
}

void UpdateControls() {
  patch.DebounceControls();
  patch.UpdateAnalogControls();

  // knobs
  float ctrl[4];
  for (int i = 0; i < 4; i++) {
    ctrl[i] = patch.GetCtrlValue((DaisyPatch::Ctrl)i);
  }

  // Fundamental in 1v/octive
  float fundamental = analog_to_voltage(ctrl[3]) + voltage_offset;
  fundamental       = quantize_e1vo(fundamental);

  for (int i = 0; i < 3; i++) {
    ctrl[i] = analog_to_voltage(ctrl[i]);  // voltage
    ctrl[i] += fundamental;
    ctrl[i] = e1vo_to_hz(ctrl[i]);  // Hz
  }

  // encoder
  waveform += patch.encoder.Increment();
  waveform = (waveform % final_wave + final_wave) % final_wave;

  // Adjust oscillators based on inputs
  for (int i = 0; i < 3; i++) {
    osc[i].SetFreq(ctrl[i]);
    osc[i].SetWaveform((uint8_t)waveform);
  }
}
