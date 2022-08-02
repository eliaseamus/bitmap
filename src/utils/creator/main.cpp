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
  string path, scriptPath;
  int opt;
  while ((opt = getopt(argc, argv, "p:P:s:S:")) != EOF) {
    switch (tolower(opt)) {
      case 'p':
        path = optarg;
        break;
      case 's':
        scriptPath = optarg;
        break;
    }
  }

  try {
    if (path.empty()) {
      throw runtime_error("path of the database must be set via -p argument");
    } else if (scriptPath.empty()) {
      throw runtime_error("SQL script creating tables must be set via -s argument");
    }
    ifstream scriptFile(scriptPath);
    std::stringstream script;
    script << scriptFile.rdbuf();
    scriptFile.close();
    Bitmap::create(path, script.str());
  } catch (exception& ex) {
    std::cerr << ex.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
