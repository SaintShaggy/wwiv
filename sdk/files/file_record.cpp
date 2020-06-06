/**************************************************************************/
/*                                                                        */
/*                          WWIV Version 5.x                              */
/*                Copyright (C)2020, WWIV Software Services               */
/*                                                                        */
/*    Licensed  under the  Apache License, Version  2.0 (the "License");  */
/*    you may not use this  file  except in compliance with the License.  */
/*    You may obtain a copy of the License at                             */
/*                                                                        */
/*                http://www.apache.org/licenses/LICENSE-2.0              */
/*                                                                        */
/*    Unless  required  by  applicable  law  or agreed to  in  writing,   */
/*    software  distributed  under  the  License  is  distributed on an   */
/*    "AS IS"  BASIS, WITHOUT  WARRANTIES  OR  CONDITIONS OF ANY  KIND,   */
/*    either  express  or implied.  See  the  License for  the specific   */
/*    language governing permissions and limitations under the License.   */
/**************************************************************************/
#include "sdk/files/file_record.h"

#include "core/file.h"
#include "core/strings.h"
#include "local_io/keycodes.h"
#include "sdk/vardec.h"
#include "sdk/files/files_ext.h"
#include <string>
#include <utility>

using std::endl;
using std::string;
using namespace wwiv::core;
using namespace wwiv::strings;

namespace wwiv::sdk::files {

static void _align(char* fn) {
  // TODO Modify this to handle long file names
  char name[40], ext[40];

  auto invalid = false;
  if (fn[0] == '.') {
    invalid = true;
  }

  for (size_t i = 0; i < size(fn); i++) {
    if (fn[i] == '\\' || fn[i] == '/' || fn[i] == ':' || fn[i] == '<' || fn[i] == '>' ||
        fn[i] == '|') {
      invalid = true;
    }
  }
  if (invalid) {
    strcpy(fn, "        .   ");
    return;
  }
  auto* s2 = strrchr(fn, '.');
  if (s2 == nullptr || strrchr(fn, '\\') > s2) {
    ext[0] = '\0';
  } else {
    strcpy(ext, &(s2[1]));
    ext[3] = '\0';
    s2[0] = '\0';
  }
  strcpy(name, fn);

  for (int j = strlen(name); j < 8; j++) {
    name[j] = ' ';
  }
  name[8] = '\0';
  auto has_wildcard = false;
  auto has_space = false;
  for (auto k = 0; k < 8; k++) {
    if (name[k] == '*') {
      has_wildcard = true;
    }
    if (name[k] == ' ') {
      has_space = true;
    }
    if (has_space) {
      name[k] = ' ';
    }
    if (has_wildcard) {
      name[k] = '?';
    }
  }

  for (int i2 = strlen(ext); i2 < 3; i2++) {
    ext[i2] = ' ';
  }
  ext[3] = '\0';
  has_wildcard = false;
  for (int i3 = 0; i3 < 3; i3++) {
    if (ext[i3] == '*') {
      has_wildcard = true;
    }
    if (has_wildcard) {
      ext[i3] = '?';
    }
  }

  char buffer[MAX_PATH];
  for (auto i4 = 0; i4 < 12; i4++) {
    buffer[i4] = SPACE;
  }
  strcpy(buffer, name);
  buffer[8] = '.';
  strcpy(&(buffer[9]), ext);
  strcpy(fn, buffer);
  for (auto i5 = 0; i5 < 12; i5++) {
    fn[i5] = to_upper_case<char>(fn[i5]);
  }
}

std::string align(const std::string& file_name) {
  if (file_name.size() >= 1024) {
    throw std::runtime_error("size to align >=1024 chars");
  }
  char s[1024];
  to_char_array(s, file_name);
  _align(s);
  return s;
}

std::string unalign(const std::string& file_name) {
  auto s{file_name};
  s.erase(remove(s.begin(), s.end(), ' '), s.end());
  if (s.empty()) {
    return {};
  }
  if (s.back() == '.') {
    s.pop_back();
  }
  return ToStringLowerCase(s);
}

FileName::FileName(std::string aligned_filename)
    : aligned_filename_(std::move(aligned_filename)), unaligned_filename_(unalign(aligned_filename_)) {}

FileName::FileName(const FileName& that)
    : aligned_filename_(that.aligned_filename()), unaligned_filename_(that.unaligned_filename()) {}

const std::string& FileName::aligned_filename() const noexcept {
  return aligned_filename_;
}

const std::string& FileName::unaligned_filename() const noexcept {
  return unaligned_filename_;
}

//static
std::optional<FileName> FileName::FromUnaligned(const std::string& unaligned_name) {
  return {FileName(align(unaligned_name))};
}

FileRecord::FileRecord() : FileRecord(uploadsrec{}) {}

FileRecord::FileRecord(const FileRecord& that) : u_(that.u()) {}

FileRecord& FileRecord::operator=(const FileRecord& that) {
  u_ = that.u();
  return *this;
}


bool FileRecord::has_extended_description() const noexcept {
  return mask(mask_extended);
}

void FileRecord::set_extended_description(bool b) {
  set_mask(mask_extended, b);
}

void FileRecord::set_mask(int mask, bool on) {
  if (on) {
    u_.mask |= mask;
  } else {
    u_.mask &= ~mask;
  }
}

bool FileRecord::mask(int mask) const {
  return u_.mask & mask;
}

bool FileRecord::set_filename(const std::string& unaligned_filename) {
  if (unaligned_filename.size() > 12) {
    return false;
  }
  to_char_array(u_.filename, align(unaligned_filename));
  return true;
}

bool FileRecord::set_description(const std::string& desc) {
  to_char_array(u_.description, desc);
  return true;
}

FileName FileRecord::filename() const {
  return FileName(u_.filename);
}

std::string FileRecord::aligned_filename() const {
  return align(u_.filename);
}

std::string FileRecord::unaligned_filename() const {
  return unalign(u_.filename);
}


std::string FilePath(const std::filesystem::path& directory_name, const FileRecord& f) {
  if (directory_name.empty()) {
    return f.unaligned_filename();
  }
  return PathFilePath(directory_name, f).string();
}

std::filesystem::path PathFilePath(const std::filesystem::path& directory_name,
                                   const FileRecord& f) {
  if (directory_name.empty()) {
    return f.unaligned_filename();
  }
  return directory_name / f.unaligned_filename();
}

std::string FilePath(const std::filesystem::path& directory_name, const FileName& f) {
  if (directory_name.empty()) {
    return f.unaligned_filename();
  }
  return PathFilePath(directory_name, f).string();
}

std::filesystem::path PathFilePath(const std::filesystem::path& directory_name,
                                   const FileName& f) {
  if (directory_name.empty()) {
    return f.unaligned_filename();
  }
  return directory_name / f.unaligned_filename();
}

} // namespace wwiv::sdk::files
