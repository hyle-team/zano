set -e
curr_path=${BASH_SOURCE%/*}

function build_fancy_dmg() # $1 - path to package folder, $2 - dmg output filename
{
  if [ -z "$1" ] || [ -z "$2" ]
  then
    echo "build_fancy_dmg is called with no or invalid parameters"
    return 1
  fi

  $curr_path/contrib/create-dmg/create-dmg \
    --volname "Zano installer" \
    --volicon "$curr_path/../src/gui/qt-daemon/app.icns" \
    --background "$curr_path/../resources/dmg_installer_bg.png" \
    --window-pos 200 120 \
    --window-size 487 290 \
    --icon-size 128	 \
    --icon Zano.app 112 115 \
    --hide-extension Zano.app \
    --app-drop-link 365 115 \
    --no-internet-enable \
    $2 \
    $1

  return $?
}

