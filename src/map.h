/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   map.h
 * Author: Hyun Suk Noh <hsnoh@philstar.biz>
 *
 * Created on August 27, 2016, 11:16 AM
 */

#include <list>
#include <string>
#include <utility>

#ifndef MAP_H
#define MAP_H

struct triple {
    std::string v1;
    std::string v2;
    std::string v3;
};

struct mapping {
    triple key;
    triple value;
};

struct node {
    mapping item;
    node *next;
};

class map {
private:
    node *head;
    node *tail;
    node *prev;
    node *curr;
    
    std::list<mapping> dlink_list;
    // std::list<mapping>::iterator curr;
    
public:
    map();
    map(const map& orig);
    virtual ~map();
    
    void insert(triple key, triple value);
    triple lookup(triple key);
    void remove_curr();
    
private:
    bool key_less_than(triple k1, triple k2);
    bool key_equal(triple k1, triple k2);
};

#endif /* MAP_H */

