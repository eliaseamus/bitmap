#include "viewer.h"

#include <string>
#include <stdexcept>
#include <iostream>

using std::string;
using std::vector;
using std::exception;
using std::unique_ptr;
using std::thread;

Viewer::ResourceView::ResourceView(const Bitmap::ResourceInfo& info) :
  info(info)
{
}

Viewer::TableView::TableView(const string& name, QAbstractItemModel* model, std::vector<ResourceView>&& resources) :
  name(name),
  model(model),
  resources(resources)
{
}

Viewer::Viewer(std::string basePath, QWidget* parent) : QWidget(parent) {
  if (basePath.empty()) {
    basePath = QFileDialog::getOpenFileName().toStdString();
  }

  try {
    if (basePath.empty()) {
      QMessageBox::warning(nullptr, "Bitmap open failure", "name of the database wasn't set");
    } else {
      m_db.open(basePath);
      buildBitmapTree();
    }
  } catch (exception& ex) {
    QMessageBox::warning(nullptr, "Bitmap open failure", ex.what());
  }

  connect(this, &Viewer::newIntValue, this, &Viewer::updateIntValue);
  connect(this, &Viewer::newUIntValue, this, &Viewer::updateUIntValue);
  connect(this, &Viewer::newDoubleValue, this, &Viewer::updateDoubleValue);
}

void Viewer::buildBitmapTree() {
  QVBoxLayout* appLayout = new QVBoxLayout;

  for (const auto& table : m_db.tables()) {
    QLabel* bitmapName = new QLabel(QString::fromStdString(table), this);
    QTableWidget* bitmap = new QTableWidget(this);
    bitmap->verticalHeader()->setVisible(false);
    bitmap->horizontalHeader()->setVisible(false);
    bitmap->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    bitmap->setColumnCount(2);
    vector<ResourceView> resViews;

    for (const auto& resource : m_db.resources(table)) {
      QTableWidgetItem* name = new QTableWidgetItem(QString::fromStdString(resource.name));
      name->setFlags(Qt::NoItemFlags);
      name->setToolTip(QString::fromStdString(resource.alias));
      name->setBackgroundColor(resourceColor(resource.rclass));
      name->setTextColor(Qt::black);

      QTableWidgetItem* value = new QTableWidgetItem;
      value->setFlags(Qt::NoItemFlags);
      value->setTextColor(Qt::black);
      value->setTextAlignment(Qt::AlignRight);

      bitmap->insertRow(bitmap->rowCount());
      bitmap->setItem(bitmap->rowCount() - 1, 0, name);
      bitmap->setItem(bitmap->rowCount() - 1, 1, value);

      resViews.emplace_back(ResourceView(resource));
    }

    appLayout->addWidget(bitmapName);
    appLayout->addWidget(bitmap);
    m_tableViews.emplace_back(TableView(table, bitmap->model(), std::move(resViews)));
  }

  setLayout(appLayout);
}

void Viewer::run() {
  std::thread t(&Viewer::runThread, this);
  t.detach();
}

void Viewer::updateBitmapTree() {
  static bool ok = false;
  static int i = 0;
  static unsigned int u = 0;
  static double d = 0;

  for (const auto& table : m_tableViews) {
    int row = 0;
    for (const auto& res : table.resources) {
      switch (res.info.type) {
        case Bitmap::ResourceType::kBit:
          i = (bool)m_db.readIntFromTable(res.info.name, table.name, ok);
          if (ok) {
            emit newIntValue(table.model, table.model->index(row, 1), i);
          }
          break;
        case Bitmap::ResourceType::kInt8:
        case Bitmap::ResourceType::kInt16:
        case Bitmap::ResourceType::kInt32:
          i = m_db.readIntFromTable(res.info.name, table.name, ok);
          if (ok) {
            emit newIntValue(table.model, table.model->index(row, 1), i);
          }
          break;
        case Bitmap::ResourceType::kUInt:
          u = m_db.readUIntFromTable(res.info.name, table.name, ok);
          if (ok) {
            emit newUIntValue(table.model, table.model->index(row, 1), u);
          }
          break;
        case Bitmap::ResourceType::kFloat:
        case Bitmap::ResourceType::kDouble:
          d = m_db.readDoubleFromTable(res.info.name, table.name, ok);
          if (ok) {
            emit newDoubleValue(table.model, table.model->index(row, 1), d);
          }
          break;
        case Bitmap::ResourceType::kBLOB:
          break;
      }
      ++row;
    }
    table.model->submit();
  }
}

void Viewer::runThread() {
  for(;;) {
    updateBitmapTree();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
}

QColor Viewer::resourceColor(const string& rclass) {
  QColor res;

  if (rclass == "INPUTS") {
    res = Qt::green;
  } else if (rclass == "OUTPUTS") {
    res = Qt::blue;
  } else if (rclass == "USERS") {
    res = Qt::darkYellow;
  } else {
    res = Qt::lightGray;
  }

  return res;
}

void Viewer::updateIntValue(QAbstractItemModel* model, const QModelIndex& index, int value) {
  model->setData(index, value);
}

void Viewer::updateUIntValue(QAbstractItemModel* model, const QModelIndex& index, unsigned int value) {
  model->setData(index, value);
}

void Viewer::updateDoubleValue(QAbstractItemModel* model, const QModelIndex& index, double value) {
  model->setData(index, value);
}
