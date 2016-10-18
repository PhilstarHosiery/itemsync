/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   fileFinder.h
 * Author: Hyun Suk Noh <hsnoh@philstar.biz>
 *
 * Created on August 30, 2016, 3:49 PM
 */

#ifndef FILEFINDER_H
#define FILEFINDER_H

#include <string>
#include <map>

using namespace std;

class fileFinder {
public:
    fileFinder();
    fileFinder(const fileFinder& orig);
    virtual ~fileFinder();
    
    void openDir(string dir);
    void closeDir();
    string findFile(string filename);
    
    void dump();
    
private:
    map<string, string> fn_map;
    
    string toLower(string str);
};

#endif /* FILEFINDER_H */
