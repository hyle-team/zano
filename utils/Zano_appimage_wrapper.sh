#!/bin/bash
script_dir=$( dirname "$(readlink -f "$0")" )

desktop_dir=$HOME/.local/share/applications
icon_dir=$HOME/.local/share/icons/hicolor/256x256/apps
version="$(echo ${APPIMAGE} | rev | cut -d '-' -f1,2 | rev | sed 's/\.AppImage$//')"
out_file_name="${desktop_dir}/Zano-${version}.desktop"

export QTWEBENGINE_DISABLE_SANDBOX=1

call_app()
{
  pushd $script_dir
  usr/bin/Zano "$@"
  if [ $? -ne 0 ]; then
    echo $'\n\n\x1b[1mIf Zano fails to launch, it might need to install xinerama extension for the X C Binding with this command:\n\x1b[2m   sudo apt-get install libxcb-xinerama0\n\n'
  fi

  popd
  exit
}


create_desktop_icon()
{
    target_file_name=$1
    echo "Generating icon file: $target_file_name..."
    mkdir -p "${desktop_dir}"
    mkdir -p "${icon_dir}"
    rm -f $target_file_name
    rm -f "${icon_dir}/Zano.png"
    cp -f "${APPDIR}/usr/share/icons/hicolor/256x256/apps/Zano.png" "${icon_dir}/Zano.png"
    icon_path="${icon_dir}/Zano.png"
    echo [Desktop Entry] | tee -a $target_file_name  > /dev/null
    echo Version=1.0 | tee -a $target_file_name  > /dev/null
    echo Name=Zano | tee -a $target_file_name > /dev/null
    echo GenericName=Zano | tee -a $target_file_name  > /dev/null
    echo Comment=Privacy blockchain | tee -a $target_file_name > /dev/null
    echo Icon=${icon_path} | tee -a $target_file_name > /dev/null
    echo TryExec="${APPIMAGE}" | tee -a "${target_file_name}" >/dev/null
    echo Exec=\"${APPIMAGE}\" --deeplink-params=%u | tee -a $target_file_name  > /dev/null
    echo Terminal=false | tee -a $target_file_name  > /dev/null
    echo Type=Application | tee -a $target_file_name  > /dev/null
    echo "Categories=Qt;Utility;" | tee -a $target_file_name  > /dev/null
    echo "MimeType=x-scheme-handler/zano;" | tee -a $target_file_name  > /dev/null
    echo "StartupWMClass=Zano" | tee -a $target_file_name  > /dev/null
}

create_desktop_icon $out_file_name

xdg-mime default "$(basename "$out_file_name")" x-scheme-handler/zano
xdg-desktop-menu install --novendor "${out_file_name}"

call_app "$@"
