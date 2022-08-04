#include "bitmap.h"

#include <iostream>
#include <algorithm>

#include "sqlite3.h"

using std::string;
using std::vector;
using std::runtime_error;
using std::logic_error;
using std::mutex;
using std::lock_guard;
using std::unique_ptr;

using SQLite::Statement;
using SQLite::Transaction;
using SQLite::Database;

Bitmap::Resource::Resource(const Database& db, const string& query, const string& name, void* addr, ResourceType type, std::size_t size) :
  statement(db, query),
  name(name),
  addr(addr),
  type(type),
  size(size)
{
}

Bitmap::Resource::Resource(const SQLite::Database& db, const string& query, const string& name, void* addr, ResourceType type, int bit) :
  statement(db, query),
  name(name),
  addr(addr),
  type(type),
  bit(bit)
{
}

Bitmap::Bitmap() {
  setTypes();
}

Bitmap::Bitmap(const string& path) :
  m_db(unique_ptr<Database>(new Database(path, SQLite::OPEN_READWRITE)))
{
  setTypes();
}

void Bitmap::create(const string& path, const string& query) {
  Database db(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
  db.exec(query);
}

void Bitmap::open(const string& path) {
  m_db = unique_ptr<Database>(new Database(path, SQLite::OPEN_READWRITE));
}

void Bitmap::reg(const string& name, void* addr, std::size_t size, AccessType accessType) {
  for (const auto& table : tables()) {
    Statement typeQuery(*m_db, "SELECT type FROM " + table + " WHERE name = '" + name + "'");
    if (typeQuery.executeStep()) {
      _regImpl(table, name, typeQuery.getColumn("type").getString(), addr, size, accessType);
      return;
    }
  }
  throw runtime_error("Bitmap registration failure: resource '" + name + "' was not found");
}

void Bitmap::regBit(const string& name, void* addr, int bit, AccessType accessType) {
  for (const auto& table : tables()) {
    Statement typeQuery(*m_db, "SELECT type FROM " + table + " WHERE name = '" + name + "'");
    if (typeQuery.executeStep()) {
      _regBitImpl(table, name, typeQuery.getColumn("type").getString(), addr, bit, accessType);
      return;
    }
  }
  throw runtime_error("Bitmap registration failure: resource '" + name + "' was not found");
}

void Bitmap::regAtTable(const string& table, const string& name, void* addr, std::size_t size, AccessType accessType) {
  Statement typeQuery(*m_db, "SELECT type FROM " + table + " WHERE name = '" + name + "'");
  if (typeQuery.executeStep()) {
    _regImpl(table, name, typeQuery.getColumn("type").getString(), addr, size, accessType);
  } else {
    throw runtime_error("Bitmap registration failure: resource '" + name + "' was not found");
  }
}

void Bitmap::regBitAtTable(const string& table, const string& name, void* addr, int bit, AccessType accessType) {
  Statement typeQuery(*m_db, "SELECT type FROM " + table + " WHERE name = '" + name + "'");
  if (typeQuery.executeStep()) {
    _regBitImpl(table, name, typeQuery.getColumn("type").getString(), addr, bit, accessType);
  } else {
    throw runtime_error("Bitmap registration failure: resource '" + name + "' was not found");
  }
}

int Bitmap::readInt(const string& name, bool& ok) {

}

unsigned int Bitmap::readUInt(const string& name, bool& ok) {

}

double Bitmap::readDouble(const string& name, bool& ok) {

}

int Bitmap::readIntFromTable(const string& name, const string& tableName, bool& ok) {
  Statement query(*m_db, "SELECT i FROM " + tableName + " WHERE name = '" + name + "'");
  ok = executeStatement(&query, WaitPolicy::kReturnImmediately);
  if (!ok) {
    return {};
  }
  return query.getColumn("i").getInt();
}

unsigned int Bitmap::readUIntFromTable(const string& name, const string& tableName, bool& ok) {
  Statement query(*m_db, "SELECT i FROM " + tableName + " WHERE name = '" + name + "'");
  ok = executeStatement(&query, WaitPolicy::kReturnImmediately);
  if (!ok) {
    return {};
  }
  return query.getColumn("i").getUInt();
}

double Bitmap::readDoubleFromTable(const string& name, const string& tableName, bool& ok) {
  Statement query(*m_db, "SELECT r FROM " + tableName + " WHERE name = '" + name + "'");
  ok = executeStatement(&query, WaitPolicy::kReturnImmediately);
  if (!ok) {
    return {};
  }
  return query.getColumn("r").getDouble();
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
    if (!executeStatement(&res.statement)) {
      continue;
    }
    switch (res.type) {
      case ResourceType::kBit:
        memcpy(&i8, res.addr, sizeof(i8));
        if (res.statement.getColumn("i").getInt()) {
          i8 |= (1 << res.bit);
        } else {
          i8 &= ~(1 << res.bit);
        }
        memcpy(res.addr, &i8, sizeof(i8));
        break;
      case ResourceType::kInt8:
        i8 = static_cast<int8_t>(res.statement.getColumn("i").getInt());
        memcpy(res.addr, &i8, sizeof(i8));
        break;
      case ResourceType::kInt16:
        i16 = static_cast<int16_t>(res.statement.getColumn("i").getInt());
        memcpy(res.addr, &i16, sizeof(i16));
        break;
      case ResourceType::kInt32:
        i32 = res.statement.getColumn("i").getInt();
        memcpy(res.addr, &i32, sizeof(i32));
        break;
      case ResourceType::kUInt:
        u = res.statement.getColumn("i").getUInt();
        memcpy(res.addr, &u, res.size);
        break;
      case ResourceType::kFloat:
        f32 = static_cast<float>(res.statement.getColumn("r").getDouble());
        memcpy(res.addr, &f32, sizeof(f32));
        break;
      case ResourceType::kDouble:
        f64 = res.statement.getColumn("r").getDouble();
        memcpy(res.addr, &f64, sizeof(f64));
        break;
      case ResourceType::kBLOB:
        memcpy(res.addr, res.statement.getColumn("b").getBlob(), res.statement.getColumn("b").getBytes());
        break;
    }
    res.statement.tryReset();
  }
}

void Bitmap::write() {
  static int32_t  i32;
  static int16_t  i16;
  static int8_t   i8;
  static uint32_t u;
  static float    f32;
  static double   f64;

  lock_guard<mutex> lg(lock());
//  Transaction transaction(*m_db);
  for (auto& res : m_resOnWrite) {
    switch (res.type) {
      case ResourceType::kBit:
        memcpy(&i8, res.addr, sizeof(i8));
        i8 = i8 & (1 << res.bit);
        res.statement.bind(1, i8);
        break;
      case ResourceType::kInt8:
        memcpy(&i8, res.addr, sizeof(i8));
        res.statement.bind(1, i8);
        break;
      case ResourceType::kInt16:
        memcpy(&i16, res.addr, sizeof(i16));
        res.statement.bind(1, i16);
        break;
      case ResourceType::kInt32:
        memcpy(&i32, res.addr, sizeof(i32));
        res.statement.bind(1, i32);
        break;
      case ResourceType::kUInt:
        memcpy(&u, res.addr, sizeof(u));
        res.statement.bind(1, u);
        break;
      case ResourceType::kFloat:
        memcpy(&f32, res.addr, sizeof(f32));
        res.statement.bind(1, f32);
        break;
      case ResourceType::kDouble:
        memcpy(&f64, res.addr, sizeof(f64));
        res.statement.bind(1, f64);
        break;
      case ResourceType::kBLOB:
        res.statement.bindNoCopy(1, res.addr, res.size);
        break;
    }
    executeStatement(&res.statement);
    res.statement.tryReset();
  }
//  transaction.commit();
}

vector<string> Bitmap::tables() {
  vector<string> res;
  string tableName;
  Statement tableQuery(*m_db, "SELECT name FROM sqlite_master WHERE type = 'table' ORDER BY 1");

  while (tableQuery.executeStep()) {
    tableName = tableQuery.getColumn("name").getString();
    if (tableName != "sqlite_sequence") {
      res.push_back(tableName);
    }
  }
  return res;
}

vector<Bitmap::ResourceInfo> Bitmap::resources(const string& table) {
  vector<Bitmap::ResourceInfo> res;
  ResourceInfo info;
  Statement query(*m_db, "SELECT * FROM " + table);

  while (query.executeStep()) {
    info.name = query.getColumn("name").getString();
    info.type = m_types[query.getColumn("type").getString()];
    info.rclass = query.getColumn("class").getString();
    info.alias = query.getColumn("alias").getString();
    info.subclass = query.getColumn("subclass").getString();
    if (info.type == ResourceType::kBLOB) {
      info.size = query.getColumn("b").getBytes();
    } else {
      info.size = m_typeSizes[info.type];
    }
    res.push_back(info);
  }

  return res;
}

mutex& Bitmap::lock() {
  static mutex _lock;
  return _lock;
}

void Bitmap::setTypes() {
  m_types.insert(std::make_pair("bit",            ResourceType::kBit));
  m_types.insert(std::make_pair("char",           ResourceType::kInt8));
  m_types.insert(std::make_pair("int8_t",         ResourceType::kInt8));
  m_types.insert(std::make_pair("short",          ResourceType::kInt16));
  m_types.insert(std::make_pair("int16_t",        ResourceType::kInt16));
  m_types.insert(std::make_pair("int",            ResourceType::kInt32));
  m_types.insert(std::make_pair("int32_t",        ResourceType::kInt32));
  m_types.insert(std::make_pair("unsigned char",  ResourceType::kUInt));
  m_types.insert(std::make_pair("uint8_t",        ResourceType::kUInt));
  m_types.insert(std::make_pair("unsigned short", ResourceType::kUInt));
  m_types.insert(std::make_pair("uint16_t",       ResourceType::kUInt));
  m_types.insert(std::make_pair("unsigned int",   ResourceType::kUInt));
  m_types.insert(std::make_pair("uint32_t",       ResourceType::kUInt));
  m_types.insert(std::make_pair("float",          ResourceType::kFloat));
  m_types.insert(std::make_pair("double",         ResourceType::kDouble));
  m_types.insert(std::make_pair("blob",           ResourceType::kBLOB));

  m_typeSizes.insert(std::make_pair(ResourceType::kBit,    0));
  m_typeSizes.insert(std::make_pair(ResourceType::kInt8,   sizeof(int8_t)));
  m_typeSizes.insert(std::make_pair(ResourceType::kInt16,  sizeof(int16_t)));
  m_typeSizes.insert(std::make_pair(ResourceType::kInt32,  sizeof(int32_t)));
  m_typeSizes.insert(std::make_pair(ResourceType::kUInt,   sizeof(uint_t)));
  m_typeSizes.insert(std::make_pair(ResourceType::kFloat,  sizeof(float)));
  m_typeSizes.insert(std::make_pair(ResourceType::kDouble, sizeof(double)));
  m_typeSizes.insert(std::make_pair(ResourceType::kBLOB,   0));
}

string Bitmap::query(AccessType accessType, const string& table, const string& name, ResourceType resType) {
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

string Bitmap::field(ResourceType type) {
  string res;
  switch (type) {
    case ResourceType::kBit:
    case ResourceType::kInt8:
    case ResourceType::kInt16:
    case ResourceType::kInt32:
    case ResourceType::kUInt:
      res = "i";
      break;
    case ResourceType::kFloat:
    case ResourceType::kDouble:
      res = "r";
      break;
    case ResourceType::kBLOB:
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

  ResourceType type = m_types[resType];
  if (type == ResourceType::kBit && size) {
    type = ResourceType::kInt8;
  }

  if (type != ResourceType::kUInt && size < m_typeSizes[type]) {
    std::cerr << "Bitmap registration warning: resource '" << name << "' has a size bigger than "
              << size << " bytes" << std::endl;
    return;
  }

  switch (accessType) {
    case AccessType::kRead:
      m_resOnRead.emplace_back(Resource(*m_db, query(accessType, table, name, type), name, addr, type, size));
      break;
    case AccessType::kWrite:
      if (!resourceHasBeenRegistered(name, m_resOnWrite)) {
        m_resOnWrite.emplace_back(Resource(*m_db, query(accessType, table, name, type), name, addr, type, size));
      } else {
        std::cerr << "Bitmap registration warning: resource '" << name << "' has already been registered on write" << std::endl;
      }
      break;
    case AccessType::kReadWrite:
      m_resOnRead.push_back(Resource(*m_db, query(AccessType::kRead, table, name, type), name, addr, type, size));
      if (!resourceHasBeenRegistered(name, m_resOnWrite)) {
        m_resOnWrite.push_back(Resource(*m_db, query(AccessType::kWrite, table, name, type), name, addr, type, size));
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

  ResourceType type = m_types[resType];
  switch (accessType) {
    case AccessType::kRead:
      m_resOnRead.emplace_back(Resource(*m_db, query(accessType, table, name, type), name, addr, type, bit));
      break;
    case AccessType::kWrite:
      if (!resourceHasBeenRegistered(name, m_resOnWrite)) {
        m_resOnWrite.emplace_back(Resource(*m_db, query(accessType, table, name, type), name, addr, type, bit));
      } else {
        std::cerr << "Bitmap registration warning: resource '" << name << "' has already been registered on write" << std::endl;
      }
      break;
    case AccessType::kReadWrite:
      m_resOnRead.push_back(Resource(*m_db, query(AccessType::kRead, table, name, type), name, addr, type, bit));
      if (!resourceHasBeenRegistered(name, m_resOnWrite)) {
        m_resOnWrite.push_back(Resource(*m_db, query(AccessType::kWrite, table, name, type), name, addr, type, bit));
      } else {
        std::cerr << "Bitmap registration warning: resource '" << name << "' has already been registered on write" << std::endl;
      }
      break;
    }
}

bool Bitmap::executeStatement(SQLite::Statement *statement, WaitPolicy policy) {
  int errCode = 0;
  bool done = false, locked = false;

  do {
    errCode = statement->tryExecuteStep();
    done = errCode == SQLITE_DONE || errCode == SQLITE_ROW;
    locked = errCode == SQLITE_BUSY || errCode == SQLITE_LOCKED;

    if (policy == WaitPolicy::kWaitUntilUnlocked && locked) {
      statement->tryReset();
    } else if (!done) {
      if (!locked) {
        std::cerr << "Bitmap execute statement failure: " << sqlite3_errstr(errCode) << "\n";
      }
      return false;
    }
  } while (!done);
  return true;
}
