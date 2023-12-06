#pragma once
#include <iostream>
#include <fstream>
#include <istream>
#include <vector>

class Wad
{
    Wad(const std::string &path);
    std::fstream wadFile;                       // wad file as fstream object

    std::string magic;                          // wad data magic
    uint32_t numOfDesc;                         // number of descriptors
    uint32_t descOffset;                        // descriptor offset

    // tree structure
    struct Node
    {
        // file data
        std::string name;                       // file name including extension
        uint32_t elementOffset;                 // lump data offset
        uint32_t elementLength;                 // file size
        bool isContent = false;

        Node* parent;
        std::vector<Node*> children;
    };

    // utility functions
    void reset();                                               // cleanup after calling each function
    std::vector<std::string> parsePath(std::string path);       // parse path string
    Node* verifyPath(std::string path);                         // returns nullptr if invalid path
    uint32_t getPosition(std::vector<std::string> parsedPath, bool writing);  // returns position in file of directory

    Node* root = nullptr;                       // root directory

public:
    static Wad* loadWad(const std::string &path);
    std::string getMagic();
    bool isContent(const std::string &path);
    bool isDirectory(const std::string &path);
    int getSize(const std::string &path);
    int getContents(const std::string &path, char *buffer, int length, int offset = 0);
    int getDirectory(const std::string &path, std::vector<std::string> *directory);
    void createDirectory(const std::string &path);
    void createFile(const std::string &path);
    int writeToFile(const std::string &path, const char *buffer, int length, int offset = 0);
};