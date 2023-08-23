#include <iostream>
#include <fstream>
#include <cstdlib>
#include <pqxx/pqxx>

using namespace std;

#include "dbfReader.h"
#include "fileFinder.h"

struct article_type {
    unsigned int article_id;
    string artcono;
    string name;
    string customer;
    // string subclass; // No such information exists in source DBF
};

struct color_type {
    unsigned int color_id;
    unsigned int article_id;
    string name;
};

struct size_type {
    unsigned int size_id;
    unsigned int article_id;
    unsigned int size_index;
    string name;
};

struct stat_type {
    string type;
    int orig;
    int pass;
    int update;
    int update_row;
    int add;
    int del;
    int total;
    
    stat_type() {
        type = "";
        orig = 0;
        pass = 0;
        update = 0;
        update_row = 0;
        add = 0;
        del = 0;
        total = 0;
    }
};

void list_target(pqxx::work &txn);
void generate_maps(pqxx::work &txn, map<string, article_type> &articleMap, map<string, color_type> &colorMap, map<string, size_type> &sizeMap);
void generate_itemMap(pqxx::work &txn, map<string, int> &m);
int syncArt(pqxx::work &txn, map<string, article_type> &m, article_type &art);
int syncCol(pqxx::work &txn, map<string, color_type> &m, color_type &col);
int syncSiz(pqxx::work &txn, map<string, size_type> &m, size_type &siz);
int syncItem(pqxx::work &txn, map<string, int> &m, int artId, int colId, int sizId);
string key(int art, string id);
string key(int art, int col, int siz);
void updateStat(stat_type &stat, int res);
void displayStat(stat_type stat);

int main(int argc, char** argv) {

    if (argc != 4) {
        cout << "Usage: itemsync [db.conf] [dbf file] [artcolor dir]" << endl;
        return 1;
    }

    // Parameters
    // 1 - configuration filename
    // 2 - prosheet.DBF location
    // 3 - ARTCOLOR directory
    string dbconf(argv[1]);
    string dbffile(argv[2]);
    string artcolor_dir(argv[3]);

    // Get dbstring from 1st parameter
    ifstream dbconfin;
    dbconfin.open(dbconf);
    string dbstring;
    getline(dbconfin, dbstring);

    try {
        dbfReader reader;
        dbfReader color_reader;
        string color_fn;
        string size_index;
        string size;
        string color;

        map<string, string> size_map;

        map<string, article_type> artMap;
        map<string, color_type> colMap;
        map<string, size_type> sizMap;
        map<string, int> itemMap;

        // map<string, int> colIdMap;
        map<string, int> sizeIdMap;

        // Database Connection & Transaction
        pqxx::connection c(dbstring);
        pqxx::work txn(c);

        // Build the map from DB
        generate_maps(txn, artMap, colMap, sizMap);
        generate_itemMap(txn, itemMap);

        // Prepare the filename map
        fileFinder artcolor;
        artcolor.openDir(artcolor_dir);

        // Stats
        stat_type itmStat;
        stat_type artStat;
        stat_type colStat;
        stat_type sizStat;

        itmStat.type = "item";
        artStat.type = "article";
        colStat.type = "color";
        sizStat.type = "size";

        itmStat.orig = itemMap.size();
        artStat.orig = artMap.size();
        colStat.orig = colMap.size();
        sizStat.orig = sizMap.size();

        
        // Prepared Statements
        c.prepare("add_art", "INSERT INTO production.sock_article (artcono, name, customer, subclass) VALUES ($1, $2, $3, '') RETURNING article_id");
        c.prepare("add_col", "INSERT INTO production.sock_color (article_id, name) VALUES ($1, $2) RETURNING color_id");
        c.prepare("add_siz", "INSERT INTO production.sock_size (article_id, size_index, name) VALUES ($1, $2, $3) RETURNING size_id");
        c.prepare("add_item", "INSERT INTO production.sock_item (article_id, color_id, size_id) VALUES ($1, $2, $3)");

        c.prepare("update_art_name", "UPDATE production.sock_article SET name=$1 WHERE article_id=$2");
        c.prepare("update_art_customer", "UPDATE production.sock_article SET customer=$1 WHERE article_id=$2");
        // c.prepare("update_art_subclass", "UPDATE production.sock_article SET subclass=$1 WHERE article_id=$2");
        c.prepare("update_col", "UPDATE production.sock_color SET name=$1 WHERE color_id=$2");
        c.prepare("update_siz", "UPDATE production.sock_size SET name=$1 WHERE size_id=$2");

        c.prepare("del_article", "DELETE FROM production.sock_article WHERE article_id=$1");
        c.prepare("del_color", "DELETE FROM production.sock_color WHERE color_id=$1");
        c.prepare("del_size", "DELETE FROM production.sock_size WHERE size_id=$1");
        c.prepare("del_item", "DELETE FROM production.sock_item WHERE item_id=$1");

        // Open articles.dbf and process each article
        reader.open(dbffile);

        int artconoIdx = reader.getFieldIndex("artcono");
        int articleIdx = reader.getFieldIndex("article");
        int custvarIdx = reader.getFieldIndex("custvar");
        int size01Idx = reader.getFieldIndex("size01");
        int size02Idx = reader.getFieldIndex("size02");
        int size03Idx = reader.getFieldIndex("size03");
        int size04Idx = reader.getFieldIndex("size04");
        int size05Idx = reader.getFieldIndex("size05");
        int size06Idx = reader.getFieldIndex("size06");

        int colorIdx;

        string artcono;
        string article;
        string custvar;
        int article_id;

        while (reader.next()) {

            if (reader.isClosedRow()) {
                continue;
            }

            artcono = reader.getString(artconoIdx);
            article = reader.getString(articleIdx);
            custvar = reader.getString(custvarIdx);

            article_type tmpArt;
            tmpArt.artcono = artcono;
            tmpArt.name = article;
            tmpArt.customer = custvar;
            updateStat(artStat,
                    syncArt(txn, artMap, tmpArt));
            article_id = tmpArt.article_id;

            color_fn = artcolor.findFile(artcono + ".DBF");
            if (color_fn.empty()) {
                cerr << artcono + ".DBF skipped!" << endl;
                continue; // skip if no ARTCOLOR file exists
            }

            // Find present sizes, creates a map size_index -> size_name
            size_map.clear();
            if (!reader.getString(size01Idx).empty()) size_map["1"] = reader.getString(size01Idx);
            if (!reader.getString(size02Idx).empty()) size_map["2"] = reader.getString(size02Idx);
            if (!reader.getString(size03Idx).empty()) size_map["3"] = reader.getString(size03Idx);
            if (!reader.getString(size04Idx).empty()) size_map["4"] = reader.getString(size04Idx);
            if (!reader.getString(size05Idx).empty()) size_map["5"] = reader.getString(size05Idx);
            if (!reader.getString(size06Idx).empty()) size_map["6"] = reader.getString(size06Idx);

            // Syncs all sizes while creating the map size_index -> size_id
            sizeIdMap.clear();
            for (auto s : size_map) {
                size_type tmpSiz;
                tmpSiz.article_id = article_id;
                tmpSiz.size_index = stoi(s.first);
                tmpSiz.name = s.second;
                updateStat(sizStat,
                        syncSiz(txn, sizMap, tmpSiz));

                sizeIdMap[s.first] = tmpSiz.size_id;
            }

            // Open artcolor file and process each color
            color_reader.open(artcolor_dir + color_fn);
            colorIdx = color_reader.getFieldIndex("colorway");

            // Process each color
            while (color_reader.next()) {

                // Skip deleted rows in color
                if (color_reader.isClosedRow()) {
                    continue;
                }

                color = color_reader.getString(colorIdx);

                // Sync color
                color_type tmpCol;
                tmpCol.article_id = article_id;
                tmpCol.name = color;
                updateStat(colStat,
                        syncCol(txn, colMap, tmpCol));

                for (auto itr : size_map) {
                    size_index = itr.first;
                    size = itr.second;

                    updateStat(itmStat,
                            syncItem(txn, itemMap, article_id, tmpCol.color_id, sizeIdMap[size_index]));
                }
            }
        }

        // Delete non-existing things from target DB
        for (auto i : itemMap) {
            if (i.second == 1) continue; // VOID item id

            cout << " DELETE ITEM [" << i.first << "] at " << i.second << endl;
            txn.exec_prepared("del_item", i.second);
            itmStat.del++;
        }

        for (auto i : sizMap) {
            if (i.second.size_id == 1) continue; // VOID size id

            cout << " DELETE SIZE [" << i.second.size_id << "] --> " << i.second.article_id << ", " << i.second.size_index << " / " << i.second.name << endl;
            txn.exec_prepared("del_size", i.second.size_id);
            sizStat.del++;
        }

        for (auto i : colMap) {
            if (i.second.color_id == 1) continue; // VOID color id

            cout << " DELETE COLOR [" << i.second.color_id << "] --> " << i.second.article_id << ", " << i.second.name << endl;
            txn.exec_prepared("del_color", i.second.color_id);
            colStat.del++;
        }

        for (auto i : artMap) {
            if (i.second.article_id == 1) continue; // VOID article id

            cout << " DELETE ARTICLE [" << i.second.article_id << "] --> " << i.second.artcono << ", " << i.second.customer << " / " << i.second.name << endl;
            txn.exec_prepared("del_article", i.second.article_id);
            artStat.del++;
        }

        txn.commit();

        // Display stats
        displayStat(artStat);
        displayStat(colStat);
        displayStat(sizStat);
        displayStat(itmStat);

        // c.disconnect();

    } catch (const pqxx::sql_error &e) {
        cerr << "sql_error: " << e.what() << " / " << e.query() << endl;
    } catch (const std::exception &e) {
        cerr << "exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}

void generate_maps(pqxx::work &txn, map<string, article_type> &articleMap, map<string, color_type> &colorMap, map<string, size_type> &sizeMap) {
    pqxx::result res;

    res = txn.exec("SELECT * FROM production.sock_article");
    for (auto i = 0; i != res.size(); ++i) {
        article_type art;

        res[i]["article_id"].to(art.article_id);
        res[i]["artcono"].to(art.artcono);
        res[i]["name"].to(art.name);
        res[i]["customer"].to(art.customer);

        articleMap[art.artcono] = art;
    }

    res = txn.exec("SELECT * FROM production.sock_color");
    for (auto i = 0; i != res.size(); ++i) {
        color_type col;

        res[i]["color_id"].to(col.color_id);
        res[i]["article_id"].to(col.article_id);
        res[i]["name"].to(col.name);

        colorMap[key(col.article_id, col.name)] = col;
    }

    res = txn.exec("SELECT * FROM production.sock_size");
    for (auto i = 0; i != res.size(); ++i) {
        size_type siz;

        res[i]["size_id"].to(siz.size_id);
        res[i]["article_id"].to(siz.article_id);
        res[i]["size_index"].to(siz.size_index);
        res[i]["name"].to(siz.name);

        sizeMap[key(siz.article_id, to_string(siz.size_index))] = siz;
    }
}

void generate_itemMap(pqxx::work &txn, map<string, int> &m) {
    pqxx::result res = txn.exec("SELECT * FROM production.sock_item");
    for (auto i = 0; i != res.size(); ++i) {
        int id, art, col, siz;

        res[i]["item_id"].to(id);
        res[i]["article_id"].to(art);
        res[i]["color_id"].to(col);
        res[i]["size_id"].to(siz);

        m[key(art, col, siz)] = id;
    }
}


// return: -1 if new insert, 0+ for number of updates (0 means found without update, ie pass)

int syncArt(pqxx::work &txn, map<string, article_type> &m, article_type &art) {
    auto itr = m.find(art.artcono);
    article_type artdb;
    int rtn = 0;

    if (itr == m.end()) { // Not found
        cout << " ARTICLE NOT FOUND: INSERT " << art.artcono << ", " << art.name << ", " << art.customer;

        pqxx::result r = txn.exec_prepared("add_art", art.artcono, art.name, art.customer);
        r[0]["article_id"].to(art.article_id);
        rtn = -1;

        cout << " -> " << art.article_id << endl;
    } else { // Found
        artdb = itr->second;
        art.article_id = artdb.article_id;

        if (artdb.name != art.name) {
            cout << " UPDATE ARTICLE " << itr->first << " name at " << artdb.article_id << " : " << artdb.name << " -> " << art.name << endl;
            txn.exec_prepared("update_art_name", art.name, artdb.article_id);
            rtn++;
        }

        if (artdb.customer != art.customer) {
            cout << " UPDATE ARTICLE " << itr->first << " customer at " << artdb.article_id << " : " << artdb.customer << " -> " << art.customer << endl;
            txn.exec_prepared("update_art_customer", art.customer, artdb.article_id);
            rtn++;
        }

        m.erase(itr);
    }

    return rtn;
}

int syncCol(pqxx::work &txn, map<string, color_type> &m, color_type &col) {
    auto itr = m.find(key(col.article_id, col.name));
    color_type coldb;
    int rtn = 0;

    if (itr == m.end()) { // Not found
        cout << " COLOR NOT FOUND: INSERT article_id=" << col.article_id << ", color=" << col.name;

        pqxx::result r = txn.exec_prepared("add_col", col.article_id, col.name);
        r[0]["color_id"].to(col.color_id);
        rtn = -1;

        cout << " -> color_id=" << col.color_id << endl;
        
        // for (auto item : m) {
        //     cout << "  - [" << item.first << "] -> " << item.second << endl;
        // }
    } else { // Found
        coldb = itr->second;
        col.color_id = coldb.color_id;

        if (coldb.name != col.name) {
            cout << " UPDATE COLOR " << itr->first << " name at " << coldb.article_id << " : " << coldb.name << " -> " << col.name << endl;
            txn.exec_prepared("update_col", col.name, coldb.color_id);
            rtn++;
        }

        m.erase(itr);
    }

    return rtn;
}

int syncSiz(pqxx::work &txn, map<string, size_type> &m, size_type &siz) {
    auto itr = m.find(key(siz.article_id, to_string(siz.size_index)));
    size_type sizdb;
    int rtn = 0;

    if (itr == m.end()) { // Not found
        cout << " SIZE NOT FOUND: INSERT " << siz.article_id << ", " << siz.size_index << ", " << siz.name;

        pqxx::result r = txn.exec_prepared("add_siz", siz.article_id, siz.size_index, siz.name);
        r[0]["size_id"].to(siz.size_id);
        rtn = -1;

        cout << " -> " << siz.size_id << endl;
    } else { // Found
        sizdb = itr->second;
        siz.size_id = sizdb.size_id;

        if (sizdb.name != siz.name) {
            cout << " UPDATE SIZE " << itr->first << " name at " << sizdb.article_id << " : " << sizdb.name << " -> " << siz.name << endl;
            txn.exec_prepared("update_siz", siz.name, sizdb.size_id);
            rtn++;
        }

        m.erase(itr);
    }

    return rtn;
}

int syncItem(pqxx::work &txn, map<string, int> &m, int artId, int colId, int sizId) {
    auto itr = m.find(key(artId, colId, sizId));
    int rtn = 0;

    if (itr == m.end()) { // Not found
        cout << " ITEM NOT FOUND: INSERT " << artId << ", " << colId << ", " << sizId << endl;
        txn.exec_prepared("add_item", artId, colId, sizId);
        rtn = -1;
    } else { // Found
        m.erase(itr);
    }

    return rtn;
}

string key(int art, string id) {
    return to_string(art) + "|||" + id;
}

string key(int art, int col, int siz) {
    return to_string(art) + "|||" + to_string(col) + "|||" + to_string(siz);
}

void updateStat(stat_type &stat, int res) {
    if (res < 0) {
        stat.add++;
    } else if (res == 0) {
        stat.pass++;
    } else {
        stat.update++;
        stat.update_row += res;
    }
}

void displayStat(stat_type stat) {
    stat.total = stat.orig + stat.add - stat.del;

    cout << "Stats (" << stat.type << " synchronization)" << endl;

	if (stat.update == 0 && stat.add == 0 && stat.del == 0) {
		cout << " Total         = " << stat.orig << endl;
	} else {
		cout << " Total Initial = " << stat.orig << endl
			 << " Pass          = " << stat.pass << endl
			 << " Update        = " << stat.update << " (" << stat.update_row << " info)" << endl
			 << " Insert    (+) = " << stat.add << endl
			 << " Delete    (-) = " << stat.del << endl
			 << " Total Final   = " << stat.total << endl;
	}
}
