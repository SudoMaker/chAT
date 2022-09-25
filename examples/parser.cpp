/*
    This file is part of cpp_at.
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

#include <cstdio>
#include <cstring>

#include <chAT.hpp>

using namespace SudoMaker;

int main() {
	chAT::at_parser p;

	auto test = [&](const char *s){
		printf("\n[test] Start\n\n");

		printf("[test] %s\n", s);

		printf("[test] Parsing with entire string\n");
		p.parse((uint8_t *)s, strlen(s));
		p.show();
		p.reset();

		printf("[test] Parsing with 1 char per time\n");
		for (size_t i=0; i< strlen(s); i++) {
			p.parse(reinterpret_cast<const uint8_t *>(&s[i]), 1);
		}
		p.show();
		p.reset();

		printf("\n[test] End\n\n");
	};

	test("AT\r\n");

	test("AT+FOO\r\n");
	test("AT+FOO?\r\n");
	test("AT+FOO=?\r\n");
	test("AT+FOO=1,7\r\n");
	test("AT+FO__OOOdfsbgsgswrfvgsw=1,\"asfasfasfa\",asfasfasf,22423322354,\r\n");
	test("AT+WEE=111111111111111111\r\n");
	test("AT+WEE2=\r\n");

	test("\xde\xad\xbe\xef\r\n");

	return 0;
}
