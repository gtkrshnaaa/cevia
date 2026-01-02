#include "../include/pattern.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Maximum number of matching patterns to return
#define MaxMatchingPatterns 100

// Create a new pattern index
PatternIndex* createPatternIndex(int initialCapacity, int maxPatternLength) {
    if (initialCapacity <= 0 || maxPatternLength <= 0) return NULL;
    
    PatternIndex* index = (PatternIndex*)malloc(sizeof(PatternIndex));
    if (!index) return NULL;
    
    index->patterns = (Pattern*)calloc(initialCapacity, sizeof(Pattern));
    if (!index->patterns) {
        free(index);
        return NULL;
    }
    
    index->size = 0;
    index->capacity = initialCapacity;
    index->maxPatternLength = maxPatternLength;
    
    return index;
}

// Free a pattern
static void freePattern(Pattern* pattern) {
    if (pattern && pattern->tokens) {
        free(pattern->tokens);
    }
}

// Free pattern index
void freePatternIndex(PatternIndex* index) {
    if (!index) return;
    
    if (index->patterns) {
        for (int i = 0; i < index->size; i++) {
            freePattern(&index->patterns[i]);
        }
        free(index->patterns);
    }
    
    free(index);
}

// Add a pattern to the index
void addPattern(PatternIndex* index, const uint32_t* tokens, int length) {
    if (!index || !tokens || length <= 0 || length > index->maxPatternLength) return;
    
    // Check if we need to resize the patterns array
    if (index->size >= index->capacity) {
        int newCapacity = index->capacity * 2;
        Pattern* newPatterns = (Pattern*)realloc(index->patterns, newCapacity * sizeof(Pattern));
        if (!newPatterns) return;  // Out of memory
        
        index->patterns = newPatterns;
        index->capacity = newCapacity;
    }
    
    // Initialize new pattern
    Pattern* pattern = &index->patterns[index->size];
    pattern->tokens = (PatternToken*)malloc(length * sizeof(PatternToken));
    if (!pattern->tokens) return;  // Out of memory
    
    // Copy tokens
    for (int i = 0; i < length; i++) {
        pattern->tokens[i].tokenId = tokens[i];
        pattern->tokens[i].isWildcard = (tokens[i] == WildcardToken);
    }
    
    pattern->length = length;
    pattern->count = 1;  // Initial count
    index->size++;
}

// Check if a pattern matches a sequence of tokens
static bool patternMatches(const Pattern* pattern, const uint32_t* tokens, int length) {
    if (pattern->length != length) return false;
    
    for (int i = 0; i < length; i++) {
        if (!pattern->tokens[i].isWildcard && 
            pattern->tokens[i].tokenId != tokens[i]) {
            return false;
        }
    }
    
    return true;
}

// Find all patterns that match the given sequence of tokens
void findMatchingPatterns(const PatternIndex* index, const uint32_t* tokens, int length, 
                         uint32_t* matches, int* numMatches) {
    if (!index || !tokens || length <= 0 || !matches || !numMatches) return;
    
    *numMatches = 0;
    
    for (int i = 0; i < index->size; i++) {
        const Pattern* pattern = &index->patterns[i];
        
        if (pattern->length == length && patternMatches(pattern, tokens, length)) {
            if (*numMatches < MaxMatchingPatterns) {
                matches[(*numMatches)++] = i;  // Store pattern index
            } else {
                break;  // Reached maximum number of matches
            }
        }
    }
}

// Extract patterns from a sequence of tokens
void extractPatternsFromSequence(PatternIndex* index, const uint32_t* tokens, int length) {
    if (!index || !tokens || length <= 0) return;
    
    // For each possible starting position
    for (int start = 0; start < length; start++) {
        // For each possible pattern length
        for (int patternLen = 1; patternLen <= index->maxPatternLength && 
                                (start + patternLen) <= length; patternLen++) {
            
            // Create a copy of the token sequence with some wildcards
            uint32_t pattern[patternLen];
            
            // Simple pattern extraction: replace some tokens with wildcards
            for (int i = 0; i < patternLen; i++) {
                // Example: replace every 3rd token with a wildcard
                if ((i + 1) % 3 == 0) {
                    pattern[i] = WildcardToken;
                } else {
                    pattern[i] = tokens[start + i];
                }
            }
            
            // Add the pattern to the index
            addPattern(index, pattern, patternLen);
        }
    }
}
