# itemsync

A command-line utility that synchronizes product catalog data from DBF (dBASE) files into a PostgreSQL database. Manages articles, colors, sizes, and item combinations for sock manufacturing.

## Requirements

- clang++ with C++23 support
- libpqxx (PostgreSQL C++ client)
- Boost (system, algorithm/string)
- pthread

## Building

```bash
make all
```

## Usage

```bash
./itemsync <db.conf> <articles.dbf> <artcolor-dir>
```

**Arguments:**
- `db.conf` - PostgreSQL connection string file
- `articles.dbf` - Main articles DBF file
- `artcolor-dir` - Directory containing per-article color DBF files

## Configuration

Create a `db.conf` file with your PostgreSQL connection string:

```
host=localhost dbname=mydb user=myuser password=mypass
```

## Data Sources

**articles.DBF** contains article records with fields:
- `artcono` - Article code (unique identifier)
- `article` - Article name
- `custvar` - Customer name
- `size01`-`size06` - Size names (up to 6 sizes per article)

**ARTCOLOR directory** contains individual `{artcono}.DBF` files with a `colorway` field for each color variant.

Items are generated as all combinations of article + color + size.

## Database Schema

Target tables in `production` schema:

| Table | Columns |
|-------|---------|
| sock_articles | id, artcono, name, customer, subclass, phsystem |
| sock_colors | id, article_id, name, phsystem |
| sock_sizes | id, article_id, size_index, name, phsystem |
| sock_items | id, article_id, color_id, size_id |

## License

Proprietary - Philstar Hosiery
