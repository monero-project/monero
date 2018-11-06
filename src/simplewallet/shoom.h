#ifndef SHOOM_H
#define SHOOM_H

// Define SHOOM_IMPLEMENTATION in one CPP file.

#include <cstdint>
#include <string>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif  // _WIN32

namespace shoom {

enum ShoomError {
  kOK = 0,
  kErrorCreationFailed = 100,
  kErrorMappingFailed = 110,
  kErrorOpeningFailed = 120,
};

class Shm {
 public:
  // path should only contain alpha-numeric characters, and is normalized
  // on linux/macOS.
  explicit Shm(std::string path, size_t size);

  // create a shared memory area and open it for writing
  inline ShoomError Create() { return CreateOrOpen(true); };

  // open an existing shared memory for reading
  inline ShoomError Open() { return CreateOrOpen(false); };

  inline size_t Size() { return size_; };
  inline const std::string& Path() { return path_; }
  inline uint8_t* Data() { return data_; }

  ~Shm();

 private:
  ShoomError CreateOrOpen(bool create);

  std::string path_;
  uint8_t* data_ = nullptr;
  size_t size_ = 0;
#if defined(_WIN32)
  HANDLE handle_;
#else
  int fd_ = -1;
#endif
};
}

#endif // SHOOM_H
#if defined(SHOOM_IMPLEMENTATION)

#if defined(__linux) || defined(__APPLE__)

#include <fcntl.h>     // for O_* constants
#include <sys/mman.h>  // mmap, munmap
#include <sys/stat.h>  // for mode constants
#include <unistd.h>    // unlink

#if defined(__APPLE__)
#include <errno.h>
#endif // __APPLE__

#include <stdexcept>

namespace shoom {

Shm::Shm(std::string path, size_t size) : size_(size) { path_ = "/" + path; };

ShoomError Shm::CreateOrOpen(bool create) {
  if (create) {
    // shm segments persist across runs, and macOS will refuse
    // to ftruncate an existing shm segment, so to be on the safe
    // side, we unlink it beforehand.
    // TODO(amos) check errno while ignoring ENOENT?
    int ret = shm_unlink(path_.c_str());
    if (ret < 0) {
      if (errno != ENOENT) {
        return kErrorCreationFailed;
      }
    }
  }

  int flags = create ? (O_CREAT | O_RDWR) : O_RDONLY;

  fd_ = shm_open(path_.c_str(), flags, 0755);
  if (fd_ < 0) {
    if (create) {
      return kErrorCreationFailed;
    } else {
      return kErrorOpeningFailed;
    }
  }

  if (create) {
    // this is the only way to specify the size of a
    // newly-created POSIX shared memory object
    int ret = ftruncate(fd_, size_);
    if (ret != 0) {
      return kErrorCreationFailed;
    }
  }

  int prot = create ? (PROT_READ | PROT_WRITE) : PROT_READ;

  data_ = static_cast<uint8_t *>(mmap(nullptr,     // addr
                                      size_,       // length
                                      prot,        // prot
                                      MAP_SHARED,  // flags
                                      fd_,         // fd
                                      0            // offset
                                      ));

  if (!data_) {
    return kErrorMappingFailed;
  }

  return kOK;
}

Shm::~Shm() {
  munmap(data_, size_);
  close(fd_);
  shm_unlink(path_.c_str());
}

}  // namespace shoom

#elif defined(_WIN32)
#include <io.h>  // CreateFileMappingA, OpenFileMappingA, etc.

namespace shoom {

Shm::Shm(std::string path, size_t size) : path_(path), size_(size){};

ShoomError Shm::CreateOrOpen(bool create) {
  if (create) {
    DWORD size_high_order = 0;
    DWORD size_low_order = static_cast<DWORD>(size_);
    handle_ = CreateFileMappingA(INVALID_HANDLE_VALUE,  // use paging file
                                 NULL,                  // default security
                                 PAGE_READWRITE,        // read/write access
                                 size_high_order, size_low_order,
                                 path_.c_str()  // name of mapping object
                                 );

    if (!handle_) {
      return kErrorCreationFailed;
    }
  } else {
    handle_ = OpenFileMappingA(FILE_MAP_READ,  // read access
                               FALSE,          // do not inherit the name
                               path_.c_str()   // name of mapping object
                               );

    if (!handle_) {
      return kErrorOpeningFailed;
    }
  }

  DWORD access = create ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;

  data_ = static_cast<uint8_t*>(MapViewOfFile(handle_, access, 0, 0, size_));

  if (!data_) {
    return kErrorMappingFailed;
  }

  return kOK;
}

/**
 * Destructor
 */
Shm::~Shm() {
  if (data_) {
    UnmapViewOfFile(data_);
    data_ = nullptr;
  }

  CloseHandle(handle_);
}

}  // namespace shoom
#endif // _WIN32
#endif // SHOOM_IMPLEMENTATION
