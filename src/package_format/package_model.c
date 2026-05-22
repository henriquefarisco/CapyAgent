#include "package_model.h"

static void capy_pkg_memzero(void *ptr, size_t len) {
  uint8_t *p = (uint8_t *)ptr;
  while (len--) {
    *p++ = 0;
  }
}

static int capy_pkg_streq(const char *a, const char *b) {
  if (!a || !b) {
    return 0;
  }
  while (*a && *b) {
    if (*a != *b) {
      return 0;
    }
    ++a;
    ++b;
  }
  return *a == *b;
}

static struct capy_package_info *capy_pkg_find_installed(
    struct capy_package_db *db, const char *name) {
  if (!db || !name) {
    return 0;
  }
  for (uint32_t i = 0; i < db->installed_count; ++i) {
    if (capy_pkg_streq(db->installed[i].name, name)) {
      return &db->installed[i];
    }
  }
  return 0;
}

static struct capy_package_info *capy_pkg_find_available(
    struct capy_package_db *db, const char *name) {
  if (!db || !name) {
    return 0;
  }
  for (uint32_t i = 0; i < db->available_count; ++i) {
    if (capy_pkg_streq(db->available[i].name, name)) {
      return &db->available[i];
    }
  }
  return 0;
}

static const struct capy_package_info *capy_pkg_find_installed_const(
    const struct capy_package_db *db, const char *name) {
  if (!db || !name) {
    return 0;
  }
  for (uint32_t i = 0; i < db->installed_count; ++i) {
    if (capy_pkg_streq(db->installed[i].name, name)) {
      return &db->installed[i];
    }
  }
  return 0;
}

static const struct capy_package_info *capy_pkg_find_available_const(
    const struct capy_package_db *db, const char *name) {
  if (!db || !name) {
    return 0;
  }
  for (uint32_t i = 0; i < db->available_count; ++i) {
    if (capy_pkg_streq(db->available[i].name, name)) {
      return &db->available[i];
    }
  }
  return 0;
}

void capy_package_db_init(struct capy_package_db *db) {
  if (!db) {
    return;
  }
  capy_pkg_memzero(db, sizeof(*db));
}

int capy_package_db_add_available(struct capy_package_db *db,
                                  const struct capy_package_info *pkg) {
  if (!db || !pkg || !pkg->name[0] ||
      db->available_count >= CAPY_PACKAGE_MAX_AVAILABLE) {
    return -1;
  }
  if (capy_pkg_find_available(db, pkg->name)) {
    return 0;
  }
  db->available[db->available_count++] = *pkg;
  db->available[db->available_count - 1u].state = CAPY_PACKAGE_STATE_AVAILABLE;
  return 0;
}

int capy_package_db_add_installed(struct capy_package_db *db,
                                  const struct capy_package_info *pkg) {
  if (!db || !pkg || !pkg->name[0] ||
      db->installed_count >= CAPY_PACKAGE_MAX_INSTALLED) {
    return -1;
  }
  if (capy_pkg_find_installed(db, pkg->name)) {
    return 0;
  }
  db->installed[db->installed_count++] = *pkg;
  db->installed[db->installed_count - 1u].state = CAPY_PACKAGE_STATE_INSTALLED;
  return 0;
}

static int capy_package_install_depth(struct capy_package_db *db,
                                      const char *name, uint32_t depth) {
  struct capy_package_info *available;
  if (!db || !name) {
    return -1;
  }
  if (depth > CAPY_PACKAGE_MAX_AVAILABLE) {
    return -1;
  }
  if (capy_pkg_find_installed(db, name)) {
    return 0;
  }
  available = capy_pkg_find_available(db, name);
  if (!available) {
    return -1;
  }
  for (uint32_t i = 0; i < available->dep_count; ++i) {
    if (!capy_pkg_find_installed(db, available->deps[i])) {
      if (capy_package_install_depth(db, available->deps[i],
                                     depth + 1u) != 0) {
        return -1;
      }
    }
  }
  return capy_package_db_add_installed(db, available);
}

int capy_package_install(struct capy_package_db *db, const char *name) {
  return capy_package_install_depth(db, name, 0u);
}

int capy_package_remove(struct capy_package_db *db, const char *name) {
  if (!db || !name) {
    return -1;
  }
  for (uint32_t i = 0; i < db->installed_count; ++i) {
    if (capy_pkg_streq(db->installed[i].name, name)) {
      for (uint32_t j = i; j + 1u < db->installed_count; ++j) {
        db->installed[j] = db->installed[j + 1u];
      }
      --db->installed_count;
      return 0;
    }
  }
  return -1;
}

int capy_package_update(struct capy_package_db *db, const char *name) {
  struct capy_package_info *installed;
  struct capy_package_info *available;
  if (!db || !name) {
    return -1;
  }
  installed = capy_pkg_find_installed(db, name);
  available = capy_pkg_find_available(db, name);
  if (!installed || !available) {
    return -1;
  }
  *installed = *available;
  installed->state = CAPY_PACKAGE_STATE_INSTALLED;
  return 0;
}

int capy_package_update_all(struct capy_package_db *db) {
  int updated = 0;
  if (!db) {
    return -1;
  }
  for (uint32_t i = 0; i < db->installed_count; ++i) {
    struct capy_package_info *available =
        capy_pkg_find_available(db, db->installed[i].name);
    if (available && !capy_pkg_streq(available->version,
                                     db->installed[i].version)) {
      if (capy_package_update(db, db->installed[i].name) == 0) {
        ++updated;
      }
    }
  }
  return updated;
}

int capy_package_info_get(const struct capy_package_db *db, const char *name,
                          struct capy_package_info *out) {
  const struct capy_package_info *pkg;
  if (!db || !name || !out) {
    return -1;
  }
  pkg = capy_pkg_find_installed_const(db, name);
  if (!pkg) {
    pkg = capy_pkg_find_available_const(db, name);
  }
  if (!pkg) {
    return -1;
  }
  *out = *pkg;
  return 0;
}

int capy_package_check_deps(const struct capy_package_db *db,
                            const char *name) {
  const struct capy_package_info *pkg;
  if (!db || !name) {
    return -1;
  }
  pkg = capy_pkg_find_available_const(db, name);
  if (!pkg) {
    pkg = capy_pkg_find_installed_const(db, name);
  }
  if (!pkg) {
    return -1;
  }
  for (uint32_t i = 0; i < pkg->dep_count; ++i) {
    if (!capy_pkg_find_installed_const(db, pkg->deps[i])) {
      return -1;
    }
  }
  return 0;
}

void capy_package_stats_get(const struct capy_package_db *db,
                            struct capy_package_stats *out) {
  if (!db || !out) {
    return;
  }
  capy_pkg_memzero(out, sizeof(*out));
  out->installed_count = db->installed_count;
  out->available_count = db->available_count;
  for (uint32_t i = 0; i < db->installed_count; ++i) {
    const struct capy_package_info *available;
    out->total_installed_bytes += db->installed[i].size_bytes;
    available = capy_pkg_find_available_const(db, db->installed[i].name);
    if (available && !capy_pkg_streq(available->version,
                                     db->installed[i].version)) {
      ++out->updates_available;
    }
  }
}
