#ifndef _UPDATE_H_
#define _UPDATE_H_

#include "structure.h"

using namespace std;

//support
bool equal_point(Point a, Point b);
bool compare_row(Row a, Row b);
bool compare_point_x(Point a, Point b);
vector<int> range_inst(double width, double height, Rect rect);
int FindIndex(vector<Row>& row, double y);
int FindUp_cur(vector<Row>& row, double y);
int FindLow_cur(vector<Row>& row, double y);
int FindUp_fea(vector<Row>& row, double y);
int FindLow_fea(vector<Row>& row, double y);
bool sort_x(interval a, interval b);
bool sort_y(interval a, interval b);
double quality(Die& die, string name);

//initial
void construct(Die& die);
void recursive(Die& die, Pin* ff_pin, Pin* pin, FF_relation ff_rlt_pre, vector<Inst*>& next_ff);
void ff_connect(Die& die);
void sort_ff_library(Die& die, vector<vector<string>>& ff_sorted);
void save_ff(Die& die);
void read_ff(Die& die);

//bank
Inst* banking(Die& die, vector<Inst*> inst, string model);

//debank
void debanking(Die& die, Inst* inst, string model);

//INTEGRA
Point coordinate_transform_converse(Point p);
Point coordinate_transform(Point p);
Rect feasible_region(Die& die, Inst* inst, string model);
void transform(Inst* inst, Rect& fp, vector<Point>& fr);
void slack_cal(Die& die, vector <interval>& X, string model);
void max_clique(Die& die, vector<Inst*>& candidate, vector <interval>& Y, Inst* essential);
bool find_candidate(Die& die, vector<interval>& X, vector<Inst*>& to_be_bank, string model, vector<Point>& feasible_p);
bool INTEGRA(Die& die);

//legalization
void update_cur(Die& die, vector<vector<Point>>& cur, Inst* inst, double w, double h);
void cut_row(Die& die, vector<vector<Point>>& cur, string model);
Point best_point(Die& die, vector<vector<Point>>& fp, string model, int low, Inst* inst);
vector<vector<Point>> feasible_point(Die& die, vector<Point> fr, Rect rect, Inst* inst, vector<vector<Point>>& cur, int mode);
bool legalization(Die& die, vector<vector<string>>& ff_sorted);

//utilize
//void utilize(Die& die);



#endif _UPDATE_H_