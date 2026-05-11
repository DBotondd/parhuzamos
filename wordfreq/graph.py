import pandas as pd         # import: könyvtár beöltése; pandas: adatelemző csomag; as pd: rövidítés a kódhoz
import matplotlib.pyplot as plt # matplotlib.pyplot: grafikonrajzoló modul; as plt: rövidebb hivatkozás

# Beolvasás: külön szakaszok a .csv fájlból
with open("results/measurements.csv") as f: # with: kontextuskezelő (automatikusan bezárja a fájlt); open: fájlnyitás; as f: fájlobjektum neve
    lines = f.readlines()   # readlines(): beolvassa a fájl összes sorát egy listába

# Szétválasztás (Adatlisták inicializálása)
file_sizes = []             # []: üres lista létrehozása a fájlméreteknek (X tengely 1)
file_times = []             # üres lista a fájlmérethez tartozó időknek (Y tengely 1)
thread_counts = []          # üres lista a szálak számának (X tengely 2)
thread_times = []           # üres lista a szálakhoz tartozó időknek (Y tengely 2)

mode = None                 # mode: segédváltozó a szakaszok felismeréséhez; None: üres érték (null)
for line in lines:          # for: ciklus, amely végigmegy a beolvasott sorok listáján
    line = line.strip()     # strip(): eltávolítja a sor elejéről és végéről a szóközöket/újsor karaktereket
    if line.startswith("#"): # startswith("#"): megvizsgálja, hogy a sor kommenttel (fejléccel) kezdődik-e
        if "Problémaméret" in line: # in: tartalmazás vizsgálat; ha a sorban szerepel ez a szó
            mode = "file"   # átváltunk fájlméret-feldolgozó módba
        elif "Szálak" in line: # elif: egyébként ha; ha a másik szakasz fejlécét találjuk
            mode = "thread" # átváltunk szálkezelés-feldolgozó módba
        continue            # continue: átugorja a ciklus maradék részét és a következő sorra lép
    if "," not in line or line.startswith("Méret") or line.startswith("Szálak"): # logikai szűrés
        continue            # ha nincs benne vessző vagy fejléc szöveg, átugorjuk (üres/hibás sorok)
    x, y = line.split(",")  # split(","): a sort a vessző mentén két darabba vágja; x és y változóba teszi
    if mode == "file":      # ha épp a fájlméret szakaszt olvassuk
        file_sizes.append(int(x)) # append(): elem hozzáadása a listához; int(x): egész számmá alakítás
        file_times.append(float(y.replace("m", "").replace("s", ""))) # float(): tizedes tört; replace(): betűk eltávolítása az időből
    elif mode == "thread":  # ha épp a szálak számának szakaszát olvassuk
        thread_counts.append(int(x)) # szálak számának eltárolása listába
        thread_times.append(float(y.replace("m", "").replace("s", ""))) # tiszta időkiszámítás és tárolás

# 1. Fájlméret vs futásidő grafikon készítése
plt.figure()                # figure(): új grafikon ablak/vászon létrehozása
plt.plot(file_sizes, file_times, marker="o") # plot(): vonaldiagram rajzolása; marker="o": kör alakú pontok az adatoknál
plt.title("Futásidő a fájlméret függvényében (4 szál)") # title(): a grafikon főcímének beállítása
plt.xlabel("Fájlméret (MB)") # xlabel(): vízszintes tengely feliratozása
plt.ylabel("Futásidő (s)")   # ylabel(): függőleges tengely feliratozása
plt.grid(True)               # grid(True): négyzetháló megjelenítése a könnyebb olvashatóságért
plt.savefig("results/graph_fajlmeret.png") # savefig(): a grafikon mentése képfájlként a megadott útvonalra

# 2. Szálak száma vs futásidő grafikon készítése
plt.figure()                # új vászon a második grafikonnak
plt.plot(thread_counts, thread_times, marker="o") # adatok kirajzolása (szálak száma vs idő)
plt.title("Futásidő a szálak számának függvényében (100MB fájl)") # második grafikon címe
plt.xlabel("Szálak száma")  # X tengely: hány szálon futott a program
plt.ylabel("Futásidő (s)")  # Y tengely: mennyi ideig tartott a munka
plt.grid(True)               # háló bekapcsolása
plt.savefig("results/graph_szalak.png") # második grafikon mentése képként