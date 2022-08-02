#ifndef BITMAP_H
#define BITMAP_H

#include <vector>
#include <unordered_map>
#include <mutex>

#include "SQLiteCpp.h"

enum class AccessType : uint8_t {kRead, kWrite, kReadWrite};

class Bitmap {
private:
  struct Resource {
    enum class Type {
      kBit    = 0x0000,
      kInt8   = 0x0001,
      kInt16  = 0x0002,
      kInt32  = 0x0004,
      kUInt   = 0x0010,
      kFloat  = 0x0020,
      kDouble = 0x0040,
      kBLOB   = 0x0100
    };

    Resource(const SQLite::Database& db, const std::string& query, const std::string& name, void* addr, Type type, std::size_t size);
    Resource(const SQLite::Database& db, const std::string& query, const std::string& name, void* addr, Type type, int bit);
    SQLite::Statement statement;
    std::string name;
    void* addr{};
    Type type;
    std::size_t size{};
    int bit{};
  };

private:
  SQLite::Database m_db;
  std::unordered_map<std::string, Resource::Type> m_types;
  std::map<Resource::Type, std::size_t> m_typeSizes;
  std::vector<Resource> m_resOnRead;
  std::vector<Resource> m_resOnWrite;

public:

  Bitmap(const std::string& path);

  static void create(const std::string& path, const std::string& query);

  void reg(const std::string& name, void* addr, std::size_t size, AccessType accessType);

  void regBit(const std::string& name, void* addr, int bit, AccessType accessType);

  void regAtTable(const std::string& table, const std::string& name, void* addr, std::size_t size, AccessType accessType);

  void regBitAtTable(const std::string& table, const std::string& name, void* addr, int bit, AccessType accessType);

  void read();

  void write();

  std::vector<std::string> tables();

  static std::mutex& lock();

private:

  void setTypes();

  std::string query(AccessType accessType, const std::string& table, const std::string& name, Resource::Type resType);

  std::string field(Resource::Type type);

  bool resourceHasBeenRegistered(const std::string& name, const std::vector<Resource>& collection);

  void _regImpl(const std::string& table, const std::string& name, const std::string& resType, void* addr, std::size_t size, AccessType accessType);

  void _regBitImpl(const std::string& table, const std::string& name, const std::string& resType, void* addr, int bit, AccessType accessType);
};

#endif // BITMAP_H
