#include "TSvSt.h"
#include <cstdio>
#include <iostream>

using namespace std;

void TStSv::Tstate::set(int d, int r, int a, int c, int de, int re, int ae, int ce) {
#define SETSTATE(X, V)  do {V == 2 ? X = X : X = V} while(0);
	SETSTATE(def, d);
	SETSTATE(run, r);
	SETSTATE(acc, a);
	SETSTATE(con, c);
	SETSTATE(defe, de);
	SETSTATE(rune, re);
	SETSTATE(acce, ae);
	SETSTATE(cone, ce);
#undef SETSTATE(X, V) 
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
#undef CHECKSTATE(X, V)
}

char* TSvSt::Tstate::sprintf_state(char * sprinf_buff, size_t buff_size) {
	if (buff_size < 19 * 8) {
		return "1st arg: sprintf_buff may be too small: about 160 bytes required";
	}
	char* t[2]{
		"false/off",
		"true/on"
	};
#define SPRINTFSTATE(X) do { sprintf(sprint_buff, "%s : %s\n", #X, t[X]);  } while(0);
	SPRINTFSTATE(X);
	SPRINTFSTATE(X);
	SPRINTFSTATE(X);
	SPRINTFSTATE(X);
	SPRINTFSTATE(X);
	SPRINTFSTATE(X);
	SPRINTFSTATE(X);
	SPRINTFSTATE(X);
	return sprint_buff;
#undef SPRINTFSTATE(X)
}

ostream& operator<<(ostream& out, Tstate& st) {
#define PRINTST(X) do { out << "[ " << #X << " : " << st.X << " ]" } while(0);
	PRINTST(def);
	PRINTST(run);
	PRINTST(acc);
	PRINTST(con);
	PRINTST(defe);
	PRINTST(rune);
	PRINTST(acce);
	PRINTST(cone);
#undef PRINTST(X)
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
#undef STEQ(X) 
}

ostream& operator<<(ostream& out, TSvSt& st)
{
	out << "demand : " << st.demand << endl;
	out << "current: " << st.current << endl;
	return out;
}
