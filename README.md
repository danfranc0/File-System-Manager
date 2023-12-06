# File-System-Manager
Project comes in 2 parts
1. Library for reading and writing WAD files and creating and managing filesystems from these files.
2. Daemon built using FUSE API to mount and implement the filesystem on Linux devices.

## Command Format
 ./wadfs/wadfs -s somewadfile.wad /some/mount/directory

## Build Using Library
### Compile
* cd libWad && make && cd ../
### Build Program
* c++ -o name sourcefile.cpp -L ./libWad -lWad

## Build Daemon
### Install FUSE Library for Unix Systems
* sudo apt install libfuse-dev fuse
* sudo chmod 666 /dev/fuse
### Build
* ./wadfs/wadfs -s wadfile.wad /mountdirectory
