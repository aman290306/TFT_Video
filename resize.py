from pathlib import Path
from PIL import Image, ImageOps
import re

INPUT_FOLDER = Path("frames")
OUTPUT_FOLDER = Path("resized_frames")

WIDTH = 160
HEIGHT = 128
JPEG_QUALITY = 70

OUTPUT_FOLDER.mkdir(exist_ok=True)

def natural_order(path):
    """Sort frame2 before frame10."""
    return [
        int(part) if part.isdigit() else part.lower()
        for part in re.split(r"(\d+)", path.name)
    ]

extensions = {".jpg", ".jpeg", ".png", ".bmp"}
frames = sorted(
    (file for file in INPUT_FOLDER.iterdir()
     if file.suffix.lower() in extensions),
    key=natural_order
)

print(f"Found {len(frames)} frames")

for frame_number, input_path in enumerate(frames):
    with Image.open(input_path) as image:
        image = image.convert("RGB")

        # Resize to fill 160×128 while preserving aspect ratio.
        resized = ImageOps.fit(
            image,
            (WIDTH, HEIGHT),
            method=Image.Resampling.LANCZOS,
            centering=(0.5, 0.5)
        )

        output_path = OUTPUT_FOLDER / f"frame_{frame_number:04d}.jpg"

        resized.save(
            output_path,
            format="JPEG",
            quality=JPEG_QUALITY,
            optimize=True
        )

    print(f"\rResized {frame_number + 1}/{len(frames)}", end="")

print(f"\nFinished. Frames saved in: {OUTPUT_FOLDER}")