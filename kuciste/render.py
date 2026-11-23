"""
Slike za proveru modela i provera sudara delova sa komponentama.

Pokretanje:  .venv\\Scripts\\python.exe render.py      -> slike\\*.png
"""
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

import kuciste as k

COL = {
    "telo": (238, 196, 92), "poklopac": (236, 236, 230), "gornja_ploca": (84, 140, 190),
    "stezaljke": (200, 90, 80), "dugme": (60, 150, 140),
    "zvucnik": (70, 70, 78), "baterija": (70, 160, 100), "rfid": (40, 110, 200),
    "prekidac": (30, 30, 30), "enkoder": (150, 150, 160), "tp4056": (40, 90, 170),
    "dfplayer": (190, 60, 60), "esp32c3": (60, 60, 60), "stepup": (120, 80, 170),
    "figurica": (230, 120, 60),
}


def tris_of(m):
    mesh = m.to_mesh()
    v = np.asarray(mesh.vert_properties, dtype=np.float64)[:, :3]
    return v[np.asarray(mesh.tri_verts)]


def render(items, path, azim, elev, width=1500, ss=2, margin=30, labels=(), title=None):
    """items: lista (Manifold, (r, g, b)). Ortografska projekcija sa z-baferom.
    labels: lista (tekst, tacka u 3D, pomeraj natpisa u pikselima)."""
    az, el = np.radians(azim), np.radians(elev)
    c = np.array([np.cos(el) * np.cos(az), np.cos(el) * np.sin(az), np.sin(el)])
    up = np.array([0.0, 1.0, 0.0]) if abs(elev) > 89 else np.array([0.0, 0.0, 1.0])
    r = np.cross(-c, up)
    r /= np.linalg.norm(r)
    u = np.cross(r, -c)
    light = c + 0.55 * u - 0.45 * r
    light /= np.linalg.norm(light)

    tris, cols, norms = [], [], []
    for m, col in items:
        t = tris_of(m)
        n = np.cross(t[:, 1] - t[:, 0], t[:, 2] - t[:, 0])
        ln = np.linalg.norm(n, axis=1)
        ok = ln > 1e-12
        t, n = t[ok], n[ok] / ln[ok, None]
        front = n @ c > 1e-6
        t, n = t[front], n[front]
        shade = 0.38 + 0.62 * np.clip(n @ light, 0, 1)
        tris.append(t)
        norms.append(n)
        cols.append(np.asarray(col, dtype=float)[None, :] * shade[:, None])
    t, n, col = np.concatenate(tris), np.concatenate(norms), np.concatenate(cols)

    sx, sy, sz = t @ r, t @ u, t @ c
    wp = width * ss
    mg = margin * ss
    scale = (wp - 2 * mg) / (sx.max() - sx.min())
    hp = int(np.ceil((sy.max() - sy.min()) * scale)) + 2 * mg
    px = (sx - sx.min()) * scale + mg
    py = (sy.max() - sy) * scale + mg

    zbuf = np.full((hp, wp), -np.inf)
    img = np.full((hp, wp, 3), 255.0)
    nbuf = np.zeros((hp, wp, 3))
    for i in range(len(t)):
        x0, x1, x2 = px[i]
        y0, y1, y2 = py[i]
        den = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2)
        if abs(den) < 1e-9:
            continue
        ax, bx = max(int(min(x0, x1, x2)), 0), min(int(max(x0, x1, x2)) + 1, wp - 1)
        ay, by = max(int(min(y0, y1, y2)), 0), min(int(max(y0, y1, y2)) + 1, hp - 1)
        if ax > bx or ay > by:
            continue
        gx = np.arange(ax, bx + 1)[None, :] + 0.5
        gy = np.arange(ay, by + 1)[:, None] + 0.5
        w0 = ((y1 - y2) * (gx - x2) + (x2 - x1) * (gy - y2)) / den
        w1 = ((y2 - y0) * (gx - x2) + (x0 - x2) * (gy - y2)) / den
        w2 = 1 - w0 - w1
        z = w0 * sz[i, 0] + w1 * sz[i, 1] + w2 * sz[i, 2]
        zb = zbuf[ay:by + 1, ax:bx + 1]
        hit = (w0 >= -1e-6) & (w1 >= -1e-6) & (w2 >= -1e-6) & (z > zb)
        if hit.any():
            zb[hit] = z[hit]
            img[ay:by + 1, ax:bx + 1][hit] = col[i]
            nbuf[ay:by + 1, ax:bx + 1][hit] = n[i]

    # ivice: skok u dubini ili prelom izmedju povrsina
    zc = np.where(np.isfinite(zbuf), zbuf, -1e6)
    edge = np.zeros((hp, wp), dtype=bool)
    for ax_ in (0, 1):
        dzv = np.abs(np.diff(zc, axis=ax_)) > 1.2
        a, b = (nbuf[1:], nbuf[:-1]) if ax_ == 0 else (nbuf[:, 1:], nbuf[:, :-1])
        za, zb2 = (zc[1:], zc[:-1]) if ax_ == 0 else (zc[:, 1:], zc[:, :-1])
        crease = (np.sum(a * b, axis=2) < 0.9) & ((za > -1e5) | (zb2 > -1e5))
        e = dzv | crease
        if ax_ == 0:
            edge[1:] |= e
            edge[:-1] |= e
        else:
            edge[:, 1:] |= e
            edge[:, :-1] |= e
    img[edge] *= 0.45

    im = Image.fromarray(np.clip(img, 0, 255).astype(np.uint8))
    im = im.resize((wp // ss, hp // ss), Image.LANCZOS)
    x_min, y_max = sx.min(), sy.max()

    def to_px(p3):
        p3 = np.asarray(p3, dtype=float)
        return ((p3 @ r - x_min) * scale + mg) / ss, ((y_max - p3 @ u) * scale + mg) / ss

    annotate(im, labels, to_px, title)
    im.save(path)
    return path


def font(size):
    for name in ("segoeui.ttf", "arial.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            pass
    return ImageFont.load_default()


def annotate(im, labels, to_px, title):
    d = ImageDraw.Draw(im)
    ink = (25, 25, 30)
    if title:
        d.text((22, 14), title, fill=ink, font=font(30))
    f = font(23)
    for text, p3, (dx, dy) in labels:
        ax, ay = to_px(p3)
        l, tp, rt, b = d.textbbox((0, 0), text, font=f)
        w, h = rt - l + 18, b - tp + 14
        x0 = min(max(ax + dx - w / 2, 6), im.width - w - 6)
        y0 = min(max(ay + dy - h / 2, 6), im.height - h - 6)
        lx, ly = min(max(ax, x0), x0 + w), min(max(ay, y0), y0 + h)
        d.line([(lx, ly), (ax, ay)], fill=ink, width=2)
        d.ellipse([ax - 4, ay - 4, ax + 4, ay + 4], fill=ink)
        d.rounded_rectangle([x0, y0, x0 + w, y0 + h], radius=7, fill=(255, 255, 255), outline=ink, width=2)
        d.text((x0 + 9 - l, y0 + 7 - tp), text, fill=ink, font=f)


def screw(x, y, z_seat, down=True):
    """M3 sraf sa pan glavom; z_seat je ravan na koju glava naleze."""
    ln, hr, hh = k.SCREW_L, 2.8, 2.0
    if down:
        return k.union([k.cyl_z(x, y, z_seat - ln, z_seat, 1.5), k.cyl_z(x, y, z_seat, z_seat + hh, hr)])
    return k.union([k.cyl_z(x, y, z_seat, z_seat + ln, 1.5), k.cyl_z(x, y, z_seat - hh, z_seat, hr)])


STEEL = (72, 76, 88)
BRASS = (176, 104, 36)


def insert(x, y):
    """Heat insert utopljen u vrh stuba (spoljni precnik 4.6)."""
    return k.cyl_z(x, y, k.Z_POST - k.INSERT_L, k.Z_POST, 2.3) - k.cyl_z(x, y, k.Z_POST - 10, k.Z_POST + 1, 1.5)


def changes(p, g, out):
    """Slike sa natpisima: gde ide koji sraf i kako se sklapa gornji deo."""
    W, D, HB = k.W, k.D, k.HB
    z_top = HB + k.LID_T + k.TOP_T
    z_seat = HB + k.LID_T + k.SEAT_T
    lid_screws = k.union([screw(x, y, z_seat) for x, y in k.SCREWS])
    inserts = k.union([insert(x, y) for x, y in k.SCREWS])
    rf_screws = k.union([screw(k.XCB, by, k.Z_RF0 - k.TAB_T, down=False) for by in k.RF_BOSS_Y])
    tp_screw = screw(k.X_TP0 - 1.7, k.TP_Y, k.Z_TP1)
    base = [(p[n], COL[n]) for n in ("telo", "poklopac", "gornja_ploca")]
    lift = lambda m, dz: m.translate((0, 0, dz))

    # 1) ceo sklop
    render(base + [(lid_screws, STEEL), (g["enkoder"], COL["enkoder"]), (g["tp4056"], COL["tp4056"])],
           out / "20_sklop_srafovi.png", -62, 30, margin=120, title="Ceo sklop",
           labels=[
               ("4 × M3×7 u upustima", (k.POST_C, k.POST_C, z_top), (60, -150)),
               ("gornja ploča: nju drže šrafovi", (W * 0.78, D * 0.12, z_top), (250, 130)),
               ("poklopac: stegnut između ploče i tela", (W * 0.5, 0, HB + k.LID_T / 2), (-40, 110)),
               ("ležište figurice", (W / 2, D / 2, HB + k.LID_T), (0, 0)),
               ("USB-C, 9,7 mm od dna", (W, k.TP_Y, k.Z_USB), (40, 110)),
           ])

    # 2) ugao u preseku
    reg = k.box(W - 27, W + 1, k.POST_C, 24, HB - 15, z_top + 4)
    fr = screw(W - k.POST_C, k.POST_C, z_seat)
    xs_ = W - k.POST_C
    render([(m ^ reg, c) for m, c in [(fr, STEEL), (inserts, BRASS)] + base],
           out / "21_ugao_presek.png", -80, 13, width=1400, margin=130, title="Ugao u preseku",
           labels=[
               ("šraf M3×7", (xs_, k.POST_C, z_seat + 1.2), (-190, -90)),
               ("gornja ploča", (W - 21, k.POST_C, z_top - 1.2), (-70, -110)),
               ("ispod glave ostaje 1,2 mm ploče", (xs_ + 2.6, k.POST_C, HB + k.LID_T + 0.6), (-40, -230)),
               ("poklopac", (W - 21, k.POST_C, HB + k.LID_T / 2), (-90, 120)),
               ("stub tela prolazi kroz poklopac", (xs_ - 2.7, k.POST_C, HB + 0.9), (-330, 230)),
               ("insert M3 4,6 × 5,7 utopljen u stub", (xs_ - 1.9, k.POST_C, k.Z_POST - 3.6), (-330, 330)),
           ])

    # 3) redosled sklapanja
    render([(inserts, BRASS), (p["telo"], COL["telo"])]
           + [(g[n], COL[n]) for n in g if n not in ("figurica", "rfid")]
           + [(lift(g["rfid"], 34), COL["rfid"]), (lift(p["stezaljke"], 34), COL["stezaljke"]),
              (lift(p["poklopac"], 34), COL["poklopac"]), (lift(p["gornja_ploca"], 62), COL["gornja_ploca"]),
              (lift(lid_screws, 84), STEEL)],
           out / "22_redosled_sklapanja.png", -62, 22, width=1300, margin=130, title="Redosled sklapanja",
           labels=[
               ("stub sa insertom, viri 1,9 mm iznad zida", (W - k.POST_C + 2, k.POST_C, k.Z_POST), (150, 60)),
               ("1. poklopac sa RFID-om: rupe Ø8,6 nalegnu na stubove", (W, D * 0.55, HB + 34 + 1), (-60, 95)),
               ("2. gornja ploča preko poklopca", (W, D * 0.55, HB + k.LID_T + 62 + 1.5), (60, 80)),
               ("3. 4 × M3×7 kroz gornju ploču u inserte", (W - k.POST_C, k.POST_C, z_seat + 84), (40, -100)),
           ])

    # 4) TP4056 u preseku
    reg = k.box(W - 47, W + 1, k.TP_Y, k.TP_Y + 13, -1, 19)
    xb = k.X_TP0 - 1.7
    render([(m ^ reg, c) for m, c in [(p["telo"], COL["telo"]), (g["tp4056"], COL["tp4056"]), (tp_screw, STEEL)]],
           out / "23_tp4056_presek.png", -82, 14, width=1400, margin=130, title="TP4056 u preseku",
           labels=[
               ("TP4056", (k.X_TP0 + 13, k.TP_Y, k.Z_TP0 + k.TP_PCB_T / 2), (-20, -170)),
               ("USB-C otvor", (W - k.WALL / 2, k.TP_Y, k.Z_USB), (-60, -200)),
               ("M3×7: graničnik i pritiskač", (xb, k.TP_Y, k.Z_TP1 + 1.2), (-150, -150)),
               ("šine podignute na 4,5 mm", (k.X_TP0 + 15, k.TP_Y + k.TP_W / 2 - 0.6, k.FLOOR + 2.2), (60, 215)),
               ("do dna ostaje 1,1 mm", (xb, k.TP_Y, 0.55), (-190, 110)),
           ])

    # 5) poklopac odozdo
    up = lambda m: m.rotate((180, 0, 0))
    fl = lambda x, y, z: (x, -y, -z)
    render([(up(k.lid_arrow().translate((0, 0, -0.03))), (205, 70, 55)),
            (up(p["poklopac"]), COL["poklopac"]), (up(g["rfid"]), COL["rfid"]),
            (up(p["stezaljke"]), COL["stezaljke"]), (up(rf_screws), STEEL)],
           out / "24_poklopac_odozdo.png", -60, 40, margin=130, title="Poklopac odozdo",
           labels=[
               ("2 × M3×7 kroz stezaljke", fl(k.XCB, k.RF_BOSS_Y[0], k.Z_RF0 - k.TAB_T - 2), (300, -190)),
               ("stezaljka pritiska ivicu pločice", fl(k.XCB + 3, k.RF_BOSS_Y[1] - 6, k.Z_RF0 - k.TAB_T), (-360, 220)),
               ("RC522", fl(k.XCB - 12, k.YCB, k.Z_RF0), (0, 0)),
               ("rupe Ø8,6 za stubove tela", fl(k.POST_C, D - k.POST_C, HB), (40, 120)),
               ("strelica: prednja strana", fl(20, 14, HB - 0.6), (-230, -80)),
               ("pritiskač zvučnika", fl(k.XS, 5, k.ZS + k.SPK_R + 0.4), (150, -90)),
           ])


def check(parts, ghosts):
    """Zapremina preseka izmedju delova i komponenti; sve treba da bude 0."""
    bad = 0
    names = list(parts)
    for i, a in enumerate(names):
        for b in names[i + 1:]:
            v = (parts[a] ^ parts[b]).volume()
            if v > 0.5:
                bad += 1
                print(f"  SUDAR  {a} / {b}: {v:.1f} mm3")
        for gname, g in ghosts.items():
            v = (parts[a] ^ g).volume()
            if v > 0.5:
                bad += 1
                print(f"  SUDAR  {a} / {gname}: {v:.1f} mm3")
    gn = list(ghosts)
    for i, a in enumerate(gn):
        for b in gn[i + 1:]:
            v = (ghosts[a] ^ ghosts[b]).volume()
            if v > 0.5:
                bad += 1
                print(f"  SUDAR  {a} / {b}: {v:.1f} mm3")
    print("provera sudara:", "OK, nema preseka" if bad == 0 else f"{bad} preseka")


def main():
    out = Path(__file__).parent / "slike"
    out.mkdir(exist_ok=True)
    p, g = k.build(), k.build_ghosts()
    check(p, g)
    p["dugme"] = k.knob_assembled()
    g["enkoder"] = g["enkoder"] - p["dugme"]

    def P(*names):
        return [(p[n], COL[n]) for n in names]

    def G(*names):
        return [(g[n], COL[n]) for n in (names or g)]

    inside = [n for n in g if n not in ("figurica", "rfid")]
    lift = lambda m, dz: m.translate((0, 0, dz))

    render(P("telo", "poklopac", "gornja_ploca", "dugme") + G("figurica", "enkoder", "prekidac"),
           out / "1_sklop_napred.png", -62, 24)
    # dugme: lice, zadnja strana (D i nazubljena osovina) i presek na osovini
    kd, kk = k.build_knob("D"), k.build_knob("K")
    render([(kd.rotate((180, 0, 0)).translate((0, 0, k.KNOB_H)), COL["dugme"]),
            (kk.translate((32, 0, 0)), COL["dugme"]), (kd.translate((64, 0, 0)), COL["dugme"])],
           out / "25_dugme.png", -80, 58, width=1400, margin=120, title="Dugme enkodera",
           labels=[("lice sa oznakom", (0, -12, k.KNOB_H), (0, 120)),
                   ("pozadi: nazubljena osovina", (32, -12, k.KNOB_H), (0, 120)),
                   ("pozadi: D osovina", (64, -12, k.KNOB_H), (0, 120))])
    cut = [(m.trim_by_plane((-1, 0, 0), -k.XE), c) for m, c in
           [(p["dugme"], COL["dugme"]), (g["enkoder"], COL["enkoder"]), (p["telo"] ^ k.box(70, 120, -30, 30, 5, 52), COL["telo"])]]
    render(cut, out / "26_dugme_presek.png", 0, 0, width=1200, margin=120, title="Dugme na osovini, presek",
           labels=[("osovina hvata 8 mm", (k.XE, -12, k.ZE + 2.0), (-120, -190)),
                   ("navrtka je u udubljenju", (k.XE, -2.5, k.ZE + 6.5), (230, -150)),
                   ("1,2 mm do zida", (k.XE, -0.6, k.ZE - 11), (220, 160))])
    render(P("telo", "poklopac", "gornja_ploca", "dugme") + G("enkoder", "prekidac", "tp4056"),
           out / "2_sklop_nazad_desno.png", 28, 24)
    render(P("telo"), out / "3_telo_prazno.png", -68, 52)
    render(P("telo") + G(*inside), out / "4_telo_komponente.png", -68, 52)
    render(P("telo") + G(*inside), out / "5_telo_odozgo.png", -90, 90)
    lid_up = p["poklopac"].rotate((180, 0, 0))
    rf_up = union_up(g["rfid"], p["stezaljke"])
    render([(lid_up, COL["poklopac"])], out / "6_poklopac_donja_strana.png", -60, 40)
    render([(lid_up, COL["poklopac"]), (rf_up[0], COL["rfid"]), (rf_up[1], COL["stezaljke"])],
           out / "7_poklopac_sa_rfid.png", -60, 40)
    # presek kroz kopcu baterije
    xc = k.BAT_XA + 0.27 * k.BAT_SPAN
    cut = [(p["telo"].trim_by_plane((-1, 0, 0), -xc), COL["telo"]),
           (g["baterija"].trim_by_plane((-1, 0, 0), -xc), COL["baterija"])]
    render(cut, out / "8_presek_baterija.png", 0, 0, width=1100)
    # presek kroz zvucnik (gleda se sa leve strane)
    cut = [(m.trim_by_plane((1, 0, 0), k.XS), col) for m, col in
           P("telo", "poklopac", "gornja_ploca") + G("zvucnik", "figurica")]
    render(cut, out / "9_presek_zvucnik.png", 180, 0, width=1100)
    # prednji zid iznutra (sine zvucnika), gleda se otpozadi
    half = [(m.trim_by_plane((0, -1, 0), -30), col) for m, col in P("telo") + G("zvucnik")]
    render(half, out / "11_prednji_zid_iznutra.png", 112, 32, width=1300)
    render([(p["telo"].trim_by_plane((0, -1, 0), -30), COL["telo"])], out / "12_sine_zvucnika.png", 112, 32, width=1300)
    # presek duz ose baterije (ceoni zidovi i zlebovi za kontakte), gleda se spreda
    cut = [(p["telo"].trim_by_plane((0, 1, 0), k.YB), COL["telo"]),
           (g["baterija"].trim_by_plane((0, 1, 0), k.YB), COL["baterija"])]
    render(cut, out / "13_presek_duz_baterije.png", -90, 0, width=1500)
    # presek kroz prednje stubove (sraf drzi gornju plocu, stub prolazi kroz poklopac)
    cut = [(m.trim_by_plane((0, 1, 0), k.POST_C), col) for m, col in P("telo", "poklopac", "gornja_ploca")]
    render(cut, out / "14_presek_stubovi.png", -90, 0, width=1500)
    # rastavljeno
    render([(p["telo"], COL["telo"])] + G(*inside)
           + [(lift(g["rfid"], 30), COL["rfid"]), (lift(p["stezaljke"], 30), COL["stezaljke"]),
              (lift(p["poklopac"], 30), COL["poklopac"]), (lift(p["gornja_ploca"], 55), COL["gornja_ploca"]),
              (lift(g["figurica"], 80), COL["figurica"])],
           out / "10_rastavljeno.png", -62, 22, width=1300)
    changes(p, g, out)
    print("slike:", out)


def union_up(rfid, tabs):
    return rfid.rotate((180, 0, 0)), tabs.rotate((180, 0, 0))


if __name__ == "__main__":
    main()
