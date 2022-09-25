# chAT

AT command server written in C++

## Features
- Written in standard C++17
- Modern and self-explanatory APIs
- Low RAM footprint
- Robust and high efficiency I/O handling
- "Inhibit mode" to allow raw data transmission on same I/O medium
- The AT syntax parser can be used in standalone mode
- CRLF and LF compatible

The French word "chat" is "cat" in English.

## Design principles
- Prefer usability and performance to low resource usage 
- Does not assume anything
- No strict syntax checking and stupide limitations
- Max flexibility

## Usage
TODO

## Examples
See the `examples` directory.

- `parser.cpp` - Standalone AT command syntax parsing
- `posix_stdio_blocking.cpp` - AT command server with blocking POSIX standard I/O
- `posix_stdio_nonblocking.cpp` - AT command server with **non**blocking POSIX standard I/O

## License
AGPLv3
