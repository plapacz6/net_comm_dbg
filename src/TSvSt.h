#ifndef TSVST_H
#define TSVST_H
#include <iostream>

class TSvSt {
public:
	struct Tstate {
		int def;
		int run;
		int acc;
		int con;

		int defe;  //def enable
		int rune;
		int acce;
		int cone;  //cone == 1 if con == 0

		/**
		* set state: 0 - false/off, 1 - true/on, 2 - don't change
		*/
		void set(int d, int r, int a, int c, int de, int re, int ae, int ce);
		/**
		* check if state is equal : 0 - false/off, 1 - true/on, 2 - any
		*/
		bool eq(int d, int r, int a, int c, int de, int re, int ae, int ce) const;
		const char* sprintf_state(char *sprinf_buff, size_t buff_size);
		friend std::ostream& operator<<(std::ostream& out, Tstate& st);
		
	} current, demand;

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