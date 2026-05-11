import pandas as pd         # import: könyvtár betöltése; pandas: táblázatos adatkezelés; as pd: rövid hivatkozás
import matplotlib.pyplot as plt # matplotlib.pyplot: alapvető grafikonrajzoló modul; as plt: rövid név
import seaborn as sns       # seaborn: statisztikai adatvizualizációs könyvtár (szebb grafikonok)

# Alap stílus beállítása
sns.set(style="whitegrid")  # sns.set: globális megjelenés beállítása; "whitegrid": fehér háttér szürke négyzethálóval

# --- CSV beolvasása ---
df = pd.read_csv(           # read_csv: CSV fájl betöltése DataFrame objektumba (df)
    "results.csv",          # a beolvasandó fájl neve
    names=["matrix_size", "iterations", "threads", "schedule", "chunk_size", "time"], # names: oszlopnevek manuális megadása
    header=0                # header=0: az eredeti fájl első sorát (ha van fejléc) figyelmen kívül hagyja
)

# Tisztítás: Strip, ha szóközök lennének az oszlopokban (pl. " static" helyett "static")
df["schedule"] = df["schedule"].str.strip()     # str.strip(): levágja a láthatatlan szóközöket a szövegek végéről
df["matrix_size"] = df["matrix_size"].str.strip() # matrix_size oszlop tisztítása (pl. "100x100")

# Típuskonverziók (Biztosítjuk, hogy a számok valóban számként legyenek tárolva)
df["threads"] = df["threads"].astype(int)       # astype(int): egész számmá alakítja a szálak oszlopát
df["time"] = df["time"].astype(float)           # astype(float): tizedes tört számmá alakítja a futásidőt

# === 1. Problémaméret vs. futásidő (fix szál: 4, fix ütemezés: static) ===
threads_fixed = 4           # változó: rögzítjük a szálak számát az összehasonlíthatóság miatt
schedule_fixed = "static"   # változó: rögzítjük az ütemezési módot

# Szűrés (subset): Csak azokat a sorokat tartjuk meg, ahol a szál=4 és az ütemezés=static
subset = df[(df["threads"] == threads_fixed) & (df["schedule"] == schedule_fixed)]

plt.figure(figsize=(8, 5))  # figure: új grafikon ablak; figsize: méret meghatározása hüvelykben (szélesség, magasság)
sns.barplot(data=subset, x="matrix_size", y="time") # barplot: oszlopdiagram; x: kategóriák; y: értékek
plt.title(f"Problémaméret vs. Futásidő\n(threads={threads_fixed}, schedule={schedule_fixed})") # f-string: változó behelyettesítése a címbe
plt.xlabel("Mátrixméret")   # xlabel: X tengely felirata
plt.ylabel("Futásidő [s]")  # ylabel: Y tengely felirata (idő másodpercben)
plt.tight_layout()          # tight_layout(): automatikus margóigazítás, hogy ne lógjanak le a feliratok
plt.savefig("scaling_problem_size.png") # savefig(): kép mentése lemezre
plt.close()                 # close(): memória felszabadítása (lekapcsolja az aktuális grafikont)

# === 2. Szálak száma vs. futásidő (fix problémaméret: 1000x1000, fix ütemezés: static) ===
matrix_fixed = "1000x1000"  # rögzítjük a mátrix méretét
# Újabb szűrés: nézzük meg, hogyan gyorsul a program, ha egyre több szálat adunk neki
subset = df[(df["matrix_size"] == matrix_fixed) & (df["schedule"] == schedule_fixed)]

plt.figure(figsize=(8, 5))  # új grafikon méretezése
sns.lineplot(data=subset, x="threads", y="time", marker="o") # lineplot: vonaldiagram; marker="o": körök a mérési pontoknál
plt.title(f"Szálak száma vs. Futásidő\n(matrix={matrix_fixed}, schedule={schedule_fixed})") # cím beállítása
plt.xlabel("Szálak száma")  # X tengely: processzormagok száma
plt.ylabel("Futásidő [s]")  # Y tengely: idő
plt.tight_layout()          # margók igazítása
plt.savefig("scaling_threads.png") # második kép mentése
plt.close()                 # grafikon bezárása