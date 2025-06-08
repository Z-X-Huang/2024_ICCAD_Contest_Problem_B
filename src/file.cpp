#include <string>
#include <iostream>
#include <fstream>

using namespace std;

#include "structure.h"
#include "file.h"

void readfile(string filename, Die &die) 
{
	cout << "readfile " << filename << endl;

	ifstream file;
	file.open(filename);

	if (!file) {
		cerr << "無法打開文件" <<  filename << "或文件不存在" << endl;
		exit(1);
	}

	string s;
	int num = 0;

	//weight_factor
	for (int i = 0; i < 4; i++) {
		file >> s >> die.weight_factor[i];
		//cout << die.weight_factor[i] << endl;
	}

	//diesize
	file >> s >> die.diesize.lb.x >> die.diesize.lb.y >> die.diesize.rt.x >> die.diesize.rt.y;
	//cout << die.diesize.lb.x << " " << die.diesize.lb.y << " " << die.diesize.rt.x << " " << die.diesize.rt.y << endl;


	//input_pin
	file >> s >> num;
	for (int i = 0; i < num; i++) {
		Pin* temp_pin = new Pin();
		file >> s >> temp_pin->name >> temp_pin->p.x >> temp_pin->p.y;
		die.input_pin[temp_pin->name] = temp_pin;
		//cout << temp_pin->name << " " << temp_pin->p.x << " " << temp_pin->p.y << endl;
	}

	//output_pin
	file >> s >> num;
	for (int i = 0; i < num; i++) {
		Pin* temp_pin = new Pin();
		file >> s >> temp_pin->name >> temp_pin->p.x >> temp_pin->p.y;
		die.output_pin[temp_pin->name] = temp_pin;
		//cout << temp_pin->name << " " << temp_pin->p.x << " " << temp_pin->p.y << endl;
	}

	//ff_library 儲存可以用的FF
	file >> s;
	while (s == "FlipFlop") {
		FF_info temp_ff;
		file >> temp_ff.bits >> temp_ff.name >> temp_ff.w >> temp_ff.h >> temp_ff.pin_count;
		for (int i = 0; i < temp_ff.pin_count; i++) {
			Pin temp_pin;
			file >> s >> temp_pin.name >> temp_pin.p.x >> temp_pin.p.y;
			//cout << temp_pin.name << " " << temp_pin.p.x << " " << temp_pin.p.y << endl;
			//temp_pin.belong = temp_ff.name;
			temp_ff.pin.push_back(temp_pin);
		}
		die.ff_library[temp_ff.name] = temp_ff;
		//cout << temp_ff.bits << " " << temp_ff.name << " " << temp_ff.w << " ";
		//cout << temp_ff.h << " " << temp_ff.pin_count  << endl;
		file >> s;
	}

	//gate_library 儲存可以用的gate
	while (s == "Gate") {
		Gate_info temp_gate;
		file >> temp_gate.name >> temp_gate.w >> temp_gate.h >> temp_gate.pin_count;
		for (int i = 0; i < temp_gate.pin_count; i++) {
			Pin temp_pin;
			file >> s >> temp_pin.name >> temp_pin.p.x >> temp_pin.p.y;
			//cout << temp_pin.name << " " << temp_pin.p.x << " " << temp_pin.p.y << endl;
			//temp_pin.belong = temp_gate.name;
			temp_gate.pin.push_back(temp_pin);
		}
		die.gate_library[temp_gate.name] = temp_gate;
		//cout  << temp_gate.name << " " << temp_gate.w << " ";
		//cout << temp_gate.h << " " << temp_gate.pin_count  << endl;
		file >> s;
	}

	//Instance 用pointer儲存 分成2個vector儲存 FF 和 Gate
	file >> num;
	for (int i = 0; i < num; i++) {
		Inst* temp_inst = new Inst();
		file >> s >> temp_inst->name >> temp_inst->type >> temp_inst->lb.x >> temp_inst->lb.y;
		//cout << temp_inst->name << " " << temp_inst->type << " " << temp_inst->lb.x << " " << temp_inst->lb.y << endl;
		if (die.ff_library.count(temp_inst->type)) {
			FF_info curr_ff = die.ff_library[temp_inst->type];
			temp_inst->rt.x = temp_inst->lb.x + curr_ff.w;
			temp_inst->rt.y = temp_inst->lb.y + curr_ff.h;

			for (int i = 0;i < curr_ff.pin_count;i++) {
				Pin* pin = new Pin();
				pin->belong = temp_inst;
				pin->name = curr_ff.pin[i].name;
				pin->p.x = curr_ff.pin[i].p.x + temp_inst->lb.x;
				pin->p.y = curr_ff.pin[i].p.y + temp_inst->lb.y;
				pin->map.push_back(pair<string, string>(temp_inst->name, pin->name));
				temp_inst->pin[pin->name] = pin;
			}
			die.ff_list_inbin[temp_inst->name] = temp_inst;
		}
		else {
			Gate_info curr_gate = die.gate_library[temp_inst->type];
			temp_inst->rt.x = temp_inst->lb.x + curr_gate.w;
			temp_inst->rt.y = temp_inst->lb.y + curr_gate.h;

			for (int i = 0;i < curr_gate.pin_count;i++) {
				Pin* pin = new Pin();
				pin->belong = temp_inst;
				pin->name = curr_gate.pin[i].name;
				pin->p.x = curr_gate.pin[i].p.x + temp_inst->lb.x;
				pin->p.y = curr_gate.pin[i].p.y + temp_inst->lb.y;
				temp_inst->pin[pin->name] = pin;
			}
			die.gate_list[temp_inst->name] = temp_inst;
		}
	}

	//copy to inbin & map
	//cout << "copy to inbin & map" << endl;
	/*
	for (auto& n : die.ff_list_initial) {
		//cout << n.first << endl;
		Inst* newff = n.second->copy();
		//cout << newff->name << endl;
		die.ff_list_inbin[n.first] = newff;
		//cout << die.ff_list_inbin[n.first]->name << endl;
		for (auto& p : n.second->pin) {
			Pin* newpin = p.second->copy();
			newpin->belong = newff;
			pair<string, string> pin_name(n.first, p.first);
			newpin->map.push_back(pin_name);
			p.second->map.push_back(pin_name);
			newff->pin[p.first] = newpin;
		}
	}
	*/

	//netlist
	//cout << "netlist" << endl;
	file >> s >> num;
	for (int i = 0; i < num; i++) {
		int pin_count;
		file >> s >> s >> pin_count;
		//cout << "pin_count " << pin_count << endl;

		if (pin_count == 1) {
			file >> s >> s;
			continue;
		}
		Pin* out = NULL;
		file >> s >> s;
		//cout << s << endl;
		size_t start = 0;
		size_t end = s.find("/");
		if (end == std::string::npos) {
			out = die.input_pin[s];
		}
		else {
			string inst_name = s.substr(start, end - start);
			start = end + 1;
			string pin_name = s.substr(start);
			//cout << inst_name << " " << pin_name << endl;
			if (die.ff_list_inbin.count(inst_name)) {
				out = die.ff_list_inbin[inst_name]->pin[pin_name];
			}
			else {
				out = die.gate_list[inst_name]->pin[pin_name];
			}
		}

		for (int j = 1; j < pin_count; j++) {
			file >> s >> s;
			//cout << s << endl;
			size_t start = 0;
			size_t end = s.find("/");
			Pin* in = NULL;
			if (end == std::string::npos) {
				in = die.output_pin[s];
			}
			else {
				string inst_name = s.substr(start, end - start);
				start = end + 1;
				string pin_name = s.substr(start);
				//cout << inst_name << " " << pin_name << endl;
				if (die.ff_list_inbin.count(inst_name)) {
					in = die.ff_list_inbin[inst_name]->pin[pin_name];
				}
				else {
					in = die.gate_list[inst_name]->pin[pin_name];
				}
			}
			out->connect.push_back(in);
			in->connect.push_back(out);
		}
	}

	//bin
	for (int i = 0; i < 3; i++) {
		file >> s >> die.bin[i];
		//cout << die.bin[i] << endl;
	}

	//placementrow
	file >> s;
	while (s == "PlacementRows") {
		Row r;
		file >> r.start.x >> r.start.y >> r.w >> r.h >> r.num;
		//cout << r.start.x << " " << r.start.y << " " << r.w << " " << r.h << " " << r.num << endl;
		die.row.push_back(r);
		file >> s;
	}

	//slack
	file >> die.ddc >> s;
	//cout << die.ddc << " " << s << endl;
	while (s == "QpinDelay") {
		file >> s;
		file >> die.ff_library[s].q_pin_delay;
		file >> s;
	}
	while (s == "TimingSlack") {
		string temp;
		file >> temp >> s;
		//cout << temp << endl;
		file >> die.ff_list_inbin[temp]->pin[s]->slack >> s;
		//cout << die.ff_list_initial[temp]->slack << endl;
	}

	//gatepower
	while (s == "GatePower") {
		//cout << "nima" << endl;
		file >> s;
		//cout << s << endl;
		file >> die.ff_library[s].gate_power;
		//cout << die.ff_library[s].gate_power << endl;
		file >> s;
	}

	cout << "readfile complete" << endl;
	file.close();
}

void output(string filename, Die& die) {
	ofstream newFile;
	newFile.open(filename);
	newFile << fixed;

	int index = die.output_index;
	newFile << "CellInst " << die.ff_list_inbin.size() << endl;
	for (auto& n : die.ff_list_inbin) {
		n.second->name = "C" + to_string(index); //註解註解註解
		newFile << "Inst " << n.second->name << " " << n.second->type << " ";
		newFile << n.second->lb.x << " " << n.second->lb.y << endl;
		index++;
	}

	for (auto& n : die.ff_list_inbin) {
		for (auto& p : n.second->pin) {
			for (int i = 0; i < p.second->map.size(); i++) {
				newFile << p.second->map[i].first << "/" << p.second->map[i].second << " map ";
				newFile << n.second->name << "/" << p.first << endl;
			}
		}
	}
}

void matlab(Die& die)
{
	cout << "matlab" << endl;
	ofstream newFile;
	newFile.open("oldfat2.m");
	newFile << fixed;

	newFile << "axis equal;\n" << "hold on;\n" << "grid on;\n";
	//newFile << "block_x=[0 0 " << die.diesize.rt.x << " " << die.diesize.rt.x << " 0];" << endl;
	//newFile << "block_y=[0 " << die.diesize.rt.y << " " << die.diesize.rt.y << " 0 0];" << endl;
	//newFile << "fill(block_x, block_y, 'c');" << endl;

	for (int i = 0; i < die.chip.size(); i++)
	{
		for (int j = 0; j < die.chip[i].size(); j++) {
			newFile << "block_x=[" << die.chip[i][j].rect.lb.x << " " << die.chip[i][j].rect.lb.x << " ";
			newFile << die.chip[i][j].rect.rt.x << " " << die.chip[i][j].rect.rt.x << " " << die.chip[i][j].rect.lb.x << "];" << endl;
			newFile << "block_y=[" << die.chip[i][j].rect.lb.y << " " << die.chip[i][j].rect.rt.y << " ";
			newFile << die.chip[i][j].rect.rt.y << " " << die.chip[i][j].rect.lb.y << " " << die.chip[i][j].rect.lb.y << "];" << endl;
			newFile << "fill(block_x, block_y, 'y');" << endl;
		}
	}
		
	for (int i = 0; i < die.row.size(); i++)
	{
		Point s = die.row[i].start;
		Point e;
		e.x = s.x + die.row[i].w * die.row[i].num;
		e.y = s.y + die.row[i].h;

		newFile << "block_x=[" << s.x << " " << s.x << " ";
		newFile << e.x << " " << e.x << " " << s.x << "];" << endl;
		newFile << "block_y=[" << s.y << " " << e.y << " ";
		newFile << e.y << " " << s.y << " " << s.y << "];" << endl;
		newFile << "fill(block_x, block_y, 'w');" << endl;
	}

	/*for (int i = 0; i < die.row.size(); i++)
	{
		Point s = die.row[i].start;
		Point e;
		e.x = s.x + die.row[i].w;
		e.y = s.x + die.row[i].h;

		for (int j = 0; j < die.row[i].num; j++) {
			newFile << "block_x=[" << s.x << " " << s.x << " ";
			newFile << e.x << " " << e.x << " " << s.x << "];" << endl;
			newFile << "block_y=[" << s.y << " " << e.y << " ";
			newFile << e.y << " " << s.y << " " << s.y << "];" << endl;
			s = e;
			e.x += die.row[i].w;
			e.y += die.row[i].h;
			//newFile << "fill(block_x, block_y, 'y');" << endl;
		}
	}*/
}



