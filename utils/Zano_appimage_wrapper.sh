#!/bin/bash
script_dir=$( dirname "$(readlink -f "$0")" )

parse_manual_binary_arguments()
{
  local input="$1"
  local current_argument=""
  local state="unquoted"
  local character
  local argument_started=false
  local i

  manual_binary_arguments=()

  for ((i = 0; i < ${#input}; ++i)); do
    character="${input:i:1}"

    case "$state" in
      unquoted)
        case "$character" in
          " "|$'\t'|$'\n')
            if [ "$argument_started" = true ]; then
              manual_binary_arguments+=("$current_argument")
              current_argument=""
              argument_started=false
            fi
            ;;
          "'")
            state="single-quoted"
            argument_started=true
            ;;
          '"')
            state="double-quoted"
            argument_started=true
            ;;
          "\\")
            state="unquoted-escape"
            argument_started=true
            ;;
          *)
            current_argument+="$character"
            argument_started=true
            ;;
        esac
        ;;
      single-quoted)
        if [ "$character" = "'" ]; then
          state="unquoted"
        else
          current_argument+="$character"
        fi
        ;;
      double-quoted)
        case "$character" in
          '"')
            state="unquoted"
            ;;
          "\\")
            state="double-quoted-escape"
            ;;
          *)
            current_argument+="$character"
            ;;
        esac
        ;;
      unquoted-escape)
        current_argument+="$character"
        state="unquoted"
        ;;
      double-quoted-escape)
        current_argument+="$character"
        state="double-quoted"
        ;;
    esac
  done

  if [ "$state" != "unquoted" ]; then
    echo "Invalid value for --exec-args: unmatched quote or trailing escape." >&2
    return 1
  fi

  if [ "$argument_started" = true ]; then
    manual_binary_arguments+=("$current_argument")
  fi
}


call_manual_binary()
{
  local manual_binary="$1"
  local manual_binary_path

  case "$manual_binary" in
    ""|*/*|"."|"..")
      echo "Invalid value for --exec: specify an executable name from usr/bin." >&2
      exit 2
      ;;
  esac

  manual_binary_path="${script_dir}/usr/bin/${manual_binary}"
  if [ ! -f "$manual_binary_path" ] || [ ! -x "$manual_binary_path" ]; then
    echo "Executable '${manual_binary}' was not found in the AppImage usr/bin directory." >&2
    exit 2
  fi

  shift
  cd "$script_dir" || exit 1
  exec "$manual_binary_path" "$@"
}


manual_binary=""
manual_binary_arguments_string=""
manual_binary_option_seen=false
manual_binary_arguments_option_seen=false
manual_execution_extra_arguments=()

for argument in "$@"; do
  case "$argument" in
    --exec=*)
      if [ "$manual_binary_option_seen" = true ]; then
        echo "--exec may only be specified once." >&2
        exit 2
      fi
      manual_binary="${argument#*=}"
      manual_binary_option_seen=true
      ;;
    --exec-args=*)
      if [ "$manual_binary_arguments_option_seen" = true ]; then
        echo "--exec-args may only be specified once." >&2
        exit 2
      fi
      manual_binary_arguments_string="${argument#*=}"
      manual_binary_arguments_option_seen=true
      ;;
    *)
      manual_execution_extra_arguments+=("$argument")
      ;;
  esac
done

if [ "$manual_binary_option_seen" = true ] || [ "$manual_binary_arguments_option_seen" = true ]; then
  if [ "$manual_binary_option_seen" != true ]; then
    echo "--exec-args requires --exec." >&2
    exit 2
  fi

  if [ ${#manual_execution_extra_arguments[@]} -ne 0 ]; then
    echo "Arguments for a manually selected executable must be passed through --exec-args." >&2
    exit 2
  fi

  parse_manual_binary_arguments "$manual_binary_arguments_string" || exit 2
  call_manual_binary "$manual_binary" "${manual_binary_arguments[@]}"
fi

desktop_dir=~/.local/share/applications
icon_dir=~/.local/share/icons/hicolor/256x256/apps
version="$(echo ${APPIMAGE} | rev | cut -d '-' -f1,2 | rev | sed 's/\.AppImage$//')"
out_file_name="${desktop_dir}/Zano-${version}.desktop"

export QTWEBENGINE_DISABLE_SANDBOX=1

call_app()
{
  pushd "$script_dir" >/dev/null

  # handle situation with empy deeplink, for example when app launched by double click on icon
  if [ -n "$1" ] && printf '%s' "$1" | grep -qiE '^zano:'; then
    usr/bin/Zano --deeplink-params="$1"
  else
    usr/bin/Zano "$@"
  fi

  rc=$?
  if [ $rc -ne 0 ]; then
    echo $'\n\n\x1b[1mIf Zano fails to launch, it might need to install xinerama extension for the X C Binding with this command:\n\x1b[2m   sudo apt-get install libxcb-xinerama0\n\n'
  fi

  popd >/dev/null
  exit $rc
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
    echo Exec=$APPIMAGE %u | tee -a $target_file_name  > /dev/null
    echo Terminal=false | tee -a $target_file_name  > /dev/null
    echo Type=Application | tee -a $target_file_name  > /dev/null
    echo "Categories=Qt;Utility;" | tee -a $target_file_name  > /dev/null
    echo "MimeType=x-scheme-handler/zano;" | tee -a $target_file_name  > /dev/null
    echo "StartupWMClass=Zano" | tee -a $target_file_name  > /dev/null
}


create_desktop_icon $out_file_name

xdg-mime default Zano.desktop x-scheme-handler/zano
xdg-desktop-menu install --novendor "${out_file_name}"

call_app "$@"
