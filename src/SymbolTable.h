#include <stdexcept>
#include <string>
#include <vector>
#include <map>

template<class T>
struct SymbolTable {
  std::vector<std::map<std::string, T>> mp; 
  SymbolTable(): mp(1) {}
  void push() {mp.emplace_back();}
  void pop() {mp.pop_back();}
  void insert(const std::string &name, T v) {
    if (mp.front().count(name)) {
      throw std::runtime_error{"var: " + name + " already exist"};
    }
    mp.front()[name] = v;
  }
  T get(const std::string& name) {
    for (auto it = rbegin(mp); it != rend(mp); ++it) {
      if (mp.count(name)) {
        return mp[name];
      }
    }
    throw std::runtime_error{"var: " + name + " don't exist"};
  }

};

