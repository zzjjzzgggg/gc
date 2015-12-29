#ifndef __LZ4IO_H__
#define __LZ4IO_H__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cassert>
#include <string>

#include "lib/lz4.h"


namespace lz4 {
    enum {
        BLOCK_BYTES = 1024 * 64,
    };

    static const size_t DATA_CAPACITY = BLOCK_BYTES;
    static const size_t CHUNK_CAPACITY = LZ4_COMPRESSBOUND(BLOCK_BYTES);

    class LZ4Out {
    private:
        char *data_buf_, *chunk_buf_;
        size_t len_dat_;
        FILE *output_;
    private:
        void WriteChunk();
    public:
        LZ4Out();
        LZ4Out(const char* file_name,
               const bool append=false): LZ4Out() {
            SetFileName(file_name, append);
        }
        ~LZ4Out();

        void SetFileName(const char* file_name,
                         const bool append=false);
        // add data to buffer, the data may be larger than
        // buffer size.
        void Write(const void* data, const size_t length);
        void Save(const int val) { Write(&val, sizeof(int)); }
        void Save(const double val){ Write(&val, sizeof(double)); };
        void Close();
        bool IsClosed() const { return output_ == NULL; }

        void CompressFile(const char *input_file_name);
    };

    class LZ4In {
    private:
        char *chunk_buf_, *data_buf_;
        size_t len_data_, num_read_;
        FILE* input_;
    private:
        bool FillBuffer();
    public:
        LZ4In();
        LZ4In(const char* file_name): LZ4In() {
            input_ = fopen(file_name, "rb");
            if(input_ == NULL) {
                fprintf(stderr, "Open file '%s' failed!\n", file_name);
                exit(1);
            }
        }
        ~LZ4In();

        void SetFileName(const char* file_name);
        bool ReadOriChunk(char* data, size_t& length);
        bool ReadChunk(char* data, size_t& length);
        void Close();

        bool Eof();
        size_t Read(const void* data, const size_t len);
        void Load(int& val) { Read(&val, sizeof(int)); }
        void Load(double& val) { Read(&val, sizeof(double)); }

        void DecompressFile(const char* output_file_name);
    };

}


#endif /* __LZ4IO_H__ */
