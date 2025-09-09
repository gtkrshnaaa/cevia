#ifndef PatternHeader
#define PatternHeader

#include "common.h"
#include "vocab.h"

// Wildcard token
#define WildcardToken 0xFFFFFFFF

// Pattern token
typedef struct {
    uint32_t tokenId;  // Token ID or WildcardToken
    bool isWildcard;   // True if this is a wildcard
} PatternToken;

// Pattern structure
typedef struct {
    PatternToken* tokens;  // Array of tokens in the pattern
    int length;            // Number of tokens in the pattern
    uint32_t count;        // Frequency count of this pattern
} Pattern;

// Pattern index structure
typedef struct {
    Pattern* patterns;     // Array of patterns
    int size;             // Number of patterns
    int capacity;         // Current capacity of the array
    int maxPatternLength;  // Maximum pattern length
} PatternIndex;

// Function declarations
PatternIndex* createPatternIndex(int initialCapacity, int maxPatternLength);
void freePatternIndex(PatternIndex* index);
void addPattern(PatternIndex* index, const uint32_t* tokens, int length);
void findMatchingPatterns(const PatternIndex* index, const uint32_t* tokens, int length, 
                         uint32_t* matches, int* numMatches);
void extractPatternsFromSequence(PatternIndex* index, const uint32_t* tokens, int length);

#endif // PatternHeader
