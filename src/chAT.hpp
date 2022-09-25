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

#pragma once

#include <any>
#include <tuple>
#include <variant>
#include <vector>
#include <deque>
#include <unordered_map>
#include <string>
#include <functional>
#include <memory>

#include <cstdio>
#include <cstring>
#include <cinttypes>

namespace SudoMaker::chAT {
	enum class command_mode : unsigned int {
		Run, Write, Read, Test
	};

	class at_parser {
	public:
		enum class state : unsigned int {
			Keyword, Command, Argument, Done, Malformed
		};

		bool malformed;
		state state;
		command_mode cmd_mode;
		size_t keyword_count;
		std::string command;
		std::vector<std::string> args;
		bool args_quote;
#ifdef CPPAT_STRICT_CRLF
		uint8_t last_data[2];
#endif
		size_t bytes_parsed;

		at_parser() {
			reset();
		}

		void reset();
		void show();
		size_t parse(const uint8_t *buf, size_t len);
	};

	struct io_interface {
		typedef std::function<ssize_t(uint8_t *, size_t)> io_callback_t;

		io_callback_t callback_io_read, callback_io_write;
	};

	template<size_t S>
	class input_buffer {
	protected:
		mutable uint8_t data_[S];
		size_t usage_ = 0, position_ = 0;

	public:
		void reset() noexcept {
			usage_ = position_ = 0;
		}

		void reset_if_done() noexcept {
			if (usage_ == position_) {
				reset();
			}
		}

		[[nodiscard]] uint8_t *data() const noexcept {
			return data_;
		}

		[[nodiscard]] size_t fresh_size() const noexcept {
			return usage_ - position_;
		}

		[[nodiscard]] uint8_t *fresh_begin() const noexcept {
			return data_ + position_;
		}

		[[nodiscard]] uint8_t *fresh_end() const noexcept {
			return data_ + usage_;
		}

		[[nodiscard]] size_t& usage() noexcept {
			return usage_;
		}

		[[nodiscard]] size_t& position() noexcept {
			return position_;
		}

		[[nodiscard]] size_t left() const noexcept {
			return S - usage_;
		}


	};

	struct data_holder {
		uint8_t *data;
		size_t position, size;

		std::unique_ptr<std::any> holder;

		data_holder() = default;

		template<typename T>
		explicit data_holder(T v) {
			load_container(std::move(v));
		}

		explicit data_holder(char *v, ssize_t len = -1) {
			load_cstring(v, len);
		}

		data_holder(void *buf, size_t len) {
			load_data(buf, len);
		}

		template<typename T>
		void load_container(T v) {
			holder = std::make_unique<std::any>(std::move(v));
			const T* o = std::any_cast<T>(holder.get());
			load_data((void *)o->data(), o->size());
		}

		void load_data(void *buf, size_t len) {
			data = static_cast<uint8_t *>(buf);
			size = len;
			position = 0;
		}

		void load_cstring(char *s, ssize_t len = -1) {
			load_data(s, len == -1 ? strlen(s) : len);
		}
	};

	enum class command_status {
		OK, ERROR, CUSTOM
	};

	class server;

	typedef std::function<command_status(server& svc, at_parser& parser)> command_callback_t;

	struct command {
		std::string name, description;
		command_callback_t callback;
	};

	class server {
	protected:
		input_buffer<128> buf_read;
		std::deque<data_holder> buf_write;

		bool read_inhibited = false;

		void do_parse();
	public:
		io_interface io;
		at_parser parser;
		std::unordered_map<std::string, command> commands;
		bool nonblocking = false;

		enum class run_status {
			Idle, Working, WantRead, WantWrite
		};

		void inhibit_read(bool enabled) noexcept {
			read_inhibited = enabled;
		}

		void write_raw(data_holder d) {
			buf_write.emplace_back(std::move(d));
		}

		void write_data(const void *buf, size_t len) {
			write_raw(data_holder((void *) buf, len));
		}

		void write_cstr(const char *buf, ssize_t len = -1) {
			write_data((void *) buf, len == -1 ? strlen(buf) : len);
		}

		template<typename T>
		void write(T&& s) {
			write_raw(data_holder(std::forward<T>(s)));
		}

		void write_error() {
			static const char str[] = "ERROR\r\n";
			write_cstr(str, sizeof(str) - 1);
		}

		void write_ok() {
			static const char str[] = "OK\r\n";
			write_cstr(str, sizeof(str) - 1);
		}

		void write_response_prompt() {
			write(parser.command + ": ");
		}

		void write_error_prompt() {
			static const char str[] = "+ERROR: ";
			write_cstr(str, sizeof(str) - 1);
		}

		void write_line_end() {
			static const char str[] = "\r\n";
			write_cstr(str, sizeof(str) - 1);
		}

		void write_command_list();

		void add_command(command cmd) {
			commands.insert({cmd.name, std::move(cmd)});
		}

		void add_commands(std::initializer_list<command> cmds) {
			for (auto &cmd: cmds) {
				commands.insert({cmd.name, cmd});
			}
		}

		run_status run();
	};
}