#include "bitmap.h"
#include <iostream>
#include <algorithm>

using std::string;
using std::vector;
using std::runtime_error;
using std::logic_error;
using std::mutex;
using std::lock_guard;

using SQLite::Statement;
using SQLite::Transaction;
using SQLite::Database;

Bitmap::Resource::Resource(const Database& db, const string& query, const string& name, void* addr, Type type, std::size_t size) :
  statement(db, query),
  name(name),
  addr(addr),
  type(type),
  size(size)
{
}

Bitmap::Resource::Resource(const SQLite::Database &db, const string &query, const string& name, void *addr, Type type, int bit) :
  statement(db, query),
  name(name),
  addr(addr),
  type(type),
  bit(bit)
{
}

Bitmap::Bitmap(const string& path) :
  m_db(path, SQLite::OPEN_READWRITE)
{
  setTypes();
}

void Bitmap::create(const string& path, const string& query) {
  Database db(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
  db.exec(query);
}

void Bitmap::reg(const string& name, void* addr, std::size_t size, AccessType accessType) {
  for (const auto& table : tables()) {
    Statement typeQuery(m_db, "SELECT type FROM " + table + " WHERE name = '" + name + "'");
    if (typeQuery.executeStep()) {
      _regImpl(table, name, typeQuery.getColumn("type").getString(), addr, size, accessType);
      break;
    }
  }
  throw runtime_error("Bitmap registration failure: resource '" + name + "' was not found");
}

void Bitmap::regBit(const string& name, void* addr, int bit, AccessType accessType) {
  for (const auto& table : tables()) {
    Statement typeQuery(m_db, "SELECT type FROM " + table + " WHERE name = '" + name + "'");
    if (typeQuery.executeStep()) {
      _regBitImpl(table, name, typeQuery.getColumn("type").getString(), addr, bit, accessType);
      break;
    }
  }
  throw runtime_error("Bitmap registration failure: resource '" + name + "' was not found");
}

void Bitmap::regAtTable(const string& table, const string& name, void* addr, std::size_t size, AccessType accessType) {
  Statement typeQuery(m_db, "SELECT type FROM " + table + " WHERE name = '" + name + "'");
  if (typeQuery.executeStep()) {
    _regImpl(table, name, typeQuery.getColumn("type").getString(), addr, size, accessType);
  } else {
    throw runtime_error("Bitmap registration failure: resource '" + name + "' was not found");
  }
}

void Bitmap::regBitAtTable(const string& table, const string& name, void* addr, int bit, AccessType accessType) {
  Statement typeQuery(m_db, "SELECT type FROM " + table + " WHERE name = '" + name + "'");
  if (typeQuery.executeStep()) {
    _regBitImpl(table, name, typeQuery.getColumn("type").getString(), addr, bit, accessType);
  } else {
    throw runtime_error("Bitmap registration failure: resource '" + name + "' was not found");
  }
}

void Bitmap::read() {
  static int32_t  i32;
  static int16_t  i16;
  static int8_t   i8;
  static uint32_t u;
  static float    f32;
  static double   f64;

  lock_guard<mutex> lg(lock());
  for (auto& res : m_resOnRead) {
    res.statement.executeStep();
    switch (res.type) {
      case Resource::Type::kBit:
        memcpy(&i8, res.addr, sizeof(i8));
        if (res.statement.getColumn("i").getInt()) {
          i8 |= (1 << res.bit);
        } else {
          i8 &= ~(1 << res.bit);
        }
        memcpy(res.addr, &i8, sizeof(i8));
        break;
      case Resource::Type::kInt8:
        i8 = static_cast<int8_t>(res.statement.getColumn("i").getInt());
        memcpy(res.addr, &i8, sizeof(i8));
        break;
      case Resource::Type::kInt16:
        i16 = static_cast<int16_t>(res.statement.getColumn("i").getInt());
        memcpy(res.addr, &i16, sizeof(i16));
        break;
      case Resource::Type::kInt32:
        i32 = res.statement.getColumn("i").getInt();
        memcpy(res.addr, &i32, sizeof(i32));
        break;
      case Resource::Type::kUInt:
        u = static_cast<uint32_t>(res.statement.getColumn("i").getInt());
        memcpy(res.addr, &u, res.size);
        break;
      case Resource::Type::kFloat:
        f32 = static_cast<float>(res.statement.getColumn("r").getDouble());
        memcpy(res.addr, &f32, sizeof(f32));
        break;
      case Resource::Type::kDouble:
        f64 = res.statement.getColumn("r").getDouble();
        memcpy(res.addr, &f64, sizeof(f64));
        break;
      case Resource::Type::kBLOB:
        memcpy(res.addr, res.statement.getColumn("b").getBlob(), res.statement.getColumn("b").getBytes());
        break;
    }
    res.statement.reset();
  }
}

void Bitmap::write() {
  static int32_t  i32;
  static int16_t  i16;
  static int8_t   i8;
  static uint32_t u;
  static float    f32;
  static double   f64;

  Transaction transaction(m_db);
  lock_guard<mutex> lg(lock());
  for (auto& res : m_resOnWrite) {
    switch (res.type) {
      case Resource::Type::kBit:
        memcpy(&i8, res.addr, sizeof(i8));
        i8 = i8 & (1 << res.bit);
        res.statement.bind(1, i8);
        break;
      case Resource::Type::kInt8:
        memcpy(&i8, res.addr, sizeof(i8));
        res.statement.bind(1, i8);
        break;
      case Resource::Type::kInt16:
        memcpy(&i16, res.addr, sizeof(i16));
        res.statement.bind(1, i16);
        break;
      case Resource::Type::kInt32:
        memcpy(&i32, res.addr, sizeof(i32));
        res.statement.bind(1, i32);
        break;
      case Resource::Type::kUInt:
        memcpy(&u, res.addr, sizeof(u));
        res.statement.bind(1, u);
        break;
      case Resource::Type::kFloat:
        memcpy(&f32, res.addr, sizeof(f32));
        res.statement.bind(1, f32);
        break;
      case Resource::Type::kDouble:
        memcpy(&f64, res.addr, sizeof(f64));
        res.statement.bind(1, f64);
        break;
      case Resource::Type::kBLOB:
        res.statement.bindNoCopy(1, res.addr, res.size);
        break;
    }
    res.statement.exec();
    res.statement.reset();
  }
  transaction.commit();
}

vector<string> Bitmap::tables() {
  vector<string> res;
  string tableName;
  Statement tableQuery(m_db, "SELECT name FROM sqlite_master WHERE type = 'table' ORDER BY 1");

  while (tableQuery.executeStep()) {
    tableName = tableQuery.getColumn("name").getString();
    if (tableName != "sqlite_sequence") {
      res.push_back(tableName);
    }
  }
  return res;
}

mutex& Bitmap::lock() {
  static mutex _lock;
  return _lock;
}

void Bitmap::setTypes() {
  m_types.insert(std::make_pair("bit",            Resource::Type::kBit));
  m_types.insert(std::make_pair("char",           Resource::Type::kInt8));
  m_types.insert(std::make_pair("int8_t",         Resource::Type::kInt8));
  m_types.insert(std::make_pair("short",          Resource::Type::kInt16));
  m_types.insert(std::make_pair("int16_t",        Resource::Type::kInt16));
  m_types.insert(std::make_pair("int",            Resource::Type::kInt32));
  m_types.insert(std::make_pair("int32_t",        Resource::Type::kInt32));
  m_types.insert(std::make_pair("unsigned char",  Resource::Type::kUInt));
  m_types.insert(std::make_pair("uint8_t",        Resource::Type::kUInt));
  m_types.insert(std::make_pair("unsigned short", Resource::Type::kUInt));
  m_types.insert(std::make_pair("uint16_t",       Resource::Type::kUInt));
  m_types.insert(std::make_pair("unsigned int",   Resource::Type::kUInt));
  m_types.insert(std::make_pair("uint32_t",       Resource::Type::kUInt));
  m_types.insert(std::make_pair("float",          Resource::Type::kFloat));
  m_types.insert(std::make_pair("double",         Resource::Type::kDouble));
  m_types.insert(std::make_pair("blob",           Resource::Type::kBLOB));

  m_typeSizes.insert(std::make_pair(Resource::Type::kBit,    0));
  m_typeSizes.insert(std::make_pair(Resource::Type::kInt8,   sizeof(int8_t)));
  m_typeSizes.insert(std::make_pair(Resource::Type::kInt16,  sizeof(int16_t)));
  m_typeSizes.insert(std::make_pair(Resource::Type::kInt32,  sizeof(int32_t)));
  m_typeSizes.insert(std::make_pair(Resource::Type::kUInt,   sizeof(uint_t)));
  m_typeSizes.insert(std::make_pair(Resource::Type::kFloat,  sizeof(float)));
  m_typeSizes.insert(std::make_pair(Resource::Type::kDouble, sizeof(double)));
}

string Bitmap::query(AccessType accessType, const string& table, const string& name, Resource::Type resType) {
  string res;
  if (accessType == AccessType::kRead) {
    res = "SELECT " + field(resType) + " FROM " + table + " WHERE name = '" + name + "'";
  } else if (accessType == AccessType::kWrite) {
    res = "UPDATE " + table + " SET " + field(resType) + " = ? WHERE name = '" + name + "'";
  } else {
    throw logic_error("Bitmap building query failure: there is READ or WRITE access modes only");
  }
  return res;
}

string Bitmap::field(Resource::Type type) {
  string res;
  switch (type) {
    case Resource::Type::kBit:
    case Resource::Type::kInt8:
    case Resource::Type::kInt16:
    case Resource::Type::kInt32:
    case Resource::Type::kUInt:
      res = "i";
      break;
    case Resource::Type::kFloat:
    case Resource::Type::kDouble:
      res = "r";
      break;
    case Resource::Type::kBLOB:
      res = "b";
      break;
  }
  return res;
}

bool Bitmap::resourceHasBeenRegistered(const string &name, const vector<Resource>& collection) {
  return std::end(collection) !=
         std::find_if(std::begin(collection), std::end(collection),
                      [&name](const Resource& elem){return elem.name == name;});
}

void Bitmap::_regImpl(const string& table, const string& name, const string& resType, void* addr, std::size_t size, AccessType accessType) {
  if (!m_types.count(resType)) {
    throw runtime_error("Bitmap registration failure: resource '" + name +
                             "' has an unknown type '" + resType + "'");
  }

  Resource::Type type = m_types[resType];
  if (type == Resource::Type::kBit && size) {
    type = Resource::Type::kInt8;
  }

  if (type != Resource::Type::kUInt && size < m_typeSizes[type]) {
    std::cerr << "Bitmap registration warning: resource '" << name << "' has a size bigger than "
              << size << " bytes" << std::endl;
    return;
  }

  switch (accessType) {
    case AccessType::kRead:
      m_resOnRead.emplace_back(Resource(m_db, query(accessType, table, name, type), name, addr, type, size));
      break;
    case AccessType::kWrite:
      if (!resourceHasBeenRegistered(name, m_resOnWrite)) {
        m_resOnWrite.emplace_back(Resource(m_db, query(accessType, table, name, type), name, addr, type, size));
      } else {
        std::cerr << "Bitmap registration warning: resource '" << name << "' has already been registered on write" << std::endl;
      }
      break;
    case AccessType::kReadWrite:
      m_resOnRead.push_back(Resource(m_db, query(AccessType::kRead, table, name, type), name, addr, type, size));
      if (!resourceHasBeenRegistered(name, m_resOnWrite)) {
        m_resOnWrite.push_back(Resource(m_db, query(AccessType::kWrite, table, name, type), name, addr, type, size));
      } else {
        std::cerr << "Bitmap registration warning: resource '" << name << "' has already been registered on write" << std::endl;
      }
      break;
    }
}

void Bitmap::_regBitImpl(const string& table, const std::string& name, const string& resType, void* addr, int bit, AccessType accessType) {
  if (!m_types.count(resType)) {
    throw runtime_error("Bitmap registration failure: resource '" + name +
                             "' has an unknown type '" + resType + "'");
  }

  Resource::Type type = m_types[resType];
  switch (accessType) {
    case AccessType::kRead:
      m_resOnRead.emplace_back(Resource(m_db, query(accessType, table, name, type), name, addr, type, bit));
      break;
    case AccessType::kWrite:
      if (!resourceHasBeenRegistered(name, m_resOnWrite)) {
        m_resOnWrite.emplace_back(Resource(m_db, query(accessType, table, name, type), name, addr, type, bit));
      } else {
        std::cerr << "Bitmap registration warning: resource '" << name << "' has already been registered on write" << std::endl;
      }
      break;
    case AccessType::kReadWrite:
      m_resOnRead.push_back(Resource(m_db, query(AccessType::kRead, table, name, type), name, addr, type, bit));
      if (!resourceHasBeenRegistered(name, m_resOnWrite)) {
        m_resOnWrite.push_back(Resource(m_db, query(AccessType::kWrite, table, name, type), name, addr, type, bit));
      } else {
        std::cerr << "Bitmap registration warning: resource '" << name << "' has already been registered on write" << std::endl;
      }
      break;
    }
}
