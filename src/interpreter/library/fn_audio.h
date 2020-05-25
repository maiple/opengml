///// list data structure /////

FNDEF1(audio_is_playing, audio)

FNDEF1(audio_play_sound, s)
FNDEF3(audio_play_sound, s, priority, loop)
FNDEF3(audio_sound_gain, s, volume, time)
FNDEF1(audio_stop_sound, s)
FNDEF1(audio_set_gain, s)
FNDEF1(audio_set_gain, s)
FNDEF0(audio_system)
FNDEF0(audio_pause_all)

ALIAS(audio_pause_all, pause_all_audio)

FNDEF1(play_sound, s)

CONST(audio_mono, 1)
CONST(audio_stereo, 2)
CONST(audio_3D, 3)

CONST(audio_old_system, 0)
CONST(audio_new_system, 1)