#include "../include/ngram.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Create a new n-gram index
NgramIndex* createNgramIndex(int maxN) {
    if (maxN < 1) return NULL;
    
    NgramIndex* index = (NgramIndex*)malloc(sizeof(NgramIndex));
    if (!index) return NULL;
    
    index->root = (NgramNode*)calloc(1, sizeof(NgramNode));
    if (!index->root) {
        free(index);
        return NULL;
    }
    
    index->maxN = maxN;
    index->totalNgrams = 0;
    return index;
}

// Free a node and all its children recursively
static void freeNgramNode(NgramNode* node) {
    if (!node) return;
    
    // Free siblings
    if (node->next) {
        freeNgramNode(node->next);
    }
    
    // Free children
    if (node->children) {
        freeNgramNode(node->children);
    }
    
    free(node);
}

// Free n-gram index
void freeNgramIndex(NgramIndex* index) {
    if (!index) return;
    
    if (index->root) {
        freeNgramNode(index->root);
    }
    
    free(index);
}

// Add an n-gram to the index
void addNgram(NgramIndex* index, const uint32_t* tokens, int n) {
    if (!index || !tokens || n < 1 || n > index->maxN) return;
    
    NgramNode* current = index->root;
    
    for (int i = 0; i < n; i++) {
        uint32_t tokenId = tokens[i];
        NgramNode** next = &(current->children);
        
        // Find the node for this token
        while (*next && (*next)->tokenId != tokenId) {
            next = &((*next)->next);
        }
        
        // Create new node if not found
        if (!*next) {
            *next = (NgramNode*)calloc(1, sizeof(NgramNode));
            if (!*next) return;  // Memory allocation failed
            (*next)->tokenId = tokenId;
        }
        
        current = *next;
    }
    
    // Increment count for this n-gram
    current->count++;
    index->totalNgrams++;
}

// Add an n-gram with an explicit count (used for loading from disk)
void addNgramWithCount(NgramIndex* index, const uint32_t* tokens, int n, uint32_t count) {
    if (!index || !tokens || n < 1 || n > index->maxN || count == 0) return;
    
    NgramNode* current = index->root;
    
    for (int i = 0; i < n; i++) {
        uint32_t tokenId = tokens[i];
        NgramNode** next = &(current->children);
        while (*next && (*next)->tokenId != tokenId) {
            next = &((*next)->next);
        }
        if (!*next) {
            *next = (NgramNode*)calloc(1, sizeof(NgramNode));
            if (!*next) return;
            (*next)->tokenId = tokenId;
        }
        current = *next;
    }
    // Add the provided count
    current->count += count;
    index->totalNgrams += count;
}

// Get the count of a specific n-gram
uint32_t getNgramCount(const NgramIndex* index, const uint32_t* tokens, int n) {
    if (!index || !tokens || n < 1 || n > index->maxN) return 0;
    
    NgramNode* current = index->root;
    
    for (int i = 0; i < n; i++) {
        uint32_t tokenId = tokens[i];
        NgramNode* next = current->children;
        
        // Find the node for this token
        while (next && next->tokenId != tokenId) {
            next = next->next;
        }
        
        if (!next) return 0;  // N-gram not found
        current = next;
    }
    
    return current->count;
}

// Create an iterator for traversing all n-grams
NgramIterator* ngramIterator(const NgramIndex* index) {
    if (!index) return NULL;
    
    NgramIterator* it = (NgramIterator*)malloc(sizeof(NgramIterator));
    if (!it) return NULL;
    
    it->node = index->root;
    it->depth = 0;
    return it;
}

// Get the next n-gram from the iterator
int nextNgram(NgramIterator* it, uint32_t* tokens, int maxN, uint32_t* count) {
    if (!it || !it->node || !tokens || maxN < 1) return 0;
    
    // This is a simplified implementation that only returns unigrams
    // A full implementation would need to traverse the tree properly
    static NgramNode* current = NULL;
    
    if (!current) {
        current = it->node->children;
    } else {
        current = current->next;
    }
    
    if (!current) {
        current = NULL;
        return 0;
    }
    
    tokens[0] = current->tokenId;
    *count = current->count;
    return 1;  // Return 1 for unigram
}

// Free an n-gram iterator
void freeNgramIterator(NgramIterator* it) {
    if (it) free(it);
}

// Update n-gram counts for a sequence of tokens
void updateNgrams(NgramIndex* index, const uint32_t* tokens, int length) {
    if (!index || !tokens || length < 1) return;
    
    // For each position in the sequence
    for (int i = 0; i < length; i++) {
        // For each possible n-gram length starting at this position
        for (int n = 1; n <= index->maxN && (i + n) <= length; n++) {
            addNgram(index, tokens + i, n);
        }
    }
}

// Find the node corresponding to a prefix sequence of length n
NgramNode* findPrefixNode(const NgramIndex* index, const uint32_t* tokens, int n) {
    if (!index || !tokens || n < 1 || n > index->maxN) return NULL;
    NgramNode* current = index->root;
    for (int i = 0; i < n; i++) {
        uint32_t tokenId = tokens[i];
        NgramNode* child = current ? current->children : NULL;
        while (child && child->tokenId != tokenId) {
            child = child->next;
        }
        if (!child) return NULL;
        current = child;
    }
    return current;
}
