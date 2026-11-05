#!/usr/bin/env bash
# =============================================================================
# fix-qt6-static.sh
#
# Corregge la struttura del pacchetto MSYS2 "mingw-w64-ucrt-x86_64-qt6-static"
# in modo che qmake (e Qt Creator) trovino tutto dove se lo aspettano.
#
# Il pacchetto installa alcune parti sotto qt6-static/share/qt6 e
# qt6-static/include/qt6, mentre qmake le cerca direttamente sotto qt6-static.
# Lo script copia:
#   share/qt6/mkspecs     -> mkspecs
#   share/qt6/plugins     -> plugins
#   share/qt6/bin/*       -> bin      (uic, moc, rcc, ...; senza toccare i file del pacchetto)
#   include/qt6/*         -> include
# e controlla che ci siano le librerie statiche richieste dal link.
#
# Da rilanciare dopo ogni aggiornamento di qt6-static con pacman.
# Uso (nella shell "MSYS2 UCRT64"):
#     bash fix-qt6-static.sh
# Poi in Qt Creator: Build -> Run qmake, Build -> Rebuild All.
# =============================================================================
set -euo pipefail

QT="${QT6S:-/ucrt64/qt6-static}"
PKG="mingw-w64-ucrt-x86_64-qt6-static"
SHARE="$QT/share/qt6"

die() { echo "ERRORE: $*" >&2; exit 1; }
ok()  { echo "  [ok] $*"; }

[ -d "$QT" ]    || die "$QT non esiste. Il pacchetto $PKG e' installato?"
[ -d "$SHARE" ] || die "$SHARE non esiste: la struttura del pacchetto e' cambiata, controllare a mano."

echo "== Qt statica in: $QT"

# Un vecchio qt.conf (tentativo non riuscito) puo' solo confondere: lo rimuovo.
if [ -f "$QT/bin/qt.conf" ]; then
  rm -f "$QT/bin/qt.conf"
  ok "rimosso vecchio $QT/bin/qt.conf"
fi

# --- 1. mkspecs e plugins: cartelle non del pacchetto, le ricreo da zero -----
for d in mkspecs plugins; do
  [ -d "$SHARE/$d" ] || die "manca $SHARE/$d"
  rm -rf "${QT:?}/$d"
  cp -r "$SHARE/$d" "$QT/$d"
  ok "$d copiato"
done

# --- 2. header: include/qt6/<Modulo> -> include/<Modulo> ----------------------
if [ -d "$QT/include/qt6" ]; then
  n=0
  for src in "$QT/include/qt6"/*; do
    name="$(basename "$src")"
    rm -rf "${QT:?}/include/$name"
    cp -r "$src" "$QT/include/$name"
    n=$((n+1))
  done
  ok "header copiati ($n moduli)"
else
  ok "include/qt6 non presente: header gia' nella posizione standard"
fi

# --- 3. strumenti (uic, moc, rcc, ...): share/qt6/bin -> bin -------------------
# Non sovrascrivo mai i file che appartengono al pacchetto (es. qmake.exe).
if [ -d "$SHARE/bin" ]; then
  owned="$(pacman -Qlq "$PKG" 2>/dev/null || true)"
  n=0
  for src in "$SHARE/bin"/*; do
    [ -f "$src" ] || continue
    dst="$QT/bin/$(basename "$src")"
    if printf '%s\n' "$owned" | grep -qxF "$dst"; then
      continue
    fi
    cp -f "$src" "$dst"
    n=$((n+1))
  done
  ok "strumenti copiati in bin ($n file)"
fi

# --- 4. verifiche --------------------------------------------------------------
echo "== Verifiche"
missing=0
for f in \
  "mkspecs/win32-g++/qmake.conf" \
  "bin/qmake.exe" "bin/uic.exe" "bin/moc.exe" "bin/rcc.exe" \
  "include/QtWidgets/QApplication" \
  "lib/libQt6Core.a" \
  "plugins/platforms/libqwindows.a" \
  "plugins/imageformats/libqico.a"
do
  if [ -e "$QT/$f" ]; then ok "$f"; else echo "  [MANCA] $f"; missing=1; fi
done

# Librerie statiche esterne usate nel blocco statico del .pro
libs="harfbuzz freetype graphite2 png16 bz2 brotlidec brotlicommon z glib-2.0 intl iconv pcre2-8"
miss_libs=""
for l in $libs; do
  [ -f "/ucrt64/lib/lib$l.a" ] || miss_libs="$miss_libs lib$l.a"
done
if [ -n "$miss_libs" ]; then
  echo "  [MANCANO] librerie statiche:$miss_libs"
  echo "  Installale con:"
  echo "  pacman -S --needed mingw-w64-ucrt-x86_64-{libpng,glib2,graphite2,harfbuzz,freetype,bzip2,brotli,zlib,pcre2,gettext-runtime,libiconv}"
  missing=1
else
  ok "librerie statiche esterne presenti"
fi

echo
echo "Qt versione: $("$QT/bin/qmake.exe" -query QT_VERSION 2>/dev/null || echo '?')"
if [ "$missing" -eq 0 ]; then
  echo "Tutto a posto. In Qt Creator: Build -> Run qmake, poi Build -> Rebuild All."
else
  echo "Ci sono elementi mancanti (vedi sopra)."
  exit 2
fi
