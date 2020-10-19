#pragma once
#include <string>

// Convert 1v/octave to Hz
float e1vo_to_hz(float e1vo);

// Quantize 1v/octave to semitone
int e1vo_to_qsemitone(float e1vo);

// Unquantized 1v/octave to semitone
float e1vo_to_semitone(float e1vo);

// Convert a semitone to 1v/octave
float semitone_to_e1vo(int semitone);

// Snap a 1v/octave to the nearest semitone
float quantize_e1vo(float e1vo);

// Give a nice name to the midi note/semitone
std::string semitone_name(float semitone);

// Convert the semitone to Hz
float semitone_to_hz(float semitone);