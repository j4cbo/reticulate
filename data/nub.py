import struct

def nub(points, knots):
    assert len(points) + 3 == len(knots)

    out = struct.pack("<cccci", "n", "u", "b", "\003", len(points))

    for pt in points:
        out += struct.pack("fff", *pt)

    for k in knots:
        out += struct.pack("f", k)

    return out

with file("circle.nub", "w") as f:
    f.write(nub(
        (
            (1, 0, 1),
            (1, 1, 0.7071067811865476),
            (0, 1, 1),
            (-1, 1, 0.7071067811865476),
            (-1, 0, 1),
            (-1, -1, 0.7071067811865476),
            (0, -1, 1),
            (1, -1, 0.7071067811865476),
            (1, 0, 1)
        ),
        (0, 0, 0, 0.25, 0.25, 0.5, 0.5, 0.75, 0.75, 1, 1, 1)
    ))

