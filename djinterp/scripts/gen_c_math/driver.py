"""gen_c_math.driver - the outputs and the script's main (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .common import (CORE, ROOT, align_params, no_contract, os)
from .float_order import (float_order, interval_float)
from .elementary import (elementary)
from .exponential import (exponential)
from .tangent import (tangent)
from .hyperbolic import (hyperbolic)
from .power import (power)
from .remainder import (remainder)
from .nearest_integer import (nearest_integer)
from .scalar import (scalar)
from .vec import (vec)
from .mat import (mat)
from .quat import (quat)
from .coord import (coord)
from .isect import (isect)
from .plane import (plane)
from .decomp import (decomp)
from .cplx import (cplx)
from .cmat import (cmat)
from .dd import (dd)
from .self_test import (self_test)
from .split import (SPLITS, split_header)


OUTPUTS = {
    "dd.h": dd,
    "self_test.h": self_test,
    "float_order.h": float_order,
    "interval_float.h": interval_float,
    "elementary.h": elementary,
    "exponential.h": exponential,
    "tangent.h": tangent,
    "hyperbolic.h": hyperbolic,
    "power.h": power,
    "remainder.h": remainder,
    "nearest_integer.h": nearest_integer,
    "scalar.h": scalar,
    "vec.h": vec,
    "mat.h": mat,
    "quat.h": quat,
    "coord.h": coord,
    "isect.h": isect,
    "plane.h": plane,
    "decomp.h": decomp,
    "cplx.h": cplx,
    "cmat.h": cmat,
}


def outputs():
    """{file: text}: every header the script writes, the largest in parts."""
    files = {}
    for name, make in OUTPUTS.items():
        text = align_params(no_contract(make()))
        if name in SPLITS:
            files.update(split_header(name, text, SPLITS[name]))
        else:
            files[name] = text
    return files


def main(argv):
    check = "--check" in argv
    status = 0
    for name, text in outputs().items():
        rel = CORE + "/" + name
        path = os.path.join(ROOT, rel)
        if check:
            try:
                with open(path) as f:
                    current = f.read()
            except FileNotFoundError:
                current = None
            if current != text:
                print("differs: " + rel)
                status = 1
        else:
            with open(path, "w") as f:
                f.write(text)
            print("wrote " + rel)
    return status
