#include <iostream>
#include <thread>
#include <chrono>

#include "bitmap.h"

struct SomeData {
  int en018;
  float fp020;
};

int main() {

  try {
    Bitmap db("/ram/URAL");
    SomeData someData;
    int lt010{};

    db.reg("EN018", &someData.en018, sizeof(someData.en018), AccessType::kReadWrite);
    db.reg("FP020", &someData.fp020, sizeof(someData.fp020), AccessType::kReadWrite);
    db.reg("LT010", &lt010, sizeof(lt010), AccessType::kReadWrite);

    for (;;) {
      db.read();
//      std::cout << "EN018 :: " << someData.en018 << std::endl;
//      std::cout << "FP020 :: " << someData.fp020 << std::endl;
//      std::cout << "LT010 :: " << lt010 << std::endl;
      someData.en018 += 1;
      lt010 += 2;
      someData.fp020 += 0.1;
      db.write();
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

  } catch (std::exception& ex) {
    std::cout << ex.what() << std::endl;
  }

  return 0;
}
