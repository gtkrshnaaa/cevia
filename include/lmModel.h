#ifndef LmModelHeader
#define LmModelHeader

#include "vocab.h"
#include "ngram.h"
#include "pattern.h"

// Language model structure
typedef struct {
    Vocabulary* vocab;       // Vocabulary mapping
    NgramIndex* ngrams;     // N-gram index
    PatternIndex* patterns; // Pattern index
    int maxN;              // Maximum n-gram order
    uint64_t totalTokens;   // Total number of tokens in the training data
} LMModel;

// Function declarations
LMModel* createLMModel(int maxN);
void freeLMModel(LMModel* model);
void trainFromFile(LMModel* model, const char* filename);
void saveModel(const LMModel* model, const char* basePath);
void loadModel(LMModel* model, const char* basePath);
void predictNextToken(const LMModel* model, const char* context, 
                     uint32_t* topTokens, float* scores, int k);

// Auto-regressive text generation
void generateResponse(const LMModel* model, const char* input,
                     char* output, int maxTokens, float temperature);

#endif // LmModelHeader

