#!/usr/bin/env bash
# ============================================================
#  Build (i opcjonalnie flash) z ustawionym T i poll interval
# ============================================================
#
#   ./build.sh <SEND_INTERVAL_S> [POLL_INTERVAL_S] [--flash] [--erase]
#
#   ./build.sh 10                 # T=10 s, poll 3600 s (domyslny)  <- typowy pomiar
#   ./build.sh 10 --flash         # to samo + wgranie na plytke
#   ./build.sh 10 3600 --flash    # to samo, poll podany wprost
#   ./build.sh 60 60 --flash      # osobny eksperyment: poll rowny T
#   ./build.sh 300 --deep --flash # System OFF miedzy raportami (reboot co cykl)
#   ./build.sh 10 --erase --flash # pelne kasowanie flasha przed wgraniem
#
# POLL_INTERVAL_S jest OPCJONALNY i domyslnie wynosi 3600 s. Krotki poll to
# najdrozsza rzecz, jaka mozna tu ustawic - przy poll ~10 s urzadzenie
# przestaje wchodzic w gleboki sen i pobor rosnie z ~4 uA do ~150 uA.
# Dlatego skrypt ostrzega, gdy poll < 300 s.
#
# Zakres obu parametrow: 1..4200 s (limit ZB_MILLISECONDS_TO_BEACON_INTERVAL:
# makro mnozy ms x 1000 w 32-bitowym zb_time_t).

set -euo pipefail

APP_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
BOARD=BTZ_EndDevice/nrf54l15/cpuapp
IMAGE=zigbee-internal-btz
BOARD_ROOT=$(cd "${APP_DIR}/.." && pwd)
NCS_VERSION=${NCS_VERSION:-v2.9.2}
JLINK_ID=${JLINK_ID:-801037150}

SEND_INTERVAL_S=""
POLL_INTERVAL_S=3600
DO_FLASH=0
DO_ERASE=0
DEEP=0

for arg in "$@"; do
	case "$arg" in
	--flash) DO_FLASH=1 ;;
	--erase) DO_ERASE=1 ;;
	--deep) DEEP=1 ;;
	-h | --help)
		sed -n '2,25p' "${BASH_SOURCE[0]}"
		exit 0
		;;
	*[!0-9]*)
		echo "nieznany argument: $arg" >&2
		exit 1
		;;
	*)
		if [ -z "$SEND_INTERVAL_S" ]; then SEND_INTERVAL_S=$arg; else POLL_INTERVAL_S=$arg; fi
		;;
	esac
done

if [ -z "$SEND_INTERVAL_S" ]; then
	echo "uzycie: $0 <SEND_INTERVAL_S> [POLL_INTERVAL_S] [--flash] [--erase] [--deep]" >&2
	exit 1
fi

for v in "$SEND_INTERVAL_S" "$POLL_INTERVAL_S"; do
	if [ "$v" -lt 1 ] || [ "$v" -gt 4200 ]; then
		echo "blad: wartosci musza miescic sie w 1..4200 s (dostalem $v)" >&2
		exit 1
	fi
done

if [ "$POLL_INTERVAL_S" -lt 300 ]; then
	echo "UWAGA: poll = ${POLL_INTERVAL_S} s. Krotki poll blokuje gleboki sen -"
	echo "       spodziewaj sie poboru rzedu setek uA zamiast kilku uA."
	echo "       Do zwyklych pomiarow zostaw poll na 3600 s."
fi

ncs() { nrfutil toolchain-manager launch --ncs-version "${NCS_VERSION}" -- "$@"; }

if [ "$DO_ERASE" = 1 ]; then
	echo "== erase (J-Link ${JLINK_ID})"
	nrfutil device erase --serial-number "${JLINK_ID}" --core Application
fi

echo "== build: SEND_INTERVAL_S=${SEND_INTERVAL_S}  POLL_INTERVAL_S=${POLL_INTERVAL_S}"
ncs west build -b "${BOARD}" --sysbuild --pristine \
	-d "${APP_DIR}/build" "${APP_DIR}" -- \
	-DBOARD_ROOT="${BOARD_ROOT}" \
	-D${IMAGE}_CONFIG_ZB_SEND_INTERVAL_S="${SEND_INTERVAL_S}" \
	-D${IMAGE}_CONFIG_ZB_POLL_INTERVAL_S="${POLL_INTERVAL_S}" \
	-D${IMAGE}_CONFIG_ZB_DEEP_SLEEP="$([ "$DEEP" = 1 ] && echo y || echo n)"

# Weryfikacja: co NAPRAWDE trafilo do obrazu. Bez tego latwo wgrac obraz
# z innymi wartosciami, niz sie wpisalo (stary katalog builda, literowka).
echo "== w obrazie:"
grep -E "^CONFIG_ZB_(SEND_INTERVAL_S|POLL_INTERVAL_S|DEEP_SLEEP)=" \
	"${APP_DIR}/build/${IMAGE}/zephyr/.config" | sed 's/^/   /'

if [ "$DO_FLASH" = 1 ]; then
	echo "== flash (J-Link ${JLINK_ID})"
	ncs west flash -d "${APP_DIR}/build" --dev-id "${JLINK_ID}"
fi
