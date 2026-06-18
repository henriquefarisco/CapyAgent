CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2 -g
CPPFLAGS ?=
LDFLAGS ?=
BUILD_DIR := build
SRC := src/package_format/package_model.c src/update_core/release_manifest.c src/component_index/component_index.c src/component_index/component_plan.c src/component_index/component_manifest.c src/signer/sha512.c src/signer/ed25519.c src/signer/capyagent_signer.c
TEST_SRC := tests/test_agent_contracts.c tests/test_manifest.c tests/test_signer.c
INCLUDES := -Isrc/package_format -Isrc/update_core -Isrc/component_index -Isrc/signer
TEST_BIN := $(BUILD_DIR)/test_agent_contracts

# capypkg packaging (Etapa 9 alpha)
#
# `make package` produces, in build/capypkg/, the artefact and the
# line-oriented manifest that the CapyOS in-tree `services/capypkg`
# adapter consumes. Override PUBLISH_URL_BASE when publishing to a
# different HTTPS endpoint (the default points to the suggested
# GitHub release tag URL).
CAPY_PKG_NAME := org.capyos.agent.core
CAPY_PKG_VERSION := $(shell cat VERSION)
CAPY_PKG_SUMMARY := CapyAgent host-testable package/component models
CAPY_PKG_INSTALL_ROOT := /var/capypkg/$(CAPY_PKG_NAME)
PUBLISH_URL_BASE ?= https://github.com/henriquefarisco/CapyAgent/releases/download/v$(CAPY_PKG_VERSION)
CAPY_PKG_DIR := $(BUILD_DIR)/capypkg
CAPY_PKG_BIN := $(CAPY_PKG_DIR)/$(CAPY_PKG_NAME)-$(CAPY_PKG_VERSION).bin
CAPY_PKG_MANIFEST := $(CAPY_PKG_DIR)/$(CAPY_PKG_NAME).manifest

.PHONY: all clean lint security test validate version-check package package-clean

all: test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TEST_BIN): $(SRC) $(TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDES) $(SRC) $(TEST_SRC) $(LDFLAGS) -o $@
	chmod 755 $@

test: $(TEST_BIN)
	$(TEST_BIN)

lint:
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDES) -fsyntax-only $(SRC)
	git diff --check
	test "$$(cat VERSION)" = "0.0.8"

security:
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDES) -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fPIE -fsyntax-only $(SRC)

version-check:
	test "$$(cat VERSION)" = "0.0.8"
	grep -q "Version: 0.0.8" README.md

validate: lint security test version-check

# package: build the artefact (deterministic tar of src+docs+VERSION),
# compute sha256+size and emit the manifest entry the in-tree adapter
# consumes (see CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md).
package: $(CAPY_PKG_MANIFEST)

$(CAPY_PKG_BIN): $(SRC) | $(BUILD_DIR)
	@mkdir -p $(CAPY_PKG_DIR)
	@tar --format=ustar --owner=0 --group=0 --numeric-owner \
	     --mtime='@0' --sort=name \
	     -cf $@ src docs VERSION 2>/dev/null || \
	  tar -cf $@ src docs VERSION
	@echo "[package] $@"

$(CAPY_PKG_MANIFEST): $(CAPY_PKG_BIN)
	@SHA=$$(shasum -a 256 $(CAPY_PKG_BIN) 2>/dev/null | awk '{print $$1}' | tr 'A-F' 'a-f') ; \
	if [ -z "$$SHA" ]; then SHA=$$(sha256sum $(CAPY_PKG_BIN) | awk '{print $$1}' | tr 'A-F' 'a-f'); fi ; \
	SIZE=$$(wc -c < $(CAPY_PKG_BIN) | tr -d ' ') ; \
	URL="$(PUBLISH_URL_BASE)/$(CAPY_PKG_NAME)-$(CAPY_PKG_VERSION).bin" ; \
	{ \
	  echo "name=$(CAPY_PKG_NAME)" ; \
	  echo "version=$(CAPY_PKG_VERSION)" ; \
	  echo "summary=$(CAPY_PKG_SUMMARY)" ; \
	  echo "payload_url=$$URL" ; \
	  echo "payload_sha256=$$SHA" ; \
	  echo "payload_size=$$SIZE" ; \
	  echo "install_root=$(CAPY_PKG_INSTALL_ROOT)" ; \
	  echo "depends=" ; \
	  echo "---" ; \
	} > $@
	@echo "[package] manifest: $@"
	@echo "[package] payload : $(CAPY_PKG_BIN) ($$(wc -c < $(CAPY_PKG_BIN) | tr -d ' ') bytes)"

package-clean:
	rm -rf $(CAPY_PKG_DIR)

clean:
	rm -rf $(BUILD_DIR)
