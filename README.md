# chAT

AT command server written in C++

## Features
- Written in standard C++17
- Modern and self-explanatory APIs
- Robust and high efficiency I/O handling
- "Inhibit mode" to allow raw data transmission on same I/O medium
- The AT syntax parser can be used in standalone mode
- Rich built-in helper functions

The French word "chat" is "cat" in English.

## Design principles
- Prefer usability and speed over low RAM usage 
- No strict syntax checking and man-made limitations
- Does not assume anything: Max flexibility

## Usage
### If you're using [CPM](https://github.com/TheLartians/CPM.cmake)
```cmake
CPMAddPackage(
        GITHUB_REPOSITORY SudoMaker/chAT
        GIT_TAG <some tag, e.g. v1.0.0 or commit id>
        GIT_SHALLOW ON
)

target_link_libraries(foo chAT)
```
### Include & namespace
```c++
#include <chAT.hpp>
using namespace SudoMaker;
```

### Creating a server
```c++
chAT::Server at_srv;
```

### Install command handler
```c++
at_srv.set_command_callback([](chAT::Server& srv, const std::string& command) {
	auto &parser = srv.parser();        // type: chAT::ATParser&
	auto cmd_mode = parser.cmd_mode;    // type: chAT::CommandStatus
	auto &argv = parser.args;           // type: std::vector<std::string>&
	auto argc = argv.size();            // type: size_t
	
	// Now assume user typed "AT+FOO=abc,123\r\n", then:
	// 'command' contains "+FOO"
	// 'cmd_mode' contains chAT::CommandMode::Write
	// 'argv' contains ["abc", "123"] (yes it starts from 0)
	// 'argc' contains 2
	
	// ... do your stuff
	
	if (success) {
		return chAT::CommandStatus::OK;       // User will see "OK\r\n"
	}
	
	if (cpu_is_on_fire) {
		return chAT::CommandStatus::ERROR;    // User will see "ERROR\r\n"
	}
	
	return chAT::CommandStatus::CUSTOM;       // No output will be produced by default
	
	// You can always add custom output via chAT::Server::write_*() functions.
});
```

### Install I/O handlers
These functions need to return the number of bytes actually read/written. If the operation can't be completed (i.e. not a single byte has been read/written), simply return -1. However the server will not read `errno`. It up to the handler to handle unrecoverable errors.

```c++
at_srv.set_io_callback({
	.callback_io_read = [](auto buf, auto len){
		return read(STDIN_FILENO, buf, len);
	},
	.callback_io_write = [](auto buf, auto len){
		return write(STDOUT_FILENO, buf, len);
	},
});
```

If you're using non-blocking I/O (i.e. these functions return immediately), enable the nonblocking mode.

```c++
at_srv.set_nonblocking_mode(true);
```

### Using the write helper functions
These functions can be used anywhere. But they're not thread-safe (i.e. a mutex is needed if you call them from more than 1 thread).

#### Server::write_data(const void *buf, size_t len);
Write data. It's simple enough.

The data is not copied and needs to be available before `Server::run()` completed all write tasks.

#### Server::write_cstr(const char *buf, ssize_t len = -1);
Write C-style string. It's basically the same as `write_data`, but it will comupte the string length if `len` is not specified.

The data is not copied and needs to be available before `Server::run()` completed all write tasks.

#### void Server::write_str(std::string str);
Write C++ `std::string`. `std::move` is used internally to save potential copies.

#### void Server::write_vec8(std::vector<uint8_t> vec8);
Write C++ `std::vector<uint8_t>`. `std::move` is used internally to save potential copies.

#### void Server::write_error();
Write `ERROR\r\n`.

#### void Server::write_error_reason(std::string str);
Write error reason. `std::move` is used internally to save potential copies.

Example: If `str` is "putain", it will write `+ERROR: putain\r\n`.

#### void Server::write_ok();
Write `OK\r\n`.

#### void Server::write_response_prompt(std::string str);
Write response. `std::move` is used internally to save potential copies.

Example: If the current command is `+FOO` and `str` is "bon", it will write `+FOO: bon\r\n`.

#### void Server::write_response_prompt();
Write the response prompt. 

Example: If the current command is `+FOO`, it will write `+FOO: `.

#### void Server::write_error_prompt();
Write `+ERROR: `.

#### void Server::write_line_end();
Write `\r\n`.

### Running the server
You only need to care about the return value in nonblocking mode.
```c++
auto rc = at_srv.run();         // type: chAT::Server::RunStatus

using RunStatus = chAT::Server::RunStatus;

if (rc & RunStatus::WantRead) {
	// Enable IN event
}

if (rc & RunStatus::WantWrite) {
	// Enable OUT event
}
```

## Examples
See the `examples` directory.

- `parser.cpp` - Standalone AT command syntax parsing
- `posix_stdio_blocking.cpp` - AT command server with blocking POSIX standard I/O
- `posix_stdio_nonblocking.cpp` - AT command server with **non**blocking POSIX standard I/O

## License
AGPLv3
