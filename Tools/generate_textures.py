import math
import os
import struct
import zlib

SIZE = 128
OUTPUT = os.path.join(os.path.dirname(__file__), "SourceTextures")

PALETTES = {
    "Grass": ((67, 139, 73), (101, 173, 86), (39, 101, 63)),
    "Dirt": ((124, 77, 49), (158, 103, 65), (91, 56, 42)),
    "Stone": ((91, 108, 121), (137, 151, 158), (59, 72, 86)),
    "Wood": ((113, 70, 42), (166, 112, 62), (76, 49, 39)),
    "Leaves": ((37, 105, 61), (78, 151, 73), (24, 76, 53)),
    "Sand": ((193, 165, 104), (229, 205, 143), (159, 130, 79)),
    "Crystal": ((41, 143, 166), (91, 222, 214), (53, 89, 166)),
    "Water": ((34, 91, 145), (70, 151, 193), (25, 62, 115)),
    "Creature": ((39, 114, 67), (92, 163, 79), (24, 76, 53)),
}


def hash01(x, y, seed):
    value = (x * 374761393 + y * 668265263 + seed * 1442695041) & 0xFFFFFFFF
    value = ((value ^ (value >> 13)) * 1274126177) & 0xFFFFFFFF
    value ^= value >> 16
    return value / 4294967295.0


def value_noise(x, y, grid, seed):
    px = x * grid / SIZE
    py = y * grid / SIZE
    x0 = int(px)
    y0 = int(py)
    tx = px - x0
    ty = py - y0
    sx = tx * tx * (3.0 - 2.0 * tx)
    sy = ty * ty * (3.0 - 2.0 * ty)

    def sample(ix, iy):
        return hash01(ix % grid, iy % grid, seed)

    top = sample(x0, y0) * (1.0 - sx) + sample(x0 + 1, y0) * sx
    bottom = sample(x0, y0 + 1) * (1.0 - sx) + sample(x0 + 1, y0 + 1) * sx
    return top * (1.0 - sy) + bottom * sy


def write_png(path, pixels):
    raw = bytearray()
    for row in pixels:
        raw.append(0)
        raw.extend(row)

    def chunk(name, data):
        return struct.pack(">I", len(data)) + name + data + struct.pack(">I", zlib.crc32(name + data) & 0xFFFFFFFF)

    image = b"\x89PNG\r\n\x1a\n"
    image += chunk(b"IHDR", struct.pack(">2I5B", SIZE, SIZE, 8, 6, 0, 0, 0))
    image += chunk(b"IDAT", zlib.compress(bytes(raw), 9))
    image += chunk(b"IEND", b"")
    with open(path, "wb") as file:
        file.write(image)


def make_texture(name, seed, palette):
    low, high, shadow = palette
    colors = []
    heights = []
    for y in range(SIZE):
        color_row = bytearray()
        height_row = []
        for x in range(SIZE):
            broad = value_noise(x, y, 4, seed)
            medium = value_noise(x, y, 16, seed + 3)
            fine = value_noise(x, y, 64, seed + 7)
            fleck = value_noise(x, y, 32, seed + 11)
            grain = (broad - 0.5) * 0.72 + (medium - 0.5) * 0.78 + (fine - 0.5) * 0.46
            pattern = 0.0
            if name == "Wood":
                pattern = math.sin((x * 5 + y) * math.tau / SIZE) * 0.19
            elif name == "Crystal":
                pattern = math.sin((x + y * 2) * math.tau * 7 / SIZE) * 0.18
            elif name == "Sand":
                pattern = math.sin((x + y * 3) * math.tau * 9 / SIZE) * 0.10
            amount = max(0.0, min(1.0, 0.50 + grain + pattern))
            if fleck < 0.09:
                target = shadow
                amount = 0.88
            elif fleck > 0.94:
                target = high
                amount = 0.90
            else:
                target = high
            rgb = tuple(int(max(0, min(255, low[i] * (1.0 - amount) + target[i] * amount))) for i in range(3))
            color_row.extend((*rgb, 255))
            height_row.append(int((broad * 0.45 + medium * 0.4 + fine * 0.15) * 255))
        colors.append(color_row)
        heights.append(height_row)

    normals = []
    for y in range(SIZE):
        row = bytearray()
        for x in range(SIZE):
            dx = (heights[y][(x + 1) % SIZE] - heights[y][(x - 1) % SIZE]) / 510.0
            dy = (heights[(y + 1) % SIZE][x] - heights[(y - 1) % SIZE][x]) / 510.0
            nx = -dx * 1.8
            ny = -dy * 1.8
            nz = 1.0
            length = math.sqrt(nx * nx + ny * ny + nz * nz)
            row.extend((int((nx / length * 0.5 + 0.5) * 255), int((ny / length * 0.5 + 0.5) * 255), int((nz / length * 0.5 + 0.5) * 255), 255))
        normals.append(row)

    write_png(os.path.join(OUTPUT, f"T_{name}_BaseColor.png"), colors)
    write_png(os.path.join(OUTPUT, f"T_{name}_Normal.png"), normals)


def main():
    os.makedirs(OUTPUT, exist_ok=True)
    for index, (name, palette) in enumerate(PALETTES.items()):
        make_texture(name, 4409 + index * 97, palette)
    print(f"Generated {len(PALETTES) * 2} tileable textures in {os.path.abspath(OUTPUT)}")


if __name__ == "__main__":
    main()
