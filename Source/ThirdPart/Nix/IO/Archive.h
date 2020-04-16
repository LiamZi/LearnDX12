/*
 * @Author: yanzi1221
 * @Email: yanzi1221@hotmail.com
 * @Date: 2020-04-08 18:33:19
 * @Last Modified by: yanzi1221
 * @Last Modified time: 2020-04-08 23:07:32
 * @Description: 此头文件我的朋友 bhlzlx 感谢他的贡献
 */

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string>

namespace Nix
{
    enum SeekFlag
    {
      SeekCur = 0,
      SeekEnd,
      SeekSet  
    };

    struct IFile
    {   
        /* data */
        typedef void (*MemoryFreeCB)(void *);
        virtual size_t read(size_t bytes, IFile *out) = 0;
        virtual size_t read(size_t bytes, void *out) = 0;
        virtual bool readable() = 0;
        
        virtual size_t write(size_t bytes, IFile *out) = 0;
        virtual size_t write(size_t bytes, const void *out) = 0;
        virtual bool writable() =0 ;
        
        virtual size_t tell() = 0;
        virtual bool seek(SeekFlag flag, long offset) = 0;
        virtual bool seekable() = 0;

        virtual const void *constData() const { return nullptr; }
        virtual size_t size() = 0;
        virtual void release() = 0;

        virtual ~IFile() {};

    };

    class IArchive
    {
      public:
        //OPEN
        virtual IFile *open(const std::string &path, uint8_t memoryMode = 0) = 0;
        //SAVE
        virtual bool save(const std::string &path, const void *data, size_t length) = 0;
        virtual const char *root() = 0;
        //TODO: LIST
        //TODO: Delete
        //TODO: Create
        //TODO: Destory
        virtual void release() = 0;

        virtual ~IArchive() {}
    };

    IArchive *CreateStdArchieve(const std::string &path);
    IFile *CreateMemoryBuffer(void *ptr, size_t length, IFile::MemoryFreeCB freeCB = nullptr);
    IFile *CreateMemoryBuffer(size_t length, IFile::MemoryFreeCB freeCB = [](void *ptr) {
        free(ptr);
    });

    class TextReader
    {
        private:
            Nix::IFile *_textMemory;
        
        public:
            TextReader() : _textMemory(nullptr) {};
            ~TextReader() { if(_textMemory) _textMemory->release(); }
            bool openFile(Nix::IArchive *arch, const std::string &filePath);
            const char *getText();
    };
} // namespace Nix