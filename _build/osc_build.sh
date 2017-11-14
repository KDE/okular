set -e

cd _buildcode
packName=`grep 'https://github.com/ConTextMe/' ./*/obs_service|sed -e 's/.*.ConTextMe\///' -e 's/.git.*//'`
name=`ls ./`
find $name/ -not -path "*/.git/*" -print | cpio -o > ../$name.obscpio
cd ./../ && osc build --noinit -o

sudo zypper --no-refresh -n --no-gpg-checks in --oldpackage /tmp/build-root/openSUSE_Leap_42.3-x86_64/home/abuild/rpmbuild/RPMS/x86_64/$packName*.rpm

