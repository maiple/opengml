///// list data structure /////

FNDEF1(audio_is_playing, audio)

FNDEF1(audio_play_sound, s)
FNDEF3(audio_play_sound, s, priority, loop)
FNDEF3(audio_sound_gain, s, volume, time)
FNDEF1(audio_stop_sound, s)
FNDEF1(audio_set_gain, s)
FNDEF1(audio_set_gain, s)

FNDEF1(play_sound, s)

CONST(audio_mono, 1)
CONST(audio_stereo, 2)
CONST(audio_3D, 3)
