#include "record.h"



Record::Record(vector<string*>* v) {
  record = v;
}

string* Record::At(int i) {
  return record->at(i);
}

int Record::FindIndex(const string& find_string) {
  vector<string*>::iterator vit = record->begin();

  for(; vit != record->end(); vit++) {
    if(!((*vit)->compare(find_string))) {
     break;
    }
  }
  if(vit == record->end()) {
    cout << "TABLE ERROR" << endl;
    return -1;
  }
  return distance(record->begin(), vit);
}

Table::Table(char* filename) {
  list<vector<string*> *> vector_list;
  vector<int> size_vector;
  ifstream file;
  string line;
  file.open(filename, ios::in|ios::binary);
  while(getline(file, line)) {
    istringstream iss(line);
    string* data = new string();
    vector<string*>* string_vector = new vector<string*>();
    while(getline(iss, *data, ' ')) {
      if(data->find("--") != string::npos) {
        size_vector = ReadSize(line);
        break;
      }
      else if(data->size() == 0)
        continue;
      else if(!(data->compare("\x0d")))
        break;
      int offset = data->find("\x0d");
      if(offset != string::npos)
        data->replace(offset, 1, "");
      string_vector->push_back(data);
      data = new string();
    }
    if(string_vector->size() > 0) {
      vector_list.push_back(string_vector);
    }
    else
      delete string_vector;
    if(size_vector.size() > 0)
      break;
  }

  while(getline(file, line)) {
    istringstream iss(line);
    vector<string*>* string_vector = new vector<string*>();
    int index = 0;
    char d[1024] = "";
    while(iss.read(d, size_vector.at(index))) {
      string* data = new string(d);
      memset(d, 0, 1024);
      if(data->size() == 0)
        continue;
      else if(!data->compare("\x0d"))
        break;
      int offset = data->find("\x0d");
      if(offset != string::npos)
        data->replace(offset, 1, "");
      string_vector->push_back(data);
      iss.read(d, 1);
      index ++;
      if(index >= size_vector.size())
        break;
    }
    if(string_vector->size() > 0)
    vector_list.push_back(string_vector);
    else 
      delete string_vector;
  }
  for(list<vector<string*>*>::iterator it = vector_list.begin(); it != vector_list.end(); it++) {
    vector<string*>* v = *it;
    Record* record = new Record(v);
    table.push_back(record);
  }
  // debug
  //PrintVector(size_vector);
  //PrintList(vector_list);
}

vector<int> Table::ReadSize(string line) {
  istringstream iss(line);
  string data; 
  vector<int> result;
  while(getline(iss, data, ' ')) {
    result.push_back(data.size());
  }
  return result;
}

list<string*>* Table::FindToronto() {
  Record* re = *table.begin();
  int zonedesc_index = re->FindIndex("ZONEDESC");
  int zoneid_index = re->FindIndex("ZONEID");
  list<string*>* result = new list<string*> ();
  if(zonedesc_index == -1 || zoneid_index == -1)
    return NULL;
  for(list<Record*>::iterator it = table.begin(); it != table.end(); it++) {
  Record* record = *it;
    if(!(record->At(zonedesc_index)->find("Toronto"))) {
      result->push_back(record->At(zoneid_index));
    }
  }
  if(result->size() == 0) {
      cout << "Toronto doesn't exist." << endl;
      return NULL;
  }
  else {
    return result;
  }
}

list<string*>* Table::TwicePurchase() {
  map<string, pair<int,set<string>*>*> barcode_purchase_number;
  list<Record*>::iterator it = table.begin();
  int barcode_index = ((Record*)*it)->FindIndex("BARCODE");
  int name_index = ((Record*)*it)->FindIndex("UNAME");
  if(barcode_index == -1)
    return NULL;
  for(; it != table.end(); it++) {
    Record* record = *it;
    map<string, pair<int, set<string>*>*>::iterator mit = barcode_purchase_number.find(rtrim(*record->At(barcode_index)));
    if(mit == barcode_purchase_number.end()) {
      set<string>* s = new set<string>();
      s->insert(*record->At(name_index));
      barcode_purchase_number.insert(pair<string, pair<int, set<string>*>*> (rtrim(*record->At(barcode_index)), new pair<int,set<string>*> (1, s)));
    }
    else {
      if(mit->second->second->find(*record->At(name_index)) == mit->second->second->end()) {
        mit->second->first = mit->second->first + 1;
        mit->second->second->insert(*record->At(name_index));
      }

    }
  }
  list<string*>* result_list = new list<string*>();
  for(map<string, pair<int,set<string>*>*>::iterator mit = barcode_purchase_number.begin(); mit != barcode_purchase_number.end(); mit++) {
    if(mit->second->first >= 2) {
      result_list->push_back(new string(mit->first));
    }
    pair<int, set<string>*>* p = mit->second;
    delete p->second;
    delete p;
  }


  return result_list;
}

list<string*>* Table::AcquireTable(list<string*>* zone_id) {
  Record* it = *table.begin();
  int zone_index = it->FindIndex("ZONE");
  int lname_index = it->FindIndex("LNAME");
  int active_index = it->FindIndex("ACTIVE");
  if(zone_index == -1 || lname_index == -1 || active_index == -1)
    return NULL;
  list<string*>* llist = new list<string*>();
  for(list<Record*>::iterator it = table.begin(); it != table.end(); it++) {
    for(list<string*>::iterator lit = zone_id->begin(); lit != zone_id -> end(); lit++) {
      string t_zone_id = rtrim(*(string *)(*lit));
      Record* record = *it;
      if(!(rtrim(*record->At(zone_index)).compare(t_zone_id))) {
        if(!(rtrim(*record->At(active_index)).compare("1")))
          llist->push_back(record->At(lname_index));
      }
    }
  }
  return llist;
}

map<string*, string*>* Table::AnalyzeTable(list<string*>* barcode_list) {
  Record* it = *table.begin();
  int barcode_index = it->FindIndex("BARCODE");
  int proddesc_index = it->FindIndex("PRODDESC");
  if(barcode_index == -1 || proddesc_index == -1)
    return NULL;

  map<string*, string*>* result_map = new map<string*, string*>();
  for(list<Record*>::iterator it = table.begin(); it != table.end(); it++) {
  Record* record = *it;
    for(list<string*>::iterator sit = barcode_list->begin(); sit != barcode_list->end(); sit++) {
      if(!(rtrim(*record->At(barcode_index)).compare(**sit))) {
        result_map->insert(pair<string*, string*>(*sit, record->At(proddesc_index)));
      }
    }
  }
  return result_map;
}

