#include <cstdlib>
#include <unistd.h>

#include <string>

#include <QApplication>

#include "viewer.h"

using std::string;

int main(int argc, char* argv[]) {
  string basePath;
  int opt;
  while ((opt = getopt(argc, argv, "b:B:")) != EOF) {
    switch (tolower(opt)) {
      case 'b':
        basePath = optarg;
        break;
    }
  }
  QApplication a(argc, argv);
  Viewer w(basePath);
  w.showMaximized();
  w.run();
  return a.exec();
}
