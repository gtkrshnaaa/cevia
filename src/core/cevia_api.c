#include "../include/cevia.h"
#include "../include/lmModel.h"
#include <string.h>

// Version information
const char* cevia_version(void) {
    return "1.0.0";
}

// Model lifecycle
CeviaModel* cevia_create(int maxN) {
    return (CeviaModel*)createLMModel(maxN);
}

void cevia_free(CeviaModel* model) {
    freeLMModel((LMModel*)model);
}

// Training & persistence
void cevia_train_from_file(CeviaModel* model, const char* corpus_file) {
    if (!model || !corpus_file) return;
    LMModel* lm = (LMModel*)model;
    trainModel(lm, corpus_file);
}

void cevia_save(const CeviaModel* model, const char* prefix) {
    if (!model || !prefix) return;
    saveModel((const LMModel*)model, prefix);
}

void cevia_load(CeviaModel* model, const char* prefix) {
    if (!model || !prefix) return;
    loadModel((LMModel*)model, prefix);
}

// Inference
void cevia_predict(const CeviaModel* model,
                   const char* context,
                   char** top_tokens,
                   float* scores,
                   int k) {
    if (!model || !context || !top_tokens || !scores || k <= 0) return;
    
    const LMModel* lm = (const LMModel*)model;
    uint32_t token_ids[64];
    float token_scores[64];
    int actual_k = (k > 64) ? 64 : k;
    
    predictNextToken(lm, context, token_ids, token_scores, actual_k);
    
    for (int i = 0; i < actual_k; i++) {
        const char* token = getTokenById(lm->vocab, token_ids[i]);
        top_tokens[i] = strdup(token);  // Caller must free
        scores[i] = token_scores[i];
    }
}

void cevia_generate(const CeviaModel* model,
                   const char* input,
                   char* output,
                   int max_tokens,
                   float temperature) {
    if (!model || !input || !output) return;
    generateResponse((const LMModel*)model, input, output, max_tokens, temperature);
}

// Evaluation
float cevia_evaluate(const CeviaModel* model,
                    const char* corpus_file,
                    int top_k) {
    if (!model || !corpus_file) return 0.0f;
    
    // This would need to be implemented based on existing eval logic
    // For now, return 0.0
    // TODO: Implement proper evaluation
    (void)top_k;  // Suppress warning
    return 0.0f;
}

// Utilities
uint32_t cevia_vocab_size(const CeviaModel* model) {
    if (!model) return 0;
    const LMModel* lm = (const LMModel*)model;
    if (!lm->vocab) return 0;
    return lm->vocab->size;
}

uint64_t cevia_total_tokens(const CeviaModel* model) {
    if (!model) return 0;
    const LMModel* lm = (const LMModel*)model;
    return lm->totalTokens;
}
