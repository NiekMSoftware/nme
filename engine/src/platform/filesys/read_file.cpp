#include "nme/platform/filesys/read_file.h"

#include "nme/platform/memory/allocator.h"

namespace nme::fs {

namespace {

Result<usize, FileError> read_fully(File* f, u8* dst, const usize sz) {
    usize total = 0;
    while (total < sz) {
        auto r = file_read(f, dst + total, sz - total);
        if (result_is_err(&r)) return r;
        const usize got = result_value(&r);
        if (got == 0) break;    // EOF
        total += got;
    }
    return result_ok<usize, FileError>(total);
}

}  // namespace

Result<usize, FileError> file_read_into(const char* path, void* dst, usize cap) {
    auto ro = file_open(path, FileMode::Read);
    if (result_is_err(&ro)) return result_err<usize, FileError>(result_error(&ro));
    File f = result_value(&ro);

    auto rs = file_size(&f);
    if (result_is_err(&rs)) {
        file_close(&f);
        return rs;
    }
    const usize sz = result_value(&rs);

    if (sz > cap) {
        file_close(&f);
        return result_err<usize, FileError>(FileError::Io);
    }

    const auto rr = read_fully(&f, static_cast<u8*>(dst), sz);
    file_close(&f);
    return rr;
}

Result<FileBlob, FileError> file_read_entire(const char* path, const Allocator* allocator) {
    auto ro = file_open(path, FileMode::Read);
    if (result_is_err(&ro)) return result_err<FileBlob, FileError>(result_error(&ro));
    File f = result_value(&ro);

    auto rs = file_size(&f);
    if (result_is_err(&rs)) {
        file_close(&f);
        return result_err<FileBlob, FileError>(result_error(&rs));
    }
    const usize sz = result_value(&rs);

    if (sz == 0) {
        file_close(&f);
        return result_ok<FileBlob, FileError>(FileBlob{ nullptr, 0 });
    }

    u8* mem = static_cast<u8*>(alloc(allocator, sz, alignof(std::max_align_t)));
    if (!mem) {
        file_close(&f);
        return result_err<FileBlob, FileError>(FileError::OutOfMemory);
    }

    auto rr = read_fully(&f, mem, sz);
    file_close(&f);

    if (result_is_err(&rr)) {
        free(allocator, mem, sz);
        return result_err<FileBlob, FileError>(result_error(&rr));
    }
    if (result_value(&rr) != sz) {
        free(allocator, mem, sz);
        return result_err<FileBlob, FileError>(FileError::Io);
    }

    return result_ok<FileBlob, FileError>(FileBlob{ mem, sz });
}

void file_blob_free(FileBlob* blob, const Allocator* allocator) {}

}  // namespace nme::fs