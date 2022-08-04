#include <cstdlib>
#include <unistd.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "bitmap.h"

using std::string;
using std::ifstream;
using std::exception;
using std::runtime_error;

int main(int argc, char* argv[]) {
  string basePath, scriptPath;
  int opt;
  while ((opt = getopt(argc, argv, "b:B:s:S:")) != EOF) {
    switch (tolower(opt)) {
      case 'b':
        basePath = optarg;
        break;
      case 's':
        scriptPath = optarg;
        break;
    }
  }

  try {
    if (basePath.empty()) {
      throw runtime_error("path of the database must be set via -b argument");
    } else if (scriptPath.empty()) {
      throw runtime_error("SQL script creating tables must be set via -s argument");
    }
    ifstream scriptFile(scriptPath);
    std::stringstream script;
    script << scriptFile.rdbuf();
    scriptFile.close();
    Bitmap::create(basePath, script.str());
  } catch (exception& ex) {
    std::cerr << ex.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
