/*
Copyright 2024-2024 plapacz6@gmail.com

This file is part of net_comm_dbg.

net_comm_dbg is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

net_comm_dbg is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
 with net_comm_dbg. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef TSVST_H
#define TSVST_H
#include <iostream>
#include <mutex>

class TSvSt {
public:
	struct Tstate {
		int def;
		int run;
		int acc;
		int con;

		/**
		* set state: 0 - false/off, 1 - true/on, 2 - don't change, 3 - not set
		*/
		void set(int d, int r, int a, int c);
		/**
		* check if state is equal : 0 - false/off, 1 - true/on, 2 - any, 3 - not set
		*/
		bool eq(int d, int r, int a, int c) const;
		const char* sprintf_state(char *sprinf_buff, size_t buff_size);
		friend std::ostream& operator<<(std::ostream& out, Tstate& st);
		
	} current, demand;
	static std::mutex mtx_curr;
	static std::mutex mtx_dem;

	struct Toptions {
		int show_POLL_monitor;
		int show_RD_data;
		int show_WD_data;
	} opt;

	bool reached() const;
	friend std::ostream& operator<<(std::ostream& out, TSvSt& st);
};

std::ostream& operator<<(std::ostream& out, TSvSt::Tstate& st);


std::ostream& operator<<(std::ostream& out, TSvSt& st);
#endif // TSVST_H