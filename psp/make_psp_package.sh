#!/bin/sh
# Assemble a PSP memory-stick folder for KeeperFX.
#
#   psp/make_psp_package.sh <dk_dir> <kfx_release_dir> <eboot> <out_dir>
#
# Run from a KeeperFX source checkout: its configs, campaign/level files and
# language files (built with "make pkg-languages") replace the release's, so
# the data matches the code the EBOOT was built from.
#
# <dk_dir>          original Dungeon Keeper files (e.g. the GOG installer
#                   extracted with innoextract: DATA/, LDATA/, SOUND/, keeper0*.ogg)
# <kfx_release_dir> an extracted KeeperFX "complete" release archive
# <eboot>           out-psp/EBOOT.PBP
# <out_dir>         folder to copy to ms0:/PSP/GAME/KeeperFX/
#
# Mirrors what the KeeperFX launcher does on Windows: KeeperFX's own files win,
# original DK files only fill in what is missing. File names are lowercased.
set -eu

DK=$1
KFX=$2
EBOOT=$3
OUT=$4
HERE=$(cd "$(dirname "$0")" && pwd)
SRC=$(dirname "$HERE")

mkdir -p "$OUT"

# KeeperFX release data, without the Windows executables/launchers/libraries.
rsync -a \
    --exclude='*.exe' --exclude='*.dll' --exclude='*.DLL' --exclude='*.map' \
    --exclude='launch.sh' --exclude='launchermn.jpg' --exclude='launcher-*.txt' \
    "$KFX"/ "$OUT"/

# Data from this source tree, so configs and string tables match the EBOOT.
make -C "$SRC" pkg-languages >/dev/null
rsync -a "$SRC/config/fxdata/" "$OUT/fxdata/"
rsync -a "$SRC/config/creatrs/" "$OUT/creatrs/"
rsync -a "$SRC/campgns/" "$OUT/campgns/"
rsync -a "$SRC/levels/" "$OUT/levels/"
rsync -a "$SRC/pkg/" "$OUT/"

# The release and the source tree name a few files with different case
# (MAP00001.TXT vs map00001.txt); the PSP's FAT filesystem can only hold one.
# Keep the copy that exists in the source tree.
src_path_of() {
    case "$1" in
        fxdata/*|creatrs/*) echo "$SRC/config/$1" ;;
        *) echo "$SRC/$1" ;;
    esac
}
(cd "$OUT" && find . -type f | sed 's|^\./||') \
    | awk '{ k = tolower($0); if (k in seen) { print seen[k]; print $0 } else seen[k] = $0 }' \
    | sort -u | while read -r f; do
        [ -e "$(src_path_of "$f")" ] || rm -f "$OUT/$f"
    done

# Copy one DK folder into a lowercase destination, never overwriting.
copy_dk_dir() {
    src=$1
    dst=$2
    [ -d "$src" ] || return 0
    mkdir -p "$dst"
    find "$src" -maxdepth 1 -type f | while read -r f; do
        name=$(basename "$f" | tr 'A-Z' 'a-z')
        [ -e "$dst/$name" ] || cp "$f" "$dst/$name"
    done
}

for d in DATA data; do copy_dk_dir "$DK/$d" "$OUT/data"; done
for d in LDATA ldata; do copy_dk_dir "$DK/$d" "$OUT/ldata"; done
for d in SOUND sound; do copy_dk_dir "$DK/$d" "$OUT/sound"; done

# GOG ships the CD music as keeper02..07.ogg.
mkdir -p "$OUT/music"
for f in "$DK"/keeper0*.ogg; do
    [ -e "$f" ] && [ ! -e "$OUT/music/$(basename "$f")" ] && cp "$f" "$OUT/music/"
done

cp "$EBOOT" "$OUT/EBOOT.PBP"
cp "$HERE/keeperfx.cfg" "$OUT/keeperfx.cfg"
mkdir -p "$OUT/save" "$OUT/scrshots"

echo "PSP package ready: $OUT"
