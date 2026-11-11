// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#pragma once

// Descriptor persistence seam of native_wallet_provisioning.cpp (source
// overlay, materialized into src/wallet beside it before configure).
//
// Linux/Android keep exactly the existing calls: fsync of the file and of the
// attempt directory. The occupied-directory listing reads an independent
// open file description of the same held directory (see entries()); for the
// provisioning path's single listing of a freshly opened directory the result
// is the one the base's CLOEXEC duplicate gave. Darwin (iOS device, iOS simulator, and the macOS host the QA Mac
// runs the SDK unit tests on) uses the same descriptor sequence, with one
// platform rule for durability:
//
//   sync  Linux:  fsync (unchanged).
//         Darwin: fcntl(F_FULLFSYNC), which also asks the device to flush its
//                 write cache; Darwin fsync does not. Only when the file
//                 system reports it cannot perform a full sync (ENOTSUP,
//                 ENOTTY or EINVAL) does it fall back to fsync, which is then
//                 the strongest persistence that file system offers. Any
//                 other F_FULLFSYNC failure (EIO, EBADF, ...) is returned as a
//                 failure and never retried, so the caller still reports
//                 OutcomeUnknown/DurabilityUncertain for it.
//
// Every other primitive the provisioning path uses (open with O_DIRECTORY |
// O_NOFOLLOW | O_CLOEXEC, fstat, openat(".") for the listing, fdopendir/readdir, openat
// with O_CREAT | O_EXCL | O_NOFOLLOW, write, close) is plain POSIX with the
// same meaning on both platforms. No rename or replacement is involved.
//
// Any other platform: ZANO_NATIVE_PROVISIONING_DESCRIPTORS is 0 and
// provisioning keeps its existing UnsupportedCapability refusal before any
// effect. __linux__ is never defined here.

#if defined(__linux__)
#define ZANO_NATIVE_PROVISIONING_DESCRIPTORS 1
#define ZANO_NATIVE_PROVISIONING_DARWIN 0
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IOS || TARGET_OS_OSX
#define ZANO_NATIVE_PROVISIONING_DESCRIPTORS 1
#define ZANO_NATIVE_PROVISIONING_DARWIN 1
#else
#define ZANO_NATIVE_PROVISIONING_DESCRIPTORS 0
#define ZANO_NATIVE_PROVISIONING_DARWIN 0
#endif
#else
#define ZANO_NATIVE_PROVISIONING_DESCRIPTORS 0
#define ZANO_NATIVE_PROVISIONING_DARWIN 0
#endif

#if ZANO_NATIVE_PROVISIONING_DESCRIPTORS
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace tools { namespace native_session { namespace durable_descriptor
{
  // 0, or -1 with errno set. See the header comment for the Darwin rule.
  inline int sync(int fd) noexcept
  {
#if ZANO_NATIVE_PROVISIONING_DARWIN
    if (::fcntl(fd, F_FULLFSYNC) == 0) return 0;
    const int error = errno;
    if (error != ENOTSUP && error != ENOTTY && error != EINVAL)
    {
      errno = error;
      return -1;
    }
    return ::fsync(fd);
#else
    return ::fsync(fd);
#endif
  }

  enum class listing { empty, occupied, unavailable };

  // Any entry other than "." and ".." in the held directory makes it
  // occupied.
  //
  // The listing reads its OWN open file description of the held directory,
  // opened relative to the caller's descriptor (openat(directory, ".")), never
  // a dup: a dup shares the caller's description and its directory offset, so
  // one readdir pass would leave the caller's cursor at the end and a later
  // listing through the same descriptor would see an empty directory
  // (fdopendir does not rewind; rewinding would move the caller's cursor
  // instead). The independent description is accepted only when fstat
  // reports a directory with the same st_dev/st_ino as the caller's
  // descriptor. "." is resolved inside the held inode itself, so no path
  // name, no link and no /proc path is involved. Any failure (closed or
  // non-directory caller descriptor, no search permission, identity
  // mismatch) is listing::unavailable, never empty.
  inline listing entries(int directory) noexcept
  {
    struct stat held{};
    if (::fstat(directory, &held) != 0 || !S_ISDIR(held.st_mode)) return listing::unavailable;
    const int own = ::openat(directory, ".", O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (own < 0) return listing::unavailable;
    struct stat opened{};
    if (::fstat(own, &opened) != 0 || !S_ISDIR(opened.st_mode) ||
        opened.st_dev != held.st_dev || opened.st_ino != held.st_ino)
    {
      ::close(own);
      return listing::unavailable;
    }
    DIR* list = ::fdopendir(own);
    if (!list)
    {
      ::close(own);
      return listing::unavailable;
    }
    listing result = listing::empty;
    errno = 0;
    while (const dirent* entry = ::readdir(list))
    {
      if (std::strcmp(entry->d_name, ".") != 0 && std::strcmp(entry->d_name, "..") != 0)
      {
        result = listing::occupied;
        break;
      }
    }
    if (result == listing::empty && errno != 0) result = listing::unavailable;
    ::closedir(list);
    return result;
  }
}}}
#endif
