#ifndef DELTA_UTILPP_H_
#define DELTA_UTILPP_H_

#define DELTA_UTILPP_PASTE_TOKEN_(x, y) x##y
#define DELTA_UTILPP_PASTE_TOKEN(x, y) DELTA_UTILPP_PASTE_TOKEN_(x, y)

// DELTA_UNIQUE_SYMBOL(name) generates a unique name prefixes by name.
#define DELTA_UNIQUE_SYMBOL(name) DELTA_UTILPP_PASTE_TOKEN(name, __LINE__)

#endif  // DELTA_UTILPP_H_
