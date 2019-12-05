/*
 * Tencent is pleased to support the open source community by making
 * MMKV available.
 *
 * Copyright (C) 2018 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use
 * this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *       https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MMKV_MMKV_H
#define MMKV_MMKV_H

#include "MMKVPredef.h"

#include "InterProcessLock.h"
#include "MMBuffer.h"
#include "MMKVMetaInfo.hpp"
#include "MemoryFile.h"
#include "ThreadLock.h"
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mmkv {
class AESCrypt;
class CodedOutputData;
} // namespace mmkv

enum MMKVMode : uint32_t {
    MMKV_SINGLE_PROCESS = 0x1,
    MMKV_MULTI_PROCESS = 0x2,
};

enum MMKVRecoverStrategic : int {
    OnErrorDiscard = 0,
    OnErrorRecover,
};

enum MMKVErrorType : int {
    MMKVCRCCheckFail = 0,
    MMKVFileLength,
};

class MMKV {
    MMKV(const std::string &mmapID,
         MMKVMode mode,
         std::string *cryptKey,
         std::string *relativePath);

    ~MMKV();

    std::unordered_map<std::string, mmkv::MMBuffer> m_dic;
    std::string m_mmapID;
    std::string m_path;
    std::string m_crcPath;

    mmkv::MemoryFile m_file;
    size_t m_actualSize;
    mmkv::CodedOutputData *m_output;

    bool m_needLoadFromFile;
    bool m_hasFullWriteback;

    uint32_t m_crcDigest;
    mmkv::MemoryFile m_metaFile;
    mmkv::MMKVMetaInfo m_metaInfo;

    mmkv::AESCrypt *m_crypter;

    mmkv::ThreadLock m_lock;
    mmkv::FileLock m_fileLock;
    mmkv::InterProcessLock m_sharedProcessLock;
    mmkv::InterProcessLock m_exclusiveProcessLock;

    void loadFromFile();

    void partialLoadFromFile();

    void checkDataValid(bool &loadFromFile, bool &needFullWriteback);

    void checkLoadData();

    bool isFileValid();

    bool checkFileCRCValid(size_t acutalSize, uint32_t crcDigest);

    void recaculateCRCDigestWithIV(const unsigned char *iv);

    void updateCRCDigest(const uint8_t *ptr, size_t length);

    size_t readActualSize();

    void oldStyleWriteActualSize(size_t actualSize);

    bool writeActualSize(size_t actualSize,
                         uint32_t crcDigest,
                         const unsigned char *iv,
                         bool increaseSequence);

    bool ensureMemorySize(size_t newSize);

    bool fullWriteback();

    bool doFullWriteBack(mmkv::MMBuffer &&allData);

    const mmkv::MMBuffer &getDataForKey(const std::string &key);

    bool setDataForKey(mmkv::MMBuffer &&data, const std::string &key);

    bool removeDataForKey(const std::string &key);

    bool appendDataWithKey(const mmkv::MMBuffer &data, const std::string &key);

    void checkReSetCryptKey(int fd, int metaFD, std::string *cryptKey);

public:
    // call this before getting any MMKV instance
    static void initializeMMKV(const std::string &rootDir);

    // a generic purpose instance
    static MMKV *defaultMMKV(MMKVMode mode = MMKV_SINGLE_PROCESS, std::string *cryptKey = nullptr);

    // mmapID: any unique ID (com.tencent.xin.pay, etc)
    // if you want a per-user mmkv, you could merge user-id within mmapID
    static MMKV *mmkvWithID(const std::string &mmapID,
                            MMKVMode mode = MMKV_SINGLE_PROCESS,
                            std::string *cryptKey = nullptr,
                            std::string *relativePath = nullptr);

    static void onExit();

    const std::string &mmapID();

    const bool m_isInterProcess;

    std::string cryptKey();

    // transform plain text into encrypted text
    // or vice versa by passing cryptKey = null
    // or change existing crypt key
    bool reKey(const std::string &cryptKey);

    // just reset cryptKey (will not encrypt or decrypt anything)
    // usually you should call this method after other process reKey() the inter-process mmkv
    void checkReSetCryptKey(const std::string *cryptKey);

    bool set(bool value, const std::string &key);

    bool set(int32_t value, const std::string &key);

    bool set(uint32_t value, const std::string &key);

    bool set(int64_t value, const std::string &key);

    bool set(uint64_t value, const std::string &key);

    bool set(float value, const std::string &key);

    bool set(double value, const std::string &key);

    bool set(const char *value, const std::string &key);

    bool set(const std::string &value, const std::string &key);

    bool set(const mmkv::MMBuffer &value, const std::string &key);

    bool set(const std::vector<std::string> &vector, const std::string &key);

    // avoid unexpected type conversion (pointer to bool, etc)
    template <typename T>
    bool set(T value, const std::string &key) = delete;

    bool getBool(const std::string &key, bool defaultValue = false);

    int32_t getInt32(const std::string &key, int32_t defaultValue = 0);

    uint32_t getUInt32(const std::string &key, uint32_t defaultValue = 0);

    int64_t getInt64(const std::string &key, int64_t defaultValue = 0);

    uint64_t getUInt64(const std::string &key, uint64_t defaultValue = 0);

    float getFloat(const std::string &key, float defaultValue = 0);

    double getDouble(const std::string &key, double defaultValue = 0);

    bool getString(const std::string &key, std::string &result);

    mmkv::MMBuffer getBytes(const std::string &key);

    bool getVector(const std::string &key, std::vector<std::string> &result);

    size_t getValueSize(const std::string &key, bool acutalSize);

    int32_t writeValueToBuffer(const std::string &key, void *ptr, int32_t size);

    bool containsKey(const std::string &key);

    size_t count();

    size_t totalSize();

    std::vector<std::string> allKeys();

    void removeValueForKey(const std::string &key);

    void removeValuesForKeys(const std::vector<std::string> &arrKeys);

    void clearAll();

    // MMKV's size won't reduce after deleting key-values
    // call this method after lots of deleting f you care about disk usage
    // note that `clearAll` has the similar effect of `trim`
    void trim();

    // call this method if the instance is no longer needed in the near future
    // any subsequent call to the instance is undefined behavior
    void close();

    // call this method if you are facing memory-warning
    // any subsequent call to the instance will load all key-values from file again
    void clearMemoryCache();

    // you don't need to call this, really, I mean it
    // unless you care about out of battery
    void sync(SyncFlag flag = MMKV_SYNC);

    // check if content changed by other process
    void checkContentChanged();

    static bool isFileValid(const std::string &mmapID);

    void lock() { m_exclusiveProcessLock.lock(); }
    void unlock() { m_exclusiveProcessLock.unlock(); }

    typedef MMKVRecoverStrategic (*ErrorHandler)(const std::string &mmapID,
                                                 MMKVErrorType errorType);
    static void regiserErrorHandler(ErrorHandler handler);
    static void unRegisetErrorHandler();

    static void regiserLogHandler(mmkv::LogHandler handler);
    static void unRegisetLogHandler();

    static void setLogLevel(MMKVLogLevel level);

    // just forbid it for possibly misuse
    MMKV(const MMKV &other) = delete;
    MMKV &operator=(const MMKV &other) = delete;
};

#endif // MMKV_MMKV_H