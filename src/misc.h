#ifndef COLMAP_SRC_UTIL_MISC_H_
#define COLMAP_SRC_UTIL_MISC_H_

#define CHECK(p)
#define CHECK_EQ(a,b)
#define CHECK_GE(a,b)
#define CHECK_GT(a,b)
#define CHECK_LT(a,b)
#define CHECK_NOTNULL(t)

template <typename T1, typename T2>
T2 TruncateCast(const T1 value) {
  return std::min(
      static_cast<T1>(std::numeric_limits<T2>::max()),
      std::max(static_cast<T1>(std::numeric_limits<T2>::min()), value));
}

#endif  // COLMAP_SRC_UTIL_MISC_H_