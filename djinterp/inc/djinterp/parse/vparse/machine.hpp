/******************************************************************************
* djinterp [parse]                                                  machine.hpp
*
*
* path:      /inc/djinterp/parse/vparse/machine.hpp
******************************************************************************/
#ifndef VPARSE_MACHINE_HPP
#define VPARSE_MACHINE_HPP
// Operator-agnostic substrate. Knows nothing about any opcode or parsing family.
// Operator families live in their own modules and attach private state via `ext`.

// std
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

struct machine 
{
    // ---- shared substrate (all families) ----
    const std::string* input = nullptr;   // subject text (char-level families)
    int  sp = 0;                          // char cursor
    long steps = 0;
    bool halted = false, ok = false;
    std::string error;
    std::vector<std::string>* trace = nullptr;
    long stepLimit = 100000;

    // ---- family-private state ----
    // A driver allocates its family's State and points `ext` at it for the run.
    // The core never dereferences this; only that family's operators do.
    void* ext = nullptr;
};

// A voperator: an effect on the machine. Open by construction.
using VOp = std::function<void(machine&)>;

// A per-family operator registry (opcode -> handler). Each family owns one.
struct op_set 
{
    std::unordered_map<int, VOp> h;
    std::unordered_map<int, std::string> nm;
    void def(int code, std::string name, VOp fn) { h[code] = std::move(fn); nm[code] = std::move(name); }
    const VOp* find(int code) const { auto it = h.find(code); return it == h.end() ? nullptr : &it->second; }
    std::string name(int code) const { auto it = nm.find(code); return it == nm.end() ? "?" : it->second; }
};

#endif
