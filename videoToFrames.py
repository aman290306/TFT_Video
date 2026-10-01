import cv2
import numpy as np
import os

video_file = "input_video.mp4"
cap = cv2.VideoCapture(video_file)

if not cap.isOpened():
    print("Error opening video file")
    exit()

frames = []
while True:
    ret, frame = cap.read()
    if not ret:
        break
    # Convert to RGB if needed
    frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    frames.append(frame)

cap.release()
frames = np.array(frames)  # shape = (num_frames, height, width, 3)
print("Frames shape:", frames.shape)

# Example: save frames as PNGs (optional)
os.makedirs("frames", exist_ok=True)
for i, f in enumerate(frames):
    cv2.imwrite(f"frames/frame_{i:04d}.png", cv2.cvtColor(f, cv2.COLOR_RGB2BGR))