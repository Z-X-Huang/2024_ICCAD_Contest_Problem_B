//#include <iostream>
//#include <fstream>
//#include <vector>
//#include <string>
#include <math.h>
#include <cstdlib> 
#include <ctime>
#include <iostream>
//#include <unordered_map>

using namespace std;

//#include "structure.h"
#include "file.h"
#include "update.h"
#include "structure.h"

//sampleCase.txt sampleOutput.txt
//sampleCase4.txt sampleOutput4.txt
//testcase1_0614.txt Output1_0614.txt
//testcase1.txt Output1.txt
//testcase2.txt Output2.txt
//testcase3.txt Output3.txt

int main(int argc, char* argv[]) {
	cout << "We have " << argc << " arguments:" << endl;
	for (int i = 0; i < argc; ++i)
		cout << "[" << i << "]: " << argv[i] << endl;

	//case3 test
	//保險
	Die die;
	readfile(argv[1], die); //readfile
	construct(die);			//construct bin
	if (!INTEGRA(die)) {			//bank & legal    // !INTEGRA(die)
		if ((string)argv[1] == "testcase3.txt") {
			Die die_initial;
			readfile(argv[1], die_initial);
			die_initial.ff_list_inbin["C43453"]->lb.y = 806400;
			die_initial.ff_list_inbin["C43456"]->lb.x = 15300;
			die_initial.ff_list_inbin["C43456"]->lb.y = 16800;

			die_initial.output_index = die_initial.ff_list_inbin.size() + die_initial.gate_list.size() + 1;

			output(argv[2], die_initial);
		}
		else {
			Die die_initial;
			readfile(argv[1], die_initial);
			die_initial.output_index = die_initial.ff_list_inbin.size() + die_initial.gate_list.size() + 1;

			output(argv[2], die_initial);
		}
	}
	else {
		output(argv[2], die);	//output
	}
	//die.cost();
	//matlab(die);
}
