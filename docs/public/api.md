# C API

Include the umbrella header:

```c
#include <libvna/vna.h>

const char *version = vna_version();
```

`vna_version()` is currently the only public function. It returns a static,
library-owned string; callers must not free it. The API supports C and C++
callers and does not expose libuv headers.

`libvna/utility.h` provides `LIBVNA_API` for symbol visibility and the common
`vna_status` type:

| Status | Meaning |
| --- | --- |
| `VNA_STATUS_SUCCESS` | Operation completed successfully |
| `VNA_STATUS_INVALID_ARGUMENT` | An input argument is invalid |
| `VNA_STATUS_OUT_OF_RANGE` | A value/result is outside its supported range |

These are conventions for future operation APIs, not additional implemented
operations. Status-returning APIs will leave output parameters unchanged on
error. Transport-specific statuses and connection lifetime rules will be
specified when connection APIs are implemented; no network status contract
is implied yet.

When linking a shared build manually, consumers must define `LIBVNA_SHARED`.
The exported CMake target propagates this automatically. Only the library
build defines `LIBVNA_BUILDING_LIBRARY`.

Implementations belong in module-specific sources under `src`; public
module headers belong under `include/libvna` and are included by the umbrella
header when introduced. Version implementation is isolated in `src/version.c`.
`VERSION` is the single source for the generated version header.
