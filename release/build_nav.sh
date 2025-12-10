#!/usr/bin/bash -e

BUILD_DIR=/data/openpilot
cd $BUILD_DIR
rm -rf .git
git init
git remote add origin https://jihulab.com/fishop/openpilot.git

# Cleanup
echo "cleanup"
find . -name '*.a' -delete
find . -name '*.o' -delete
find . -name '*.os' -delete
find . -name '*.pyc' -delete
find . -name 'moc_*' -delete
find . -name '__pycache__' -delete
rm -rf .sconsign.dblite Jenkinsfile release/
rm -rf selfdrive/modeld/models/*.onnx
touch prebuilt

# Add built files to git
echo "add all file"
git add -f .

VERSION="sunnypilot_v0971_$(date +%y%m%d)"
git commit -m $VERSION
git branch -m "nav097"
git push -f origin "nav097"
