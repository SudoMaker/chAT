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

#include "chAT.hpp"

using namespace SudoMaker::chAT;

void at_parser::reset() {
	malformed = false;
	state = state::Keyword;
	cmd_mode = command_mode::Run;
	keyword_count = 0;
	command.clear();
	args.clear();
	args_quote = false;
#ifdef CPPAT_STRICT_CRLF
	last_data[0] = 0;
	last_data[1] = 0;
#endif
	bytes_parsed = 0;
}

void at_parser::show() {
	const char *mode_str = NULL;

	switch (cmd_mode) {
		case command_mode::Run:
			mode_str = "run";
			break;
		case command_mode::Write:
			mode_str = "write";
			break;
		case command_mode::Read:
			mode_str = "read";
			break;
		case command_mode::Test:
			mode_str = "test";
			break;
	}

	printf("[parser] malformed: %s, mode: %s, args_count: %zu\n", malformed ? "true" : "false", mode_str, args.size());

	if (!command.empty()) {
		printf("[parser] command: %s\n", command.c_str());
	}

	if (!args.empty()) {
		printf("[parser] args: ");

		for (size_t i=0; i<args.size(); i++) {
			printf("[%zu]=%s ", i, args[i].c_str());
		}

		printf("\n");

	}

}

size_t at_parser::parse(const uint8_t *buf, size_t len) {
	static const uint8_t keyword[2] = {'A', 'T'};

	for (size_t i = 0; i < len; i++) {
		uint8_t c = buf[i];

#ifdef CPPAT_STRICT_CRLF
		last_data[0] = last_data[1];
		last_data[1] = c;

		if (last_data[0] == '\r' && last_data[1] == '\n') {
#else
		if (c == '\n') {
#endif
			if (state == state::Malformed) {
				malformed = true;
			}
			state = state::Done;
			bytes_parsed += (i+1);
			return i+1;
		}

		switch (state) {
			case state::Keyword: {
				uint8_t kc = keyword[keyword_count];

				if (c == kc) {
					keyword_count++;
				} else {
					state = state::Malformed;
				}

				if (keyword_count == sizeof(keyword)) {
					state = state::Command;
				}

				break;
			}

			case state::Command: {
				switch (c) {
					case '?':
						if (cmd_mode == command_mode::Run) {
							cmd_mode = command_mode::Read;
						} else if (cmd_mode == command_mode::Write) {
							cmd_mode = command_mode::Test;
						} else {
							state = state::Malformed;
						}
						break;
					case '=':
						if (cmd_mode == command_mode::Run) {
							cmd_mode = command_mode::Write;
						} else {
							state = state::Malformed;
						}
						break;
					case '\r':
					case '\n':
						break;
					default:
						if (cmd_mode == command_mode::Run) {
							command.push_back((char) c);
						} else {
							args.emplace_back();
							state = state::Argument;
							i--;
						}
						break;
				}
				break;
			}

			case state::Argument: {
				switch (c) {
					case '"':
						args_quote = !args_quote;
						break;
					case ',':
						args.emplace_back();
						break;
					case '\r':
					case '\n':
						break;
					default:
						auto &cur_arg = args.back();
						cur_arg.push_back((char) c);
						break;
				}
				break;
			}

			default:
				break;
		}

	}

	bytes_parsed += len;
	return len;
}

void server::do_parse() {
	while (true) {
		auto ibuf_fresh_size = buf_read.fresh_size();

		if (ibuf_fresh_size) {
			size_t parsed_len = parser.parse(buf_read.fresh_begin(), ibuf_fresh_size);
			buf_read.position() += parsed_len;
			buf_read.reset_if_done();

			if (parser.state == at_parser::state::Done) {
				if (parser.malformed) {
					write_error();
				} else {
					if (parser.command.empty()) {
						write_ok();
					} else {
						auto it_c = commands.find(parser.command);

						if (it_c == commands.end()) {
							write_error();
						} else {
							auto &cmd = it_c->second;

							auto rc_cmd = cmd.callback(*this, parser);

							if (rc_cmd == command_status::OK) {
								write_ok();
							} else if (rc_cmd == command_status::ERROR) {
								write_error();
							}
						}
					}
				}

				parser.show();
				parser.reset();
			}
		} else {
			break;
		}
	}
}

server::run_status server::run() {
	// 0: nothing attempted, 1: success, 2: blocked
	int read_state = 0, write_state = 0;

	// Step 1: Read
	if (!read_inhibited) {
		while (true) {
			auto ibuf_left = buf_read.left();

			if (ibuf_left) {
				ssize_t rc_read = io.callback_io_read(buf_read.fresh_end(), ibuf_left);

				if (rc_read > 0) {
					read_state = 1;
					buf_read.usage() += rc_read;

					if (!nonblocking) {
						do_parse();
						break;
					}
				} else {
					if (read_state == 0) {
						read_state = 2;
					}
					break;
				}
			} else {
				break;
			}
		}
	}

	// Step 2: Parse
	do_parse();

	// Step 3: Write
	while (true) {
		if (!buf_write.empty()) {
			auto &last_data = buf_write.front();

			if (last_data.size) {
				ssize_t rc_write = io.callback_io_write(last_data.data + last_data.position, last_data.size - last_data.position);

				if (rc_write > 0) {
					write_state = 1;
					last_data.position += rc_write;
					if (last_data.position == last_data.size) {
						buf_write.pop_front();
					}
				} else {
					if (write_state == 0) {
						write_state = 2;
					}
					break;
				}
			} else {
				buf_write.pop_front();
			}
		} else {
			break;
		}
	}

	if (write_state == 2) {
		return run_status::WantWrite;
	} else if (read_state == 2) {
		return run_status::WantRead;
	}

	if (read_state == 0 && write_state == 0) {
		return run_status::Idle;
	} else {
		return run_status::Working;
	}
}

void server::write_command_list() {
	for (auto &it: commands) {
		static const char str[] = "+COMMAND: \"";
		write_cstr(str, sizeof(str) - 1);

		write(it.first);

		static const char str2[] = "\",\"";
		write_cstr(str2, sizeof(str2) - 1);

		write(it.second.description);

		static const char str3[] = "\"\r\n";
		write_cstr(str3, sizeof(str3) - 1);
	}

}

