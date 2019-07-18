#!/bin/sh

set LD_LIBRARY_PATH=${0%}
set LD_LIBRARY_PATH=$LD_LIBRARY_PATH/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PWD/lib
export QT_PLUGIN_PATH=$LD_LIBRARY_PATH

echo $LD_LIBRARY_PATH
echo $QT_PLUGIN_PATH

out_file_name=~/.local/share/applications/Zano.desktop
script_dir=$( cd $(dirname $0) ; pwd -P )    

call_app()
{
  ./Zano
  exit
}


create_desktop_icon()
{
    target_file_name=$1
    echo "Generating icon file: $target_file_name..."
    rm -f $target_file_name
    echo [Desktop Entry] | tee -a $target_file_name  > /dev/null
    echo Version=1.0 | tee -a $target_file_name  > /dev/null
    echo Name=My Application | tee -a $target_file_name > /dev/null
    echo GenericName=My Application | tee -a $target_file_name  > /dev/null
    echo Comment=Doing some funny stuff | tee -a $target_file_name > /dev/null
    echo Icon=$script_dir/html/files/desktop_linux_icon.png | tee -a $target_file_name > /dev/null
    echo Exec=$script_dir/Zano | tee -a $target_file_name  > /dev/null
    echo Terminal=true | tee -a $target_file_name  > /dev/null
    echo Type=Application | tee -a $target_file_name  > /dev/null
    echo "Categories=Qt;Utility;" | tee -a $target_file_name  > /dev/null
}


create_desktop_icon $out_file_name

call_app




