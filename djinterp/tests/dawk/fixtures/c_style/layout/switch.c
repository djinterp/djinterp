/* switch.c: what stands under a case label, and where */

/*
d_switch
  A statement under a label is one level in from the label. The label's own
braced body stands on the label's column; a block further down the same case
is a statement among the others.
*/
int
d_switch(
    int _kind
)
{
    int result = 0;

    switch (_kind)
    {
        case 1:
            result = 1;

            break;

        case 2:
        case 3:
        {
            result = 2;

            break;
        }

        case 4:
            result = 4;

            {
                int twice = result * 2;

                result = twice;
            }

            break;

        case 5:
            result = 5;

        {
            result = 6;
        }

            break;

      case 6:
            break;

        default:
              result = -1;

            break;
    }

    return result;
}
