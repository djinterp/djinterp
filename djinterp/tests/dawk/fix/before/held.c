/* held.c: a shift that can never be made does not hold the rest back */
#include "./held.h"


/*
d_held_switch
  The second label's statement has no room to move, so the switch stays where
it is on every run. The brace, the empty line and the summary are put right
all the same: they wait for no shift that will not come.
*/
int
d_held_switch(
    int _kind
)
{
    const char* text = "";

    switch (_kind)
    {
    case 1:
        text = "short";

        break;

    case 2:
        text = "a line that four columns more would carry beyond the eightieth";

        break;

    default:
        break;
    }

    if (_kind > 2) {
        text = "more";
    }
    return (text[0] != 0);
}
