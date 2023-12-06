# File-System-Manager
Project comes in 2 parts
1. Library for reading and writing WAD files and creating and managing filesystems from these files.
2. Daemon built using Fuse API to mount and implement the filesystem.

## Command Format
 ./wadfs/wadfs -s somewadfile.wad /some/mount/directory
