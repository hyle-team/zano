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

if [[ "${XDG_DATA_HOME:-}" == /* ]]; then
  data_home="$XDG_DATA_HOME"
elif [[ "${HOME:-}" == /* ]]; then
  data_home="${HOME}/.local/share"
else
  data_home=""
fi

desktop_dir="${data_home}/applications"
icon_dir="${data_home}/icons/hicolor/256x256/apps"
desktop_file_name=""

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


escape_desktop_exec_argument()
{
  local value="$1"
  local escaped=""
  local character
  local index

  # exec values have both desktop entry and command-line escaping layers.
  for ((index = 0; index < ${#value}; ++index)); do
    character="${value:index:1}"
    case "$character" in
      "\\") escaped+='\\\\' ;;
      '"')  escaped+='\\"' ;;
      '`')  escaped+='\\`' ;;
      '$')  escaped+='\\$' ;;
      '%')  escaped+='%%' ;;
      *)    escaped+="$character" ;;
    esac
  done

  printf '"%s"' "$escaped"
}


escape_desktop_string_value()
{
  local value="$1"
  local escaped=""
  local character
  local index

  for ((index = 0; index < ${#value}; ++index)); do
    character="${value:index:1}"
    case "$character" in
      "\\") escaped+='\\' ;;
      ' ')  escaped+='\s' ;;
      *)    escaped+="$character" ;;
    esac
  done

  printf '%s' "$escaped"
}


desktop_file_suffix_from_appimage()
{
  local appimage_file_name="${1##*/}"
  local suffix="$appimage_file_name"
  local prefix
  local last_component

  case "$suffix" in
    *.[Aa][Pp][Pp][Ii][Mm][Aa][Gg][Ee]) suffix="${suffix%.*}" ;;
  esac

  if [ -z "$suffix" ]; then
    suffix="AppImage"
  elif [[ "$suffix" == *-*-* ]]; then
    last_component="${suffix##*-}"
    prefix="${suffix%-*}"
    suffix="${prefix##*-}-${last_component}"
  fi

  printf '%s' "$suffix"
}


install_desktop_entry()
{
  local source_icon="${APPDIR:-}/usr/share/icons/hicolor/256x256/apps/Zano.png"
  local target_icon="${icon_dir}/Zano.png"
  local target_file_name
  local temporary_desktop_file
  local temporary_icon_file
  local desktop_file_suffix
  local escaped_appimage
  local escaped_try_exec

  if [ -z "$data_home" ]; then
    echo "Skipping desktop integration: neither XDG_DATA_HOME nor HOME is set." >&2
    return 1
  fi

  if [[ "$data_home" == *[[:cntrl:]]* ]]; then
    echo "Skipping desktop integration: the XDG data path contains control characters." >&2
    return 1
  fi

  if [ -z "${APPIMAGE:-}" ]; then
    echo "Skipping desktop integration: APPIMAGE is not set." >&2
    return 1
  fi

  if [[ "$APPIMAGE" != /* ]]; then
    echo "Skipping desktop integration: APPIMAGE is not an absolute path." >&2
    return 1
  fi

  if [[ "$APPIMAGE" == *"="* ]]; then
    echo "Skipping desktop integration: desktop Exec paths cannot contain '='." >&2
    return 1
  fi

  if [[ "$APPIMAGE" == *[[:cntrl:]]* ]]; then
    echo "Skipping desktop integration: the AppImage path contains control characters." >&2
    return 1
  fi

  if [ ! -f "$APPIMAGE" ] || [ ! -x "$APPIMAGE" ]; then
    echo "Skipping desktop integration: APPIMAGE does not point to an executable file." >&2
    return 1
  fi

  if [ -z "${APPDIR:-}" ]; then
    echo "Skipping desktop integration: APPDIR is not set." >&2
    return 1
  fi

  desktop_file_suffix="$(desktop_file_suffix_from_appimage "$APPIMAGE")"
  if [[ "$desktop_file_suffix" == *";"* ]]; then
    echo "Skipping desktop integration: the launcher name cannot contain ';'." >&2
    return 1
  fi

  desktop_file_name="Zano-${desktop_file_suffix}.desktop"
  target_file_name="${desktop_dir}/${desktop_file_name}"

  if [ ! -f "$source_icon" ]; then
    echo "Skipping desktop integration: bundled icon was not found at '$source_icon'." >&2
    return 1
  fi

  if ! mkdir -p -- "$desktop_dir" "$icon_dir"; then
    echo "Skipping desktop integration: failed to create XDG data directories." >&2
    return 1
  fi

  temporary_icon_file="$(mktemp "${icon_dir}/.Zano.png.XXXXXX")" || {
    echo "Skipping desktop integration: failed to create a temporary icon file." >&2
    return 1
  }

  if ! cp -f -- "$source_icon" "$temporary_icon_file" || \
     ! chmod 0644 "$temporary_icon_file" || \
     ! mv -fT -- "$temporary_icon_file" "$target_icon"; then
    rm -f -- "$temporary_icon_file"
    echo "Skipping desktop integration: failed to install the application icon." >&2
    return 1
  fi

  temporary_desktop_file="$(mktemp "${desktop_dir}/.Zano.desktop.XXXXXX")" || {
    echo "Skipping desktop integration: failed to create a temporary desktop entry." >&2
    return 1
  }

  escaped_appimage="$(escape_desktop_exec_argument "$APPIMAGE")"
  escaped_try_exec="$(escape_desktop_string_value "$APPIMAGE")"
  if ! {
    printf '%s\n' \
      '[Desktop Entry]' \
      'Version=1.0' \
      'Type=Application' \
      'Name=Zano' \
      'GenericName=Zano' \
      'Comment=Privacy blockchain' \
      'Icon=Zano' \
      "TryExec=${escaped_try_exec}" \
      "Exec=${escaped_appimage} %u" \
      'Terminal=false' \
      'Categories=Qt;Utility;' \
      'MimeType=x-scheme-handler/zano;' \
      'StartupWMClass=Zano'
  } > "$temporary_desktop_file"; then
    rm -f -- "$temporary_desktop_file"
    echo "Skipping desktop integration: failed to write the desktop entry." >&2
    return 1
  fi

  if ! chmod 0644 "$temporary_desktop_file"; then
    rm -f -- "$temporary_desktop_file"
    echo "Skipping desktop integration: failed to set desktop entry permissions." >&2
    return 1
  fi

  if ! mv -fT -- "$temporary_desktop_file" "$target_file_name"; then
    rm -f -- "$temporary_desktop_file"
    echo "Skipping desktop integration: failed to install the desktop entry." >&2
    return 1
  fi

  echo "Installed desktop entry: $target_file_name"
  return 0
}


if install_desktop_entry; then
  if command -v update-desktop-database >/dev/null 2>&1; then
    XDG_DATA_HOME="$data_home" update-desktop-database "$desktop_dir" >/dev/null 2>&1 || \
      echo "Failed to update the desktop entry database." >&2
  fi

  if command -v xdg-desktop-menu >/dev/null 2>&1; then
    XDG_DATA_HOME="$data_home" xdg-desktop-menu forceupdate --mode user >/dev/null 2>&1 || \
      echo "Failed to refresh the desktop menu." >&2
  fi

  if command -v xdg-icon-resource >/dev/null 2>&1; then
    XDG_DATA_HOME="$data_home" xdg-icon-resource forceupdate --theme hicolor --mode user >/dev/null 2>&1 || \
      echo "Failed to refresh the icon cache." >&2
  fi

  if command -v xdg-mime >/dev/null 2>&1; then
    XDG_DATA_HOME="$data_home" xdg-mime default "$desktop_file_name" x-scheme-handler/zano || \
      echo "Failed to register the zano URL scheme handler." >&2
  fi
fi

call_app "$@"
