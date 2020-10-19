#include "notes.h"

#include <string>

#include "daisy.h"
#include "daisysp.h"

float e1vo_to_hz(float e1vo) { return semitone_to_hz(e1vo * 12); }

int e1vo_to_qsemitone(float e1vo) {
  return static_cast<int>(std::round(e1vo * 12.0));
}

float e1vo_to_semitone(float e1vo) {
  return e1vo * 12.0;
}

float semitone_to_e1vo(int semitone) {
  return static_cast<float>(semitone) / 12.0f;
}

float quantize_e1vo(float e1vo) {
  return semitone_to_e1vo(e1vo_to_semitone(e1vo));
}

const char* semitones[] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B",
};

std::string semitone_name(float fSemitone) {
  // C0 is 12
  // B8 is 119
  int semitone = static_cast<int>(std::round(fSemitone));
  if (semitone < 12 || semitone > 119) {
    return "???";
  }

  float cents = fSemitone - semitone;

  semitone -= 12;
  int octave = semitone / 12;
  int note   = semitone % 12;

  std::string out(semitones[note]);
  out.push_back('0' + octave);

  if (cents >= 0.15) {
    out.push_back('+');
  } else if (cents <= -0.15) {
    out.push_back('-');
  }
  return out;
}

float semitone_to_hz(float semitone) { return daisysp::mtof(semitone); }