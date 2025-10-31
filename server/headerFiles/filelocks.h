#ifndef FILE_LOCKS_H
#define FILE_LOCKS_H

#include <mutex>
#include <map>
#include <string>
#include <shared_mutex>
using namespace std;
extern map<string, recursive_mutex> fileMutexes;
extern shared_mutex fileMutexesRegistry;

recursive_mutex& GetFileMutex(const string& filepath);

#endif
