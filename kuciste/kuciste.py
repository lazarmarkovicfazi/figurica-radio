"""
Kuciste za muzicku igracku sa NFC figuricama ("radio").

Delovi (svi se stampaju bez potpora):
  telo          - kutija sa lezistem baterije, drzacem zvucnika i otvorima
  poklopac      - ravna ploca, sa donje strane nosi RFID citac (stampa se naopako)
  gornja_ploca  - ploca sa otvorom O60 u koji seda figurica
  stezaljke     - plocice koje srafom pritiskaju RFID plocicu

Koordinate sklopa: X = sirina (levo -> desno), Y = dubina (napred -> nazad), Z = gore.
Prednja strana (zvucnik, enkoder) je na Y = 0.

Pokretanje:  .venv\\Scripts\\python.exe kuciste.py      -> stl\\*.stl
"""
import struct
from pathlib import Path

import numpy as np
from manifold3d import (CrossSection, JoinType, Manifold, OpType,
                        set_min_circular_angle, set_min_circular_edge_length)

set_min_circular_angle(4.0)
set_min_circular_edge_length(0.4)

# ============================ PARAMETRI (mm) ============================
# -- kutija --
W, D = 115.0, 86.0      # spoljna sirina i dubina
HI = 52.0               # unutrasnja visina tela
WALL = 2.4
FLOOR = 2.4
CORNER_R = 10.0
LID_T = 2.0             # poklopac (kroz njega cita RFID)
TOP_T = 3.0             # gornja ploca = dubina lezista figurice
FIT = 0.3               # zazor izmedju delova koji nalezu

# -- srafovi (M3, svi iste duzine) --
SCREW_L = 7.0           # duzina ispod glave
SCREW_PILOT = 2.6       # rupa u koju sraf sam urezuje navoj
SCREW_CLEAR = 3.4       # prolazna rupa
HEAD_D = 7.0            # upust za glavu na gornjoj ploci
SEAT_T = 1.2            # debljina gornje ploce ispod glave
POST_C = 7.0            # centar stuba od spoljne ivice
POST_R = 4.5
POST_TOP_R = 4.0        # vrh stuba koji prolazi kroz poklopac

# -- heat insert u stubovima poklopca (M3, spoljni precnik 4.6, duzina 5.7) --
INSERT_HOLE = 4.0       # rupa u koju se insert utapa; za insert 5 x 4 oko 4.4
INSERT_L = 5.7          # duzina inserta; za insert 5 x 4 staviti 4.0

# -- figurica --
FIG_D = 60.0            # precnik postolja figurice
FIG_CLEAR = 0.6         # zazor po poluprecniku

# -- RFID citac (RC522) --
RF_L, RF_W = 60.0, 40.0
RF_PCB_T = 1.6
RF_GAP = 4.0            # razmak plocica - poklopac (mesto za komponente)
RF_ANT_OFF = 10.0       # centar antene je ovoliko od centra plocice, ka kraju bez pinova

# -- zvucnik (okrugli, ravan) --
SPK_D = 40.0
SPK_RIM_T = 2.2         # debljina oboda
SPK_DEPTH = 4.8         # ukupna dubina
SPK_MAGNET_D = 15.6
SPK_X = 32.0            # centar, od unutrasnje leve strane
SPK_Z = 26.0            # centar, od poda
SPK_SLOT = SPK_RIM_T + 0.4   # zleb za obod zvucnika
GRILLE_HOLE = 2.6
GRILLE_PITCH = 4.2

# -- enkoder (EC11 / KY-040) --
ENC_D = 7.4
ENC_X = 19.5            # centar, od unutrasnje desne strane
ENC_Z = 26.0
ENC_SHAFT_L = 20.0      # osovina merena od tela enkodera, zajedno sa navojem

# -- dugme enkodera --
KNOB_D, KNOB_H = 24.0, 18.0
KNOB_CAP = 1.6          # debljina lica iznad vrha osovine
KNOB_FLUTES = 20        # zlebovi za hvat
NUT_RECESS_D, NUT_RECESS_H = 14.0, 5.5   # udubljenje sa zadnje strane koje pokriva navrtku
SHAFT_D = 6.0
SHAFT_FLAT = 4.5        # D osovina: mera preko ravnog dela
SHAFT_GRIP_L = 8.0      # duzina dela koji hvata osovinu

# -- prekidac (klackalica KCD1, ispod flanse 20 x 12) --
SW_W, SW_H = 19.3, 12.4   # 20 je preko zubaca; otvor je uzi da zupci uskoce
SW_PANEL_T = 1.6        # zid se oko otvora stanjuje na ovoliko da zupci uskoce
SW_Y = D / 2            # centar po dubini
SW_Z = 37.0             # centar, od poda

# -- TP4056 (USB-C) --
TP_L, TP_W = 28.0, 17.4
TP_PCB_T = 1.2
TP_RAISE = 4.5          # visina sina ispod plocice
TP_Y = 45.0             # centar po dubini
USB_W, USB_H = 9.6, 4.2
USB_ABOVE_PCB = 1.6     # centar USB-C porta iznad gornje strane plocice
USB_WALL_T = 1.0        # debljina zida koja ostaje oko porta

# -- baterija 18650 --
BAT_D = 18.6
BAT_L = 65.2
SPRING_ROOM = 6.0       # opruga (sabijena) + ispupcenje plus kontakta
CONTACT_W, CONTACT_H = 13.0, 11.0
CONTACT_T = 0.5
CONTACT_LIP = 0.8       # debljina usne ispred plocice kontakta
CLIP_T = 1.6            # debljina kopce
CLIP_WRAP = 30.0        # koliko stepeni kopca obuhvata bateriju iznad sredine
CLIP_LEN = 12.0

# -- pregrade za module na podu: (ime, x, y, duzina, sirina, visina modula) --
BAYS = [
    ("dfplayer", 3.4, 13.6, 21.2, 21.2, 8.0),
    ("esp32c3", 3.4, 38.4, 23.2, 18.6, 4.5),
    ("stepup", 30.2, 11.0, 50.6, 28.6, 15.0),     # 50 x 28; ispod enkodera sme do ~19 visine
]
# ========================================================================

HB = FLOOR + HI
SCREWS = [(POST_C, POST_C), (W - POST_C, POST_C), (POST_C, D - POST_C), (W - POST_C, D - POST_C)]


# ------------------------------ pomocne ---------------------------------
def box(x0, x1, y0, y1, z0, z1):
    x0, x1 = sorted((x0, x1))
    y0, y1 = sorted((y0, y1))
    z0, z1 = sorted((z0, z1))
    return Manifold.cube((x1 - x0, y1 - y0, z1 - z0)).translate((x0, y0, z0))


def cyl_z(x, y, z0, z1, r, seg=0):
    return Manifold.cylinder(z1 - z0, r, r, seg).translate((x, y, z0))


def cyl_y(x, z, y0, y1, r, seg=0):
    return Manifold.cylinder(y1 - y0, r, r, seg).rotate((-90, 0, 0)).translate((x, y0, z))


def cyl_x(y, z, x0, x1, r, seg=0):
    return Manifold.cylinder(x1 - x0, r, r, seg).rotate((0, 90, 0)).translate((x0, y, z))


def union(parts):
    return Manifold.batch_boolean(parts, OpType.Add)


def poly(pts):
    p = np.asarray(pts, dtype=float)
    x, y = p[:, 0], p[:, 1]
    if np.sum(x * np.roll(y, -1) - np.roll(x, -1) * y) < 0:
        p = p[::-1]
    return CrossSection([p])


def rrect_c(w, h, r):
    return CrossSection.square((w - 2 * r, h - 2 * r), True).offset(r, JoinType.Round)


def extrude_x(cs, x0, x1):
    """Profil zadat u (y, z), izvlaci se duz X."""
    t = np.array([[0, 0, 1, x0], [1, 0, 0, 0], [0, 1, 0, 0]], dtype=float)
    return Manifold.extrude(cs, x1 - x0).transform(t)


def outline():
    r = CORNER_R
    return CrossSection.square((W - 2 * r, D - 2 * r)).translate((r, r)).offset(r, JoinType.Round)


def to_bed(m):
    b = m.bounding_box()
    return m.translate((-b[0], -b[1], -b[2]))


# ------------------------------ izvedene mere ----------------------------
XS, ZS, SPK_R = WALL + SPK_X, FLOOR + SPK_Z, SPK_D / 2
XE, ZE = W - WALL - ENC_X, FLOOR + ENC_Z
ZW = FLOOR + SW_Z
X_WALL = W - WALL
X_TP0 = X_WALL - TP_L - 0.3                 # zadnja ivica TP4056
Z_TP0 = FLOOR + TP_RAISE
Z_TP1 = Z_TP0 + TP_PCB_T
Z_USB = Z_TP1 + USB_ABOVE_PCB

BAT_RC = BAT_D / 2 + 0.25                   # poluprecnik lezista
YB = D - WALL - 1.5 - CLIP_T - BAT_RC       # osa baterije
ZB = FLOOR + 2.0 + 1.35 + BAT_RC
BAT_SLOT = CONTACT_T + 0.5
BAT_EW = CONTACT_LIP + BAT_SLOT + 2.2       # debljina ceonog zida
BAT_SPAN = BAT_L + SPRING_ROOM - 2 * CONTACT_LIP
BAT_XA = (W - BAT_SPAN) / 2                 # unutrasnja lica ceonih zidova
BAT_XB = BAT_XA + BAT_SPAN

XCB, YCB = W / 2 + RF_ANT_OFF, D / 2        # centar RFID plocice
RF_BOSS_R = 3.75
RF_BOSS_Y = (YCB - RF_W / 2 - FIT - 0.3 - RF_BOSS_R, YCB + RF_W / 2 + FIT + 0.3 + RF_BOSS_R)
Z_RF1 = HB - RF_GAP                         # strana plocice ka poklopcu
Z_RF0 = Z_RF1 - RF_PCB_T
Z_POST = HB + LID_T - 0.1                   # vrh stuba; 0.1 nize da gornja ploca stegne poklopac

TAB_L, TAB_W, TAB_T, TAB_HOLE = 12.0, 8.0, 2.4, 4.0


# ------------------------------- telo ------------------------------------
def post_cs():
    c, r = POST_C, POST_R
    arc = [(c + r * np.cos(a), c + r * np.sin(a)) for a in np.radians(np.linspace(0, 90, 13))]
    return poly([(1.0, 1.0), (c + r, 1.0)] + arc + [(1.0, c + r)])


def clip_profile():
    t, w = CLIP_T, np.radians(CLIP_WRAP)
    ro, rm = BAT_RC + t, BAT_RC + t / 2
    pts = [(ro, FLOOR - 0.1), (ro, ZB)]
    pts += [(ro * np.cos(a), ZB + ro * np.sin(a)) for a in np.linspace(0, w, 9)[1:]]
    cy, cz = rm * np.cos(w), ZB + rm * np.sin(w)
    pts += [(cy + t / 2 * np.cos(a), cz + t / 2 * np.sin(a)) for a in np.linspace(w, w + np.pi, 9)[1:-1]]
    pts += [(BAT_RC * np.cos(a), ZB + BAT_RC * np.sin(a)) for a in np.linspace(w, 0, 9)]
    pts += [(BAT_RC, FLOOR - 0.1)]
    return poly(pts)


def battery_cradle():
    adds, cuts = [], []
    zf = FLOOR - 0.1
    hw = BAT_RC + CLIP_T
    z_end = ZB + CONTACT_H / 2 + 2.0
    xm = (BAT_XA + BAT_XB) / 2

    # ceoni zidovi sa zlebom za plocicu kontakta
    for xi, s in ((BAT_XA, -1), (BAT_XB, 1)):
        adds.append(box(xi, xi + s * BAT_EW, YB - hw, YB + hw, zf, z_end))
        xs0 = xi + s * CONTACT_LIP
        cuts.append(box(xs0, xs0 + s * BAT_SLOT, YB - CONTACT_W / 2 - 0.3, YB + CONTACT_W / 2 + 0.3,
                        ZB - CONTACT_H / 2 - 0.3, z_end + 1))
        cuts.append(box(xi - s * 0.1, xs0 + s * 0.1, YB - CONTACT_W / 2 + 1.2, YB + CONTACT_W / 2 - 1.2,
                        ZB - CONTACT_H / 2 + 0.8, z_end + 1))

    # sedla na kojima baterija lezi; srednje ima tunel za vezicu
    saddles = union([
        box(xm - 5, xm + 5, YB - 8, YB + 8, zf, ZB - 4.5),
        box(BAT_XA - 0.1, BAT_XA + 4, YB - 8, YB + 8, zf, ZB - 4.5),
        box(BAT_XB - 4, BAT_XB + 0.1, YB - 8, YB + 8, zf, ZB - 4.5),
    ]) - cyl_x(YB, ZB, BAT_XA - 1, BAT_XB + 1, BAT_RC)
    adds.append(saddles)
    cuts.append(box(xm - 2.5, xm + 2.5, YB - 9, YB + 9, FLOOR, FLOOR + 2.0))

    # kopce koje drze bateriju
    prof = clip_profile()
    for f in (0.27, 0.73):
        xc = BAT_XA + f * BAT_SPAN
        adds.append(extrude_x(prof.translate((YB, 0)), xc - CLIP_LEN / 2, xc + CLIP_LEN / 2))
        adds.append(extrude_x(prof.mirror((1, 0)).translate((YB, 0)), xc - CLIP_LEN / 2, xc + CLIP_LEN / 2))
    return adds, cuts


def bay(x, y, lx, ly, h=2.6, t=1.2, leg=5.0):
    parts = []
    for cx, dx in ((x, 1), (x + lx, -1)):
        for cy, dy in ((y, 1), (y + ly, -1)):
            parts.append(box(cx - dx * t, cx + dx * leg, cy - dy * t, cy, FLOOR - 0.1, FLOOR + h))
            parts.append(box(cx - dx * t, cx, cy - dy * t, cy + dy * leg, FLOOR - 0.1, FLOOR + h))
    return parts


def build_body():
    out2 = outline()
    c = 0.8
    outer = Manifold.batch_hull([
        Manifold.extrude(out2.offset(-c, JoinType.Round), 0.02),
        Manifold.extrude(out2, HB - c).translate((0, 0, c)),
    ])
    cavity = Manifold.extrude(out2.offset(-WALL, JoinType.Round), HI + 1).translate((0, 0, FLOOR))
    adds, cuts = [outer - cavity], []
    zf = FLOOR - 0.1

    # stubovi za srafove poklopca
    p = post_cs()
    for q in (p, p.mirror((1, 0)).translate((W, 0)), p.mirror((0, 1)).translate((0, D)),
              p.mirror((1, 0)).mirror((0, 1)).translate((W, D))):
        adds.append(Manifold.extrude(q, HI).translate((0, 0, FLOOR)) ^ outer)
    for x, y in SCREWS:
        adds.append(cyl_z(x, y, HB - 0.1, Z_POST, POST_TOP_R))
        # insert: rupa za utapanje, ispod nje prolaz za vrh srafa, na vrhu kosina za centriranje
        cuts.append(cyl_z(x, y, Z_POST - INSERT_L - 1.0, Z_POST + 1, INSERT_HOLE / 2))
        cuts.append(cyl_z(x, y, Z_POST - (SCREW_L - SEAT_T) - 1.5, Z_POST, SCREW_CLEAR / 2))
        cuts.append(Manifold.cylinder(0.6, INSERT_HOLE / 2, INSERT_HOLE / 2 + 0.6).translate((x, y, Z_POST - 0.5)))

    # zvucnik: sine u koje se spusta odozgo, oslonac ispod, resetka
    y_lip0 = WALL + SPK_SLOT
    y_lip1 = y_lip0 + 2.0
    z_rail = ZS + 12
    for s in (-1, 1):
        xw = XS + s * (SPK_R + 0.4)
        adds.append(box(xw, xw + s * 3.0, WALL - 0.1, y_lip1, zf, z_rail))
        adds.append(box(xw + s * 0.1, xw - s * 2.2, y_lip0, y_lip1, zf, z_rail))
    adds.append(box(XS - 6, XS + 6, WALL - 0.1, y_lip1, zf, ZS - SPK_R))
    gr = SPK_R - 3.0 - GRILLE_HOLE / 2
    dz = GRILLE_PITCH * np.sin(np.radians(60))
    for j in range(-10, 11):
        for i in range(-10, 11):
            gx = (i + 0.5 * (j % 2)) * GRILLE_PITCH
            gz = j * dz
            if gx * gx + gz * gz <= gr * gr:
                cuts.append(cyl_y(XS + gx, ZS + gz, -1, WALL + 1, GRILLE_HOLE / 2, 16))

    # enkoder
    cuts.append(cyl_y(XE, ZE, -1, WALL + 1, ENC_D / 2))

    # prekidac na levoj strani, zid stanjen iznutra
    cuts.append(box(-1, WALL + 1, SW_Y - SW_W / 2, SW_Y + SW_W / 2, ZW - SW_H / 2, ZW + SW_H / 2))
    cuts.append(box(SW_PANEL_T, WALL + 0.5, SW_Y - SW_W / 2 - 2.5, SW_Y + SW_W / 2 + 2.5,
                    ZW - SW_H / 2 - 2.5, ZW + SW_H / 2 + 2.5))

    # TP4056 na desnoj strani: sine sa usnom, graničnik-sraf iza plocice, USB-C otvor
    zl = Z_TP1 + 0.3
    for d, ye in ((1, TP_Y - TP_W / 2 - 0.25), (-1, TP_Y + TP_W / 2 + 0.25)):
        adds.append(box(X_TP0, X_WALL + 0.1, ye, ye - d * 1.2, zf, zl + 1.3))
        adds.append(box(X_TP0, X_WALL + 0.1, ye, ye + d * 1.6, zf, Z_TP0))
        lip = poly([(ye - d * 0.1, zl), (ye + d * 0.8, zl + 0.8), (ye + d * 0.8, zl + 1.3), (ye - d * 0.1, zl + 1.3)])
        adds.append(extrude_x(lip, X_WALL - 10, X_WALL + 0.1))
    adds.append(cyl_z(X_TP0 - 1.7, TP_Y, zf, Z_TP0, 3.5))
    cuts.append(cyl_z(X_TP0 - 1.7, TP_Y, Z_TP1 - SCREW_L - 0.3, Z_TP0 + 1, SCREW_PILOT / 2))
    cuts.append(extrude_x(rrect_c(USB_W, USB_H, 1.6).translate((TP_Y, Z_USB)), X_WALL - 1, W + 1))
    cuts.append(extrude_x(rrect_c(USB_W + 3.6, USB_H + 3.4, 2.5).translate((TP_Y, Z_USB)),
                          X_WALL + USB_WALL_T, W + 1))

    # pregrade za module
    for _, x, y, lx, ly, _ in BAYS:
        adds += bay(x, y, lx, ly)

    ba, bc = battery_cradle()
    return union(adds + ba) - union(cuts + bc)


# ------------------------------ poklopac ---------------------------------
def lid_arrow():
    """Strelica na donjoj strani poklopca ka prednjoj strani, da se ne montira okrenut."""
    ax, ay = 20.0, 9.0
    return (Manifold.extrude(poly([(ax - 5, ay + 8), (ax + 5, ay + 8), (ax, ay)]), 0.7)
            .translate((0, 0, HB - 0.6)))


def build_lid():
    z1 = HB + 0.1
    adds = [Manifold.extrude(outline(), LID_T).translate((0, 0, HB))]
    cuts = []

    # rebra koja centriraju poklopac u telu
    e, lt, lh = WALL + FIT, 1.6, 3.0
    m = POST_C + POST_R + 1.5
    adds += [box(m, W - m, e, e + lt, HB - lh, z1), box(m, W - m, D - e - lt, D - e, HB - lh, z1),
             box(e, e + lt, m, D - m, HB - lh, z1), box(W - e - lt, W - e, m, D - m, HB - lh, z1)]

    # pritiskac koji drzi zvucnik odozgo
    adds.append(box(XS - 6, XS + 6, e, e + 4.5, ZS + SPK_R + 0.4, z1))
    adds.append(lid_arrow())

    # RFID: oslonci u uglovima, graničnici, dva stuba za stezaljke
    px = (XCB - RF_L / 2 - FIT, XCB + RF_L / 2 + FIT)
    py = (YCB - RF_W / 2 - FIT, YCB + RF_W / 2 + FIT)
    zfence = Z_RF0 - 1.2
    for cx, dx in ((px[0], 1), (px[1], -1)):
        for cy, dy in ((py[0], 1), (py[1], -1)):
            adds.append(box(cx - dx * 1.6, cx + dx * 8, cy - dy * 1.6, cy, zfence, z1))
            adds.append(box(cx - dx * 1.6, cx, cy - dy * 1.6, cy + dy * 8, zfence, z1))
            adds.append(box(cx, cx + dx * 5, cy, cy + dy * 5, Z_RF1, z1))
    for by in RF_BOSS_Y:
        adds.append(cyl_z(XCB, by, Z_RF0, z1, RF_BOSS_R))
        cuts.append(cyl_z(XCB, by, Z_RF0 - 1, HB + 0.8, SCREW_PILOT / 2))

    # stubovi tela prolaze kroz poklopac; steze ga gornja ploca
    for x, y in SCREWS:
        cuts.append(cyl_z(x, y, HB - 1, HB + LID_T + 1, POST_TOP_R + 0.3))
    return union(adds) - union(cuts)


# ---------------------------- gornja ploca -------------------------------
def build_top():
    z0 = HB + LID_T
    out2 = outline()
    c = 1.0
    plate = Manifold.batch_hull([
        Manifold.extrude(out2, TOP_T - c),
        Manifold.extrude(out2.offset(-c, JoinType.Round), 0.02).translate((0, 0, TOP_T - 0.02)),
    ]).translate((0, 0, z0))
    r = FIG_D / 2 + FIG_CLEAR
    cuts = [cyl_z(W / 2, D / 2, z0 - 1, z0 + TOP_T + 1, r),
            Manifold.cylinder(1.5, r, r + 1.5).translate((W / 2, D / 2, z0 + TOP_T - 1.0))]
    for x, y in SCREWS:
        cuts.append(cyl_z(x, y, z0 - 1, z0 + TOP_T + 1, SCREW_CLEAR / 2))
        cuts.append(cyl_z(x, y, z0 + SEAT_T, z0 + TOP_T + 1, HEAD_D / 2))
    return plate - union(cuts)


# ------------------------------ stezaljke --------------------------------
def build_tab():
    """Plocica sa rupom na (0, 0); duzi kraj (+X) naleze na RFID plocicu."""
    body = Manifold.extrude(rrect_c(TAB_L, TAB_W, 2.0), TAB_T).translate((TAB_L / 2 - TAB_HOLE, 0, 0))
    return body - cyl_z(0, 0, -1, TAB_T + 1, SCREW_CLEAR / 2)


def tabs_assembled():
    t = build_tab()
    front = t.rotate((0, 0, 90)).translate((XCB, RF_BOSS_Y[0], Z_RF0 - TAB_T))
    back = t.rotate((0, 0, -90)).translate((XCB, RF_BOSS_Y[1], Z_RF0 - TAB_T))
    return union([front, back])


# ------------------------------- dugme -----------------------------------
def build_knob(kind="D"):
    """Dugme u polozaju za stampu: lice na Z = 0, otvor za osovinu nagore.
    kind "D" za osovinu sa ravnim delom, "K" za nazubljenu (prorezanu) osovinu."""
    r, c = KNOB_D / 2, 1.0
    body = Manifold.batch_hull([cyl_z(0, 0, 0, 0.02, r - c), cyl_z(0, 0, c, KNOB_H, r)])
    cuts = []
    # zlebovi za hvat; ugao 0 je izmedju dva zleba i tu je oznaka
    for a in np.linspace(0, 2 * np.pi, KNOB_FLUTES, endpoint=False) + np.pi / KNOB_FLUTES:
        cuts.append(cyl_z(r * np.cos(a), r * np.sin(a), 1.5, KNOB_H + 1, 1.2, 16))
    cuts.append(box(3.0, r + 1, -0.8, 0.8, -1, 0.8))
    # osovina: deo koji hvata, iznad njega prolaz, pozadi udubljenje za navrtku
    z_grip = KNOB_CAP + SHAFT_GRIP_L
    if kind == "D":
        flat = SHAFT_FLAT - SHAFT_D / 2 + 0.1
        grip = CrossSection.circle(SHAFT_D / 2 + 0.1) ^ CrossSection.square((10, 10)).translate((-10 + flat, -5))
    else:
        grip = CrossSection.circle(SHAFT_D / 2 - 0.05)
    cuts.append(Manifold.extrude(grip, SHAFT_GRIP_L + 0.01).translate((0, 0, KNOB_CAP)))
    cuts.append(cyl_z(0, 0, z_grip, KNOB_H + 1, SHAFT_D / 2 + 0.2))
    cuts.append(cyl_z(0, 0, KNOB_H - NUT_RECESS_H, KNOB_H + 1, NUT_RECESS_D / 2))
    return body - union(cuts)


def knob_assembled(kind="D"):
    """Dugme na osovini, oznaka nagore; osovina naleze na lice dugmeta."""
    gap = ENC_SHAFT_L - WALL - (KNOB_H - KNOB_CAP)
    return build_knob(kind).rotate((0, 0, -90)).rotate((-90, 0, 0)).translate((XE, -(gap + KNOB_H), ZE))


# ------------------- komponente (samo za proveru i slike) ----------------
def build_ghosts():
    g = {}
    g["zvucnik"] = union([
        cyl_y(XS, ZS, WALL + 0.05, WALL + SPK_RIM_T, SPK_R - 0.05),
        cyl_y(XS, ZS, WALL + SPK_RIM_T, WALL + SPK_DEPTH, SPK_MAGNET_D / 2),
    ])
    xm = (BAT_XA + BAT_XB) / 2
    g["baterija"] = cyl_x(YB, ZB - 0.2, xm - BAT_L / 2, xm + BAT_L / 2, 9.2)
    g["rfid"] = union([
        box(XCB - RF_L / 2, XCB + RF_L / 2, YCB - RF_W / 2, YCB + RF_W / 2, Z_RF0, Z_RF1),
        box(XCB + RF_L / 2 - 3.8, XCB + RF_L / 2 - 1.2, YCB - 10.2, YCB + 10.2, Z_RF0 - 25, Z_RF0),
    ])
    g["prekidac"] = union([
        box(-2, 0, SW_Y - 10.5, SW_Y + 10.5, ZW - 7.5, ZW + 7.5),
        box(-0.5, 22, SW_Y - SW_W / 2 + 0.15, SW_Y + SW_W / 2 - 0.15, ZW - SW_H / 2 + 0.15, ZW + SW_H / 2 - 0.15),
        box(22, 30, SW_Y - 5, SW_Y + 5, ZW - 3, ZW + 3),
    ])
    g["enkoder"] = union([
        cyl_y(XE, ZE, -(ENC_SHAFT_L - WALL), 0, SHAFT_D / 2),
        cyl_y(XE, ZE, -4.6, WALL + 0.1, 3.5),
        box(XE - 6.15, XE + 6.15, WALL, WALL + 6.5, ZE - 5.9, ZE + 5.9),
        box(XE - 17, XE + 9, WALL + 6.5, WALL + 8.1, ZE - 9.5, ZE + 9.5),
        box(XE - 39, XE - 17, WALL + 7, WALL + 11, ZE - 6.5, ZE + 6.5),
    ])
    g["tp4056"] = union([
        box(X_TP0 + 0.3, X_WALL - 0.05, TP_Y - TP_W / 2, TP_Y + TP_W / 2, Z_TP0, Z_TP1),
        box(X_WALL - 6.5, X_WALL + 1.0, TP_Y - 4.47, TP_Y + 4.47, Z_TP1, Z_TP1 + 3.2),
    ])
    for name, x, y, lx, ly, h in BAYS:
        g[name] = box(x + 0.3, x + lx - 0.3, y + 0.3, y + ly - 0.3, FLOOR, FLOOR + h)
    g["figurica"] = cyl_z(W / 2, D / 2, HB + LID_T, HB + LID_T + 5, FIG_D / 2)
    return g


# ------------------------------- izvoz -----------------------------------
def write_stl(m, path):
    mesh = m.to_mesh()
    v = np.asarray(mesh.vert_properties, dtype=np.float64)[:, :3]
    tri = v[np.asarray(mesh.tri_verts)]
    n = np.cross(tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0])
    n /= np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-12)
    rec = np.zeros(len(tri), dtype=[("n", "<f4", (3,)), ("v", "<f4", (3, 3)), ("a", "<u2")])
    rec["n"], rec["v"] = n, tri
    with open(path, "wb") as f:
        f.write(b"FiguricaRadio".ljust(80, b" "))
        f.write(struct.pack("<I", len(tri)))
        f.write(rec.tobytes())


def build():
    """Delovi u polozaju sklopa."""
    return {"telo": build_body(), "poklopac": build_lid(), "gornja_ploca": build_top(),
            "stezaljke": tabs_assembled()}


def build_print():
    """Delovi okrenuti za stampu, spusteni na Z = 0."""
    tab = build_tab()
    return {
        "telo": to_bed(build_body()),
        "poklopac": to_bed(build_lid().rotate((180, 0, 0))),
        "gornja_ploca": to_bed(build_top()),
        "stezaljke_x4": union([tab.translate((0, i * (TAB_W + 4), 0)) for i in range(4)]),
        "dugme_D_osovina": build_knob("D"),
        "dugme_nazubljena_osovina": build_knob("K"),
    }


def check_screws():
    """Koliko navoja svaki sraf hvata i da li negde probija."""
    tp_tip = Z_TP1 - SCREW_L
    rf_tip = SCREW_L - TAB_T
    assert tp_tip >= 0.9, "sraf iza TP4056 probija dno: povecaj TP_RAISE"
    assert rf_tip <= RF_GAP + RF_PCB_T, "sraf stezaljke probija poklopac: povecaj RF_GAP"
    assert POST_TOP_R - INSERT_HOLE / 2 >= 1.6, "zid stuba oko inserta je pretanak"
    print(f"poklopac: sraf hvata {min(INSERT_L, SCREW_L - SEAT_T):.1f} mm inserta, "
          f"zid oko inserta {POST_TOP_R - INSERT_HOLE / 2:.1f} mm")
    print(f"navoj u plastici: RFID {rf_tip:.1f} mm, "
          f"TP4056 {SCREW_L - TP_PCB_T:.1f} mm (do dna ostaje {tp_tip:.1f} mm)")


def main():
    check_screws()
    out = Path(__file__).parent / "stl"
    out.mkdir(exist_ok=True)
    for name, m in build_print().items():
        b = m.bounding_box()
        write_stl(m, out / f"{name}.stl")
        print(f"{name:14s} {b[3] - b[0]:6.1f} x {b[4] - b[1]:5.1f} x {b[5] - b[2]:5.1f} mm   "
              f"{m.volume() / 1000:6.1f} cm3   tela: {len(m.decompose())}   {m.status()}")


if __name__ == "__main__":
    main()
