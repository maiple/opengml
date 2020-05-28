///// list data structure /////

FNDEF1(audio_is_playing, audio)
FNDEF1(audio_is_paused, audio)
FNDEF1(audio_exists, audio)
FNDEF1(audio_get_name, audio)
FNDEF3(audio_play_sound, s, priority, loop)
FNDEF1(audio_stop_sound, s)
FNDEF0(audio_stop_all)
FNDEF1(audio_pause_sound, s)
FNDEF0(audio_pause_all)
FNDEF1(audio_resume_sound, s)
FNDEF0(audio_resume_all)
FNDEF2(audio_sound_set_track_position, s, p)
FNDEF1(audio_sound_get_track_position, s)
FNDEF1(audio_sound_length, s)
FNDEF2(audio_sound_pitch, s, p)
FNDEF1(audio_sound_get_pitch, s)
FNDEF3(audio_sound_gain, s, v, t)
FNDEF1(audio_sound_get_gain, s)
FNDEF1(audio_master_gain, v)
FNDEF1(audio_channel_num, n)

FNDEF0(audio_system)

ALIAS(audio_pause_all, pause_all_audio)

CONST(audio_mono, 1)
CONST(audio_stereo, 2)
CONST(audio_3D, 3)

CONST(audio_old_system, 0)
CONST(audio_new_system, 1)

// these just call the above
FNDEF1(sound_play, s)
FNDEF1(sound_exists, s)
FNDEF1(sound_get_name, audio)
FNDEF1(sound_loop, audio)
FNDEF1(sound_stop, audio)
FNDEF0(sound_stop_all)
FNDEF1(sound_is_playing, audio)
ALIAS(sound_is_playing, sound_isplaying)