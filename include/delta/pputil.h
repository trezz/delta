#define DELTA_CONCAT_INNER(a, b) a##b
#define DELTA_CONCAT(a, b) DELTA_CONCAT_INNER(a, b)
#define DELTA_UNIQUE(name) DELTA_CONCAT(name, __LINE__)
