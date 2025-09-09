#ifndef NgramHeader
#define NgramHeader

#include "common.h"
#include "vocab.h"

// N-gram node structure
typedef struct NgramNode {
    uint32_t tokenId;           // Token ID
    uint32_t count;             // Count of this n-gram
    struct NgramNode* next;     // Next node in the linked list
    struct NgramNode* children; // Child nodes for next token in sequence
} NgramNode;

// N-gram index structure
typedef struct {
    NgramNode* root;            // Root of the n-gram tree
    int maxN;                   // Maximum n-gram order
    uint64_t totalNgrams;       // Total number of n-grams
} NgramIndex;

// N-gram iterator for traversal
typedef struct {
    NgramNode* node;
    int depth;
} NgramIterator;

// Function declarations
NgramIndex* createNgramIndex(int maxN);
void freeNgramIndex(NgramIndex* index);
void addNgram(NgramIndex* index, const uint32_t* tokens, int n);
void addNgramWithCount(NgramIndex* index, const uint32_t* tokens, int n, uint32_t count);
uint32_t getNgramCount(const NgramIndex* index, const uint32_t* tokens, int n);
NgramIterator* ngramIterator(const NgramIndex* index);
int nextNgram(NgramIterator* it, uint32_t* tokens, int maxN, uint32_t* count);
void freeNgramIterator(NgramIterator* it);
void updateNgrams(NgramIndex* index, const uint32_t* tokens, int length);

// Helper: find the node corresponding to a prefix sequence (length n)
// Returns NULL if not found. Depth corresponds to n.
NgramNode* findPrefixNode(const NgramIndex* index, const uint32_t* tokens, int n);

#endif // NgramHeader
