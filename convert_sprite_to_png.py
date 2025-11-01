import glob
from pathlib import Path
from PIL import Image
from bitstring import ConstBitStream
import numpy as np
from concurrent.futures import ProcessPoolExecutor
import sys
import os

# CONFIG
INPUT_ROOT  = Path("sprite")      # folder that contains the dumped .sprite files
OUTPUT_ROOT = Path("png")         # where the PNGs will be written
SKIP_IF_CONTAINS = "lowend"       # skip files that contain this string (case-insensitive)
VERSION_REQUIRED = 31             # only process HD sprites (version 31)

# Ensure a directory exists
def ensure_dir(p: Path):
    p.mkdir(parents=True, exist_ok=True)


# Decode .sprite file
def decode_sprite(sprite_path: Path) -> None:
    try:
        with sprite_path.open('rb') as f:
            data = ConstBitStream(bytes=f.read())
    except Exception as e:
        print(f"[ERROR] Cannot read {sprite_path}: {e}")
        return

    # header
    data.pos = 0x04 * 8
    version = data.read('uintle:16')
    if version != VERSION_REQUIRED:
        print(f"[SKIP] {sprite_path} – version {version} != {VERSION_REQUIRED}")
        return

    data.pos = 0x08 * 8; width  = data.read('uintle:32')
    data.pos = 0x0C * 8; height = data.read('uintle:32')
    data.pos = 0x14 * 8; frames = data.read('uintle:32')
    data.pos = 0x06 * 8; frame_w = data.read('uintle:16')

    frame_off = width // frames          # distance between frames in the stream

    # output folder
    rel = sprite_path.relative_to(INPUT_ROOT) 
    out_dir = OUTPUT_ROOT / rel.parent 
    ensure_dir(out_dir)

    # decode each frame
    for frm in range(frames):
        # pre-allocate RGBA buffer (height * frame_w)
        buf = np.zeros((height, frame_w, 4), dtype=np.uint8)

        # pixel loop – same layout you already had
        for y in range(height):
            for x in range(frame_w):
                xx = frame_off * frm + x
                data.pos = (0x28 * 8) + (y * width * 4 * 8) + (xx * 4 * 8)
                buf[y, x] = [
                    data.read('uint:8'),   # R
                    data.read('uint:8'),   # G
                    data.read('uint:8'),   # B
                    data.read('uint:8'),   # A
                ]

        # numpy => PIL => PNG
        img = Image.fromarray(buf, mode="RGBA")
        out_name = f"{sprite_path.stem}.{frm:02d}.png"
        out_path = out_dir / out_name
        img.save(out_path, "PNG")
        print(f"[OK] {out_path}")

# Worker for ProcessPoolExecutor
def worker(sprite_path: Path):
    decode_sprite(sprite_path)

# Main => recursive walk + multiprocessing
def main():
    ensure_dir(OUTPUT_ROOT)

    # find **all** .sprite files recursively
    pattern = str(INPUT_ROOT / "**" / "*.sprite")
    sprite_files = [Path(p) for p in glob.glob(pattern, recursive=True)]

    # filter out "lowend"
    filtered = [
        p for p in sprite_files
        if SKIP_IF_CONTAINS.lower() not in p.name.lower()
    ]

    print(f"[INFO] Found {len(filtered)} sprite(s) to process (out of {len(sprite_files)} total).")

    # use all CPU cores
    max_workers = os.cpu_count() or 1
    with ProcessPoolExecutor(max_workers=max_workers) as executor:
        executor.map(worker, filtered)

    print("[DONE] All frames extracted.")

if __name__ == "__main__":
    # Dont run if input folder is missing
    if not INPUT_ROOT.is_dir():
        print(f"[ERROR] Input folder not found: {INPUT_ROOT}")
        sys.exit(1)
    main()