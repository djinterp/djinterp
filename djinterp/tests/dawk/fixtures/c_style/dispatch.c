/* dispatch.c: the length tripwire's exemption -- a body that is one switch,
   with at most a return after it, is the dispatch table the guides let run
   long (decision 57, ruled 2026.10.11) */
#include <stddef.h>  // size_t


/*
d_dispatch_switch_only
  negative: one switch and nothing else, past 75 lines.
*/
int
d_dispatch_switch_only(
    int _x
)
{
    switch (_x)
    {
        case 0:
            return 0;
        case 1:
            return 3;
        case 2:
            return 6;
        case 3:
            return 9;
        case 4:
            return 12;
        case 5:
            return 15;
        case 6:
            return 18;
        case 7:
            return 21;
        case 8:
            return 24;
        case 9:
            return 27;
        case 10:
            return 30;
        case 11:
            return 33;
        case 12:
            return 36;
        case 13:
            return 39;
        case 14:
            return 42;
        case 15:
            return 45;
        case 16:
            return 48;
        case 17:
            return 51;
        case 18:
            return 54;
        case 19:
            return 57;
        case 20:
            return 60;
        case 21:
            return 63;
        case 22:
            return 66;
        case 23:
            return 69;
        case 24:
            return 72;
        case 25:
            return 75;
        case 26:
            return 78;
        case 27:
            return 81;
        case 28:
            return 84;
        case 29:
            return 87;
        case 30:
            return 90;
        case 31:
            return 93;
        case 32:
            return 96;
        case 33:
            return 99;
        case 34:
            return 102;
        case 35:
            return 105;
        default:
            break;
    }
}

/*
d_dispatch_switch_return
  negative: one switch, then a return, past 75 lines.
*/
int
d_dispatch_switch_return(
    int _x
)
{
    switch (_x)
    {
        case 0:
            return 0;
        case 1:
            return 3;
        case 2:
            return 6;
        case 3:
            return 9;
        case 4:
            return 12;
        case 5:
            return 15;
        case 6:
            return 18;
        case 7:
            return 21;
        case 8:
            return 24;
        case 9:
            return 27;
        case 10:
            return 30;
        case 11:
            return 33;
        case 12:
            return 36;
        case 13:
            return 39;
        case 14:
            return 42;
        case 15:
            return 45;
        case 16:
            return 48;
        case 17:
            return 51;
        case 18:
            return 54;
        case 19:
            return 57;
        case 20:
            return 60;
        case 21:
            return 63;
        case 22:
            return 66;
        case 23:
            return 69;
        case 24:
            return 72;
        case 25:
            return 75;
        case 26:
            return 78;
        case 27:
            return 81;
        case 28:
            return 84;
        case 29:
            return 87;
        case 30:
            return 90;
        case 31:
            return 93;
        case 32:
            return 96;
        case 33:
            return 99;
        case 34:
            return 102;
        case 35:
            return 105;
        default:
            break;
    }

    return -1;
}

/*
d_dispatch_declaration_first
  positive: a declaration before the switch.
*/
int
d_dispatch_declaration_first(
    int _x
)
{
    const int y = _x;

    (void)y;

    switch (_x)
    {
        case 0:
            return 0;
        case 1:
            return 3;
        case 2:
            return 6;
        case 3:
            return 9;
        case 4:
            return 12;
        case 5:
            return 15;
        case 6:
            return 18;
        case 7:
            return 21;
        case 8:
            return 24;
        case 9:
            return 27;
        case 10:
            return 30;
        case 11:
            return 33;
        case 12:
            return 36;
        case 13:
            return 39;
        case 14:
            return 42;
        case 15:
            return 45;
        case 16:
            return 48;
        case 17:
            return 51;
        case 18:
            return 54;
        case 19:
            return 57;
        case 20:
            return 60;
        case 21:
            return 63;
        case 22:
            return 66;
        case 23:
            return 69;
        case 24:
            return 72;
        case 25:
            return 75;
        case 26:
            return 78;
        case 27:
            return 81;
        case 28:
            return 84;
        case 29:
            return 87;
        case 30:
            return 90;
        case 31:
            return 93;
        case 32:
            return 96;
        case 33:
            return 99;
        case 34:
            return 102;
        case 35:
            return 105;
        default:
            break;
    }
}

/*
d_dispatch_two_after
  positive: two statements after the switch.
*/
int
d_dispatch_two_after(
    int _x
)
{
    switch (_x)
    {
        case 0:
            return 0;
        case 1:
            return 3;
        case 2:
            return 6;
        case 3:
            return 9;
        case 4:
            return 12;
        case 5:
            return 15;
        case 6:
            return 18;
        case 7:
            return 21;
        case 8:
            return 24;
        case 9:
            return 27;
        case 10:
            return 30;
        case 11:
            return 33;
        case 12:
            return 36;
        case 13:
            return 39;
        case 14:
            return 42;
        case 15:
            return 45;
        case 16:
            return 48;
        case 17:
            return 51;
        case 18:
            return 54;
        case 19:
            return 57;
        case 20:
            return 60;
        case 21:
            return 63;
        case 22:
            return 66;
        case 23:
            return 69;
        case 24:
            return 72;
        case 25:
            return 75;
        case 26:
            return 78;
        case 27:
            return 81;
        case 28:
            return 84;
        case 29:
            return 87;
        case 30:
            return 90;
        case 31:
            return 93;
        case 32:
            return 96;
        case 33:
            return 99;
        case 34:
            return 102;
        case 35:
            return 105;
        default:
            break;
    }

    _x = _x + 1;

    return _x;
}

/*
d_dispatch_conditional
  positive: the switch inside a conditional (which the
conditional rule also reports).
*/
int
d_dispatch_conditional(
    int _x
)
{
#if defined(D_DISPATCH_FIXTURE)
    switch (_x)
    {
        case 0:
            return 0;
        case 1:
            return 3;
        case 2:
            return 6;
        case 3:
            return 9;
        case 4:
            return 12;
        case 5:
            return 15;
        case 6:
            return 18;
        case 7:
            return 21;
        case 8:
            return 24;
        case 9:
            return 27;
        case 10:
            return 30;
        case 11:
            return 33;
        case 12:
            return 36;
        case 13:
            return 39;
        case 14:
            return 42;
        case 15:
            return 45;
        case 16:
            return 48;
        case 17:
            return 51;
        case 18:
            return 54;
        case 19:
            return 57;
        case 20:
            return 60;
        case 21:
            return 63;
        case 22:
            return 66;
        case 23:
            return 69;
        case 24:
            return 72;
        case 25:
            return 75;
        case 26:
            return 78;
        case 27:
            return 81;
        case 28:
            return 84;
        case 29:
            return 87;
        case 30:
            return 90;
        case 31:
            return 93;
        case 32:
            return 96;
        case 33:
            return 99;
        case 34:
            return 102;
        case 35:
            return 105;
        default:
            break;
    }
#endif

    return _x;
}
