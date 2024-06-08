#ifndef FILE_RW_HPP
#define FILE_RW_HPP

#include <iostream>
#include <vector>

template<typename T>
inline void write(std::ostream& os, const T& data){
  os.write((const char*)&data, sizeof(T));
}

template<typename T>
inline void write(std::ostream& os, const std::vector<T>& data){
  write(os, (uint32_t)data.size());
  for(const T& d : data){
    write(os, d);
  }
}

template<typename T> 
inline void read(std::istream& is, T& data){
  is.read((char*)&data, sizeof(T));
}

template<typename T>
inline void read(std::istream& is, std::vector<T>& data){
  uint32_t size;
  read(is, size);
  data.resize(size);
  for(T& d : data){
    read(is, d);
  }
}

inline void writeString(std::ostream& os, const std::string& str){
  write(os, (uint32_t)str.size());
  os.write(str.c_str(), str.size());
}

inline void readString(std::istream& is, std::string& str){
  uint32_t size;
  read(is, size);
  str.resize(size);
  is.read(&str[0], size);
}


#endif