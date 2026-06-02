import cv2
import numpy as np


# Minimum contour area in pixels to count as a real spot (filters out noise).
MIN_SPOT_AREA = 30

# A pixel is considered "dark" if its value is below this threshold (0-255).
# Lower = only very dark spots trigger; raise it if light grey marks are missed.
DARK_THRESHOLD = 80

# Image is flagged as defective if at least this many spots are found.
MIN_SPOTS_FOR_DEFECT = 1


def detect_black_spots(image_path: str) -> dict:
    """
    Analyse a JPEG image for black spots on a white/light background.

    Returns a dict:
        {
            'status': 'clean' | 'defect',
            'spot_count': int,
            'notes': str,
        }
    """
    img = cv2.imread(image_path)

    if img is None:
        return {'status': 'pending', 'spot_count': 0, 'notes': 'Could not read image file'}

    # Convert to greyscale
    grey = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

    # Mild Gaussian blur to reduce JPEG compression artefacts
    blurred = cv2.GaussianBlur(grey, (5, 5), 0)

    # Threshold: dark pixels become white (255), light background becomes black (0)
    _, binary = cv2.threshold(blurred, DARK_THRESHOLD, 255, cv2.THRESH_BINARY_INV)

    # Remove tiny specks with morphological opening
    kernel = np.ones((3, 3), np.uint8)
    cleaned = cv2.morphologyEx(binary, cv2.MORPH_OPEN, kernel, iterations=1)

    # Find contours of dark regions
    contours, _ = cv2.findContours(cleaned, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    # Keep only contours large enough to be real spots
    spots = [c for c in contours if cv2.contourArea(c) >= MIN_SPOT_AREA]
    spot_count = len(spots)

    if spot_count >= MIN_SPOTS_FOR_DEFECT:
        status = 'defect'
        notes = f'{spot_count} black spot(s) detected'
    else:
        status = 'clean'
        notes = 'No defects found'

    return {'status': status, 'spot_count': spot_count, 'notes': notes}
