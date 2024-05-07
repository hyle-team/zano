set +e
curr_path=${BASH_SOURCE%/*}

function upload_build() # $1 - path to the file to be uploaded
{
  if [ -z "$1" ]
  then
    echo "ERROR: upload_build is called with no or invalid parameters"
    return 1
  fi

  # check if the directory contains files
  #if [ -n "$(ls -A "$ZANO_BLD_UPL_DIR" 2>/dev/null)" ]
  #then
  #  echo "ERROR: uploading folder contains files"
  #  return 1
  #fi

  cd "$ZANO_BLD_UPL_DIR" || return 2
  rm -rf ./* || return 3
  #popd || return 4

  touch WAIT || return 5
  cp $1 . || return 6
  rm WAIT || return 7  

  counter=0
  while [ ! -f DONE ]
  do
    if [ "$counter" -ge 500 ]
    then
      echo "ERROR: uploading is taking longer than expected"
      touch STOP
      return 8
    fi
    sleep 2
    echo "waiting..."
    counter=$((counter + 1))
  done

  rc=$(<DONE)

  if [ "$rc" -eq 0 ]
  then
    echo "Upload successful."
    return 0
  fi

  echo "Upload unsuccessfull, exit code = $rc."
  return $rc
}
