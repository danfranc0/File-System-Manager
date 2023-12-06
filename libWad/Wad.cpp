#include "Wad.h"

#include <cstring>

using namespace std;

void Wad::reset() {
    wadFile.clear();
    wadFile.flush();
    wadFile.seekg(0);
}

vector<string> Wad::parsePath(string path) {
    vector<string> parsedPath;
    bool finished = false;

    int index = path.find("/");
    if (index == 0) {                           // "/" at beginning
        path = path.substr(index + 1);
    }                                           // extra "/" at end
    if (path.at(path.size() - 1) == '/') {
        path = path.substr(0, path.size() - 1);
    }
    while (!finished) {
        index = path.find("/");
        if (index == -1) {
            // add to vector
            parsedPath.push_back(path.substr(0, index));

            finished = !finished;
        } else {
            // add to vector
            parsedPath.push_back(path.substr(0, index));

            // new substring
            path = path.substr(index + 1);

        }
    }
    return parsedPath;
}

Wad::Node* Wad::verifyPath(string path) {
    // empty string
    if (path.size() == 0) {
        return nullptr;
    }

    // root directory
    if (path == "/") {
        return root;
    }

    vector<string> parsedPath = parsePath(path);    // vector of file/directory names
    if (parsedPath.size() == 0) {
        return nullptr;
    }

    Node* current = root;
    bool found = false;
    for (int i = 0; i < parsedPath.size(); i++) {
        if (current->children.size() > 0) {
            for (int j = 0; j < current->children.size(); j++) {
                // if name matches
                if (current->children[j]->name == parsedPath[i]) {
                    if (current->children[j]->name == parsedPath.back()) {
                        found = true;
                        return current->children[j];
                    }
                    current = current->children[j];
                    break;
                }

                // reached end of loop (unsuccessful)
                if (j == current->children.size() - 1) {
                    return nullptr;
                }
            }
        }
    }
    if (!found) {
        return nullptr;
    }
    return current;
}

uint32_t Wad::getPosition(vector<string> parsedPath, bool writing) {
    if (parsedPath.size() == 0) {
        if (!writing) {
            return descOffset;
        }
        // writing to file in root directory
        return descOffset + (numOfDesc * 16) - 16;
    }
    uint32_t position = 0;
    string parentStart;
    string target;
    string file = parsedPath.back();
    if (writing) {
        parsedPath.pop_back();
    }

    wadFile.seekg(descOffset);
    for (string s : parsedPath) {

        parentStart = s + "_START";          // start descriptor of current target directory

        // find descriptor
        int descIndex = 0;
        while (descIndex < numOfDesc) {
            uint32_t elementOffset;
            uint32_t elementLength;
            char descriptor[9];
            descriptor[8] = '\0';
            
            // read descriptor data
            wadFile.read((char*)&elementOffset, 4);
            wadFile.read((char*)&elementLength, 4);
            wadFile.read(descriptor, 8);

            if (descriptor == parentStart) {
                break;
            }
            descIndex++;
        }
    }
    position = wadFile.tellg();
    if (writing) {          // if writing to file, find position of file
        target = file;
    } else {                // else, find parent end descriptor
        target = parsedPath.back() + "_END";
    }

    // target descriptor
    for (int i = 0; i < numOfDesc; i++) {
        uint32_t elementOffset;
        uint32_t elementLength;
        char descriptor[9];
        descriptor[8] = '\0';
        
        // read descriptor data
        wadFile.read((char*)&elementOffset, 4);
        wadFile.read((char*)&elementLength, 4);
        wadFile.read(descriptor, 8);

        if (descriptor == target) {
            position = wadFile.tellg();
            break;
        }
    }
    return position;
}

Wad::Wad(const string &path) {
    wadFile.open(path, ios::in | ios::out | ios::binary);
    
    char magic[5];
    magic[4] = '\0';
    uint32_t numOfDesc;
    uint32_t descOffset;
    
    wadFile.read(magic, 4);
    wadFile.read((char*)&numOfDesc, 4);
    wadFile.read((char*)&descOffset, 4);

    this->magic = magic;
    this->numOfDesc = numOfDesc;
    this->descOffset = descOffset;

    // return to beginning of file
    reset();

    // root directory "/" node
    Node* tempRoot = new Node;
    tempRoot->name = "";
    tempRoot->elementOffset = 0;
    tempRoot->elementLength = 0;
    tempRoot->parent = nullptr;
    root = tempRoot;

    // construct tree
    wadFile.seekg(this->descOffset);

    Node* current = root;                           // ptr used to navigate
    int index = 0;
    while (index < this->numOfDesc) {

        uint32_t elementOffset;
        uint32_t elementLength;
        char tempName[9];
        tempName[8] = '\0';
        
        // read descriptor data
        wadFile.read((char*)&elementOffset, 4);
        wadFile.read((char*)&elementLength, 4);
        wadFile.read(tempName, 8);

        string elementName = tempName;

        // if END, leave directory
        if (elementName.find("_END") != string::npos) {
            current = current->parent;
            index++;
            continue;
        }

        // temp node
        Node* temp = new Node;
        temp->name = elementName;
        temp->elementOffset = elementOffset;
        temp->elementLength = elementLength;
        temp->parent = current;                     // set parent directory
        
        // if START, new directory
        if (elementName.find("_START") != string::npos) {
            temp->name = elementName.substr(0, elementName.size() - 6);
            current->children.push_back(temp);
            current = temp;
            index++;
            continue;
        }
        
        // if map directory (E#M#)
        if (temp->name.size() == 4 && isdigit(temp->name[1]) && isdigit(temp->name[3])) {
            index++;
            
            current->children.push_back(temp);      // push to children vector

            for (int i = 0; i < 10; i++) {          // 10 children of map directory
                Node* mapChild = new Node;
                
                // read descriptor data
                wadFile.read((char*)&elementOffset, 4);
                wadFile.read((char*)&elementLength, 4);
                wadFile.read(tempName, 8);

                // set node data
                mapChild->name = tempName;
                mapChild->elementOffset = elementOffset;
                mapChild->elementLength = elementLength;
                mapChild->parent = temp;
                mapChild->isContent = true;

                temp->children.push_back(mapChild);
                index++;
            }
            continue;
        }

        // add content to tree
        temp->isContent = true;
        current->children.push_back(temp);
        index++;
    }
}

Wad* Wad::loadWad(const string &path) {
    Wad* myWad = new Wad(path);
    return myWad;
}

string Wad::getMagic() {
    return magic;
}

bool Wad::isContent(const string &path) {
    Node* target = verifyPath(path);
    
    // verify path
    if (target == nullptr) {
        return false;
    }
    return target->isContent;
}

bool Wad::isDirectory(const string &path) {
    Node* target = verifyPath(path);
    
    // verify path
    if (target == nullptr) {
        return false;
    }
    return !target->isContent;
}

int Wad::getSize(const string &path) {
    Node* target = verifyPath(path);
    
    // verify path
    if (target == nullptr) {
        return -1;
    }

    // verify if directory
    if (target->isContent) {
        return target->elementLength;
    }
    return -1;
}

int Wad::getContents(const string &path, char *buffer, int length, int offset) {
    // if not content
    if (!isContent(path)) {
        return -1;
    }

    Node* target = verifyPath(path);
    
    // verify path
    if (target == nullptr) {
        return -1;
    }

    // offset goes beyond end of file
    if (offset > target->elementLength) {
        return 0;
    }

    reset();
    wadFile.seekg(target->elementOffset + offset);

    if (length + offset > target->elementLength) {
        length = target->elementLength - offset;
    }

    wadFile.read(buffer, length);
    return length;
}

int Wad::getDirectory(const string &path, vector<string> *directory) {

    // not directory
    if (!isDirectory(path)) {
        return -1;
    }

    Node* target = verifyPath(path);
    
    // verify path
    if (target == nullptr) {
        return -1;
    }

    for (int i = 0; i < target->children.size(); i++) {
        directory->push_back(target->children[i]->name);
    }

    return directory->size();
}

void Wad::createDirectory(const string &path) {
    string parentDir;                   // directory where new directory will be created
    string dirName;                     // name of new directory
    string dirPath = path;
    string startDesc;
    string endDesc;

    // remove extra "/" at end
    if (dirPath.at(dirPath.size() - 1) == '/') {
        dirPath = dirPath.substr(0, dirPath.size() - 1);
    }

    int index = dirPath.find_last_of("/");
    if (dirPath.substr(0, index) == "") {
        parentDir = "/";
    } else {
        parentDir = dirPath.substr(0, index);
    }
    // get new directory name
    dirName = dirPath.substr(index + 1, dirPath.size());
    // name longer than 2
    if (dirName.size() > 2) {
        return;
    }

    Node* target = verifyPath(parentDir);
    
    // verify path
    if (target == nullptr) {
        return;
    }
    // ignore map markers
    if (target->name.size() == 4 && isdigit(target->name[1]) && isdigit(target->name[3])) {
        return;
    }
    
    // check if directory already exists in current directory
    for (Node* child : target->children) {
        if (child->name == dirName) {
            return;
        }
    }

    // update data structure
    wadFile.seekg(4);
    numOfDesc += 2;
    wadFile.write((char*)&numOfDesc, 4);

    Node* newDir = new Node;
    newDir->name = dirName;
    newDir->elementOffset = 0;
    newDir->elementLength = 0;
    newDir->parent = target;
    
    target->children.push_back(newDir);

    // update file
    startDesc = dirName + "_START";
    endDesc = dirName + "_END";
    uint32_t empty = 0;

    char* startBuffer = new char[9];
    strcpy(startBuffer, startDesc.c_str());
    char* endBuffer = new char[9];
    strcpy(endBuffer, endDesc.c_str());

    // if parent is root directory, append to file
    if (target->name == "") {
        wadFile.seekg(0, ios::end);
        wadFile.write((char*)&empty, 8);
        wadFile.write(startBuffer, 8);
        wadFile.write((char*)&empty, 8);
        wadFile.write(endBuffer, 8);
        reset();
        return;
    }

    // else, find path and move 32 bytes
    vector<string> parsedPath = parsePath(path);
    parsedPath.pop_back();
    uint32_t position = getPosition(parsedPath, false);         // position of end descriptor
    wadFile.seekg(descOffset);
    
    // copy rest of file
    wadFile.seekg(position - 16);
    string text;
    wadFile >> text;
    char* fileEnd = new char[text.size()];
    wadFile.seekg(position - 16);
    wadFile >> fileEnd;
    
    // write new descriptor
    wadFile.seekg(position - 16);
    wadFile.write((char*)&empty, 8);
    wadFile.write(startBuffer, 8);
    wadFile.write((char*)&empty, 8);
    wadFile.write(endBuffer, 8);

    // paste rest of file
    wadFile.write(fileEnd, text.size());
    reset();
}

void Wad::createFile(const string &path) {
    string parentDir;                                   // directory where new directory will be created
    string fileName;                                    // name of new directory
    string filePath = path;

    int index = filePath.find_last_of("/");
    if (filePath.substr(0, index) == "") {
        parentDir = "/";
    } else {
        parentDir = filePath.substr(0, index);
    }
    // get new file name
    fileName = filePath.substr(index + 1, filePath.size());

    if (fileName.size() > 8) {  // exceeds max length
        return;
    }
    
    // ignore illegal sequences
    if (fileName.find("_END") != string::npos) {
        return;
    }
    if (fileName.find("_START") != string::npos) {
        return;
    }
    if (fileName.size() >= 4 && isdigit(fileName[1]) && isdigit(fileName[3])) {
        if (fileName.at(0) == 'E' && fileName.at(2) == 'M') {
            return;
        }
    }

    Node* target = verifyPath(parentDir);
    
    // verify path
    if (target == nullptr) {
        return;
    }
    // ignore map markers
    if (target->name.size() == 4 && isdigit(target->name[1]) && isdigit(target->name[3])) {
        return;
    }
    
    // check if file already exists
    for (Node* child : target->children) {
        if (child->name == fileName) {
            return;
        }
    }

    // update data structure and file
    wadFile.seekg(4);
    numOfDesc++;
    wadFile.write((char*)&numOfDesc, 4);

    Node* newFile = new Node;
    newFile->name = fileName;
    newFile->elementOffset = 0;
    newFile->elementLength = 0;
    newFile->parent = newFile;
    newFile->isContent = true;
    
    target->children.push_back(newFile);

    // update file
    uint32_t empty = 0;

    char* nameBuffer = new char[9];
    strcpy(nameBuffer, fileName.c_str());

    // if parent is root directory, append to file
    if (target->name == "") {
        wadFile.seekg(0, ios::end);
        wadFile.write((char*)&empty, 8);
        wadFile.write(nameBuffer, 8);
        reset();
        return;
    }

    // else, find path
    vector<string> parsedPath = parsePath(path);
    parsedPath.pop_back();
    uint32_t position = getPosition(parsedPath, false);         // position of end descriptor
    wadFile.seekg(descOffset);
    
    // copy rest of file
    wadFile.seekg(position - 16);
    string text;
    wadFile >> text;
    char* fileEnd = new char[text.size()];
    wadFile.seekg(position - 16);
    wadFile >> fileEnd;

    // write new descriptor
    wadFile.seekg(position - 16);
    wadFile.write((char*)&empty, 8);
    wadFile.write(nameBuffer, 8);

    // paste rest of file
    wadFile.write(fileEnd, text.size());
    reset();
}

int Wad::writeToFile(const string &path, const char *buffer, int length, int offset) {
    Node* target = verifyPath(path);
    
    // verify path
    if (target == nullptr) {
        return -1;
    }
    // if not content
    if (!target->isContent) {
        return -1;
    }
    // non empty file
    if (target->elementLength != 0) {
        return 0;
    }

    // find and copy descriptor list
    wadFile.seekg(descOffset);
    char* fileDesc = new char[numOfDesc * 16];
    wadFile.read(fileDesc, numOfDesc * 16);
    string text = fileDesc;
    text = buffer;

    // write lump data
    wadFile.seekg(descOffset);
    wadFile.write(buffer, length);

    // paste descriptor list
    wadFile.seekg(descOffset + length);
    wadFile.write(fileDesc, numOfDesc * 16);

    // update data
    target->elementOffset = descOffset;
    target->elementLength = length;
    descOffset += length;

    // update file header
    wadFile.seekg(8);
    wadFile.write((char*)&descOffset, 4);

    // find and edit file descriptor
    vector<string> parsedPath = parsePath(path);
    uint32_t position = getPosition(parsedPath, true) - 16;         // position of end descriptor
    wadFile.seekg(position);
    wadFile.write((char*)&target->elementOffset, 4);
    wadFile.write((char*)&target->elementLength, 4);
    reset();
    return target->elementLength;
}