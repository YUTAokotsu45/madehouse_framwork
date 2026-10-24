from PIL import Image
import numpy
import ctypes
import ctypes
import random
import numpy as np


class CPOINT (ctypes.Structure) : 
    _fields_ = [
        ("x", ctypes.c_float),
        ("y", ctypes.c_float),
        ("cluster", ctypes.c_int),
        ("top3", ctypes.c_int * 150),
        ("value", ctypes.c_int),
        ("label", ctypes.c_char),
        ("padding", ctypes.c_ubyte * 3) # Alignement padding 4 octets pour GCC/Windows
    ]

def BufferTusk(image_name: str):
    image = Image.open(image_name).convert("RGBA")

    colors = numpy.array(image)
    shape = colors.shape

    return colors, shape, hex(colors + shape)
    

def CalculateRadius(dataset_ptr, size, k=5, alpha=1.5):
    points = np.array([
        [dataset_ptr[i].x, dataset_ptr[i].y]
        for i in range(size)
    ])

    distances = np.sqrt(
        ((points[:, None] - points[None, :]) ** 2).sum(axis=2)
    )

    np.fill_diagonal(distances, np.inf)

    nearest_distances = np.partition(distances, k - 1, axis=1)[:, k - 1]

    search_raduis = np.median(nearest_distances) * alpha

    return float(search_raduis)

def distance_point (point1, point2) : 
    return abs((point1.x + point1.y) - (point2.x + point2.y))

def KreamNormalize(dataset_ptr, size, search_raduis):
    print("Taille reçue :", size)
    print("Type :", type(dataset_ptr))
    print("Longueur :", len(dataset_ptr))

    for i in range(size - 1):
        dx = dataset_ptr[i + 1].x - dataset_ptr[i].x
        dy = dataset_ptr[i + 1].y - dataset_ptr[i].y

        distance = dx * dx + dy * dy

        if distance >= search_raduis ** 2:
            dataset_ptr[i].x += (search_raduis ** 2 / distance) * dx
            dataset_ptr[i].y += (search_raduis ** 2/ distance) * dy

    return dataset_ptr