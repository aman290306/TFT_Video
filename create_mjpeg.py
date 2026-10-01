from pathlib import Path
from PIL import Image, ImageOps
from io import BytesIO
import re

INPUT_FOLDER = Path("resized_frames")
OUTPUT_FILE = Path("video_160x128_30fps.mjpeg")

WIDTH = 160
HEIGHT = 128
JPEG_QUALITY = 70

def natural_order(path):
    return [
        int(part) if part.isdigit() else part.lower()
        for part in re.split(r"(\d+)", path.name)
    ]

frames = sorted(
    INPUT_FOLDER.glob("*.jpg"),
    key=natural_order
)

print(f"Found {len(frames)} JPG frames")

if not frames:
    raise RuntimeError("No JPG images found in resized_frames")

with OUTPUT_FILE.open("wb") as video_file:
    for index, frame_path in enumerate(frames):
        with Image.open(frame_path) as image:
            image = image.convert("RGB")

            image = ImageOps.fit(
                image,
                (WIDTH, HEIGHT),
                method=Image.Resampling.LANCZOS
            )

            jpeg_data = BytesIO()

            image.save(
                jpeg_data,
                format="JPEG",
                quality=JPEG_QUALITY,
                optimize=True
            )

            # Add the JPEG directly to the MJPEG stream
            video_file.write(jpeg_data.getvalue())

        print(
            f"\rProcessed {index + 1}/{len(frames)}",
            end=""
        )

print(f"\nCreated: {OUTPUT_FILE.resolve()}")
print(f"Size: {OUTPUT_FILE.stat().st_size / 1024:.1f} KB")