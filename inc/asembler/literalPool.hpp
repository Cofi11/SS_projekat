#ifndef LITERAL_POOL_HPP
#define LITERAL_POOL_HPP

#include <iostream>
#include <vector>
using namespace std;

struct Literal_pool_entry{
  uint16_t section;
  uint16_t offset;
  Literal_pool_entry* next;

  Literal_pool_entry(int32_t value, uint16_t section, uint16_t offset, Literal_pool_entry* next = nullptr){
    this->value = value;
    this->section = section;
    this->offset = offset;
    this->next = next;
  }

};



#endif