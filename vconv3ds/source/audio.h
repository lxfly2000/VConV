#pragma once

#define AUDIO_ID_SUCCESS 1
#define AUDIO_ID_ERROR 0

void audio_init();
void audio_release();
void audio_play(int id);
