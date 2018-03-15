/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   fileFinder.cpp
 * Author: Hyun Suk Noh <hsnoh@philstar.biz>
 * 
 * Created on August 30, 2016, 3:49 PM
 */

#include "fileFinder.h"

#include <iostream>

using namespace std;

#include <dirent.h>
#include <boost/algorithm/string.hpp>

fileFinder::fileFinder() {
}

fileFinder::fileFinder(const fileFinder& orig) {
}

fileFinder::~fileFinder() {
}

void fileFinder::openDir(string dir) {
    DIR *dpdf;
    struct dirent *epdf;

    dpdf = opendir(dir.c_str());
    if (dpdf != NULL){
        while ((epdf = readdir(dpdf))) {
            // printf("Filename: %s",epdf->d_name);
            // std::cout << epdf->d_name << std::endl;
            string fn(epdf->d_name);
            string k = fn;
            boost::algorithm::to_lower(k);
            fn_map[k] = fn;
        }
    }
    
    closedir(dpdf);
}

void fileFinder::closeDir() {
    fn_map.clear();
}

string fileFinder::findFile(string filename) {
    boost::algorithm::to_lower(filename);
    return fn_map[filename];
}

void fileFinder::dump() {
    int cnt = 0;
    
    for(map<string, string>::iterator itr = fn_map.begin(); itr != fn_map.end(); itr++) {
        cout << itr->first << " -> " << itr->second << endl;
        cnt++;
    }
    
    cout << "fn_map: " << cnt << " entires." << endl;
}

