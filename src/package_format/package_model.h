#ifndef CAPY_AGENT_PACKAGE_MODEL_H
#define CAPY_AGENT_PACKAGE_MODEL_H

#include <stddef.h>
#include <stdint.h>

#define CAPY_PACKAGE_NAME_MAX 64
#define CAPY_PACKAGE_VERSION_MAX 32
#define CAPY_PACKAGE_DESC_MAX 256
#define CAPY_PACKAGE_PATH_MAX 128
#define CAPY_PACKAGE_MAX_DEPS 8
#define CAPY_PACKAGE_MAX_INSTALLED 64
#define CAPY_PACKAGE_MAX_AVAILABLE 128

enum capy_package_state {
  CAPY_PACKAGE_STATE_AVAILABLE = 0,
  CAPY_PACKAGE_STATE_DOWNLOADING,
  CAPY_PACKAGE_STATE_STAGED,
  CAPY_PACKAGE_STATE_INSTALLING,
  CAPY_PACKAGE_STATE_INSTALLED,
  CAPY_PACKAGE_STATE_REMOVING,
  CAPY_PACKAGE_STATE_BROKEN
};

struct capy_package_info {
  char name[CAPY_PACKAGE_NAME_MAX];
  char version[CAPY_PACKAGE_VERSION_MAX];
  char description[CAPY_PACKAGE_DESC_MAX];
  char path[CAPY_PACKAGE_PATH_MAX];
  enum capy_package_state state;
  uint32_t size_bytes;
  uint32_t checksum;
  char deps[CAPY_PACKAGE_MAX_DEPS][CAPY_PACKAGE_NAME_MAX];
  uint32_t dep_count;
  uint64_t installed_tick;
};

struct capy_package_db {
  struct capy_package_info installed[CAPY_PACKAGE_MAX_INSTALLED];
  uint32_t installed_count;
  struct capy_package_info available[CAPY_PACKAGE_MAX_AVAILABLE];
  uint32_t available_count;
};

struct capy_package_stats {
  uint32_t installed_count;
  uint32_t available_count;
  uint32_t updates_available;
  uint64_t total_installed_bytes;
};

void capy_package_db_init(struct capy_package_db *db);
int capy_package_db_add_available(struct capy_package_db *db,
                                  const struct capy_package_info *pkg);
int capy_package_db_add_installed(struct capy_package_db *db,
                                  const struct capy_package_info *pkg);
int capy_package_install(struct capy_package_db *db, const char *name);
int capy_package_remove(struct capy_package_db *db, const char *name);
int capy_package_update(struct capy_package_db *db, const char *name);
int capy_package_update_all(struct capy_package_db *db);
int capy_package_info_get(const struct capy_package_db *db, const char *name,
                          struct capy_package_info *out);
int capy_package_check_deps(const struct capy_package_db *db,
                            const char *name);
void capy_package_stats_get(const struct capy_package_db *db,
                            struct capy_package_stats *out);

#endif
