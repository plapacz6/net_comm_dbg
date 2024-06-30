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

#include "TSvSt.h"
#include <cstdio>
#include <iostream>

using namespace std;

void TSvSt::Tstate::set(int d, int r, int a, int c, int de, int re, int ae, int ce) {
#define SETSTATE(X, V)  if(V == 2) {X = X;} else {X = V;};
	SETSTATE(def, d);
	SETSTATE(run, r);
	SETSTATE(acc, a);
	SETSTATE(con, c);
	SETSTATE(defe, de);
	SETSTATE(rune, re);
	SETSTATE(acce, ae);
	SETSTATE(cone, ce);
#undef SETSTATE 
}

bool TSvSt::Tstate::eq(int d, int r, int a, int c, int de, int re, int ae, int ce) const {
#define CHECKSTATE(X, V) ((V == 2) ? X : X == V)
	return (
		CHECKSTATE(def, d) &&
		CHECKSTATE(run, r) &&
		CHECKSTATE(acc, a) &&
		CHECKSTATE(con, c) &&
		CHECKSTATE(defe, de) &&
		CHECKSTATE(rune, re) &&
		CHECKSTATE(acce, ae) &&
		CHECKSTATE(cone, ce)
		);
#undef CHECKSTATE
}

const char* TSvSt::Tstate::sprintf_state(char *sprinf_buff, size_t buff_size) {
	if (buff_size < 19 * 8) {
		return "1st arg: sprintf_buff may be too small: about 160 bytes required";
	}
	const char* t[2] {
		"false/off",
		"true/on"
	};
#define SPRINTFSTATE(X) do { sprintf(sprinf_buff, "%s : %s\n", #X, t[X]); } while(0);
	SPRINTFSTATE(def);
	SPRINTFSTATE(run);
	SPRINTFSTATE(acc);
	SPRINTFSTATE(con);
	SPRINTFSTATE(defe);
	SPRINTFSTATE(rune);
	SPRINTFSTATE(acce);
	SPRINTFSTATE(cone);
	return sprinf_buff;
#undef SPRINTFSTATE
}

ostream& operator<<(ostream& out, TSvSt::Tstate& st) {
#define PRINTST(X) do { out << "[ " << #X << " : " << st.X << " ]"; } while(0);
	PRINTST(def);
	PRINTST(run);
	PRINTST(acc);
	PRINTST(con);
	PRINTST(defe);
	PRINTST(rune);
	PRINTST(acce);
	PRINTST(cone);
#undef PRINTST
	return out;
}

bool TSvSt::reached() const {
#define STEQ(X) (current.X == demand.X)
	if (
		STEQ(def) &&
		STEQ(run) &&
		STEQ(acc) &&
		STEQ(con) &&
		STEQ(defe) &&
		STEQ(rune) &&
		STEQ(acce) &&
		STEQ(cone)
		) {
		return true;
	}
	return false;
#undef STEQ
}

ostream& operator<<(ostream& out, TSvSt& st)
{
	out << "demand : " << st.demand << endl;
	out << "current: " << st.current << endl;
	return out;
}
