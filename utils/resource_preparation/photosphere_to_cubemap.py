#!/usr/bin/env python3

# ruff: noqa: T201 `print` found


import argparse
import os
from math import pi

import numpy as np
from PIL import Image
from scipy.ndimage import map_coordinates

# A dictionary to define the 3D vector transformations for each face.
# The key is the face name, and the value is a lambda function that
# takes a grid of (x, y) coordinates and returns a 3D vector.
FACE_CONFIG = {
    "front": lambda x, y: np.stack([np.ones_like(x) * -1, x, y]),
    "back": lambda x, y: np.stack([np.ones_like(x), -x, y]),
    "right": lambda x, y: np.stack([-x, np.ones_like(x) * -1, y]),
    "left": lambda x, y: np.stack([x, np.ones_like(x), y]),
    "down": lambda x, y: np.stack([y, -x, np.ones_like(x) * -1]),
    "up": lambda x, y: np.stack([-y, -x, np.ones_like(x)]),
}


def equirectangular_to_cubemap(
    equi_img: np.ndarray,
    face_resolution: int,
    output_dir: str,
    output_format: str = "png",
    *,
    v_flip: bool = True,
):
    """
    Converts an equirectangular image into the 6 faces of a cubemap.

    Args:
        equi_img (np.ndarray): The input equirectangular image as a NumPy array.
        face_resolution (int): The target resolution for each square cubemap face.
        output_dir (str): The directory to save the output cubemap faces.
        v_flip (bool): If True, flips the vertical axis of the output faces.
        output_format (str): The image format for the output files (e.g., 'png', 'jpg').
    """
    height, width, _ = equi_img.shape

    # Create the output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)

    # Create a grid of coordinates for a single face
    face_coords = np.linspace(-1, 1, face_resolution, dtype=np.float32)
    xx, yy = np.meshgrid(face_coords, face_coords)

    for face_name, face_transform in FACE_CONFIG.items():
        print(f"Generating face: {face_name}...")

        vec_3d = face_transform(xx, yy)
        vec_3d /= np.linalg.norm(vec_3d, axis=0, keepdims=True)
        x, y, z = vec_3d

        lon = np.arctan2(y, x)
        lat = np.arccos(z)

        u = (lon / (2 * pi) + 0.5) * (width - 1)
        v = (lat / pi) * (height - 1)

        coords = np.stack([v, u, np.zeros_like(v)])
        face_img = np.zeros((face_resolution, face_resolution, 3), dtype=np.uint8)

        for channel in range(3):
            coords[2] = channel
            face_img[..., channel] = map_coordinates(
                equi_img,
                coords,
                order=1,
                mode='wrap'
            ).reshape(face_resolution, face_resolution)

        if v_flip:
            face_img = np.flipud(face_img)

        output_path = os.path.join(output_dir, f"{face_name}.{output_format}")
        Image.fromarray(face_img).save(output_path)
        print(f"  -> Saved {output_path}")

    print("\nConversion complete!")


def main():
    """Main function to handle command-line arguments."""
    parser = argparse.ArgumentParser(description="Convert a Google Photosphere (equirectangular) image to a cubemap.")
    parser.add_argument("infile", type=str, help="Path to the input equirectangular image.")
    parser.add_argument(
        "--res", type=int, default=256, help="Target resolution for each cubemap face (e.g., 256 -> 256x256)."
    )
    parser.add_argument("--outdir", type=str, default="cubemap_faces", help="Directory to save the output.")
    parser.add_argument("--vflip", action="store_false", help="Flip the vertical axis of the output cubemap faces.")
    args = parser.parse_args()

    try:
        with Image.open(args.infile) as img:
            equi_img_np = np.array(img)

        equirectangular_to_cubemap(equi_img_np, args.res, args.outdir, v_flip=args.vflip)

    except FileNotFoundError:
        print(f"Error: Input file not found at '{args.infile}'")


if __name__ == "__main__":
    main()
