/*
    This file is part of chAT.
    Copyright (C) 2022-2023 Reimu NotMoe <reimu@sudomaker.com>

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
#include <cassert>

#include <unistd.h>
#include <fcntl.h>

#include <chAT.hpp>

using namespace SudoMaker;

int main() {
	puts("Starting chAT server");

	chAT::Server at_srv;

	bool need_exit = false;

	std::vector<uint8_t> raw_read_buf;
	size_t raw_read_pos = 0;
	bool raw_read_enabled = false;

	std::unordered_map<std::string, std::function<chAT::CommandStatus(chAT::Server&, chAT::ATParser&)>> command_table = {
		{"+FOO",[](auto &srv, auto &parser) {
			static const char msg[] = "I'm a const c-style string. I'm not being copied.\r\n";
			srv.write_cstr(msg, sizeof(msg) - 1);

			std::string s = "I'm a std::string. I'm not being copied after construction.\r\n";
			srv.write_str(std::move(s));

			static const uint8_t msg2[] = "I'm a vector. I'm not being copied after construction.\r\n";
			std::vector<uint8_t> v;
			v.insert(v.end(), msg2, msg2+sizeof(msg2)-1);
			srv.write_vec8(std::move(v));

			return chAT::CommandStatus::OK;
		}},
		{"+RAWREAD",[&](auto &srv, auto &parser) {
			switch (parser.cmd_mode) {
				case chAT::CommandMode::Write: {
					if (parser.args.size() != 1) {
						return chAT::CommandStatus::ERROR;
					}

					auto &len_str = parser.args[0];
					if (len_str.empty()) {
						return chAT::CommandStatus::ERROR;

					}

					size_t len = std::stoi(len_str);

					raw_read_buf = srv.inhibit_read(len);
					raw_read_pos = raw_read_buf.size();

					raw_read_buf.resize(len);

					raw_read_enabled = true;

					return chAT::CommandStatus::CUSTOM;
				}
				case chAT::CommandMode::Test: {
					srv.write_response_prompt();
					srv.write_cstr("<len>");
					srv.write_line_end();
					return chAT::CommandStatus::OK;
				}
				default:
					return chAT::CommandStatus::ERROR;
			}
		}},
		{"+RAWSHOW",[&](auto &srv, auto &parser) {
			switch (parser.cmd_mode) {
				case chAT::CommandMode::Run:
					srv.write_vec8(std::move(raw_read_buf));
					return chAT::CommandStatus::OK;
				default:
					return chAT::CommandStatus::ERROR;
			}
		}},
		{"+EXIT",[&need_exit](auto &srv, auto &cmd) {
			need_exit = true;
			return chAT::CommandStatus::OK;
		}}
	};

	at_srv.set_io_callback({
				       .callback_io_read = [](auto buf, auto len){return read(STDIN_FILENO, buf, len);},
				       .callback_io_write = [](auto buf, auto len){return write(STDOUT_FILENO, buf, len);},
			       });

	at_srv.set_command_callback([&](chAT::Server& srv, const std::string& command){
		auto it = command_table.find(command);

		if (it == command_table.end()) {
			return chAT::CommandStatus::ERROR;
		} else {
			return it->second(srv, srv.parser());
		}
	});

	puts("READY");

	while (1) {
		if (raw_read_enabled) {
			puts("Raw read enabled");
			while (raw_read_pos < raw_read_buf.size()) {
				ssize_t rc_read = read(STDIN_FILENO, raw_read_buf.data() + raw_read_pos, raw_read_buf.size() - raw_read_pos);
				assert(rc_read > 0);

				raw_read_pos += rc_read;
			}
			raw_read_enabled = false;
			at_srv.continue_read();
			at_srv.write_ok();
			puts("Raw read done");
		}

		at_srv.run();

		if (need_exit) {
			exit(0);
		}
	}

	return 0;
}
