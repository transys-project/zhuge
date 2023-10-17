#ifndef BITRATE_UTILS_H
#define BITRATE_UTILS_H

#include <string>
#include <vector>
#include <inttypes.h>

inline bool lessThan_64(uint64_t id1, uint64_t id2) {
    uint64_t noWarpSubtract = id2 - id1;
    uint64_t wrapSubtract = id1 - id2;
    return noWarpSubtract < wrapSubtract;
};

inline void SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c) {
  std::string::size_type pos1, pos2;
  pos2 = s.find(c);
  pos1 = 0;
  while (std::string::npos != pos2) {
    v.push_back(s.substr(pos1, pos2-pos1));      
    pos1 = pos2 + c.size();
    pos2 = s.find(c, pos1);
  }
  if (pos1 != s.length())
    v.push_back(s.substr(pos1));
};

#endif