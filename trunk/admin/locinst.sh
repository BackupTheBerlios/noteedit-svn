#!/bin/sh

if [ ! -d noteedit/icons ]
then
	echo "wrong directory!"
	echo "Please run this script in"
	echo "\"noteedit\" top directory!"
	exit
fi

loacticons16x16=`ls noteedit/icons | grep "^lo16-action"`
loacticons22x22=`ls noteedit/icons | grep "^lo22-action"`
loacticons32x32=`ls noteedit/icons | grep "^lo32-action"`

hiacticons16x16=`ls noteedit/icons | grep "^hi16-action"`
hiacticons22x22=`ls noteedit/icons | grep "^hi22-action"`
hiacticons32x32=`ls noteedit/icons | grep "^hi32-action"`

loappicons16x16=`ls noteedit/icons | grep "^lo16-app"`
loappicons22x22=`ls noteedit/icons | grep "^lo22-app"`
loappicons32x32=`ls noteedit/icons | grep "^lo32-app"`

hiappicons16x16=`ls noteedit/icons | grep "^hi16-app"`
hiappicons22x22=`ls noteedit/icons | grep "^hi22-app"`
hiappicons32x32=`ls noteedit/icons | grep "^hi32-app"`

himimeicons48x48=`ls noteedit/icons | grep "^hi48-mime"`



POFILES=`ls po/*.po`

if [ ! -d share ]
then
	mkdir share
	echo "mkdir share"
fi

if [ ! -d share/apps ]
then
	mkdir share/apps
	echo "mkdir share/apps"
fi

if [  -e share/apps/noteedit ]
then
	if [ ! -d share/apps/noteedit/icons ]
	then
		echo "something wrong with"
		echo "softlink \"share/apps/noteedit\""
		exit
	fi
else
	ln -s `pwd`/noteedit share/apps/noteedit
	echo "ln -s `pwd`/noteedit share/apps/noteedit"
fi

rm -rf share/locale
echo "rm -rf share/locale"
mkdir share/locale
echo "mkdir share/locale"

ICONPAT="share/apps/noteedit/icons"
rm -rf $ICONPAT/hicolor $ICONPAT/locolor
echo "rm -rf $ICONPAT/hicolor $ICONPAT/locolor"
mkdir $ICONPAT/hicolor $ICONPAT/locolor
echo "mkdir $ICONPAT/hicolor $ICONPAT/locolor"

mkdir $ICONPAT/hicolor/16x16
echo "mkdir $ICONPAT/hicolor/16x16"
mkdir $ICONPAT/hicolor/22x22
echo "mkdir $ICONPAT/hicolor/22x22"
mkdir $ICONPAT/hicolor/32x32
echo "mkdir $ICONPAT/hicolor/32x32"
mkdir $ICONPAT/hicolor/48x48
echo "mkdir $ICONPAT/hicolor/48x48"
mkdir $ICONPAT/locolor/16x16
echo "mkdir $ICONPAT/locolor/16x16"
mkdir $ICONPAT/locolor/22x22
echo "mkdir $ICONPAT/locolor/22x22"
mkdir $ICONPAT/locolor/32x32
echo "mkdir $ICONPAT/locolor/32x32"
mkdir $ICONPAT/locolor/48x48
echo "mkdir $ICONPAT/locolor/48x48"

mkdir $ICONPAT/hicolor/16x16/actions
echo "mkdir $ICONPAT/hicolor/16x16/actions"
mkdir $ICONPAT/hicolor/22x22/actions
echo "mkdir $ICONPAT/hicolor/22x22/actions"
mkdir $ICONPAT/hicolor/32x32/actions
echo "mkdir $ICONPAT/hicolor/32x32/actions"
mkdir $ICONPAT/locolor/16x16/actions
echo "mkdir $ICONPAT/locolor/16x16/actions"
mkdir $ICONPAT/locolor/22x22/actions
echo "mkdir $ICONPAT/locolor/22x22/actions"
mkdir $ICONPAT/locolor/32x32/actions
echo "mkdir $ICONPAT/locolor/32x32/actions"

mkdir $ICONPAT/hicolor/16x16/apps
echo "mkdir $ICONPAT/hicolor/16x16/apps"
mkdir $ICONPAT/hicolor/22x22/apps
echo "mkdir $ICONPAT/hicolor/22x22/apps"
mkdir $ICONPAT/hicolor/32x32/apps
echo "mkdir $ICONPAT/hicolor/32x32/apps"
mkdir $ICONPAT/locolor/16x16/apps
echo "mkdir $ICONPAT/locolor/16x16/apps"
mkdir $ICONPAT/locolor/22x22/apps
echo "mkdir $ICONPAT/locolor/22x22/apps"
mkdir $ICONPAT/locolor/32x32/apps
echo "mkdir $ICONPAT/locolor/32x32/apps"

mkdir $ICONPAT/hicolor/48x48/mimetypes
echo "mkdir $ICONPAT/hicolor/48x48/mimetypes"
mkdir $ICONPAT/locolor/48x48/mimetypes
echo "mkdir $ICONPAT/locolor/48x48/mimetypes"

startpath=`pwd`

rm -rf share/locale
echo "rm -rf share/locale"
mkdir share/locale
echo "mkdir share/locale"

for i in $POFILES
do
	pobase=`basename $i ".po"`
	mkdir share/locale/$pobase
	echo "	mkdir share/locale/$pobase"
	mkdir share/locale/$pobase/LC_MESSAGES
	echo "	mkdir share/locale/$pobase/LC_MESSAGES"
	cd share/locale/$pobase/LC_MESSAGES
	echo "	cd share/locale/$pobase/LC_MESSAGES"
	ln -s ../../../../po/$pobase.gmo noteedit.mo
	echo "	ln -s ../../../../po/$pobase.gmo noteedit.mo"
	cd $startpath
	echo "cd $startpath"
done

#
# app icons
#

cd $ICONPAT/locolor/16x16/actions
echo "cd $ICONPAT/locolor/16x16/actions"
for i in $loacticons16x16
do
	iconname=`echo $i | sed 's/lo16-action-//'`
	ln -s ../../../$i $iconname
	echo "	ln -s ../../../$i $iconname"
done
cd $startpath
echo "cd $startpath"

cd $ICONPAT/locolor/22x22/actions
echo "cd $ICONPAT/locolor/22x22/actions"
for i in $loacticons22x22
do
	iconname=`echo $i | sed 's/lo22-action-//'`
	ln -s ../../../$i $iconname
	echo "	ln -s ../../../$i $iconname"
done
cd $startpath
echo "cd $startpath"

cd $ICONPAT/locolor/32x32/actions
echo "cd $ICONPAT/locolor/32x32/actions"
for i in $loacticons32x32
do
	iconname=`echo $i | sed 's/lo32-action-//'`
	ln -s ../../../$i $iconname
	echo "	ln -s ../../../$i $iconname"
done
cd $startpath
echo "cd $startpath"

cd $ICONPAT/hicolor/16x16/actions
echo "cd $ICONPAT/hicolor/16x16/actions"
for i in $hiacticons16x16
do
	iconname=`echo $i | sed 's/hi16-action-//'`
	ln -s ../../../$i $iconname
	echo "	ln -s ../../../$i $iconname"
done
cd $startpath
echo "cd $startpath"

cd $ICONPAT/hicolor/22x22/actions
echo "cd $ICONPAT/hicolor/22x22/actions"
for i in $hiacticons22x22
do
	iconname=`echo $i | sed 's/hi22-action-//'`
	ln -s ../../../$i $iconname
	echo "	ln -s ../../../$i $iconname"
done
cd $startpath
echo "cd $startpath"

cd $ICONPAT/hicolor/32x32/actions
echo "cd $ICONPAT/hicolor/32x32/actions"
for i in $hiacticons32x32
do
	iconname=`echo $i | sed 's/hi32-action-//'`
	ln -s ../../../$i $iconname
	echo "	ln -s ../../../$i $iconname"
done
cd $startpath
echo "cd $startpath"

#
# app icons
#

cd $ICONPAT/locolor/16x16/apps
echo "cd $ICONPAT/locolor/16x16/apps"
for i in $loappicons16x16
do
	iconname=`echo $i | sed 's/lo16-app-//'`
	ln -s ../../../$i $iconname
	echo "	ln -s ../../../$i $iconname"
done
cd $startpath
echo "cd $startpath"

cd $ICONPAT/locolor/22x22/apps
echo "cd $ICONPAT/locolor/22x22/apps"
for i in $loappicons22x22
do
	iconname=`echo $i | sed 's/lo22-app-//'`
	ln -s ../../../$i $iconname
	echo "	ln -s ../../../$i $iconname"
done
cd $startpath
echo "cd $startpath"

cd $ICONPAT/locolor/32x32/apps
echo "cd $ICONPAT/locolor/32x32/apps"
for i in $loappicons32x32
do
	iconname=`echo $i | sed 's/lo32-app-//'`
	ln -s ../../../$i $iconname
	echo "	ln -s ../../../$i $iconname"
done
cd $startpath
echo "cd $startpath"

cd $ICONPAT/hicolor/16x16/apps
echo "cd $ICONPAT/hicolor/16x16/apps"
for i in $hiappicons16x16
do
	iconname=`echo $i | sed 's/hi16-app-//'`
	ln -s ../../../$i $iconname
	echo "	ln -s ../../../$i $iconname"
done
cd $startpath
echo "cd $startpath"

cd $ICONPAT/hicolor/22x22/apps
echo "cd $ICONPAT/hicolor/22x22/apps"
for i in $hiappicons22x22
do
	iconname=`echo $i | sed 's/hi22-app-//'`
	ln -s ../../../$i $iconname
	echo "	ln -s ../../../$i $iconname"
done
cd $startpath
echo "cd $startpath"

cd $ICONPAT/hicolor/32x32/apps
echo "cd $ICONPAT/hicolor/32x32/apps"
for i in $hiappicons32x32
do
	iconname=`echo $i | sed 's/hi32-app-//'`
	ln -s ../../../$i $iconname
	echo "	ln -s ../../../$i $iconname"
done
cd $startpath
echo "cd $startpath"

# mimetypes icons

cd $ICONPAT/hicolor/48x48/mimetypes
echo "cd $ICONPAT/hicolor/48x48/mimetypes"
for i in $himimeicons48x48
do
	iconname=`echo $i | sed 's/hi48-mime-//'`
	ln -s ../../../$i $iconname
	echo "	ln -s ../../../$i $iconname"
done
cd $startpath
echo "cd $startpath"
