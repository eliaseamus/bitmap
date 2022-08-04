#ifndef BITMAP_H
#define BITMAP_H

#include <vector>
#include <unordered_map>
#include <mutex>

#include "SQLiteCpp.h"

enum class AccessType : uint8_t {kRead, kWrite, kReadWrite};

class Bitmap {
public:
  enum class ResourceType {
    kBit    = 0x0000,
    kInt8   = 0x0001,
    kInt16  = 0x0002,
    kInt32  = 0x0004,
    kUInt   = 0x0010,
    kFloat  = 0x0020,
    kDouble = 0x0040,
    kBLOB   = 0x0100
  };

  struct ResourceInfo {
    std::string name;
    ResourceType type;
    std::string rclass;
    std::string alias;
    std::string subclass;
    std::size_t size;
  };

private:
  struct Resource {
    Resource(const SQLite::Database& db, const std::string& query, const std::string& name, void* addr, ResourceType type, std::size_t size);
    Resource(const SQLite::Database& db, const std::string& query, const std::string& name, void* addr, ResourceType type, int bit);
    SQLite::Statement statement;
    std::string name;
    void* addr{};
    ResourceType type;
    std::size_t size{};
    int bit{};
  };

private:
  std::unique_ptr<SQLite::Database> m_db;
  std::unordered_map<std::string, ResourceType> m_types;
  std::map<ResourceType, std::size_t> m_typeSizes;
  std::vector<Resource> m_resOnRead;
  std::vector<Resource> m_resOnWrite;

public:

  Bitmap();

  Bitmap(const std::string& path);

  static void create(const std::string& path, const std::string& query);

  void open(const std::string& path);

  void reg(const std::string& name, void* addr, std::size_t size, AccessType accessType);

  void regBit(const std::string& name, void* addr, int bit, AccessType accessType);

  void regAtTable(const std::string& table, const std::string& name, void* addr, std::size_t size, AccessType accessType);

  void regBitAtTable(const std::string& table, const std::string& name, void* addr, int bit, AccessType accessType);

  int readInt(const std::string& name);

  unsigned int readUInt(const std::string& name);

  double readDouble(const std::string& name);

  int readIntFromTable(const std::string& name, const std::string& tableName);

  unsigned int readUIntFromTable(const std::string& name, const std::string& tableName);

  double readDoubleFromTable(const std::string& name, const std::string& tableName);

  void read();

  void write();

  std::vector<std::string> tables();

  std::vector<ResourceInfo> resources(const std::string& table);

  static std::mutex& lock();

private:

  void setTypes();

  std::string query(AccessType accessType, const std::string& table, const std::string& name, ResourceType resType);

  std::string field(ResourceType type);

  bool resourceHasBeenRegistered(const std::string& name, const std::vector<Resource>& collection);

  void _regImpl(const std::string& table, const std::string& name, const std::string& resType, void* addr, std::size_t size, AccessType accessType);

  void _regBitImpl(const std::string& table, const std::string& name, const std::string& resType, void* addr, int bit, AccessType accessType);
};

#endif // BITMAP_H
