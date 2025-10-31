#include "headerFiles/filelocks.h"
map<string, recursive_mutex> fileMutexes;
shared_mutex fileMutexesRegistry;

recursive_mutex& GetFileMutex(const string& filepath) {
    {
        shared_lock lock(fileMutexesRegistry); 
        auto it = fileMutexes.find(filepath);
        if (it != fileMutexes.end()) return it->second;
    }
    unique_lock lock(fileMutexesRegistry); 
    return fileMutexes[filepath]; 
}