import glob
from pathlib import Path
from PIL import Image
from bitstring import ConstBitStream
import numpy as np
from concurrent.futures import ProcessPoolExecutor
import sys
import os

# Ensure a directory exists
def ensure_dir(p: Path):
    p.mkdir(parents=True, exist_ok=True)

# choose the d2r version to look at
def choose_version() -> str:
    cwd = Path.cwd()
    candidates = [
        p for p in cwd.iterdir()
        if p.is_dir() and "_" in p.name and p.name.replace("_", "").isdigit()
    ]
    if not candidates:
        print("[ERROR] No version folder found (e.g. 1_6_89560)")
        sys.exit(1)

    candidates.sort(key=lambda x: x.name)
    folder_names = [p.name for p in candidates]

    print("[INFO] Available version folders:")
    for i, name in enumerate(folder_names, 1):
        print(f"  {i}. {name}")

    while True:
        choice = input("\nEnter the number of the version to process: ").strip()
        try:
            idx = int(choice) - 1
            if 0 <= idx < len(folder_names):
                selected = folder_names[idx]
                print(f"[SELECTED] Using: {selected}")
                return selected
        except ValueError:
            pass
        print("  Invalid input – please enter a number from the list.")

# Decode .sprite file
def decode_sprite(args: tuple[Path, Path, Path]) -> None:
    sprite_path, input_root, output_root = args

    try:
        with sprite_path.open('rb') as f:
            data = ConstBitStream(bytes=f.read())
    except Exception as e:
        print(f"[ERROR] Cannot read {sprite_path}: {e}")
        return

    VERSION_REQUIRED = 31

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
    rel = sprite_path.relative_to(input_root) 
    out_dir = output_root / rel.parent 
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
def worker(task: tuple[Path, Path, Path]) -> None:
    decode_sprite(task)

# Main => recursive walk + multiprocessing
def main():
    version = choose_version()
    input_root = Path(version) / "sprite"
    output_root = Path(f"png_{version}")

    if not input_root.is_dir():
        print(f"[ERROR] Input folder not found: {input_root}")
        sys.exit(1)

    ensure_dir(output_root)

    # find **all** .sprite files recursively
    pattern = str(input_root / "**" / "*.sprite")
    sprite_files = [Path(p) for p in glob.glob(pattern, recursive=True)]

    SKIP_IF_CONTAINS = "lowend"

    # filter out "lowend"
    filtered = [
        p for p in sprite_files
        if SKIP_IF_CONTAINS.lower() not in p.name.lower()
    ]

    print(f"[INFO] Found {len(filtered)} sprite(s) to process (out of {len(sprite_files)} total).")

    # use all CPU cores and build tasks with the files and paths
    max_workers = os.cpu_count() or 1
    tasks = [(p, input_root, output_root) for p in filtered]

    with ProcessPoolExecutor(max_workers=max_workers) as executor:
        executor.map(worker, tasks)

    print("[DONE] All frames extracted.")

if __name__ == "__main__":
    main()