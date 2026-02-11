# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Build the project
make all

# Clean build artifacts
make clean

# Install binary to /storage/philstar/bin/phsystem/
make install
```

Direct compilation (equivalent to `make all`):
```bash
clang++ -o itemsync -std=c++23 -O2 \
  -I/usr/local/include -L/usr/local/lib \
  -lpthread -lpqxx -lpq \
  src/fileFinder.cpp src/dbfReader.cpp src/itemsync.cpp
```

## Running

```bash
./itemsync db.conf <dbf-file-path> <artcolor-dir-path>
```

**test.sh** uses preconfigured production paths:
```bash
./itemsync db.conf /storage/phsystem/articles.DBF /storage/phsystem/ARTICLES/ARTCOLOR/
```

## Architecture

**itemsync** is a C++23 command-line utility that synchronizes product catalog data from DBF (dBASE) files into a PostgreSQL database. It manages articles (products), colors, sizes, and item combinations for Philstar Hosiery's sock manufacturing system.

### Core Components

- **src/itemsync.cpp** - Main application with sync logic
- **src/dbfReader.cpp/h** - Custom DBF file parser (based on PgDBF by Kirk Strauser)
- **src/fileFinder.cpp/h** - Directory scanner for locating color definition files

### Data Sources

**articles.DBF** (main input file):
- Fields: `artcono` (article code), `article` (name), `custvar` (customer), `size01`-`size06`
- Sizes come from the size01-size06 fields; each non-empty field creates a size record with size_index 1-6

**ARTCOLOR directory** (color definitions):
- Contains individual DBF files named `{artcono}.DBF` for each article
- Each color DBF has a `colorway` field containing the color name
- If no matching color file exists for an article, that article is skipped with a warning

**Items** are generated as all combinations of article + color + size.

### Data Flow

1. Load existing database records (where `phsystem=true`) into in-memory maps (`artMap`, `colMap`, `sizMap`, `itemMap`)
2. Read articles.DBF file record by record
3. For each article: sync articles, sizes, colors, and items against the maps
4. Insert new records, update changed records — matched records are erased from their map
5. After processing all DBF records, delete any entries still remaining in the maps (they no longer exist in the source DBF). Records with id=1 (VOID placeholder) are never deleted.

The entire operation runs in a single PostgreSQL transaction (`pqxx::work`), so all changes are atomic.

### Database Schema

Target tables in `production` schema:
- `sock_articles` (id, artcono, name, customer, subclass, phsystem)
- `sock_colors` (id, article_id, name, phsystem)
- `sock_sizes` (id, article_id, size_index, name, phsystem)
- `sock_items` (id, article_id, color_id, size_id)

### Key Conventions

Composite string keys use `|||` separator for map lookups:
- `"article_id|||field_name"` for colors/sizes
- `"article_id|||color_id|||size_id"` for items

The `phsystem` column (boolean) distinguishes records managed by this tool (`true`) from manually-created records (`false`). Only `phsystem=true` records are loaded, synced, and subject to deletion.

Sync functions (`syncArt`, `syncCol`, `syncSiz`, `syncItem`) return: `-1` = new insert, `0` = found unchanged (pass), `1+` = number of fields updated.

### Unused Files

`src/map.cpp` and `src/map.h` are legacy files not included in the build. They contain an old custom map implementation that was replaced by `std::map`.

### Dependencies

- clang++ (C++23)
- libpqxx (PostgreSQL C++ client, modern API with `pqxx::prepped` / `pqxx::params`)
- Boost (algorithm/string, header-only)
- pthread

## Configuration

`db.conf` contains PostgreSQL connection credentials. This file is in .gitignore and should never be committed.
