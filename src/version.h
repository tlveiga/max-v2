#include <cstdio>
struct Version {
  int major = 0, minor = 0, revision = 0, build = 0;

  Version() {}
  Version(const char *version) {
    std::sscanf(version, "%d.%d.%d.%d", &major, &minor, &revision, &build);
  }

  bool operator<(const Version &other) {
    if (major < other.major)
      return true;
    if (minor < other.minor)
      return true;
    if (revision < other.revision)
      return true;
    if (build < other.build)
      return true;
    return false;
  }

  bool operator==(const Version &other) {
    return major == other.major && minor == other.minor &&
           revision == other.revision && build == other.build;
  }
};