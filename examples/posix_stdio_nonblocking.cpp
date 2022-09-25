/*
    This file is part of chAT.
    Copyright (C) 2022 Reimu NotMoe <reimu@sudomaker.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <vector>

#include <cinttypes>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>

#include <chAT.hpp>

using namespace SudoMaker;

static void set_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
	chAT::server at_srv;

	std::vector<uint8_t> raw_read_buf;
	size_t raw_read_pos = 0;
	bool raw_read_enabled = false;

	at_srv.add_command({"+FOO", "fooooo",
			    [](auto &srv, auto &parser) {
				    static const char msg[] = "Foo in a const string. You know what? I'm not being copied.\r\n";
				    srv.write_cstr(msg, sizeof(msg) - 1);

				    std::string s = "Foo in a std::string\r\n";
				    srv.write(std::move(s));

				    static const uint8_t msg2[] = "Foo in a vector\r\n";
				    std::vector<uint8_t> v;
				    v.insert(v.end(), msg2, msg2+sizeof(msg2)-1);
				    srv.write(std::move(v));

				    return chAT::command_status::OK;
			    }});

	at_srv.add_command({"+RAW_READ", "Pause AT command parsing and read <len> bytes of data directly",
			    [&](auto &srv, auto &parser) {
				    switch (parser.cmd_mode) {
					    case chAT::command_mode::Write: {
						    if (parser.args.size() != 1) {
							    return chAT::command_status::ERROR;
						    }

						    auto &len_str = parser.args[0];
						    if (len_str.empty()) {
							    return chAT::command_status::ERROR;

						    }

						    size_t len = std::stoi(len_str);

						    raw_read_buf.resize(len);
						    raw_read_pos = 0;

						    srv.inhibit_read(true);
						    raw_read_enabled = true;

						    return chAT::command_status::OK;
					    }
					    case chAT::command_mode::Test: {
						    srv.write_response_prompt();
						    srv.write_cstr("<len>");
						    srv.write_line_end();
					    }
					    default:
						    return chAT::command_status::ERROR;
				    }
			    }});

	at_srv.add_command({"+RAW_SHOW", "Output the raw data read earlier",
			    [&](auto &srv, auto &parser) {
				    switch (parser.cmd_mode) {
					    case chAT::command_mode::Read:
					    case chAT::command_mode::Run:

						    srv.write_data(raw_read_buf.data(), raw_read_buf.size());

						    return chAT::command_status::OK;

					    default:
						    return chAT::command_status::ERROR;
				    }
			    }});

	at_srv.add_command({"+HELP", "Show this help",
			    [](auto &srv, auto &cmd) {
				    srv.write_command_list();
				    return chAT::command_status::OK;
			    }});

	at_srv.add_command({"+EXIT", "Exit this program",
			    [](auto &srv, auto &cmd) {
				    exit(0);
				    return chAT::command_status::OK;
			    }});

	at_srv.io = {
		.callback_io_read = [](auto buf, auto len){return read(STDIN_FILENO, buf, len);},
		.callback_io_write = [](auto buf, auto len){return write(STDOUT_FILENO, buf, len);},
	};

	set_nonblocking(STDIN_FILENO);
	set_nonblocking(STDOUT_FILENO);

	at_srv.nonblocking = true;

	puts("Starting AT server");

	pollfd pfds[2] = {
		{STDIN_FILENO, POLLIN, 0},
		{STDOUT_FILENO, 0, 0},
	};

	while (1) {
		if (poll(pfds, 2, 2000) > 0) {
			if (raw_read_enabled) {
				while (1) {
					if (raw_read_pos < raw_read_buf.size()) {
						ssize_t rc_read = read(STDIN_FILENO, raw_read_buf.data() + raw_read_pos, raw_read_buf.size() - raw_read_pos);
						if (rc_read > 0) {
							raw_read_pos += rc_read;
						} else if (rc_read == 0){
							break;
						} else {
							abort();
						}
					} else {
						raw_read_enabled = false;
						at_srv.inhibit_read(false);
						at_srv.write_cstr("READ OK\r\n");
						at_srv.run();
						break;
					}
				}
			} else {
				while (1) {
					auto rc = at_srv.run();

					printf("[run] result: %d\n", (int)rc);

					if (rc == chAT::server::run_status::WantRead || rc == chAT::server::run_status::Idle) {
						pfds[1].events = 0;
						break;
					} else if (rc == chAT::server::run_status::WantWrite) {
						pfds[1].events = POLLOUT;
						break;
					}
				}
			}
		}
	}

	return 0;
}

