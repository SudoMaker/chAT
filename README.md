# chAT

AT command server written in C++

## Features
- Written in standard C++17
- Easy-to-use APIs
- Low RAM footprint
- Robust and high efficiency I/O handling
- "Inhibit mode" to allow raw data transmission on same I/O medium

## Design principles
- No strict syntax checking
- Does not assume anything
- Max flexibility for users

## Examples
See the `examples` directory.

- `parser.cpp` - Standalone AT command syntax parsing
- `posix_stdio_blocking.cpp` - AT command server with blocking POSIX standard I/O
- `posix_stdio_nonblocking.cpp` - AT command server with **non**blocking POSIX standard I/O

## License
AGPLv3