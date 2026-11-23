"""
Postolje figurice sa ugradjenom NFC nalepnicom.

Stampa se dnom na podlozi. Nalepnica lezi u zatvorenom udubljenju odmah iznad
tankog dna: stampac se pauzira pre sloja koji zatvara udubljenje, nalepnica se
zalepi, i stampac nastavi preko nje.

Pokretanje:  .venv\\Scripts\\python.exe postolje.py      -> stl\\postolje.stl
"""
from pathlib import Path

from manifold3d import Manifold

import kuciste as k

# ============================ PARAMETRI (mm) ============================
BASE_D = k.FIG_D        # 60; leziste na kucistu je O61,2
BASE_H = 6.0
TOP_CHAMFER = 0.8
BOTTOM_CHAMFER = 0.4    # i protiv "slonovske noge" prvog sloja

TAG_D = 26.0            # precnik nalepnice
TAG_CLEAR = 0.8         # po precniku, da nalepnica lako legne
FLOOR = 0.8             # dno ispod nalepnice: sto tanje, to blize anteni
POCKET = 0.6            # dubina udubljenja; mora biti veca od debljine nalepnice
LAYER = 0.2             # visina sloja u slajseru
# ========================================================================

# Pauza ide pre prvog sloja iznad udubljenja. Slajser sloj oznacava visinom
# njegovog vrha, pa je to ova vrednost.
PAUSE_AT = FLOOR + POCKET + LAYER

for name, v in (("FLOOR", FLOOR), ("POCKET", POCKET)):
    assert abs(v / LAYER - round(v / LAYER)) < 1e-9, f"{name} mora biti umnozak visine sloja"


def build_base():
    r = BASE_D / 2
    body = k.union([
        Manifold.cylinder(BOTTOM_CHAMFER, r - BOTTOM_CHAMFER, r),
        k.cyl_z(0, 0, BOTTOM_CHAMFER, BASE_H - TOP_CHAMFER, r),
        Manifold.cylinder(TOP_CHAMFER, r, r - TOP_CHAMFER).translate((0, 0, BASE_H - TOP_CHAMFER)),
    ])
    pocket = k.cyl_z(0, 0, FLOOR, FLOOR + POCKET, (TAG_D + TAG_CLEAR) / 2)
    return body - pocket


def build_tag():
    """Nalepnica, samo za sliku."""
    return k.cyl_z(0, 0, FLOOR, FLOOR + 0.3, TAG_D / 2)


def main():
    out = Path(__file__).parent / "stl"
    out.mkdir(exist_ok=True)
    base = build_base()
    k.write_stl(base, out / "postolje.stl")
    b = base.bounding_box()
    print(f"postolje  O{b[3] - b[0]:.1f} x {b[5] - b[2]:.1f} mm   {base.volume() / 1000:.1f} cm3   "
          f"tela: {len(base.decompose())}   {base.status()}")
    print(f"udubljenje O{TAG_D + TAG_CLEAR:.1f} x {POCKET:.1f} mm, od {FLOOR:.1f} do {FLOOR + POCKET:.1f} mm")
    print(f"pauza pred slojem na visini {PAUSE_AT:.2f} mm")


if __name__ == "__main__":
    main()
