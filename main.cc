#include "main.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"
#include <iostream>
#include <vector>
#include <algorithm>


char* map_file(const std::string& filepath, size_t& filesize) {
  FILE* hfile = fopen(filepath.c_str(), "rb");
  if (!hfile) {
    fprintf(stderr, "ERR: Failed to open '%s'\n", filepath.c_str());
    return nullptr;
  }

  fseek(hfile, 0, SEEK_END);
  filesize = ftell(hfile);
  fseek(hfile, 0, SEEK_SET);

  char* memfile = reinterpret_cast<char*>(malloc(filesize+1));
  size_t nb_read = fread(memfile, filesize, 1, hfile);
  fclose(hfile);
  if (nb_read != 1) {
    fprintf(stderr, "ERR: Failed to read '%s'\n", filepath.c_str());
    return nullptr;
  }

  return reinterpret_cast<char*>(memfile);
}

std::string get_type_name(const rapidjson::Document& document, 
                          const char* type_id) {
  std::string type_name = std::string("#") + type_id;
  if (!document.HasMember(type_id)) {
    return type_name;
  }

  if (document[type_id].HasMember("name")) {
    return std::string(document[type_id]["name"].GetString());
  } 

  if (document[type_id].HasMember("type")) {
    std::string type = document[type_id]["type"].GetString();

    // pointer
    if (type == "pointer" && document[type_id].HasMember("type_id")) {
      type_id = document[type_id]["type_id"].GetString();
      return get_type_name(document, type_id) + "*";
    }

    // const
    if (type == "const" && document[type_id].HasMember("type_id")) {
      type_id = document[type_id]["type_id"].GetString();
      return "const " + get_type_name(document, type_id);
    }

    // array
    if (type == "array" && document[type_id].HasMember("type_id")) {
      std::string suffix = "[]";
      if (document[type_id].HasMember("count")) {
        uint64_t count = document[type_id]["count"].GetUint64();
        suffix = "[" + std::to_string(count) + "]";
      }

      type_id = document[type_id]["type_id"].GetString();
      return get_type_name(document, type_id) + suffix;
    }
  }

  return type_name;
}

void dump_member(const rapidjson::Document& document,   
                 const rapidjson::Value& member, uint64_t base_offset, 
                 unsigned int deep_size) {
  if (!member.HasMember("name")) {
    return; 
  }

  // Dumped format: offset type name
  uint64_t offset = member["offset"].GetUint64() + base_offset;
  printf("    +0x%-3lx ", offset);
  for (size_t j = 0; j < deep_size; j++) {
    printf("    ");
  }

  const char* type_id = "";
  std::string type_name = "";
  if (member.HasMember("type_id")) {
    type_id = member["type_id"].GetString();
    type_name = get_type_name(document, type_id);
  }
  printf("%s %s\n", type_name.c_str(), member["name"].GetString());

  // Show the submembers
  if (document.HasMember(type_id)) {
    if (document[type_id].HasMember("members")) {
      dump_members(document, document[type_id], document[type_id]["members"], 
        offset, deep_size+1);
    }
  }
}

void dump_parent(const rapidjson::Document& document, const char* parent_id, 
                 uint64_t base_offset, unsigned int deep_size) {
  printf("    +0x%-3lx ", base_offset);
  for (size_t j = 0; j < deep_size; j++) {
    printf("    ");
  }

  if (!document.HasMember(parent_id) || 
      !document[parent_id].HasMember("name")) {
    printf(": unknown_parent\n");
    return;
  }

  const char* type = "";
  if (document[parent_id].HasMember("type")) {
    type = document[parent_id]["type"].GetString();
  }

  printf(": %s %s\n", type, document[parent_id]["name"].GetString());

  if (document[parent_id].HasMember("members")) {
    dump_members(document, document[parent_id], document[parent_id]["members"],
      base_offset, deep_size+1);
  }
}

void dump_members(const rapidjson::Document& document, 
                  const rapidjson::Value& current_element, 
                  const rapidjson::Value& members, uint64_t base_offset, 
                  unsigned int deep_size) {
  // Dealing with the parents right now
  struct parent {
    size_t offset;
    const char* id;
  };
  std::vector<parent> parents;
  if (current_element.HasMember("parents")) {
    for (size_t i = 0; i < current_element["parents"].Size(); i++) {
      parents.push_back({
        current_element["parents"][i]["offset"].GetUint(),
        current_element["parents"][i]["id"].GetString()
      });
    }

    std::sort(parents.begin(), parents.end(), [](const parent& a, 
                                                 const parent& b) {
      return a.offset < b.offset;
    });
  }

  for (size_t i = 0; i < members.Size(); i++) {
    if (parents.size() > 0 && members[i].HasMember("offset")) {
      if (parents[0].offset < members[i]["offset"].GetUint()) {
        dump_parent(document, parents[0].id, base_offset + parents[0].offset, 
          deep_size);
        parents.erase(parents.begin());
      }
    }

    dump_member(document, members[i], base_offset, deep_size);
  }
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("Usage: %s <json_path> [object_size]\n", argv[0]);
    printf("       %s <json_path> <min_size> <max_size>\n", argv[0]);
    return 1;
  }

  std::string filepath = argv[1];
  bool search_exact_size = false;
  unsigned int target_object_size = 0;
  bool search_range_size = false;
  unsigned int min_size = 0, max_size = 0;
  if (argc == 3) {
    search_exact_size = true;
    target_object_size = std::stoul(argv[2], nullptr, 0);
  } else if (argc > 3) {
    search_range_size = true;
    min_size = std::stoul(argv[2], nullptr, 0);
    max_size = std::stoul(argv[3], nullptr, 0);
  }

  printf("Mapping the file...");
  fflush(stdout);
  size_t filesize = 0;
  char* memfile = map_file(filepath, filesize);
  if (!memfile) {
    return 2;
  }
  memfile[filesize] = 0;
  printf("done\n");

  printf("Mapping the JSON...");
  fflush(stdout);
  rapidjson::Document document;
  document.Parse(memfile);
  if (document.HasParseError()) {
    printf("parsing error\n");
    printf("Error: %s (%zd)\n", GetParseError_En(document.GetParseError()), 
      document.GetErrorOffset());
    free(memfile);
    return 3;
  }
  printf("done\n");
  free(memfile);

  if (search_exact_size) {
    printf("Searching objects of size 0x%x bytes...\n", target_object_size);
  } else if (search_range_size) {
    printf("Searching objects in the range [0x%x, 0x%x] bytes...\n", 
      min_size, max_size);
  }
  printf("\n--------------------\n");

  size_t nb_found = 0;
  for (rapidjson::Value::ConstMemberIterator itr = document.MemberBegin(); 
       itr != document.MemberEnd(); ++itr) {
    if (!itr->value.HasMember("name") || !itr->value.HasMember("type")
        || !itr->value.HasMember("size")) {
      continue; 
    }

    if (search_exact_size) {
      if (itr->value["size"].GetUint() != target_object_size) {
        continue;
      }
    } else if (search_range_size) {
      if (itr->value["size"].GetUint() < min_size ||
          itr->value["size"].GetUint() > max_size) {
        continue;
      }
    }

    printf("%s %s (0x%x bytes)\n", itr->value["type"].GetString(), 
      itr->value["name"].GetString(), itr->value["size"].GetUint());
    nb_found++;
    
    if (itr->value.HasMember("members")) {
      dump_members(document, itr->value, itr->value["members"], 0, 0);
    }
    printf("--------------------\n");
  }

  printf("\n%zu objects found\n", nb_found);
  return 0;
}