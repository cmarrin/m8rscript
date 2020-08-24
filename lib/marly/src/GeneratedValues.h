// This file is generated. Do not edit

#pragma once

#include <cstdint>
#include "Atom.h"

namespace marly {

enum class SA : uint16_t {
    Once = 0,
    Repeat = 1,
    Timer = 2,
    __ctor = 3,
    __proto = 4,
    __rawptr = 5,
    and$ = 6,
    at = 7,
    atput = 8,
    band = 9,
    bnot = 10,
    bor = 11,
    break$ = 12,
    bxor = 13,
    cat = 14,
    currentTime = 15,
    dec = 16,
    delay = 17,
    dup = 18,
    eq = 19,
    exec = 20,
    filter = 21,
    fold = 22,
    for$ = 23,
    ge = 24,
    gt = 25,
    if$ = 26,
    ifte = 27,
    import = 28,
    inc = 29,
    insert = 30,
    join = 31,
    le = 32,
    length = 33,
    loop = 34,
    lt = 35,
    map = 36,
    ne = 37,
    neg = 38,
    new$ = 39,
    not$ = 40,
    or$ = 41,
    pack = 42,
    pick = 43,
    pop = 44,
    print = 45,
    println = 46,
    remove = 47,
    start = 48,
    stop = 49,
    swap = 50,
    tuck = 51,
    unpack = 52,
    while$ = 53,
};

const char** sharedAtoms(uint16_t& nelts);
const char* specialChars();
static inline m8r::Atom SAtom(SA sa) { return m8r::Atom(static_cast<m8r::Atom::value_type>(sa)); }

}
