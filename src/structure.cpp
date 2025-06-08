#include <omp.h>
#include <algorithm>
#include <cfloat>

#include "structure.h"
using namespace std;


Inst* Inst::copy()
{
	Inst* new_inst = new Inst(type, name, lb, rt);
	return new_inst;
}

double Inst::HPWL(Point new_point, FF_info model)
{
	double hpwl = 0.0;

	for (int i = 0; i < model.pin.size(); i++) {
		string pin_name = model.pin[i].name;
		if (pin_name == "clk" || pin_name == "CLK") {
			continue;
		}
		Point p = model.pin[i].p;
		p.x += new_point.x;
		p.y += new_point.y;

		size_t in = pin_name.find("Q");
		if (in != std::string::npos) {
			for (int j = 0; j < pin[pin_name]->ff_connect.size(); j++) {
				if (pin[pin_name]->ff_connect[j].front.x == pin[pin_name]->ff_connect[j].ff_con->p.x
					&& pin[pin_name]->ff_connect[j].front.y == pin[pin_name]->ff_connect[j].ff_con->p.y) {
					hpwl += abs(p.x - pin[pin_name]->ff_connect[j].ff_con->p.x);
					hpwl += abs(p.y - pin[pin_name]->ff_connect[j].ff_con->p.y);
				}
				else {
					hpwl += abs(p.x - pin[pin_name]->ff_connect[j].front.x);
					hpwl += abs(p.y - pin[pin_name]->ff_connect[j].front.y);
					//hpwl += abs(pin[pin_name]->ff_connect[j].ff_con->p.x - pin[pin_name]->ff_connect[j].end.x);
					//hpwl += abs(pin[pin_name]->ff_connect[j].ff_con->p.y - pin[pin_name]->ff_connect[j].end.y);
					//hpwl += pin[pin_name]->ff_connect[j].wl;
				}
			}
		}
		else {
			for (int j = 0; j < pin[pin_name]->ff_connect.size(); j++) {
				if (pin[pin_name]->ff_connect[j].front.x == p.x
					&& pin[pin_name]->ff_connect[j].front.y == p.y) {
					hpwl += abs(p.x - pin[pin_name]->ff_connect[j].ff_con->p.x);
					hpwl += abs(p.y - pin[pin_name]->ff_connect[j].ff_con->p.y);
				}
				else {
					hpwl += abs(p.x - pin[pin_name]->ff_connect[j].end.x);
					hpwl += abs(p.y - pin[pin_name]->ff_connect[j].end.y);
					//hpwl += abs(pin[pin_name]->ff_connect[j].ff_con->p.x - pin[pin_name]->ff_connect[j].front.x);
					//hpwl += abs(pin[pin_name]->ff_connect[j].ff_con->p.y - pin[pin_name]->ff_connect[j].front.y);
					//hpwl += pin[pin_name]->ff_connect[j].wl;
				}
			}
		}
	}
	return hpwl;
}

void Inst::update_position(Point new_point, FF_info ff, Die& die)
{
	//cout << "update_position" << endl;
	lb = new_point;
	rt.x = lb.x + ff.w;
	rt.y = lb.y + ff.h;

	for (int i = 0; i < ff.pin.size(); i++) {
		string pin_name = ff.pin[i].name;
		Point np{ 0.0,0.0 };
		np.x = lb.x + ff.pin[i].p.x;
		np.y = lb.y + ff.pin[i].p.y;

		if (pin_name != "clk" && pin_name != "CLK") {
			Pin* temp = pin[pin_name];
			for (int j = 0; j < temp->ff_connect.size(); j++) {
				if (temp->ff_connect[j].front.x == temp->p.x && temp->ff_connect[j].front.y == temp->p.y) {
					temp->ff_connect[j].front = np;
				}
				else if (temp->ff_connect[j].end.x == temp->p.x && temp->ff_connect[j].end.y == temp->p.y) {
					temp->ff_connect[j].end = np;
				}
			}
		}

		pin[pin_name]->p = np;
	}
	update_slack(die);
}

void Inst::update_slack(Die& die)
{
	omp_set_num_threads(16);
	#pragma omp parallel for default(none) shared(die, inst) collapse(1)
	for (auto& n : pin) {
		size_t in;

		in = n.second->name.find("D");
		if (in != std::string::npos) {
			double front_q_pin = 0.0;
			double wire = 0.0;
			double critical = 0.0;
			double max_critical = 0.0;

			for (int i = 0; i < n.second->ff_connect.size(); i++) {
				if (n.second->ff_connect[i].ff_con->belong != NULL) { //前一層是FF
					front_q_pin = die.ff_library[n.second->ff_connect[i].ff_con->belong->type].q_pin_delay;
				}
				else { //前一層是input pin
					front_q_pin = 0.0;
				}
				wire = n.second->ff_connect[i].wl;
				if (n.second->ff_connect[i].front.x == n.second->p.x && n.second->ff_connect[i].front.y == n.second->p.y) {
					wire = wire + abs(n.second->ff_connect[i].front.x - n.second->ff_connect[i].end.x) + abs(n.second->ff_connect[i].front.y - n.second->ff_connect[i].end.y);
				}
				else {
					wire = wire + abs(n.second->ff_connect[i].end.x - n.second->p.x) + abs(n.second->ff_connect[i].end.y - n.second->p.y);
					wire = wire + abs(n.second->ff_connect[i].front.x - n.second->ff_connect[i].ff_con->p.x) + abs(n.second->ff_connect[i].front.y - n.second->ff_connect[i].ff_con->p.y);
				}
				critical = front_q_pin + wire * die.ddc;
				if (critical > max_critical) {
					max_critical = critical;
				}
			}
			//cout << "max critical: " << max_critical << endl;
			//cout << "old critical: " << n.second->critical << endl;
			if (max_critical > n.second->critical) {
				n.second->slack -= max_critical - n.second->critical;
				n.second->critical = max_critical;
			}
			else {
				//cout << n.second->critical - max_critical << endl;
				//cout << n.second->slack << endl;
				n.second->slack += n.second->critical - max_critical;
				n.second->critical = max_critical;
				//cout << n.second->slack << endl;
			}
		}

		in = n.second->name.find("Q");
		if (in != std::string::npos) {
			#pragma omp parallel for default(none) shared(die, n) collapse(1)
			for (int i = 0; i < n.second->ff_connect.size(); i++) {
				double front_q_pin = 0.0;
				double wire = 0.0;
				double critical = 0.0;
				double max_critical = 0.0;
				Pin* temp = NULL;
				temp = n.second->ff_connect[i].ff_con;
				//cout << "-------------------------" << endl;
				//cout << "old slack: " << temp->slack << endl;
				//cout << "old critical: " << temp->critical << endl;
				for (int j = 0; j < temp->ff_connect.size(); j++) {


					if (temp->ff_connect[j].ff_con->belong != NULL) { //前一層是FF
						front_q_pin = die.ff_library[temp->ff_connect[j].ff_con->belong->type].q_pin_delay;
					}
					else { //前一層是input pin
						front_q_pin = 0.0;
					}
					wire = temp->ff_connect[j].wl;
					if (temp->ff_connect[j].front.x == temp->p.x && temp->ff_connect[j].front.y == temp->p.y) {
						wire = wire + abs(temp->ff_connect[j].front.x - temp->ff_connect[j].end.x) + abs(temp->ff_connect[j].front.y - temp->ff_connect[j].end.y);
					}
					else {
						wire = wire + abs(temp->ff_connect[j].end.x - temp->p.x) + abs(temp->ff_connect[j].end.y - temp->p.y);
						wire = wire + abs(temp->ff_connect[j].front.x - temp->ff_connect[j].ff_con->p.x) + abs(temp->ff_connect[j].front.y - temp->ff_connect[j].ff_con->p.y);
					}
					critical = front_q_pin + wire * die.ddc;
					//cout << critical << endl;
					if (critical > max_critical) {
						max_critical = critical;
					}
				}
				if (max_critical > temp->critical) {
					temp->slack -= max_critical - temp->critical;
					temp->critical = max_critical;
				}
				else {
					temp->slack += temp->critical - max_critical;
					temp->critical = max_critical;
				}
				//cout << max_critical << endl;
				//cout << "new slack: " << temp->slack << endl;
				//cout << "new critical: " << temp->critical << endl;
				//cout << "-------------------------" << endl;
			}
		}
	}
}

Pin* Pin::copy()
{
	Pin* new_pin = new Pin();
	new_pin->connect = connect;
	new_pin->ff_connect = ff_connect;
	new_pin->critical = critical;
	new_pin->slack = slack;
	new_pin->map = map;

	return new_pin;
}

double Bin::cal_area()
{
	double bin_area, used_area, utility;

	bin_area = (rect.rt.x - rect.lb.x) * (rect.rt.y - rect.lb.y);
	used_area = 0;

	double inst_l, inst_r, inst_t, inst_b;
	for (auto& n : ff) {
		if (n.second->lb.x < rect.lb.x) {
			inst_l = rect.lb.x;
		}
		else {
			inst_l = n.second->lb.x;
		}

		if (n.second->lb.y < rect.lb.y) {
			inst_b = rect.lb.y;
		}
		else {
			inst_b = n.second->lb.y;
		}

		if (n.second->rt.x > rect.rt.x) {
			inst_r = rect.rt.x;
		}
		else {
			inst_r = n.second->rt.x;
		}

		if (n.second->rt.y > rect.rt.y) {
			inst_t = rect.rt.y;
		}
		else {
			inst_t = n.second->rt.y;
		}
		used_area += (inst_r - inst_l) * (inst_t - inst_b);
	}

	for (int i = 0;i < gate.size();i++) {
		if (gate[i]->lb.x < rect.lb.x) {
			inst_l = rect.lb.x;
		}
		else {
			inst_l = gate[i]->lb.x;
		}

		if (gate[i]->lb.y < rect.lb.y) {
			inst_b = rect.lb.y;
		}
		else {
			inst_b = gate[i]->lb.y;
		}

		if (gate[i]->rt.x > rect.rt.x) {
			inst_r = rect.rt.x;
		}
		else {
			inst_r = gate[i]->rt.x;
		}

		if (gate[i]->rt.y > rect.rt.y) {
			inst_t = rect.rt.y;
		}
		else {
			inst_t = gate[i]->rt.y;
		}
		used_area += (inst_r - inst_l) * (inst_t - inst_b);
	}
	return used_area;
}

double Die::cost() {
	double totalNegativeSlack = 0.0;
	double totalArea = 0.0;
	double totalPower = 0.0;
	double totalNumOfOverflowBin = 0.0;
	//cout << "****   " << die.ff_list_initial.size() << endl;
	//cout << die.ff_list_initial["reg4"]->pin.size() << endl;

	for (const auto& pair1 : ff_list_inbin) {
		for (const auto& pair2 : pair1.second->pin) {
			//cout << pair1.second->name << endl;
			//cout << pair2.second->slack << endl;
			if (pair2.second->slack < 0) {
				totalNegativeSlack += abs(pair2.second->slack);
			}
		}
		totalArea += ff_library[pair1.second->type].h * ff_library[pair1.second->type].w;
		totalPower += ff_library[pair1.second->type].gate_power;
	}
	double maxutil = (bin[0] * bin[1] * bin[2]) / 100;
	for (int i = 0; i < chip.size(); i++) {
		for (int j = 0; j < chip[i].size(); j++) {
			if (chip[i][j].cal_area() > maxutil) {
				totalNumOfOverflowBin++;
			}
		}
	}
	cout << setw(25) << setfill(' ') << left << "totalNegativeSlack:" << setw(20) << setfill(' ') << left << totalNegativeSlack << endl;
	cout << setw(25) << setfill(' ') << left << fixed << "totalArea:" << setw(20) << setfill(' ') << left << totalArea << endl;
	cout << setw(25) << setfill(' ') << left << "totalPower:" << setw(20) << setfill(' ') << left << totalPower << endl;
	cout << setw(25) << setfill(' ') << left << "totalNumOfOverflowBin:" << setw(20) << setfill(' ') << left << totalNumOfOverflowBin << endl;
	cout << setw(25) << setfill(' ') << left << fixed << "totalCost:" << setw(20) << setfill(' ') << left << weight_factor[0] * totalNegativeSlack + weight_factor[2] * totalArea + weight_factor[1] * totalPower + weight_factor[3] * totalNumOfOverflowBin << endl;
	//cout << "NMSL" << endl;
	return weight_factor[0] * totalNegativeSlack + weight_factor[1] * totalPower + weight_factor[2] * totalArea + weight_factor[3] * totalNumOfOverflowBin;
}

bool Compare(Bin a, Bin b) {
	return a.cal_area() < b.cal_area();
}

Point Bin::refinement(Die& die, Inst* target) {
	bool mode = 0;
	int count = 1;
	double remaining_area = 0.0;
	double maxutil = (die.bin[0] * die.bin[1] * die.bin[2]) / 100;
	vector<Bin> list;

	int i = (rect.lb.x - die.diesize.lb.x) / die.bin[0];
	int j = (rect.lb.y - die.diesize.lb.y) / die.bin[1];

	list.push_back(die.chip[i][j]);

	while (maxutil - list[0].cal_area() > 2000000) {
		list.clear();

		if (!mode) {
			if ((i - count) >= 0)
				list.push_back(die.chip[i - count][j]);
			if ((i + count) < die.chip[0].size())
				list.push_back(die.chip[i + count][j]);
			if ((j - count) > 0)
				list.push_back(die.chip[i][j - count]);
			if ((j + count) < die.chip.size())
				list.push_back(die.chip[i][j + count]);

			sort(list.begin(), list.end(), Compare);

			mode = !mode;
		}
		else {
			if ((i - count) >= 0 && (j - count) >= 0)
				list.push_back(die.chip[i - count][j - count]);
			if ((i - count) >= 0 && (j + count) < die.chip.size())
				list.push_back(die.chip[i - count][j + count]);
			if ((i + count) < die.chip[0].size() && (j - count) >= 0)
				list.push_back(die.chip[i + count][j - count]);
			if ((i + count) < die.chip[0].size() && (j + count) < die.chip.size())
				list.push_back(die.chip[i + count][j + count]);

			sort(list.begin(), list.end(), Compare);

			mode = !mode;
			count++;
		}

		if (list.empty()) {
			if (count >= die.chip.size() && count >= die.chip[0].size()) {
				cout << "fail" << endl;
				return Point{ -1,-1 };
			}
			else {
				list.push_back(die.chip[i][j]);
			}
		}
	}

	Point p = list[0].rect.lb;
	float min = FLT_MAX;
	for (auto& n : ff) {
		if ((abs(n.second->lb.x - p.x) + abs(n.second->lb.y - p.y)) < min) {
			min = abs(n.second->lb.x - p.x) + abs(n.second->lb.y - p.y);
			target = n.second;
		}
	}

	Point result;
	result.x = (list[0].rect.lb.x - die.diesize.lb.x) / die.bin[0];
	result.y = (list[0].rect.lb.y - die.diesize.lb.y) / die.bin[1];

	return result;
}
