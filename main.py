
import ctypes
import os
import time

# 1. Charger la DLL
dll_path = os.path.abspath("Kream.dll")
kream = ctypes.CDLL(dll_path)

# 2. Déclarer la signature de KreamPixel
kream.KreamPixel.argtypes = [
    ctypes.c_char_p,
    ctypes.c_char_p
]
kream.KreamPixel.restype = ctypes.c_char_p

# 3. Images à tester
image_test = os.path.abspath("6.jpg")
image_temoin = os.path.abspath("7.png")

if not os.path.isfile(image_test):
    raise FileNotFoundError(image_test)

if not os.path.isfile(image_temoin):
    raise FileNotFoundError(image_temoin)

# 4. Appel de KreamPixel via la DLL
start = time.perf_counter()

resultat = kream.KreamPixel(
    os.fsencode(image_test),
    os.fsencode(image_temoin)
)

print("Résultat brut :", repr(resultat))
print("Type :", type(resultat))
temps = (time.perf_counter() - start) * 1000

if resultat is not None:
    resultat = resultat.decode("utf-8")

# 5. Affichage
print("=" * 40)
print("TEST KREAMPIXEL")
print("=" * 40)
print("Image test :", image_test)
print("Image témoin :", image_temoin)
print("Résultat brut :", resultat)

if resultat == "YES" : print("OUI")
elif resultat == "No" : print("NON")
elif resultat == "Erreur de chargement" : print("Erreur de chargement")

print(f"Temps d'exécution : {temps:.4f} ms")