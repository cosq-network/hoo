#include "RuntimeStringMethods.h"
#include "RuntimeArrayMethods.h"

// Force the linker to include this translation unit
void _hoo_runtime_methods_ensure_registration() {
    // This function forces the linker to include this translation unit,
    // which triggers the static initialization of RuntimeMethodRegistry entries.
}
