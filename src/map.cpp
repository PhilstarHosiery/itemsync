/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   map.cpp
 * Author: Hyun Suk Noh <hsnoh@philstar.biz>
 * 
 * Created on August 27, 2016, 11:16 AM
 */

#include "map.h"

map::map() {
    head = NULL;
    tail = NULL;
    prev = NULL;
    curr = NULL;
}

map::map(const map& orig) {
}

map::~map() {
    node *tmp;
    node *itr;
    tmp = head;
    itr = head;
    
    while(itr != NULL) {
        tmp = itr;
        itr = itr->next;
        delete tmp;
    }
}

void map::insert(triple key, triple value) {
    std::list<mapping>::iterator last = dlink_list.end();
    triple last_key = (*last).key;
    
    // Each new key must be greater than previous. Duplicate key is not allowed.
    if(!key_less_than(last_key, key)) {
        return;
    }
    
    mapping m = {key, value};
    dlink_list.push_back(m);
    
    node *tmp = new node;
    tmp->next = NULL;
    
    if(head == NULL) {
        head = tmp;
        tail = tmp;
        curr = tmp;
    } else {
        *tail->next = tmp;
        tail = tmp;
    }
}

triple map::lookup(triple key) {
    mapping curr_item = *curr;
    
    while (key_less_than(curr_item.key, key)) {
        ++curr;
        curr_item = *curr;
    }
    
    if (key_equal(curr_item.key, key)) {
        return curr_item.value;
    }
}

void map::remove_curr() {
    if(curr == NULL) return;
    
    prev->next = curr->next;
    node *next = curr->next;
    
    if(next == NULL) {
        
    }
    
    // dlink_list.remove(p);
}

bool map::key_less_than(triple k1, triple k2) {
    if(k1.v1 < k2.v1) {
        return true;
    } else if(k1.v1 > k2.v1) {
        return false;
    }
    // else k1.v1 == k2.v1
    
    if(k1.v2 < k2.v2) {
        return true;
    } else if(k1.v2 > k2.v2) {
        return false;
    }
    // else k1.v2 == k2.v2
    
    if(k1.v3 < k2.v3) {
        return true;
    } else {
        return false;
    }
}

bool map::key_equal(triple k1, triple k2) {
    return (k1.v1 == k2.v1) && (k1.v2 == k2.v2) && (k1.v3 == k2.v3);
}