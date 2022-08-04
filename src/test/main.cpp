#include <iostream>
#include <thread>
#include <chrono>

#include "bitmap.h"

struct SomeData {
  int en018;
  float fp020;
  union {
    struct {
      unsigned char CZ026 : 1;
      unsigned char       : 1;
      unsigned char       : 1;
      unsigned char       : 1;
      unsigned char       : 1;
      unsigned char       : 1;
      unsigned char       : 1;
      unsigned char       : 1;
    };
    unsigned char byte0;
  };
  union {
    struct {
      unsigned char       : 1;
      unsigned char CZ016 : 1;
      unsigned char       : 1;
      unsigned char       : 1;
      unsigned char       : 1;
      unsigned char       : 1;
      unsigned char       : 1;
      unsigned char       : 1;
    };
    unsigned char byte1;
  };
};

struct to_baksan711_t {
  int f1;
  int f2;
  float f3;
};

int main() {

  try {
    Bitmap db("/ram/URAL");
    SomeData someData;
    to_baksan711_t to_baksan711;
    int lt010{};

    memset(&to_baksan711, 0, sizeof(to_baksan711));

    db.reg("TOBAKSAN", &to_baksan711, sizeof(to_baksan711), AccessType::kWrite);
    db.reg("EN018", &someData.en018, sizeof(someData.en018), AccessType::kReadWrite);
    db.reg("FP020", &someData.fp020, sizeof(someData.fp020), AccessType::kReadWrite);
    db.reg("LT010", &lt010, sizeof(lt010), AccessType::kReadWrite);
    db.regBit("CZ026", &someData.byte0, 0, AccessType::kReadWrite);
    db.regBit("CZ016", &someData.byte1, 1, AccessType::kReadWrite);

    for (;;) {
      db.read();
      someData.en018 += 1;
      lt010 += 2;
      someData.fp020 += 0.1;
      someData.CZ026 = !someData.CZ026;
      someData.CZ016 = !someData.CZ026;
      to_baksan711.f1 += 3;
      to_baksan711.f2 += 11;
      to_baksan711.f3 += 0.7;
      db.write();
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

  } catch (std::exception& ex) {
    std::cout << "caught exception :: " << ex.what() << std::endl;
  }

  return 0;
}
