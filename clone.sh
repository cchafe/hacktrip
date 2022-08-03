#!/bin/bash
# create a clone of a qt project e.g., etherbridge -> jt
# make clean
# make a backup copy with different name
# ./clone.sh etherbridge jt
# mv backup original

# cp -pr etherbridge sav123tmp
# ./clone.sh etherbridge jt
# mv sav123tmp etherbridge

echo "old: $1", "new: $2"
cp -pr $1 sav123tmp

rename $1 $2 *
cd $2
rename $1 $2 *
find $NEW -type f -exec sed -i "s/$1/$2/g" {} \;

cd ..
mv sav123tmp $1
echo "Done"@
