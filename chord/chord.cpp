// This is based on the DaisyExamples PolyOsc example.

#include <string>

#include "calibration.h"
#include "daisy_patch.h"
#include "daisysp.h"
#include "notes.h"
#include "settings.h"

using namespace daisy;
using namespace daisysp;

DaisyPatch gPatch;

class ChordApp {
 public:
  void        Init(DaisyPatch *_patch);
  static void StaticAudioCallback(float **in, float **out, size_t size);
  void        AudioCallback(float **in, float **out, size_t size);
  void        UpdateControls();
  void        UpdateOled();

  void Run();

  float ReadControlInV(int i) { return ReadCalibratedCV(patch, i); }

  const int   base_note      = 24;  // C1
  const float voltage_offset = semitone_to_e1vo(base_note);
  const char *waveNames[5]   = {"sine", "triangle", "saw", "ramp", "square"};
  const int   final_wave     = Oscillator::WAVE_POLYBLEP_TRI;

  DaisyPatch *patch;

  int        waveform;
  float      fund_st;    // Fundamental frequency in semitones
  float      osc_st[3];  // Semitone freq for derived oscillators
  Oscillator osc[3];
};

ChordApp app;

void ChordApp::Init(DaisyPatch *_patch) {
  patch    = _patch;
  waveform = 0;

  auto samplerate = patch->AudioSampleRate();
  for (int i = 0; i < 3; i++) {
    osc[i].Init(samplerate);
    osc[i].SetAmp(.7);
  }
}

void ChordApp::Run() {
  patch->StartAudio(StaticAudioCallback);

  while (1) {
    UpdateOled();
  }
}
void ChordApp::StaticAudioCallback(float **in, float **out, size_t size) {
  app.AudioCallback(in, out, size);
}

void ChordApp::AudioCallback(float **in, float **out, size_t size) {
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

void ChordApp::UpdateOled() {
  patch->display.Fill(false);

  patch->display.SetCursor(0, 0);
  patch->display.WriteString("0xBEDA Chord", Font_6x8, true);

  patch->display.SetCursor(0, 10);
  patch->display.WriteString("waveform: ", Font_6x8, true);
  patch->display.WriteString(waveNames[waveform], Font_7x10, true);

  patch->display.SetCursor(0, 20);
  char        val[50];
  std::string note_name = semitone_name(fund_st);
  snprintf(val, 50, "Fund: %0.0f Hz %s", semitone_to_hz(fund_st),
           note_name.c_str());
  patch->display.WriteString(val, Font_6x8, true);

  patch->display.SetCursor(0, 30);
  snprintf(val, 50, "%4s %4s %4s", semitone_name(osc_st[0]).c_str(),
           semitone_name(osc_st[1]).c_str(), semitone_name(osc_st[2]).c_str());
  patch->display.WriteString(val, Font_6x8, true);

  patch->display.Update();
}

void ChordApp::UpdateControls() {
  patch->DebounceControls();
  patch->UpdateAnalogControls();

  // encoder
  waveform += patch->encoder.Increment();
  waveform = (waveform % final_wave + final_wave) % final_wave;

  // Figure out the fundamental frequency
  float fund_e1vo = ReadControlInV(3) + voltage_offset;
  fund_st         = e1vo_to_qsemitone(fund_e1vo);

  // Tune each osc
  for (int i = 0; i < 3; i++) {
    float osc_e1vo = ReadControlInV(i);
    osc_st[i]      = e1vo_to_semitone(osc_e1vo) + fund_st;
    float osc_hz   = semitone_to_hz(osc_st[i]);
    osc[i].SetFreq(osc_hz);
    osc[i].SetWaveform((uint8_t)waveform);
  }
}

int main(void) {
  gPatch.Init();  // Initialize hardware (daisy seed, and patch)
  gPatch.StartAdc();

  RunCalibration(&gPatch);

  LoadSettings(&gPatch.seed);

  app.Init(&gPatch);
  app.Run();
}