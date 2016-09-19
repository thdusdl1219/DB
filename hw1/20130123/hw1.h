#ifndef HW1_H
#define HW1_H

#include <vector>
#include <list>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <iterator>
#include <utility>
#include <map>
#include <set>
#include "record.h"

using namespace std;



void PrintList(list<string*>* lname_list);
void PrintList(list<vector<string*> *>* l);
void PrintMap(map<string*, string*>* m);
void PrintVector(vector<string*>* v);
void PrintVector(vector<int> v);
 
#endif
