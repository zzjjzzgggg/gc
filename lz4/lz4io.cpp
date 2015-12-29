#include "lz4io.h"

namespace lz4 {


    size_t write_int(FILE* fp, const int i) {
        return fwrite(&i, sizeof(int), 1, fp);
    }

    size_t write_bin(FILE* fp, const void* array,
                     const size_t arrayBytes) {
        return fwrite(array, 1, arrayBytes, fp);
    }

    size_t read_int(FILE* fp, int* i) {
        return fread(i, sizeof(int), 1, fp);
    }

    size_t read_bin(FILE* fp, void* array, const size_t arrayBytes) {
        return fread(array, 1, arrayBytes, fp);
    }

//===================================================================

    LZ4Out::LZ4Out() {
        data_buf_ = new char[DATA_CAPACITY];
        chunk_buf_ = new char[CHUNK_CAPACITY];
        if(data_buf_ == NULL || chunk_buf_ == NULL) {
            fprintf(stderr, "Allocate space failed!\n");
            exit(1);
        }
        len_dat_ = 0;
        output_ = NULL;
    }

    LZ4Out::~LZ4Out() {
        Close();
        if(data_buf_ != NULL) delete[] data_buf_;
        if(chunk_buf_ != NULL) delete[] chunk_buf_;
    }

    void LZ4Out::Close() {
        if(output_ != NULL) {
            WriteChunk();
            fclose(output_);
            output_ = NULL;
        }
    }

    void LZ4Out::SetFileName(const char* file_name,
                             const bool append) {
        Close();
        const char* mode = append ? "ab" : "wb";
        output_ = fopen(file_name, mode);
        if(output_ == NULL) {
            fprintf(stderr, "Open file '%s' failed!\n", file_name);
            exit(1);
        }
    }

    void LZ4Out::Write(const void* data, const size_t length) {
        size_t written = 0;
        while (length - written > 0) {
            size_t len_available = DATA_CAPACITY - len_dat_;
            if (len_available == 0) WriteChunk();
            size_t num_to_write = std::min(length - written,
                                           len_available);
            memcpy(data_buf_ + len_dat_, (char*)data + written,
                   num_to_write);
            len_dat_ += num_to_write;
            written += num_to_write;
        }
    }

    void LZ4Out::WriteChunk() {
        if (len_dat_ == 0) return;
        size_t chunk_len = LZ4_compress_fast(
            data_buf_, chunk_buf_, len_dat_,
            CHUNK_CAPACITY, 9);
        if(chunk_len > 0) {
            write_int(output_, chunk_len);
            write_bin(output_, chunk_buf_, chunk_len);
        }
        len_dat_ = 0;
    }

    void LZ4Out::CompressFile(const char *file_name) {
        FILE* file_id = fopen(file_name, "rb");
        if(file_id == NULL) {
            fprintf(stderr, "Open file '%s' failed!\n", file_name);
            exit(1);
        }
        char buffer[1024];
        size_t num_read;
        while(true) {
            num_read = fread(buffer, 1, 1024, file_id);
            Write(buffer, num_read);
            if (num_read < 1024) break;
        }
        fclose(file_id);
        Close();
    }


//===================================================================

    LZ4In::LZ4In() {
        chunk_buf_ = new char[CHUNK_CAPACITY];
        data_buf_ = new char[DATA_CAPACITY];
        if(data_buf_ == NULL || chunk_buf_ == NULL) {
            fprintf(stderr, "Allocate space failed!\n");
            exit(1);
        }
        input_ = NULL;
        len_data_ = num_read_ = 0;
    }

    LZ4In::~LZ4In() {
        Close();
        if(chunk_buf_ != NULL) delete[] chunk_buf_;
        if(data_buf_ != NULL) delete[] data_buf_;
    }

    void LZ4In::Close() {
        if(input_!=NULL) {
            fclose(input_);
            input_ = NULL;
        }
        len_data_ = num_read_ = 0;
    }

    void LZ4In::SetFileName(const char* file_name) {
        Close();
        input_ = fopen(file_name, "rb");
        if(input_ == NULL) {
            fprintf(stderr, "Open file '%s' failed!\n", file_name);
            exit(1);
        }
    }

    bool LZ4In::ReadOriChunk(char* data, size_t& length) {
        length = 0;
        int chunk_len;
        size_t num_read = read_int(input_, &chunk_len);
        if(num_read < 1 || chunk_len == 0) return false;
        length = read_bin(input_, data, chunk_len);
        return length>0;
    }

    bool LZ4In::ReadChunk(char* data, size_t& length) {
        length = 0;
        int chunk_len;
        size_t num_read = read_int(input_, &chunk_len);
        if(num_read < 1 || chunk_len == 0) return false;
        read_bin(input_, chunk_buf_, chunk_len);
        length = LZ4_decompress_safe(
            chunk_buf_, data, chunk_len, (int)DATA_CAPACITY);
        return length>0;
    }

    bool LZ4In::Eof() {
        if (num_read_ < len_data_) return false;
        return !FillBuffer();
    }

    bool LZ4In::FillBuffer() {
        num_read_ = 0;
        return ReadChunk(data_buf_, len_data_);
    }

    size_t LZ4In::Read(const void *data, const size_t length) {
        size_t have_read = 0;
        while (have_read < length) {
            size_t len_available = len_data_ - num_read_;
            if (len_available == 0) FillBuffer();
            size_t num_to_read = std::min(length - have_read,
                                          len_available);
            memcpy((char*)data + have_read, data_buf_ + num_read_,
                   num_to_read);
            num_read_ += num_to_read;
            have_read += num_to_read;
        }
        return have_read;
    }

    void LZ4In::DecompressFile(const char* output_file_name) {
        FILE* file_id = fopen(output_file_name, "wb");
        char buffer[1024];
        size_t num_read;
        while(!Eof()) {
            num_read = Read(buffer, 1024);
            write_bin(file_id, buffer, num_read);
        }
        fclose(file_id);
    }
}
