#include "hw1.h"

int main(int argc, char* argv[]) {
  if(strcmp(argv[1], "q1") == 0) {
    Table* custom_table = new Table(argv[2]);
    Table* zone_table = new Table(argv[3]);
    list<string*>* zone_id = zone_table->FindToronto();
    if(zone_id == NULL)
      return -1;
    list<string*>* lname_list = custom_table->AcquireTable(zone_id);
    if(lname_list == NULL)
      return -1;
    PrintList(lname_list);
  }
  else if(strcmp(argv[1], "q2") == 0) {
    Table* item_table = new Table(argv[2]);
    Table* product_table = new Table(argv[3]);
    list<string*>* barcode_list = item_table->TwicePurchase();
    if(barcode_list == NULL)
      return -1;
    map<string*,string*>* result_map = product_table->AnalyzeTable(barcode_list);
    if(result_map == NULL)
      return -1;
    PrintMap(result_map);
  }
  else {
    cout << "Input your query EXACTLY" << endl;
  }
}



void PrintList(list<string*>* lname_list) {

  for(list<string*>::iterator it = lname_list->begin(); it != lname_list->end(); it++) {
    cout << **it  << endl;
  }

}
void PrintList(list<vector<string*> *>* l) {
  for(list<vector<string*> *>::iterator it = l->begin(); it != l->end(); ++it) {
    PrintVector(*it);
    cout << endl;
  }

}

void PrintMap(map<string*, string*>* m) {
  for(map<string*, string*>::iterator it = m->begin(); it != m->end(); ++it) {
    cout << *it->first << " , " << *it->second << endl;
  }
}
void PrintVector(vector<string*>* v) {
  for(vector<string*>::iterator it = v->begin(); it != v->end(); it++) {
    cout << **it << " ";
  }
}
void PrintVector(vector<int> v) {
  for(vector<int>::iterator it = v.begin(); it != v.end(); it++) {
    cout << *it << " ";
  }
  cout << endl;
}

