# nme

A learning game engine build alongside Jason Gregory's *Game
Engine Architecture*, 4th edition, Volume I (2026). This is the usual Chapter 1-2 starting
point: the runtime architecture (§1.5) is scaffolded - a documented header per
subsyste - adn the Chapter 2 toolchain is wired up (cross-platform CMake,
warnings-as-errors, ASan/UBSan, opt-in Tracy, doctest/CTest). Set up for the
Visual Studio "Open Folder" CMake workflow via `CMakePresets.json`.

## Layer placement (per the §1.5 diagram)
Following the Core Systems / Platform Independence Layer figure:

- **Platform Independence Layer** holds things whose job is to smooth over
compiler/OS/CPU differences: **primitive data types**, **collections & 
iterators**, **file system**, **hi-res timer**, **threading**, and **graphics wrappers**(`rhi`).
Collections live here - not in Core - because a central purpose of 
that library is behaving identically across STL implementation.
- **Core Systems** holds engine services build *on top* of the platform layer:
module start-up/shut-down, logging, math, memory allocation, etc.

## Prerequisities
- **CMake >= 3.24**, a **C++20** compiiler (MSVC 2022, GCC 11+, Clang 14+), **git**
- A generator: Ninja (VS uses this for Open Folder) or the VS generator

