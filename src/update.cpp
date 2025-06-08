#include <iostream>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <omp.h>
#include <fstream>

#include "update.h"
#include "structure.h"
#include "file.h"

using namespace std;

bool equal_point(Point a, Point b) {
    if (a.x == b.x && a.y == b.y) {
        return true;
    }
    else {
        return false;
    }

}

bool compare_row(Row a, Row b) {
    return a.start.y < b.start.y;
}

bool compare_point_x(Point a, Point b) {
    return a.x < b.x;
}

vector<int> range_inst(double width, double height, Rect rect) 
{
    vector<int> result;
    result.push_back(floor(rect.lb.y / height));
    result.push_back(ceil(rect.rt.y / height));
    result.push_back(floor(rect.lb.x / width));
    result.push_back(ceil(rect.rt.x / width));

    return result;
}

int FindIndex(vector<Row>& row, double y)  // range = row[i].start.y ~ row[i+1].start.y
{
    //cout << "FindUp " << y << endl;
    double upbound = DBL_MAX;
    for (int i = row.size() - 1; i >= 0; i--) {
        //cout << i << endl;
        if (y < upbound && y >= row[i].start.y) {
            //cout << "find up " << i << endl;
            return i;
        }
        upbound = row[i].start.y;
    }
    return -1;
}

int FindUp_cur(vector<Row>& row, double y)  
{    
    //cout << "FindUp " << y << endl;
    if (y > row.back().start.y) {
        return row.size() - 1;
    }

    double upbound = row.back().start.y;
    for (int i = row.size() - 2; i >= 0; i--) {
        //cout << i << endl;
        if (y <= upbound && y > row[i].start.y) {
            //cout << "find up " << i << endl;
            return i;
        }
        upbound = row[i].start.y;
    }
    return -1;
}

int FindLow_cur(vector<Row>& row, double y)
{
    //cout << "FindUp " << y << endl;
    if (y < row[0].start.y) {
        return 0;
    }

    double lowbound = row[0].start.y;
    for (int i = 1; i < row.size(); i++) {
        //cout << i << endl;
        if (y >= lowbound && y < row[i].start.y) {
            //cout << "find up " << i << endl;
            return i;
        }
        lowbound = row[i].start.y;
    }
    return row.size();
}

int FindUp_fea(vector<Row>& row, double y)
{
    //cout << "FindUp " << y << endl;
    if (y >= row.back().start.y) {
        return row.size() - 1;
    }

    double upbound = row.back().start.y;
    for (int i = row.size() - 2; i >= 0; i--) {
        //cout << i << endl;
        if (y < upbound && y >= row[i].start.y) {
            //cout << "find up " << i << endl;
            return i;
        }
        upbound = row[i].start.y;
    }
    return -1;
}

int FindLow_fea(vector<Row>& row, double y)
{
    //cout << "FindUp " << y << endl;
    if (y <= row[0].start.y) {
        return 0;
    }

    double lowbound = row[0].start.y;
    for (int i = 1; i < row.size(); i++) {
        //cout << i << endl;
        if (y > lowbound && y <= row[i].start.y) {
            //cout << "find up " << i << endl;
            return i;
        }
        lowbound = row[i].start.y;
    }
    return -1;
}

bool sort_x(interval a, interval b) {
    if (a.x == b.x) {
        if (a.s_e && !b.s_e) {
            return false;
        }
        else {
            return true;
        }
    }
    return a.x < b.x;
}

bool sort_y(interval a, interval b) {
    if (a.y == b.y) {
        if (a.s_e && !b.s_e) {
            return false;
        }
        else {
            return true;
        }
    }
    return a.y < b.y;
}

double quality(Die& die, string name) {
    FF_info f = die.ff_library[name];
    double quality_factor = f.q_pin_delay * die.weight_factor[0]
        + f.gate_power * die.weight_factor[1]
        + f.h * f.w * die.weight_factor[2];
    return quality_factor;
}

void construct(Die& die)
{
    //output_index
    die.output_index = die.ff_list_inbin.size() + die.gate_list.size() + 1;
    
    //construct bin
    //cout << "construct bin" << endl;
    int num_col = ceil((die.diesize.rt.x - die.diesize.lb.x) / die.bin[0]);
    int num_row = ceil((die.diesize.rt.y - die.diesize.lb.y) / die.bin[1]);
    die.chip.resize(num_row);
    for (int i = 0; i < num_row; i++) {
        die.chip[i].resize(num_col);
    }

    for (int i = 0; i < num_row; i++) {
        for (int j = 0; j < num_col; j++) {
            die.chip[i][j].rect.lb.x = j * die.bin[0];
            die.chip[i][j].rect.lb.y = i * die.bin[1];
            die.chip[i][j].rect.rt.x = (j + 1) * die.bin[0];
            die.chip[i][j].rect.rt.y = (i + 1) * die.bin[1];
        }
    }

    //put into bin
    //cout << "put into bin" << endl;
    for (auto& n : die.ff_list_inbin) {
        Rect rect;
        rect.lb = n.second->lb;
        rect.rt = n.second->rt;

        /*
        if (rect.rt.x >= 1293600 || rect.rt.y >= 1293600) {
            cout << n.first << " " << rect.rt.x << " " << rect.rt.y << endl;
        }
        */

        vector<int> range = range_inst(die.bin[0], die.bin[1], rect);

        for (int i = range[0];i < range[1];i++) {
            for (int j = range[2];j < range[3];j++) {
                die.chip[i][j].ff[n.first] = n.second;
            }
        }
    }

    for (auto& n : die.gate_list) {
        Rect rect;
        rect.lb = n.second->lb;
        rect.rt = n.second->rt;

        /*
        if (rect.rt.x >= 1293600 || rect.rt.y >= 1293600) {
            cout << n.first << " " << rect.rt.x << " " << rect.rt.y << endl;
        }
        */

        vector<int> range = range_inst(die.bin[0], die.bin[1], rect);

        for (int i = range[0];i < range[1];i++) {
            for (int j = range[2];j < range[3];j++) {
                die.chip[i][j].gate.push_back(n.second);
            }
        }
    }
    sort(die.row.begin(), die.row.end(), compare_row);
}

void recursive(Die& die, Pin* ff_pin, Pin* pin, FF_relation ff_rlt_pre, vector<Inst*>& next_ff) {
    omp_set_num_threads(32);
#pragma omp parallel for default(none) shared(die, ff_pin, pin, ff_rlt) collapse(1)
    for (int i = 0; i < pin->connect.size(); i++) {
        FF_relation ff_rlt;
        ff_rlt = ff_rlt_pre;
        if (pin->connect[i]->name == "CLK" || pin->connect[i]->name == "clk") {
            return;
        }

        if (die.output_pin.count(pin->connect[i]->name) > 0) {
            //cout << pin->connect[i]->name << endl;
            /*if (ff_pin == pin) {
                ff_rlt.front = pin->connect[i]->p;
            }
            ff_rlt.ff_con = pin->connect[i];

            if (die.input_pin.count(pin->name)) {
                p_temp = pin->p;
            }
            else {
                p_temp = pin->belong->lb;
                p_temp.x = p_temp.x + pin->p.x;
                p_temp.y = p_temp.y + pin->p.y;
            }
            ff_rlt.end = p_temp;
            ff_pin->ff_connect.push_back(ff_rlt);
            ff_rlt.ff_con = ff_pin;
            pin->connect[i]->ff_connect.push_back(ff_rlt);
            */
        }
        else if (die.ff_library.count(pin->connect[i]->belong->type) > 0) {
            //cout << pin->connect[i]->belong->name << " " << pin->connect[i]->name << endl;
            if (ff_pin == pin) {
                ff_rlt.front = pin->connect[i]->p;
            }
            ff_rlt.ff_con = pin->connect[i];

            ff_rlt.end = pin->p;

            bool repeat = false;
#pragma omp parallel for default(none) shared(ff_pin, ff_rlt, repeat) collapse(1)
            for (int j = 0; j < ff_pin->ff_connect.size(); j++) {
                if (ff_pin->ff_connect[j].ff_con == ff_rlt.ff_con) {
                    if (equal_point(ff_pin->ff_connect[j].front, ff_rlt.front) && equal_point(ff_pin->ff_connect[j].end, ff_rlt.end)) {
                        repeat = true;
                        if (ff_rlt.wl > ff_pin->ff_connect[j].wl) {
                            ff_pin->ff_connect[j].wl = ff_rlt.wl;
                            //
                            ff_pin->ff_connect[j].copy->wl = ff_rlt.wl;
                        }
                        break;
                    }
                }
            }
            if (!repeat) {
                ff_pin->ff_connect.push_back(ff_rlt);
            }


            ff_rlt.ff_con = ff_pin;
            if (repeat) {
                /*for (int j = 0; j < pin->connect[i]->ff_connect.size(); j++) {
                    if (pin->connect[i]->ff_connect[j].ff_con == ff_rlt.ff_con) {
                        if (equal_point(pin->connect[i]->ff_connect[j].front, ff_rlt.front) && equal_point(pin->connect[i]->ff_connect[j].end, ff_rlt.end)) {
                            pin->connect[i]->ff_connect[j].wl = ff_rlt.wl;
                            break;
                        }
                    }
                }*/

            }
            else {
                pin->connect[i]->ff_connect.push_back(ff_rlt);
                ff_pin->ff_connect.back().copy = &pin->connect[i]->ff_connect.back();
            }

            double wire = ff_rlt.wl;
            if (ff_pin == pin) {
                wire = wire + abs(ff_rlt.front.x - ff_rlt.end.x) + abs(ff_rlt.front.y - ff_rlt.end.y);
            }
            else {
                wire = wire + abs(ff_rlt.end.x - pin->connect[i]->p.x) + abs(ff_rlt.end.y - pin->connect[i]->p.y);
                wire = wire + abs(ff_pin->p.x - ff_rlt.front.x) + abs(ff_pin->p.y - ff_rlt.front.y);
            }

            double critical = wire * die.ddc;
            if (die.input_pin.count(ff_pin->name) == 0) {
                critical = critical + die.ff_library[ff_pin->belong->type].q_pin_delay;
            }

            if (critical > pin->connect[i]->critical) {
                pin->connect[i]->critical = critical;
            }

            if (!repeat) {
                if (!pin->connect[i]->belong->pass) {
                    next_ff.push_back(pin->connect[i]->belong);
                    pin->connect[i]->belong->pass = true;
                }
            }
        }
        else {
            //cout << pin->connect[i]->belong->name << " " << pin->connect[i]->name << endl;
            if (ff_pin == pin) {
                ff_rlt.front = pin->connect[i]->p;
            }
            else {
                Point p_temp_1 = pin->p;
                Point p_temp_2 = pin->connect[i]->p;
                double hpwl = abs(p_temp_1.x - p_temp_2.x) + abs(p_temp_1.y - p_temp_2.y);
                ff_rlt.wl = ff_rlt.wl + hpwl;
            }

            for (auto& n : pin->connect[i]->belong->pin) {
                size_t in = n.second->name.find("OUT");
                if (in != std::string::npos) {
                    recursive(die, ff_pin, n.second, ff_rlt, next_ff);
                }
            }
        }
    }
}

void ff_connect(Die& die) {
    vector<Inst*> cur_ff;
    vector<Inst*> next_ff;

    for (auto& n : die.input_pin) {
        //cout << n.second->name << endl;
        FF_relation ff_rlt;
        ff_rlt.wl = 0;
        recursive(die, n.second, n.second, ff_rlt, next_ff);
    }

    clock_t start = clock();
    while (next_ff.size() != 0) {
        cur_ff.assign(next_ff.begin(), next_ff.end());
        next_ff.clear();
        clock_t end = clock();
        double time = double(end - start) / double(CLOCKS_PER_SEC);
        cout << "-----ff----- " << time << "s" << endl;
        int size = cur_ff.size();
        for (int i = 0; i < size; i++) {
            for (auto& n : cur_ff[i]->pin) {
                size_t in = n.second->name.find("Q");
                if (in != std::string::npos) {
                    //cout << cur_ff[i]->name << " " << n.second->name << endl;
                    FF_relation ff_rlt;
                    ff_rlt.wl = 0;
                    recursive(die, n.second, n.second, ff_rlt, next_ff);
                }
            }
        }
    }
}

void save_ff(Die& die) {
    ofstream newFile;
    newFile.open("ff_connect3.txt");
    for (auto& n : die.ff_list_inbin) {
        newFile << "inst " << n.second->name << endl;
        for (auto& k : n.second->pin) {
            if (k.second->name == "clk" || k.second->name == "CLK") {
                continue;
            }
            newFile << k.second->name << " " << k.second->ff_connect.size() << endl;
            for (int i = 0; i < k.second->ff_connect.size(); i++) {
                newFile << fixed << setprecision(0);
                FF_relation rlt = k.second->ff_connect[i];
                if (rlt.ff_con->belong != NULL) {
                    newFile << rlt.ff_con->belong->name << " " << rlt.ff_con->name << " ";
                }
                else {
                    newFile << "input " << rlt.ff_con->name << " ";
                }
                newFile << rlt.front.x << " " << rlt.front.y << " " << rlt.end.x << " " << rlt.end.y;
                newFile << " " << rlt.wl << endl;
            }
            newFile << fixed << setprecision(6);
            newFile << k.second->critical << endl;
        }
    }
    newFile.close();
}

void read_ff(Die& die) {
    ifstream file;
    file.open("ff_connect1.txt");

    string name, str;
    int len;

    while (file >> str >> name) {
        Inst* cur = die.ff_list_inbin[name];
        for (int t = 0; t < 2; t++) {
            file >> name >> len;
            FF_relation rlt;
            Pin* pin = cur->pin[name];
            for (int i = 0; i < len; i++) {
                file >> str >> name;
                file >> rlt.front.x >> rlt.front.y >> rlt.end.x >> rlt.end.y;
                file >> rlt.wl;
                if (str == "input") {
                    rlt.ff_con = die.input_pin[name];
                }
                else {
                    rlt.ff_con = die.ff_list_inbin[str]->pin[name];
                }
                pin->ff_connect.push_back(rlt);
            }
            file >> pin->critical;
        }
    }
    file.close();
}

void sort_ff_library(Die& die, vector<vector<string>>& ff_sorted) {
    for (auto &n : die.ff_library) {
        int bit_num = n.second.bits;
        if (bit_num >= ff_sorted.size()) {
            ff_sorted.resize(bit_num + 1);
        }
        ff_sorted[bit_num].push_back(n.second.name);
    }

    for (int i = 1; i < ff_sorted.size();i++) {
        //cout << i << endl;
        if (ff_sorted[i].size() != 0) {
            for (int j = 1;j < ff_sorted[i].size();j++) {
                //cout << j << endl;
                string key = ff_sorted[i][j];
                int a = j - 1;
                double key_score = quality(die, key);
                double a_score = quality(die, ff_sorted[i][a]);
                while (key_score < a_score && a >= 0) {
                    //cout << a << endl;
                    ff_sorted[i][a + 1] = ff_sorted[i][a];
                    a--;
                    if (a >= 0) {
                        a_score = quality(die, ff_sorted[i][a]);
                    }
                }
                ff_sorted[i][a + 1] = key;
            }
        }
    }
}

Inst* banking(Die& die, vector<Inst*> v, string model)
{
    //pin的位置在update position時會更新
    Inst* newinst = new Inst();
    newinst->name = "bank" + to_string(die.bank_count);
    die.bank_count++;
    newinst->type = model;
    
    //update pin info
    int dcount = 0;
    int qcount = 0;
    string pin_name = "";
    FF_info ff = die.ff_library[model];
    Pin* curr_pin = NULL;
    for (int i = 0; i < ff.pin.size(); i++) {
        //cout << ff.pin[i].name << endl;
        pin_name = ff.pin[i].name;
        if (pin_name[0] == 'D') {
            if (v[dcount]->pin.count("D")) {
                curr_pin = v[dcount]->pin["D"];
            }
            else {
                curr_pin = v[dcount]->pin["D0"];
            }
            curr_pin->name = pin_name;
            curr_pin->belong = newinst;
            newinst->pin[pin_name] = curr_pin;
            dcount++;
        }
        else if (pin_name[0] == 'Q') {
            if (v[qcount]->pin.count("Q")) {
                curr_pin = v[qcount]->pin["Q"];
            }
            else {
                curr_pin = v[qcount]->pin["Q0"];
            }
            curr_pin->name = pin_name;
            curr_pin->belong = newinst;
            newinst->pin[pin_name] = curr_pin;
            qcount++;
        }
        else {
            curr_pin = new Pin();
            curr_pin->belong = newinst;
            curr_pin->name = pin_name;
            
            for (int j = 0; j < v.size(); j++) {
                string t;
                if (v[j]->pin.count("clk")) {
                    t = "clk";
                }
                else {
                    t = "CLK";
                }
                if (j == 0) {
                    for (int k = 0; k < v[j]->pin[t]->connect.size(); k++) {
                        bool over = false;
                        for (int l = 0; l < v.size(); l++) {
                            if (v[j]->pin[t]->connect[k]->belong!=NULL && v[j]->pin[t]->connect[k]->belong->name == v[l]->name) {
                                over = true;
                                break;
                            }
                        }
                        if (!over) {
                            curr_pin->connect.push_back(v[j]->pin[t]->connect[k]);
                        }
                    }
                }
                curr_pin->map.push_back(v[j]->pin[t]->map[0]);
                //curr_pin->connect.insert(curr_pin->connect.end(), v[j]->pin[t]->connect.begin(), v[j]->pin[t]->connect.end());
                //curr_pin->ff_connect.insert(curr_pin->ff_connect.end(), v[j]->pin[t]->ff_connect.begin(), v[j]->pin[t]->ff_connect.end());
            }
            newinst->pin[pin_name] = curr_pin;
        }
    }
  
    for (int i = 0; i < v.size(); i++) {
        die.ff_list_inbin.erase(v[i]->name);
    }
    
    return newinst;
}

void debanking(Die& die, Inst* inst, string model)
{
    FF_info ff = die.ff_library[model];
    string new_name[3];
    string name[2] = { "D","Q" };
    for (int i = 0; i < ff.pin_count ; i++) {
        if (ff.pin[i].name == "D" || ff.pin[i].name == "D0") {
            new_name[0] = ff.pin[i].name;
        }
        else if (ff.pin[i].name == "Q" || ff.pin[i].name == "Q0") {
            new_name[1] = ff.pin[i].name;
        }
        else if (ff.pin[i].name == "clk" || ff.pin[i].name == "CLK") {
            new_name[2] = ff.pin[i].name;
        }
    }

    vector<Pin*> clk;
    for (int i = 0; i < (inst->pin.size() - 1) / 2; i++) {
        //cout << i << endl;
        Inst* newinst = new Inst();
        newinst->name = "debank" + to_string(die.debank_count);
        die.debank_count++;
        newinst->type = model;
        
        // Pin D & Q
        //cout << "Pin D & Q" << endl;
        for (int j = 0; j < 2; j++) {
            string pin_name = name[j] + to_string(i);;
            newinst->pin[new_name[j]] = inst->pin[pin_name];
            newinst->pin[new_name[j]]->name = name[j];
            newinst->pin[new_name[j]]->belong = newinst;
        }

        //Pin CLK
        //cout << "Pin CLK" << endl;
        string pin_name;
        if (inst->pin.count("CLK")) {
            pin_name = "CLK";
        }
        else {
            pin_name = "clk";
        }
        newinst->pin[new_name[2]] = inst->pin[pin_name]->copy();
        newinst->pin[new_name[2]]->name = new_name[2];
        newinst->pin[new_name[2]]->belong = newinst;
        clk.push_back(newinst->pin[new_name[2]]);

        //update_position & put in bin
        //cout << "update_position & put in bin" << endl;
        newinst->update_position(inst->lb, ff, die);
        die.ff_list_inbin[newinst->name] = newinst;
        Rect rect;
        rect.lb = newinst->lb;
        rect.rt = newinst->rt;
        vector<int> range = range_inst(die.bin[0], die.bin[1], rect);

        for (int a = range[0]; a < range[1]; a++) {
            for (int b = range[2]; b < range[3]; b++) {
                die.chip[a][b].ff[newinst->name] = newinst;
            }
        }
    }

    for (int i = 0; i < (inst->pin.size() - 1) / 2; i++) {
        for (int j = 0; j < (inst->pin.size() - 1) / 2;j++) {
            if (i != j) {
                clk[i]->connect.push_back(clk[j]);
            }
        }
    }

    die.ff_list_inbin.erase(inst->name);

    /*
    Rect rect;
    rect.lb = inst->lb;
    rect.rt = inst->rt;
    vector<int> range = range_inst(die.bin[0], die.bin[1], rect);

    //remove inst from bin
    for (int a = range[0]; a < range[1]; a++) {
        for (int b = range[2]; b < range[3]; b++) {
            die.chip[a][b].ff.erase(inst->name);
        }
    }
    die.ff_list_inbin.erase(inst->name);
    */
}

Point coordinate_transform_converse(Point p) {
    Point p_;
    p_.x = (p.x - p.y) / 2;
    p_.y = (p.x + p.y) / 2;
    return p_;
}

Point coordinate_transform(Point p) {
    Point p_;
    p_.x = p.x + p.y;
    p_.y = p.y - p.x;
    return p_;
}

Rect feasible_region(Die& die, Inst* inst, string model)
{
    Pin* q_pin = NULL, * d_pin = NULL;
    for (auto& k : inst->pin) {
        size_t in1 = k.second->name.find("Q");
        if (in1 != std::string::npos) {
            q_pin = k.second;
        }
        size_t in2 = k.second->name.find("D");
        if (in2 != std::string::npos) {
            d_pin = k.second;
        }
    }
    //if (inst->name == "C70907" || inst->name == "C70910" || inst->name == "C70896" || inst->name == "C70911") {
       // cout << inst->name << endl;
    //}
    Rect fp;
    fp.lb.x = die.diesize.lb.x + die.diesize.lb.y;
    fp.rt.x = die.diesize.rt.x + die.diesize.rt.y;
    fp.lb.y = die.diesize.lb.y - die.diesize.rt.x;
    fp.rt.y = die.diesize.rt.y - die.diesize.lb.x;
    const double Max = numeric_limits<double>::max();

    if (q_pin->ff_connect.size() == 0) {
        Point p = d_pin->connect[0]->p;
        double wire = abs(p.x - d_pin->p.x) + abs(p.y - d_pin->p.y);
        wire = wire - die.ff_library[model].h - die.ff_library[model].w;
        p.x = p.x - d_pin->p.x + inst->lb.x;
        p.y = p.y - d_pin->p.y + inst->lb.y;
        p = coordinate_transform(p);
        double length = d_pin->slack / die.ddc + wire;
        if (length < 0) {
            length = 0;//
        }
        if (p.x - length > fp.lb.x) {
            fp.lb.x = p.x - length;
        }
        if (p.y - length > fp.lb.y) {
            fp.lb.y = p.y - length;
        }
        if (p.x + length < fp.rt.x) {
            fp.rt.x = p.x + length;
        }
        if (p.y + length < fp.rt.y) {
            fp.rt.y = p.y + length;
        }
    }
    else {
        vector<double> min_slack;
        int size = q_pin->connect.size();
        min_slack.assign(size + 1, Max);

        for (int i = 0; i < q_pin->ff_connect.size(); i++) {
            FF_relation ff_rlt = q_pin->ff_connect[i];

            /*double wire = ff_rlt.wl;
            if (equal_point(ff_rlt.end, q_pin->p)) {
                wire = wire + abs(ff_rlt.front.x - ff_rlt.end.x) + abs(ff_rlt.front.y - ff_rlt.end.y);
            }
            else {
                wire = wire + abs(ff_rlt.end.x - ff_rlt.ff_con->p.x) + abs(ff_rlt.end.y - ff_rlt.ff_con->p.y);
                wire = wire + abs(q_pin->p.x - ff_rlt.front.x) + abs(q_pin->p.y - ff_rlt.front.y);
            }
            double slack = ff_rlt.ff_con->slack + ff_rlt.ff_con->critical - (die.ff_library[q_pin->belong->name].q_pin_delay + wire * die.ddc);
            slack = slack + (abs(q_pin->p.x - ff_rlt.front.x) + abs(q_pin->p.y - ff_rlt.front.y) )* die.ddc; //-Q offset
            */
            double wire = ff_rlt.wl + die.ff_library[model].h + die.ff_library[model].w;
            if (!equal_point(ff_rlt.end, q_pin->p)) {
                wire = wire + abs(ff_rlt.end.x - ff_rlt.ff_con->p.x) + abs(ff_rlt.end.y - ff_rlt.ff_con->p.y);
            }
            double slack = ff_rlt.ff_con->slack + ff_rlt.ff_con->critical - wire * die.ddc - die.ff_library[model].q_pin_delay;
            //if (slack < 0) {
            //    cout << "<0" << endl;
            //}

            for (int j = 0; j < size; j++) {
                if (equal_point(q_pin->connect[j]->p, ff_rlt.front)) {
                    if (min_slack[j] > slack) {
                        min_slack[j] = slack;
                    }
                    break;
                }
            }
        }
        min_slack[size] = d_pin->slack;

        vector<Rect> diamond;
        diamond.resize(size + 1);
        for (int i = 0; i < size; i++) {
            Point p = q_pin->connect[i]->p;
            p.x = p.x - q_pin->p.x + inst->lb.x;
            p.y = p.y - q_pin->p.y + inst->lb.y;
            p = coordinate_transform(p);
            double length = min_slack[i] / die.ddc;
            if (length < 0) {
                length = 0;//
            }
            diamond[i].lb.x = p.x - length;
            diamond[i].lb.y = p.y - length;
            diamond[i].rt.x = p.x + length;
            diamond[i].rt.y = p.y + length;
        }
        Point p = d_pin->connect[0]->p;
        double wire = abs(p.x - d_pin->p.x) + abs(p.y - d_pin->p.y);
        wire = wire - die.ff_library[model].h - die.ff_library[model].w;
        p.x = p.x - d_pin->p.x + inst->lb.x;
        p.y = p.y - d_pin->p.y + inst->lb.y;
        p = coordinate_transform(p);
        double length = min_slack[size] / die.ddc + wire;
        if (length < 0) {
            length = 0;//
        }
        diamond[size].lb.x = p.x - length;
        diamond[size].lb.y = p.y - length;
        diamond[size].rt.x = p.x + length;
        diamond[size].rt.y = p.y + length;

        for (int i = 0; i <= size; i++) {
            if (diamond[i].lb.x > fp.lb.x) {
                fp.lb.x = diamond[i].lb.x;
            }
            if (diamond[i].lb.y > fp.lb.y) {
                fp.lb.y = diamond[i].lb.y;
            }
            if (diamond[i].rt.x < fp.rt.x) {
                fp.rt.x = diamond[i].rt.x;
            }
            if (diamond[i].rt.y < fp.rt.y) {
                fp.rt.y = diamond[i].rt.y;
            }
        }
        //if (fp.lb.x == fp.rt.x && fp.lb.y == fp.rt.y) {
        //    cout << "lne=0" << endl;
        //}
    }

    return fp;
}

void transform(Inst* inst, Rect& fp, vector<Point>& fr) {
    Point p, p_;
    if (fp.lb.x > fp.rt.x || fp.lb.y > fp.rt.y) {
        double w = inst->rt.x - inst->lb.x;
        double h = inst->rt.y - inst->lb.y;
        p = inst->lb;
        p_ = p;
        p_.y = p_.y - h;
        fr.push_back(p_);
        p_ = p;
        p_.x = p_.x - w;
        fr.push_back(p_);
        p_ = p;
        p_.y = p_.y + h;
        fr.push_back(p_);
        p_ = p;
        p_.x = p_.x + w;
        fr.push_back(p_);
    }
    else {
        p = fp.lb;
        p_ = coordinate_transform_converse(p);
        fr.push_back(p_);
        p.y = fp.rt.y;
        p_ = coordinate_transform_converse(p);
        fr.push_back(p_);
        p.x = fp.rt.x;
        p_ = coordinate_transform_converse(p);
        fr.push_back(p_);
        p.y = fp.lb.y;
        p_ = coordinate_transform_converse(p);
        fr.push_back(p_);
    }
}

void slack_cal(Die& die, vector <interval>& X, string model) { // cal feasible region
    for (auto& n : die.ff_list_inbin) {
        if (n.second->pin.size() > 3) {
            continue;
        }
        Rect rect;

        rect = feasible_region(die, n.second, model);

        if (rect.lb.x > rect.rt.x || rect.lb.y > rect.rt.y) {
            continue;
        }

        interval start, end;
        start.ff = n.second->name;
        start.s_e = false;
        start.x = rect.lb.x;
        start.y = rect.lb.y;
        //n.second->start = &start;

        end.ff = n.second->name;
        end.s_e = true;
        end.x = rect.rt.x;
        end.y = rect.rt.x;
        //n.second->end = &end;

        X.push_back(start);
        X.push_back(end);
    }
    sort(X.begin(), X.end(), sort_x);
}

void max_clique(Die& die, vector<Inst*>& candidate, vector <interval>& Y, Inst* essential) {
    vector<Inst*> temp;
    bool check = false;
    for (int i = 0; i < Y.size(); i++) {
        if (!Y[i].s_e) {
            temp.push_back(die.ff_list_inbin[Y[i].ff]);
            if (Y[i].ff == essential->name) {
                check = true;
                candidate.assign(temp.begin(), temp.end());
            }
            if (check && temp.size() > candidate.size()) {
                candidate.assign(temp.begin(), temp.end());
            }
        }
        else {
            for (int j = 0; j < temp.size(); j++) {
                if (temp[j]->name == Y[i].ff) {
                    temp.erase(temp.begin() + j);
                    break;
                }
            }
            if (Y[i].ff == essential->name) {
                check = false;
                break;
            }
        }
    }
}

bool find_candidate(Die& die, vector<interval>& X, vector<Inst*>& to_be_bank, string model, vector<Point>& feasible_p) {
    double xl, xr, yb, yt;
    vector<Inst*> essential;
    vector<Inst*> candidate;
    //cout << essential.size() << " " << candidate.size() << endl;
    /*if (check) {
        ofstream newFile;
        newFile.open("X.txt");
        for (int t = 0; t < X.size(); t++) {
            newFile << X[t].ff << " " << X[t].s_e << " " << X[t].x << X[t].y << endl;
        }
        newFile.close();
    }*/
    vector<interval> Y;
    bool decision_p = false;
    int start = 0;
    for (int i = 0; i <= X.size(); i++) {
        if (i != X.size() && X[i].s_e) {
            decision_p = true;
            essential.push_back(die.ff_list_inbin[X[i].ff]);
        }
        if ((i == X.size() || !X[i].s_e) && decision_p) {
            int front = Y.size();
            Y.insert(Y.end(), X.begin() + start, X.begin() + i - essential.size());
            //X.erase(X.begin(), X.begin() + i);
            int back = Y.size();
            for (int j = front; j < back; j++) {
                string targetName = Y[j].ff;
                auto it = std::find_if(X.begin(), X.end(), [&targetName](const interval& temp) {return temp.ff == targetName && temp.s_e; });

                if (it != X.end()) {
                    Y.push_back(*it);
                    //X.erase(it);
                }
            }

            /*if (check) {
                cout << "aaa" << endl;
                for (int t = front; t < Y.size(); t++) {
                    //cout << Y[t].ff << " " << Y[t].s_e << " " << Y[t].x << endl;
                }
                cout << "aaa" << endl;
            }*/
            sort(Y.begin(), Y.end(), sort_y);

            for (int j = 0; j < essential.size(); j++) {
                //if (check) {
                    //cout << essential[j]->name << endl;
                //}
                max_clique(die, candidate, Y, essential[j]);
                Pin* clock = NULL;
                for (auto& k : essential[j]->pin) {
                    size_t in1 = k.second->name.find("CLK");
                    size_t in2 = k.second->name.find("clk");
                    if (in1 != std::string::npos || in2 != std::string::npos) {
                        clock = k.second;
                        break;
                    }
                }

                vector <interval> Z;
                for (int m = 0; m < candidate.size(); m++) {
                    /*for (auto& k : candidate[m]->pin) {
                        size_t in1 = k.second->name.find("CLK");
                        size_t in2 = k.second->name.find("clk");
                        if (in1 != std::string::npos || in2 != std::string::npos) {
                            if (k.second->connect[0] == clock->connect[0]) {
                                for (int n = 0; n < Y.size(); n++) {
                                    if (candidate[m]->name == Y[n].ff) {
                                        Z.push_back(Y[n]);
                                    }
                                }
                            }
                        }
                    }*/
                    if (candidate[m]->pin[clock->name]->connect[0] == clock->connect[0]) {
                        /*for (int n = 0; n < Y.size(); n++) {
                            if (candidate[m]->name == Y[n].ff) {
                                Z.push_back(Y[n]);
                            }
                        }*/
                        auto it = Y.begin();
                        int times = 0;
                        string targetName = candidate[m]->name;
                        while ((it = std::find_if(it, Y.end(), [&targetName](const interval& temp) {
                            return temp.ff == targetName; })) != Y.end()) {
                            Z.push_back(*it);
                            ++it;
                            times++;
                            if (times == 2) {
                                break;
                            }
                        }
                    }
                }
                if (Z.size() / 2 < die.ff_library[model].bits) {
                    Z.clear();
                    continue;
                }
                else {
                    sort(Z.begin(), Z.end(), sort_x);
                    /*/if (check) {
                        cout << "!!!" << endl;
                        cout << "e" << essential[j]->name << endl;
                        for (int t = 0; t < Z.size(); t++) {
                            cout << Z[t].ff << " " << Z[t].s_e << " " << Z[t].x << endl;
                        }
                    }*/
                    to_be_bank.push_back(essential[j]);
                    for (int t = 0; t < Z.size(); t++) {
                        if (to_be_bank.size() == die.ff_library[model].bits) {
                            if (Z[t].ff != essential[j]->name) {
                                string targetName = Z[t].ff;
                                Z.erase(Z.begin() + t);
                                auto it = std::find_if(Z.begin(), Z.end(), [&targetName](const interval& temp) {return temp.ff == targetName && !temp.s_e; });

                                if (it != Z.end()) {
                                    Z.erase(it);
                                    t = t - 2;
                                }
                            }
                        }
                        else {
                            if (Z[t].s_e && Z[t].ff != essential[j]->name) {
                                to_be_bank.push_back(die.ff_list_inbin[Z[t].ff]);
                            }
                        }
                    }
                    Y.clear();
                    /*if (check) {
                        cout << "!!!" << endl;
                        for (int t = 0; t < Z.size(); t++) {
                            cout << Z[t].ff << " " << Z[t].s_e << " " << Z[t].x << endl;
                        }
                        cout << "!!!" << endl;
                    }*/
                    /*for (int t = 0; t < to_be_bank.size(); t++) {
                        auto it = X.begin();
                        string targetName = to_be_bank[t]->name;
                        while ((it = std::find_if(it, X.end(), [&targetName](const interval& temp) {
                            return temp.ff == targetName; })) != X.end()) {
                            Z.push_back(std::move(*it));
                            it = X.erase(it);
                        }
                    }*/
                    X.clear();
                    std::cout << std::fixed << std::setprecision(10);
                    sort(Z.begin(), Z.end(), sort_x);
                    for (int t = 0; t < Z.size(); t++) {
                        //cout << Z[t].ff << " " << Z[t].s_e << " " << Z[t].x << endl;
                    }
                    for (int k = 0; k < Z.size(); k++) {
                        if (Z[k].s_e) {
                            xr = Z[k].x;
                            xl = Z[k - 1].x;
                            break;
                        }
                    }
                    //cout << "--" << endl;
                    sort(Z.begin(), Z.end(), sort_y);
                    for (int t = 0; t < Z.size(); t++) {
                        //cout << Z[t].ff << " " << Z[t].s_e << " " << Z[t].y << endl;
                    }
                    for (int k = 0; k < Z.size(); k++) {
                        if (Z[k].s_e) {
                            yt = Z[k].y;
                            yb = Z[k - 1].y;
                            break;
                        }
                    }
                    //cout << xl << " " << xr << " " << yb << " " << yt << endl;
                    Z.clear();
                    Point p, p_;
                    p.x = xl;
                    p.y = yb;
                    p_ = coordinate_transform_converse(p);
                    feasible_p.push_back(p_);
                    p.y = yt;
                    p_ = coordinate_transform_converse(p);
                    feasible_p.push_back(p_);
                    p.x = xr;
                    p_ = coordinate_transform_converse(p);
                    feasible_p.push_back(p_);
                    p.y = yb;
                    p_ = coordinate_transform_converse(p);
                    feasible_p.push_back(p_);

                    /*if (check) {
                        for (int t = 0; t < feasible_p.size(); t++) {
                            cout << feasible_p[t].x << " " << feasible_p[t].y << endl;
                        }
                    }*/
                    return true;
                }
            }
            for (int j = 0; j < essential.size(); j++) {
                int times = 0;
                auto it = Y.begin();
                string targetName = essential[j]->name;
                while ((it = std::find_if(it, Y.end(), [&targetName](const interval& temp) {
                    return temp.ff == targetName; })) != Y.end()) {
                    it = Y.erase(it);
                    if (times == 2) {
                        break;
                    }
                }
            }
            essential.clear();
            decision_p = false;
            start = i;
        }
    }
    return false;
}

void update_cur(Die& die, vector<vector<Point>>& cur, Inst* inst, double w, double h) {
    int low = FindLow_cur(die.row, inst->lb.y - h);
    int up = FindUp_cur(die.row, inst->rt.y);

    /*
    if (inst->name == "C107906") {
        cout << up << " " << low << endl;
    }*/

    int left = 0;
    int right = 0;  
    for (int i = low; i <= up; i++) {
        int num = die.row[i].num;
        left = max(floor((inst->lb.x - die.row[i].start.x - w) / die.row[i].w), -1.0);
        right = min(ceil((inst->rt.x - die.row[i].start.x) / die.row[i].w), double(num));

        if (left > right) {
            continue;
        }

        /*
        if (inst->name == "C107906") {
            cout << left << " " << right << endl;
        }*/

        for (int j = 0; j < cur[i].size(); j++) { // left貼合不刪
            if (left < cur[i][j].y && right > cur[i][j].x) {
                if (left < cur[i][j].x && right >= cur[i][j].y) { //移除
                    //cout << "1" << endl;
                    cur[i][j].x = num + 1;
                    cur[i][j].y = num + 1;
                }
                else if (left >= cur[i][j].x && right < cur[i][j].y) {	//切割
                    //cout << "2" << endl;
                    Point p;
                    p.x = right;
                    p.y = cur[i][j].y;
                    cur[i][j].y = left;
                    cur[i].push_back(p);
                }
                else if (left >= cur[i][j].x && right >= cur[i][j].y) { //cur[i][j].y = left
                    //cout << "3" << endl;
                    cur[i][j].y = left;
                }
                else if (left < cur[i][j].x && right < cur[i][j].y) {	//cur[i][j].x = right;
                    //cout << "4" << endl;
                    cur[i][j].x = right;
                }
                else {
                    cout << " nima" << endl;
                    cout << left << "\t" << right << endl;
                    cout << cur[i][j].x << "\t" << cur[i][j].y << endl;
                }
            }
        }
    }
}

void cut_row(Die& die, vector<vector<Point>>& cur, string model)
{
    cur.resize(die.row.size());
    for (int i = 0; i < die.row.size(); i++) {
        Point p;
        p.x = 0;
        p.y = die.row[i].num;
        cur[i].push_back(p);
    }

    double w = die.ff_library[model].w;
    double h = die.ff_library[model].h;
    for (auto& n : die.gate_list) {
        //cout << n.first << endl;
        update_cur(die, cur, n.second, w, h);
    }

    for (auto& n : die.ff_list_inbin) {
        //cout << count << "   " << n.first << endl;
        if (n.second->legal) {
            update_cur(die, cur, n.second, w, h);
        }
    }

    for (int i = 0; i < die.row.size(); i++) {
        //cout << i << endl;
        sort(cur[i].begin(), cur[i].end(), compare_point_x);
        int num = die.row[i].num;
        
        //cout << "a" << endl;
        int j = cur[i].size() - 1;
        while (cur[i][j].x == num + 1) {
            //cout << j << endl;
            cur[i].pop_back(); 
            j--;
            if (j < 0) {
                break;
            }
        }
    }
}

Point best_point(Die& die, vector<vector<Point>>& fp, string model, int low, Inst* inst)
{
    FF_info ff = die.ff_library[model];
    Point cur_point{ 0.0,0.0 };
    Point best_point{ 0.0,0.0 };
    double cur_hpwl;
    double best_hpwl = DBL_MAX;

    for (int i = 0; i < fp.size(); i++) {
        cur_point.y = die.row[i + low].start.y;
        for (int j = 0; j < fp[i].size(); j++) {    
            int mid = (fp[i][j].x + fp[i][j].y) / 2;
            cur_point.x = die.row[i + low].start.x + die.row[i + low].w * mid;
            cur_hpwl = inst->HPWL(cur_point, ff);

            if (cur_hpwl < best_hpwl) {
                best_hpwl = cur_hpwl;
                best_point = cur_point;
            }
        }
    }

    return best_point;
}

vector<vector<Point>> feasible_point(Die& die, vector<Point> fr, Rect rect, Inst* inst, vector<vector<Point>>& cur, int mode)
{
    vector<vector<Point>> fp;

    int low, up;
    if (mode == 1) {
        //cout << "mode diamond" << endl;
        low = FindLow_fea(die.row, fr[0].y);
        up = FindUp_fea(die.row, fr[2].y);
        //cout << "low up " << fr[0].y << " " << fr[2].y << endl;
    }
    else {
        //cout << "mode rect" << endl;
        low = FindLow_fea(die.row, rect.lb.y);
        up = FindUp_fea(die.row, rect.rt.y);
        //cout << "low up " << rect.lb.y << " " << rect.rt.y << endl;
    }

    if (up == -1 || low == -1) {
        return fp;
    }

    fp.resize(up - low + 1);
    //cout << "low up " << low << " " << up << endl;

    //find legal point in feasible region
    for (int i = low; i <= up; i++) {
        double y = die.row[i].start.y;
        double left, right;
        int num = die.row[i].num;
        if (mode == 1) {
            if (y > fr[1].y) {
                left = fr[2].x - (fr[2].y - y);
            }
            else {
                left = fr[0].x - (y - fr[0].y);
            }

            if (y > fr[3].y) {
                right = fr[2].x + (fr[2].y - y);
            }
            else {
                right = fr[0].x + (y - fr[0].y);
            }  
        }
        else {
            left = rect.lb.x;
            right = rect.rt.x;
        }
        //cout << "left right " << left << " " << right << endl;
        left = max(ceil((left - die.row[i].start.x) / die.row[i].w), 0.0);
        right = min(floor((right - die.row[i].start.x) / die.row[i].w), double(num));
        //cout << "left right " << left << " " << right << endl;
 
        if (left > right) {
            continue;
        }

        for (int j = 0; j < cur[i].size(); j++) {
            if (left < cur[i][j].y && right > cur[i][j].x) {	//a:feasible region   b:legal row
                if (left <= cur[i][j].x && right >= cur[i][j].y) {	//a完全覆蓋b
                    //cout << "1" << endl;
                    Point p;
                    p.x = cur[i][j].x;
                    p.y = cur[i][j].y;
                    fp[i - low].push_back(p);
                }
                else if (left > cur[i][j].x && right < cur[i][j].y) {	//a切割b
                    //cout << "2" << endl;
                    Point p;
                    p.x = left;
                    p.y = right;
                    fp[i - low].push_back(p);
                }
                else if (left > cur[i][j].x && right >= cur[i][j].y) {	//a在b右邊
                    //cout << "2" << endl;
                    Point p;
                    p.x = left;
                    p.y = cur[i][j].y;
                    fp[i - low].push_back(p);
                }
                else if (left <= cur[i][j].x && right < cur[i][j].y) {	//a在b左邊
                    //cout << "3" << endl;
                    Point p;
                    p.x = cur[i][j].x;
                    p.y = right;
                    fp[i - low].push_back(p);
                }
            }
        }
    }

    return fp;
}

bool legalization(Die& die, vector<vector<string>>& ff_sorted)
{
    double w = die.bin[1];
    double h = die.bin[0];
    clock_t start = clock();
    clock_t end = clock();
    vector<Inst*> unfit;
    for (int i = ff_sorted.size() - 1; i >= 1; i--) {
        if (!ff_sorted[i].empty()) {
            cout << i << "bits" << endl;
            unfit.clear();
            for (auto& n : die.ff_list_inbin) {
                if (n.second->pin.size() == i * 2 + 1) {
                    //cout << n.first << endl;
                    unfit.push_back(n.second);
                }
            }
            
            for (int j = 0; j < ff_sorted[i].size(); j++) {
                if (unfit.empty()) {
                    break;
                }

                end = clock();
                double time = double(end - start) / double(CLOCKS_PER_SEC);
                cout << "model " << j << " | left " << unfit.size() << " | " << time << "s" << endl;

                vector<vector<Point>> cur;
                cut_row(die, cur, ff_sorted[i][j]);

                int k = unfit.size() - 1;
                int over = 0;
                while (k >= over) {
                    if (unfit.size() % 100 == 0) {
                        end = clock();
                        double time = double(end - start) / double(CLOCKS_PER_SEC);
                        cout << "model " << j << " | left " << unfit.size() << " | " << time << "s | " << cur[12].size() << endl;
                    }
                    
                    if (unfit[k]->name == "C107906" || unfit[k]->name == "C107871") {
                        cout << k << " " << unfit[k]->name << endl;
                    }

                    //cout << unfit[k]->name << endl;
                    Rect r = feasible_region(die, unfit[k], ff_sorted[i][j]);
                    vector<Point> fr;
                    transform(unfit[k], r, fr);
                    
                    int fr_bottom = FindLow_fea(die.row, fr[0].y); 
                    vector<vector<Point>> fp = feasible_point(die, fr, Rect{ {0,0},{0,0} }, unfit[k], cur, 1);

                    int count = 0;
                    for (int i = 0; i < fp.size(); i++) {
                        if (fp[i].empty()) {
                            count++;
                        }
                    }

                    Point final{ -1,-1 };
                    if (count == fp.size()) { //no feasible point in feasible region
                        
                        //cout << "no feasible point in feasible region" << endl;
                        int a = floor(((fr[0].y + fr[2].y) / 2) / die.bin[0]);
                        int b = floor(((fr[1].x + fr[3].x) / 2) / die.bin[1]);
                        Rect rect;
                        rect.lb.x = b * w;
                        rect.lb.y = a * h;
                        rect.rt.x = (b + 1) * w;
                        rect.rt.y = (a + 1) * h;
                        int aaa = 0;
                        while (1) { // expand bounding box until find feasible point
                            //cout << unfit[k]->name << " " << aaa << endl;
                            fp = feasible_point(die, fr, rect, unfit[k], cur, 0);
                            fr_bottom = FindLow_fea(die.row, rect.lb.y);
                            count = 0;
                            for (int i = 0; i < fp.size(); i++) {
                                if (fp[i].empty()) {
                                    count++;
                                }
                            }

                            if (count == fp.size()) {
                                if (equal_point(rect.lb, die.diesize.lb) && equal_point(rect.rt, die.diesize.rt)) { // still can not find
                                    break;
                                }
                                //撞到左邊要往右推
                                if (rect.rt.x + w / 2 > die.diesize.rt.x) {
                                    rect.rt.x = die.diesize.rt.x;
                                    rect.lb.x = rect.lb.x - (rect.rt.x + w / 2 - die.diesize.rt.x);
                                }
                                else {
                                    rect.rt.x = rect.rt.x + w / 2;
                                    rect.lb.x = rect.lb.x - w / 2;
                                }

                                if (rect.rt.y + h / 2 > die.diesize.rt.y) {
                                    rect.rt.y = die.diesize.rt.y;
                                    rect.lb.y = rect.lb.y - (rect.rt.y + w / 2 - die.diesize.rt.y);
                                }
                                else {
                                    rect.rt.y = rect.rt.y + h / 2;
                                    rect.lb.y = rect.lb.y - h / 2;
                                }

                                rect.lb.x = max(rect.lb.x, die.diesize.lb.x);
                                rect.lb.y = max(rect.lb.y, die.diesize.lb.y);
                            }
                            else {
                                final = best_point(die, fp, ff_sorted[i][j], fr_bottom, unfit[k]);
                                break;
                            }
                            aaa++;
                        }
                    }
                    else {  //exist feasible point in feasible region
                        //cout << "exist feasible point in feasible region" << endl;
                        /*
                        if (k == 19287 || k == 19676 || k == 18501) {
                            cout << "exist feasible point in feasible region" << endl;
                            for (int i = 0; i < fp.size(); i++) {
                                cout << "row " << i << endl;
                                for (int j = 0; j < fp[i].size(); j++) {
                                    cout << fp[i][j].x << " " << fp[i][j].y << endl;
                                }
                            }
                        }
                        */
                        final = best_point(die, fp, ff_sorted[i][j], fr_bottom, unfit[k]);

                    }

                    if (final.x != -1 && final.y != -1) { 
                        if (final.x == 0 || final.y == 0) {
                            cout << "0.0 " << k << " " << unfit[k]->name << endl;
                        }


                        //cout << final.x << " " << final.y << endl;
                        //update inst
                        unfit[k]->update_position(final, die.ff_library[ff_sorted[i][j]], die);
                        unfit[k]->type = ff_sorted[i][j];
                        unfit[k]->legal = true;

                        //update cur
                        int low = FindLow_cur(die.row, unfit[k]->lb.y - h);
                        int up = FindUp_cur(die.row, unfit[k]->rt.y);

                        update_cur(die, cur, unfit[k], die.ff_library[ff_sorted[i][j]].w, die.ff_library[ff_sorted[i][j]].h);

                        
                        for (int i = low; i <= up; i++) {
                            int num = die.row[i].num;
                            sort(cur[i].begin(), cur[i].end(), compare_point_x);
                            int j = cur[i].size() - 1;
                            while (cur[i][j].x == num + 1) {
                                //cout << j << endl;
                                cur[i].pop_back();
                                j--;
                                if (j < 0) {
                                    break;
                                }
                            }
                        }
                        k--;
                        unfit.pop_back();
                    }
                    else {
                        swap(unfit[over], unfit[k]);
                        over++;
                    }
                    //cout << "----------------------" << endl;
                }
            }

            if (!unfit.empty()) {
                cout << "fuckfuckfuckfuckfuckfuckfuckfuckfuckfuck" << endl;

                if (i != 1) {
                    for (int j = 0; j < unfit.size(); j++) {
                        debanking(die, unfit[j], ff_sorted[1][0]);
                    }
                }
                else {
                    cout << "裂開裂開裂開裂開裂開裂開裂開裂開裂開裂開" << endl;
                    return false;
                }
            }

        }
    }
    return true;
}

/*
void utilize(Die& die)
{
    double max_utilize = die.bin[0] * die.bin[1] * die.bin[2] / 100;
    for (int i = 0; i < die.chip.size(); i++) {
        for (int j = 0; j < die.chip[i].size(); j++) {
            while (die.chip[i][j].cal_area() >= max_utilize) {
                Inst* target = NULL;
                Point p = die.chip[i][j].refinement(die, target);
                if (p.x != -1 && p.y != -1) {
                    //move inst()
                }
            }
        }
    }
}
*/

bool INTEGRA(Die& die)
{
    cout << "sort library" << endl;
    vector<vector<string>> ff_sorted;
    sort_ff_library(die, ff_sorted);

    //print library result
    for (int i = 0; i < ff_sorted.size(); i++) {
        if (ff_sorted.size() != 0) {
            cout << i << " bit" << endl;
            for (int j = 0; j < ff_sorted[i].size(); j++) {
                double score = quality(die, ff_sorted[i][j]);
                cout << ff_sorted[i][j] << " " << score << " " << die.ff_library[ff_sorted[i][j]].w << " " << die.ff_library[ff_sorted[i][j]].h << endl;
            }
        }
    }

    //debank inst which bits > 1
    vector<Inst*> multibits;
    for (auto& n : die.ff_list_inbin) {
        if (n.second->pin.size() > 3) {
            multibits.push_back(n.second);
        }
    }
    for (int i = 0; i < multibits.size(); i++) {
        debanking(die, multibits[i], ff_sorted[1][0]);
    }


    read_ff(die);
    //cout << "nima" << endl;
    //ff_connect(die);
    //save_ff(die);
    //cout << "nima" << endl;
    
    
    for (int i = ff_sorted.size() - 1; i > 1; i--) {
        if (!ff_sorted[i].empty()) {
            vector <interval> X;
            vector<Inst*> to_be_banked;
            vector<Point> feasible_p;
            bool cont = true;

            while (cont) { //bank inst until no target
                //cout << "slack_cal" << endl;
                slack_cal(die, X, ff_sorted[i][0]); // cal feasible region of 1 bit inst
                //cout << "find_candidate" << endl;
                cont = find_candidate(die, X, to_be_banked, ff_sorted[i][0], feasible_p); //find inst to be banked and its feasible region
                if (!cont) {
                    break;
                }

                //cout << "banking" << endl;
                Inst* new_inst = banking(die, to_be_banked, ff_sorted[i][0]);   //bank inst
                die.ff_list_inbin[new_inst->name] = new_inst;

                //put new inst to the middle of feasible region
                //cout << "update_position" << endl;
                int index_y = FindIndex(die.row, (feasible_p[0].y + feasible_p[2].y) / 2);
                if (index_y == -1) {
                    index_y = 0;
                }
                int index_x = max(floor(((feasible_p[1].x + feasible_p[3].x) / 2 - die.row[index_y].start.x) / die.row[index_y].w) + 1, 0.0);
                Point target{ die.row[index_y].start.x + die.row[index_y].w * index_x,die.row[index_y].start.y };
                die.ff_list_inbin[new_inst->name]->update_position(target, die.ff_library[ff_sorted[i][0]], die);

                if (new_inst->name == "bank300") {
                    i = 0;
                    break;
                }
                cout << new_inst->name << endl;
                feasible_p.clear();
                to_be_banked.clear();
            }
        }
    }

    if (legalization(die, ff_sorted)) {
        cout << "legalization complete" << endl;
        return true;
    }
    else {
        cout << "legalization fail" << endl;
        return false;
    }
    
}


//utilize(die);
    /*
    cout << "762300 " << FindStart_Y(die.row, 762300) << endl;
    cout << "772800 " << FindStart_Y(die.row, 772800) << endl;
    //cout << "25000 " << FindStart(die.row, 25000) << endl;
    //cout << "1284000 " << FindStart(die.row, 1284000) << endl;

    vector<vector<Point>> cur; //can use row;
    cut_row(die, cur, ff_sorted[4][0]);

    for (int i = ff_sorted.size() - 1; i >= 0; i--) {
        if (!ff_sorted[i].empty()) {
            vector<vector<Point>> cur; //can use row;
            //cut_row(die, cur, ff_sorted[i][0]);


        }
    }
    */

    /*
    output("output3_initial", die);
    cout << die.ff_list_inbin["C58403"]->name << endl;
    cout << die.ff_list_inbin["C58403"]->type << endl;
    cout << die.ff_list_inbin["C58403"]->lb.x << " " << die.ff_list_inbin["C58403"]->lb.y << endl;
    for (int i = 0; i < die.ff_list_inbin["C58403"]->pin["CLK"]->connect.size(); i++) {
        cout << die.ff_list_inbin["C58403"]->pin["CLK"]->connect[i]->name << endl;;
    }
    cout << "//////////////////////////////////" << endl;
    debanking(die, die.ff_list_inbin["C58403"], ff_sorted[1][0]);
    for (auto& n : die.ff_list_inbin) {
        if (n.first == "debank0" || n.first == "debank1" || n.first == "debank2" || n.first == "debank3") {
            cout << n.second->name << endl;
            cout << n.second->type << endl;
            cout << n.second->lb.x << " " << n.second->lb.y << endl;
            for (int i = 0; i < n.second->pin["CLK"]->connect.size(); i++) {
                cout << " " << n.second->pin["CLK"]->connect[i]->name << endl;
            }
        }
    }
    output("output3_debank", die);
    cout << "//////////////////////////////////" << endl;
    vector<Inst*> v;
    v.push_back(die.ff_list_inbin["debank0"]);
    v.push_back(die.ff_list_inbin["debank1"]);
    Inst* newinst = banking(die, v, ff_sorted[2][0]);
    die.ff_list_inbin[newinst->name] = newinst;
    die.ff_list_inbin[newinst->name]->update_position({ 0.0, 0.0 }, die.ff_library[ff_sorted[2][0]], die);
    cout << die.ff_list_inbin["bank0"]->name << endl;
    cout << die.ff_list_inbin["bank0"]->type << endl;
    cout << die.ff_list_inbin["bank0"]->lb.x << " " << die.ff_list_inbin["bank0"]->lb.y << endl;
    for (int i = 0; i < die.ff_list_inbin["bank0"]->pin["CLK"]->connect.size(); i++) {
        cout << die.ff_list_inbin["bank0"]->pin["CLK"]->connect[i]->name << endl;;
    }
    cout << endl;
    output("output3_bank", die);
    */

    /*
    output("test0", die);
    vector<Inst*> v;
    v.push_back(die.ff_list_inbin["reg1"]);
    v.push_back(die.ff_list_inbin["reg2"]);
    v.push_back(die.ff_list_inbin["reg3"]);
    v.push_back(die.ff_list_inbin["reg4"]);
    Inst* newinst = banking(die, v, ff_sorted[4][0]);
    die.ff_list_inbin[newinst->name] = newinst;
    die.ff_list_inbin[newinst->name]->update_position({ 0.0, 0.0 }, die.ff_library[ff_sorted[4][0]], die);
    output("test1", die);
    debanking(die, newinst, ff_sorted[1][0]);
    output("test2", die);
    v.clear();
    v.push_back(die.ff_list_inbin["debank0"]);
    v.push_back(die.ff_list_inbin["debank1"]);
    newinst = banking(die, v, ff_sorted[2][0]);
    die.ff_list_inbin[newinst->name] = newinst;
    cout << "nima" << endl;
    die.ff_list_inbin[newinst->name]->update_position({ 0, 0 }, die.ff_library[ff_sorted[2][0]], die);
    output("test3", die);
    debanking(die, newinst, ff_sorted[1][0]);
    output("test4", die);
    v.clear();
    v.push_back(die.ff_list_inbin["debank2"]);
    v.push_back(die.ff_list_inbin["debank5"]);
    newinst = banking(die, v, ff_sorted[2][0]);
    die.ff_list_inbin[newinst->name] = newinst;
    die.ff_list_inbin[newinst->name]->update_position({ 0, 0 }, die.ff_library[ff_sorted[2][0]], die);
    output("test5", die);
    debanking(die, newinst, ff_sorted[1][0]);
    output("test6", die);
    v.clear();
    v.push_back(die.ff_list_inbin["debank7"]);
    v.push_back(die.ff_list_inbin["debank3"]);
    newinst = banking(die, v, ff_sorted[2][0]);
    die.ff_list_inbin[newinst->name] = newinst;
    die.ff_list_inbin[newinst->name]->update_position({ 0, 0 }, die.ff_library[ff_sorted[2][0]], die);
    output("test7", die);
    debanking(die, newinst, ff_sorted[1][0]);
    output("test8", die);
    v.clear();
    v.push_back(die.ff_list_inbin["debank4"]);
    v.push_back(die.ff_list_inbin["debank6"]);
    v.push_back(die.ff_list_inbin["debank8"]);
    v.push_back(die.ff_list_inbin["debank9"]);
    newinst = banking(die, v, ff_sorted[4][0]);
    die.ff_list_inbin[newinst->name] = newinst;
    die.ff_list_inbin[newinst->name]->update_position({ 0, 0 }, die.ff_library[ff_sorted[4][0]], die);
    output("test9", die);
    debanking(die, newinst, ff_sorted[1][0]);
    output("test10", die);
    */
    //debanking(die, die.ff_list_inbin["bank0"], ff_sorted[1][0]);

    /*
    for (int i = low; i <= up; i++) {
        int num = die.row[i].num;
        left = max(floor((inst->lb.x - die.row[i].start.x) / die.row[i].w), 0.0);
        right = min(floor((inst->rt.x - die.row[i].start.x) / die.row[i].w), double(num));

        if (left > right) {
            continue;
        }

        if (inst->name == "C107906") {
            cout << left << " " << right << endl;
        }

        for (int j = 0; j < cur[i].size(); j++) { // left貼合刪
            if (left < cur[i][j].y && right > cur[i][j].x) {	//a:gate   b:row
                if (left <= cur[i][j].x && right >= cur[i][j].y) { //移除
                    //cout << "1" << endl;
                    cur[i][j].x = num + 1;
                    cur[i][j].y = num + 1;
                }
                else if (left > cur[i][j].x && right < cur[i][j].y) {	//切割
                    //cout << "2" << endl;
                    Point p;
                    p.x = right;
                    p.y = cur[i][j].y;
                    cur[i][j].y = left;
                    cur[i].push_back(p);
                }
                else if (left > cur[i][j].x && right >= cur[i][j].y) { //cur[i][j].y = left
                    //cout << "3" << endl;
                    cur[i][j].y = left;
                }
                else if (left <= cur[i][j].x && right < cur[i][j].y) {	//cur[i][j].x = right
                    //cout << "4" << endl;
                    cur[i][j].x = right;
                }
                else {
                    cout << " nima" << endl;
                    cout << left << "\t" << right << endl;
                    cout << cur[i][j].x << "\t" << cur[i][j].y << endl;
                }
            }
        }
    }
    */

