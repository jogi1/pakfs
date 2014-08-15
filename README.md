

./compile.sh

for mounting an already existing pak file
./pakmount -pf /example.pak mountpoint

for creating a new pak file
./pakmount -n mountpoint

read ,write and delete support no symlinks atm :< 

cat .pakinfo 
in the mountdir for some pakfile information

cp .pakfile /dir/file.pak
to get the modified version
