set -e
curr_path=${BASH_SOURCE%/*}
version_file_path=../src/version.h.in

pushd $curr_path
git pull
if [ $? -ne 0 ]; then
   echo "Failed to pull"
   popd
   exit $?
fi

build_no_before=`cat $version_file_path | grep 'PROJECT_VERSION_BUILD_NO ' | awk {'print $3'}`

../../zano-tools-last-build/connectivity_tool --increment-build-no=$version_file_path

build_no_after=`cat $version_file_path | grep 'PROJECT_VERSION_BUILD_NO ' | awk {'print $3'}`

echo "$build_no_before -> $build_no_after"

echo $(pwd -P)
git status
git commit -a -m"=== build number: $build_no_before -> $build_no_after ==="

git push
if [ $? -ne 0 ]; then
   echo "Failed to push"
   popd
   exit $?
fi

echo "Build number was succesefully incremented."
popd

