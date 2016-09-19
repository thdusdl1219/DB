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

using namespace std;

class Record {
  private:
    vector<string*>* record;
  public :
    Record(vector<string*>* v);
    string* At(int i);
    int FindIndex(const string& find_string);
};

class Table {
  private :
    list<Record*> table;
  public :
    Table(char* filename);
    vector<int> ReadSize(string line);
    list<string*>* FindToronto();
    list<string*>* TwicePurchase();
    list<string*>* AcquireTable(list<string*>* zone_id);
    map<string*, string*>* AnalyzeTable(list<string*>* barcode_list);
};


inline string rtrim(string s,const string& drop = " ")  
{  
  return s.erase(s.find_last_not_of(drop)+1);  
}
