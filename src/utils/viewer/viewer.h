#ifndef VIEWER_H
#define VIEWER_H

#include <string>
#include <memory>
#include <unordered_map>
#include <thread>

#include <QtWidgets>

#include "bitmap.h"

class Viewer : public QWidget {
  Q_OBJECT
private:

  struct ResourceView {
    ResourceView(const Bitmap::ResourceInfo& info);
    Bitmap::ResourceInfo info;
  };

  struct TableView {
    TableView(const std::string& name, QAbstractItemModel* model, std::vector<ResourceView>&& resources);
    std::string name;
    QAbstractItemModel* model;
    std::vector<ResourceView> resources;
  };

private:
  Bitmap m_db;
  std::vector<TableView> m_tableViews;

public:

  Viewer(std::string basePath, QWidget* parent = nullptr);

  void run();

private:

  void buildBitmapTree();

  void updateBitmapTree();

  void runThread();

  QColor resourceColor(const std::string& rclass);

signals:

  void newIntValue(QAbstractItemModel* model, const QModelIndex& index, int value);

  void newUIntValue(QAbstractItemModel* model, const QModelIndex& index, unsigned int value);

  void newDoubleValue(QAbstractItemModel* model, const QModelIndex& index, double value);

private slots:

  void updateIntValue(QAbstractItemModel* model, const QModelIndex& index, int value);

  void updateUIntValue(QAbstractItemModel* model, const QModelIndex& index, unsigned int value);

  void updateDoubleValue(QAbstractItemModel* model, const QModelIndex& index, double value);

};
#endif // VIEWER_H
