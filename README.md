# Description

Designed to work with the project `DWARF Dumper`, it will search objects of
specific size to exploit Use-After-Free faster. It's written in C++ to be able 
to handle large JSON file like the Chromium Dwarf generated file which weight 
more than 3GB.


# Usage

```text
Usage: bin/search <json_path> [object_size]
       bin/search <json_path> <min_size> <max_size>
```


# Example

```text
$ bin/search test.json 0x08 0x40

Mapping the file...done
Mapping the JSON...done
Searching objects in the range [0x8, 0x40] bytes...

--------------------
class classA (0x40 bytes)
    +0x0   __vtbl_ptr_type* _vptr$classA
    +0x8   : class classB
    +0x8       int member0
    +0xc       int member1
    +0x10  int memberZ
    +0x18  classB* ptrToB
    +0x20  classB inlineB
    +0x20      int member0
    +0x24      int member1
    +0x28  int memberD
    +0x2c  int8_t memberB
    +0x2d  char memberC
    +0x30  EnumType enum_member
    +0x34  float[3] float_array
    +0x0   const int kMScaleX
--------------------
class classB (0x8 bytes)
    +0x0   int member0
    +0x4   int member1
--------------------
base sizetype (0x8 bytes)
--------------------
class classB (0x8 bytes)
    +0x0   int member0
    +0x4   int member1
--------------------
```