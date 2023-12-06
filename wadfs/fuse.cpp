#include <fuse.h>
#include <unistd.h>
#include <sys/types.h>
#include <cstring>
#include "../libWad/Wad.h"

static int getattr_callback(const char *path, struct stat *stbuf) {
    // is directory
    if (strcmp(path, "/") == 0 || ((Wad*)fuse_get_context()->private_data)->isDirectory(path)) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    // is file
    if (((Wad*)fuse_get_context()->private_data)->isContent(path)) {
        stbuf->st_mode = S_IFREG | 0777;
        stbuf->st_nlink = 1;
        stbuf->st_size = ((Wad*)fuse_get_context()->private_data)->getSize(path);
        return 0;
    }
    return -ENOENT;
}

static int mknod_callback(const char* path, mode_t mode, dev_t rdev) {
    if (((Wad*)fuse_get_context()->private_data)->isContent(path)) {
        return -ENOENT;
    }

    ((Wad*)fuse_get_context()->private_data)->createFile(path);

    return 0;
}

static int mkdir_callback(const char* path, mode_t mode) {
    if (((Wad*)fuse_get_context()->private_data)->isDirectory(path)) {
        return -ENOENT;
    }

    ((Wad*)fuse_get_context()->private_data)->createDirectory(path);
    
    return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    // get contents
    if (((Wad*)fuse_get_context()->private_data)->isContent(path)) {
        int ret = ((Wad*)fuse_get_context()->private_data)->getContents(path, buf, size, offset);
        if (ret == -1) {
            return -ENOENT;
        }
        return ret;
    }
    return -ENOENT;
}

static int write_callback(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info) {
    int ret = ((Wad*)fuse_get_context()->private_data)->writeToFile(path, buffer, size, offset);
    if (ret != -1) {
        return ret;
    }
    return -ENOENT;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    std::vector<std::string> directory;
    int size = ((Wad*)fuse_get_context()->private_data)->getDirectory(path, &directory);
    if (size == -1) {
        return -ENOENT;
    }
    
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    for (std::string name : directory) {
        filler(buf, name.c_str(), NULL, 0);
    }
    return 0;
}

static struct fuse_operations operations = {
    .getattr = getattr_callback,
    .mknod = mknod_callback,
    .mkdir = mkdir_callback,
    .read = read_callback,
    .write = write_callback,
    .readdir = readdir_callback,
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Not enough arguments." << std::endl;
        exit(EXIT_SUCCESS);
    }

    std::string wadPath = argv[argc - 2];

    // relative path
    if (wadPath.at(0) != '/') {
        wadPath = std::string(get_current_dir_name()) + "/" + wadPath;
    }
    Wad* myWad = Wad::loadWad(wadPath);

    argv[argc - 2] = argv[argc - 1];
    argc--;
    return fuse_main(argc, argv, &operations, myWad);
}