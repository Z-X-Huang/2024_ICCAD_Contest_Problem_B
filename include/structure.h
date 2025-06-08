#ifndef _STRUCTURE_H_
#define _STRUCTURE_H_

#include <string>
#include <vector>
#include <unordered_map>
#include <iomanip>
#include <iostream>

using namespace std;

class Inst;
class Pin;
class Die;

struct Point
{
	double x;
	double y;
};

struct Rect
{
	Point lb;
	Point rt;
};

struct FF_info
{
	string name;
	double h;
	double w;
	double q_pin_delay;
	double gate_power;
	int bits;
	int pin_count;
	vector<Pin> pin;
};

struct Gate_info
{
	string name;
	double h;
	double w;
	int pin_count;
	vector<Pin> pin;
};

struct Row
{
	Point start;
	double w;
	double h;
	int num;
};

struct FF_relation
{
	Pin* ff_con;
	Point front;
	Point end;
	double wl;
	FF_relation* copy;
};

class Pin
{
private:

public:
	Point p;
	Inst* belong;
	string name;
	vector<Pin*> connect;
	vector<FF_relation> ff_connect;
	vector<pair<string, string>> map;
	double slack;
	double critical;

	Pin() {
		p.x = 0;
		p.y = 0;
		belong = NULL;
		name = "";
		slack = 0.0;
		critical = 0.0;
	};
	Pin(int x, int y) {
		p.x = x;
		p.y = y;
		belong = NULL;
		name = "";
		slack = 0.0;
		critical = 0.0;
	};
	~Pin() {};

	Pin* copy();
};

class Inst
{
private:

public:
	string type;
	string name;
	Point lb;
	Point rt;
	unordered_map<string, Pin*> pin;
	bool pass;
	bool legal;

	Inst() {
		type = "";
		name = "";
		lb.x = 0;
		lb.y = 0;
		rt.x = 0;
		rt.y = 0;
		legal = false;
		//area.lb.x = 0;
		//area.lb.y = 0;
		//area.rt.x = 0;
		//area.rt.y = 0;
	};
	Inst(string t, string n, Point l, Point r) {
		type = t;
		name = n;
		lb = l;
		rt = r;
	};
	~Inst() {};

	Inst* copy();
	double HPWL(Point new_point, FF_info model);
	void update_position(Point new_point, FF_info ff, Die& die);
	void update_slack(Die& die);
};

class Bin {
private:

public:
	Rect rect;
	unordered_map<string, Inst*> ff;
	vector<Inst*> gate;
	//double cua; //can use area

	Bin() {
		rect.lb.x = 0;
		rect.lb.y = 0;
		rect.rt.x = 0;
		rect.rt.y = 0;
		//cua = 0;
	};
	Bin(int x1, int y1, int x2, int y2) {
		rect.lb.x = x1;
		rect.lb.y = y1;
		rect.rt.x = x2;
		rect.rt.y = y2;
		//cua = 0;
	};
	~Bin() {};

	double cal_area();
	Point refinement(Die& die, Inst* target);
};

class Die {
private:

public:
	Rect diesize;
	double weight_factor[4]; // alpha beta gamma lambda
	vector<vector<Bin>> chip;
	unordered_map<string, Pin*> input_pin;
	unordered_map<string, Pin*> output_pin;
	unordered_map<string, FF_info> ff_library;
	unordered_map<string, Gate_info> gate_library;
	//unordered_map<string, Inst*> ff_list_initial;
	unordered_map<string, Inst*> ff_list_inbin;
	unordered_map<string, Inst*> gate_list;
	double bin[3]; // width height maxutil
	vector<Row> row;
	double ddc; // displacement delay
	int bank_count;
	int debank_count;
	int output_index;

	Die() {
		diesize.lb.x = 0;
		diesize.lb.y = 0;
		diesize.rt.x = 0;
		diesize.rt.y = 0;
		bank_count = 0;
		debank_count = 0;
		output_index = 0;
	};
	Die(int x1, int y1, int x2, int y2) {
		diesize.lb.x = x1;
		diesize.lb.y = y1;
		diesize.rt.x = x2;
		diesize.rt.y = y2;
		bank_count = 0;
		debank_count = 0;
		output_index = 0;
	};
	~Die() {};

	double cost();
};

struct interval
{
	string ff;
	bool s_e;
	double x;
	double y;
};

#endif _STRUCTURE_H_

