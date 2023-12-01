#!/bin/bash
script_dir=$( dirname "$(readlink -f "$0")" )

out_dir=~/.local/share/applications
out_file_name="${out_dir}/Zano.desktop"

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
    rm -f "${out_dir}/Zano.png"
    rm -f $target_file_name
    cp -Rv "${APPDIR}/usr/share/icons/hicolor/256x256/apps/Zano.png" "${out_dir}/Zano.png"
    echo [Desktop Entry] | tee -a $target_file_name  > /dev/null
    echo Version=1.0 | tee -a $target_file_name  > /dev/null
    echo Name=Zano | tee -a $target_file_name > /dev/null
    echo GenericName=Zano | tee -a $target_file_name  > /dev/null
    echo Comment=Privacy blockchain | tee -a $target_file_name > /dev/null
    echo Icon=${out_dir}/Zano.png | tee -a $target_file_name > /dev/null
    echo Exec=$APPIMAGE --deeplink-params=\\\"%u\\\" | tee -a $target_file_name  > /dev/null
    echo Terminal=false | tee -a $target_file_name  > /dev/null
    echo Type=Application | tee -a $target_file_name  > /dev/null
    echo "Categories=Qt;Utility;" | tee -a $target_file_name  > /dev/null
    echo "MimeType=x-scheme-handler/zano;" | tee -a $target_file_name  > /dev/null
    echo "StartupWMClass=Zano" | tee -a $target_file_name  > /dev/null
}


create_desktop_icon $out_file_name

xdg-mime default Zano.desktop x-scheme-handler/zano

call_app "$@"
