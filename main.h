#pragma once
#include "rapidjson/document.h"

char* map_file(const std::string& filepath);
std::string get_type_name(const rapidjson::Document& document, 
                          const char* type_id);
void dump_member(const rapidjson::Document& document,   
                 const rapidjson::Value& member, uint64_t base_offset, 
                 unsigned int deep_size);
void dump_parent(const rapidjson::Document& document, const char* parent_id, 
                 uint64_t base_offset, unsigned int deep_size);
void dump_members(const rapidjson::Document& document, 
                  const rapidjson::Value& current_element, 
                  const rapidjson::Value& members, uint64_t base_offset, 
                  unsigned int deep_size);