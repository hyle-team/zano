set -e
curr_path=${BASH_SOURCE%/*}
version_file_path=../src/version.h.in

pushd $curr_path

# clear old local changes if any
git checkout -- ../src/*

git pull --ff-only

build_no_before=`cat $version_file_path | grep 'PROJECT_VERSION_BUILD_NO ' | awk {'print $3'}`

../../zano-tools-last-build/connectivity_tool --increment-build-no=$version_file_path

build_no_after=`cat $version_file_path | grep 'PROJECT_VERSION_BUILD_NO ' | awk {'print $3'}`

echo "$build_no_before -> $build_no_after"

echo $(pwd -P)

git status
git commit -a -m"=== build number: $build_no_before -> $build_no_after ==="

git push

echo "Build number was succesefully incremented."
popd

