///// list data structure /////

FNDEF0(parameter_count)
FNDEF1(parameter_string, s)
FNDEF1(environment_get_variable, v)
FNDEF0(get_timer)
GETVAR(delta_time)
GETVAR(fps_real)
GETVAR(fps)
VAR(score)
VAR(health)
VAR(lives)
GETVAR(debug_mode)

// URL
FNDEF0(url_get_href)
FNDEF0(url_get_domain)
FNDEF1(url_open, url)
FNDEF2(url_open_ext, url, target)
FNDEF3(url_open_full, url, target, opts)