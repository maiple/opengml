for (var i = 0; i < 3; ++i)
{
    switch(i)
    {
    case 0:
        background_color = i;
        break;
    case 1:
        background_color[0] = i;
        break;
    case 2:
        background_color[0, 0] = i;
        break;
    default:
        ogm_assert(false);
    }

    ogm_assert(background_color[0] == i)
    ogm_assert(background_color == i)
    ogm_assert(background_color[0, 0] == i)
}