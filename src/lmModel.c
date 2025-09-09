#include "../include/lmModel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Maximum number of candidate tokens to consider
#define MaxCandidates 100

// Create a new language model
LMModel* createLMModel(int maxN) {
    if (maxN < 1) return NULL;
    
    LMModel* model = (LMModel*)malloc(sizeof(LMModel));
    if (!model) return NULL;
    
    model->vocab = createVocabulary();
    if (!model->vocab) {
        free(model);
        return NULL;
    }
    
    model->ngrams = createNgramIndex(maxN);
    if (!model->ngrams) {
        freeVocabulary(model->vocab);
        free(model);
        return NULL;
    }
    
    model->patterns = createPatternIndex(1000, maxN);  // Initial capacity 1000
    if (!model->patterns) {
        freeNgramIndex(model->ngrams);
        freeVocabulary(model->vocab);
        free(model);
        return NULL;
    }
    
    model->maxN = maxN;
    model->totalTokens = 0;
    
    return model;
}

// Free language model
void freeLMModel(LMModel* model) {
    if (!model) return;
    
    if (model->vocab) {
        freeVocabulary(model->vocab);
    }
    
    if (model->ngrams) {
        freeNgramIndex(model->ngrams);
    }
    
    if (model->patterns) {
        freePatternIndex(model->patterns);
    }
    
    free(model);
}

// Train the model on a text file
void trainFromFile(LMModel* model, const char* filename) {
    if (!model || !filename) return;
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open training file");
        return;
    }
    
    char line[4096];
    while (fgets(line, sizeof(line), file)) {
        // Remove newline character
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        // Tokenize the line
        Sentence s = initSentence();
        tokenizeLine(line, &s);
        
        if (s.length == 0) continue;
        
        // Convert tokens to IDs
        uint32_t tokenIds[s.length];
        for (int i = 0; i < s.length; i++) {
            tokenIds[i] = getOrAddToken(model->vocab, s.sequence[i].text);
            model->totalTokens++;
        }
        
        // Update n-gram counts
        updateNgrams(model->ngrams, tokenIds, s.length);
        
        // Extract patterns
        extractPatternsFromSequence(model->patterns, tokenIds, s.length);
    }
    
    fclose(file);
}

// Save model to files
void saveModel(const LMModel* model, const char* basePath) {
    if (!model || !basePath) return;
    
    char filename[1024];
    
    // Save vocabulary
    snprintf(filename, sizeof(filename), "%s.vocab", basePath);
    saveVocabulary(model->vocab, filename);
    
    // Save unigrams and totalTokens
    snprintf(filename, sizeof(filename), "%s.uni", basePath);
    FILE* f = fopen(filename, "wb");
    if (f) {
        // Write totalTokens
        fwrite(&model->totalTokens, sizeof(uint64_t), 1, f);

        // Count unigrams first
        uint32_t unigramCount = 0;
        {
            NgramIterator* it = ngramIterator(model->ngrams);
            uint32_t token; uint32_t count;
            while (nextNgram(it, &token, 1, &count)) {
                unigramCount++;
            }
            freeNgramIterator(it);
        }
        fwrite(&unigramCount, sizeof(uint32_t), 1, f);

        // Write each unigram (token, count)
        NgramIterator* it = ngramIterator(model->ngrams);
        uint32_t token; uint32_t count;
        while (nextNgram(it, &token, 1, &count)) {
            fwrite(&token, sizeof(uint32_t), 1, f);
            fwrite(&count, sizeof(uint32_t), 1, f);
        }
        freeNgramIterator(it);

        fclose(f);
    }

    // Save bigrams: pairs (prev, next, count)
    snprintf(filename, sizeof(filename), "%s.bi", basePath);
    FILE* f2 = fopen(filename, "wb");
    if (f2 && model->ngrams && model->ngrams->root) {
        // Count bigrams first
        uint32_t bigramCount = 0;
        NgramNode* first = model->ngrams->root->children;
        while (first) {
            NgramNode* second = first->children;
            while (second) { bigramCount++; second = second->next; }
            first = first->next;
        }
        fwrite(&bigramCount, sizeof(uint32_t), 1, f2);
        // Write all bigrams
        first = model->ngrams->root->children;
        while (first) {
            NgramNode* second = first->children;
            while (second) {
                fwrite(&first->tokenId, sizeof(uint32_t), 1, f2);
                fwrite(&second->tokenId, sizeof(uint32_t), 1, f2);
                fwrite(&second->count, sizeof(uint32_t), 1, f2);
                second = second->next;
            }
            first = first->next;
        }
        fclose(f2);
    } else if (f2) {
        uint32_t zero = 0; fwrite(&zero, sizeof(uint32_t), 1, f2); fclose(f2);
    }

    // Save trigrams: triples (prev1, prev2, next, count)
    snprintf(filename, sizeof(filename), "%s.tri", basePath);
    FILE* f3 = fopen(filename, "wb");
    if (f3 && model->ngrams && model->ngrams->root) {
        uint32_t trigramCount = 0;
        // Count trigrams
        NgramNode* first = model->ngrams->root->children;
        while (first) {
            NgramNode* second = first->children;
            while (second) {
                NgramNode* third = second->children;
                while (third) { trigramCount++; third = third->next; }
                second = second->next;
            }
            first = first->next;
        }
        fwrite(&trigramCount, sizeof(uint32_t), 1, f3);
        // Write trigrams
        first = model->ngrams->root->children;
        while (first) {
            NgramNode* second = first->children;
            while (second) {
                NgramNode* third = second->children;
                while (third) {
                    fwrite(&first->tokenId, sizeof(uint32_t), 1, f3);
                    fwrite(&second->tokenId, sizeof(uint32_t), 1, f3);
                    fwrite(&third->tokenId, sizeof(uint32_t), 1, f3);
                    fwrite(&third->count, sizeof(uint32_t), 1, f3);
                    third = third->next;
                }
                second = second->next;
            }
            first = first->next;
        }
        fclose(f3);
    } else if (f3) {
        uint32_t zero = 0; fwrite(&zero, sizeof(uint32_t), 1, f3); fclose(f3);
    }
}

// Load model from files
void loadModel(LMModel* model, const char* basePath) {
    if (!model || !basePath) return;
    
    char filename[1024];
    
    // Load vocabulary
    snprintf(filename, sizeof(filename), "%s.vocab", basePath);
    loadVocabulary(model->vocab, filename);
    
    // Load unigrams and totalTokens
    snprintf(filename, sizeof(filename), "%s.uni", basePath);
    FILE* f = fopen(filename, "rb");
    if (f) {
        // Read totalTokens
        if (fread(&model->totalTokens, sizeof(uint64_t), 1, f) != 1) {
            fclose(f);
            return;
        }
        uint32_t unigramCount = 0;
        if (fread(&unigramCount, sizeof(uint32_t), 1, f) != 1) {
            fclose(f);
            return;
        }
        for (uint32_t i = 0; i < unigramCount; i++) {
            uint32_t token; uint32_t count;
            if (fread(&token, sizeof(uint32_t), 1, f) != 1) break;
            if (fread(&count, sizeof(uint32_t), 1, f) != 1) break;
            addNgramWithCount(model->ngrams, &token, 1, count);
        }
        fclose(f);
    }

    // Load bigrams: pairs (prev, next, count)
    snprintf(filename, sizeof(filename), "%s.bi", basePath);
    FILE* f2 = fopen(filename, "rb");
    if (f2) {
        uint32_t bigramCount = 0;
        if (fread(&bigramCount, sizeof(uint32_t), 1, f2) == 1) {
            for (uint32_t i = 0; i < bigramCount; i++) {
                uint32_t prev; uint32_t next; uint32_t count;
                if (fread(&prev, sizeof(uint32_t), 1, f2) != 1) break;
                if (fread(&next, sizeof(uint32_t), 1, f2) != 1) break;
                if (fread(&count, sizeof(uint32_t), 1, f2) != 1) break;
                uint32_t pair[2] = { prev, next };
                addNgramWithCount(model->ngrams, pair, 2, count);
            }
        }
        fclose(f2);
    }

    // Load trigrams: triples (prev1, prev2, next, count)
    snprintf(filename, sizeof(filename), "%s.tri", basePath);
    FILE* f3 = fopen(filename, "rb");
    if (f3) {
        uint32_t trigramCount = 0;
        if (fread(&trigramCount, sizeof(uint32_t), 1, f3) == 1) {
            for (uint32_t i = 0; i < trigramCount; i++) {
                uint32_t p1, p2, next, count;
                if (fread(&p1, sizeof(uint32_t), 1, f3) != 1) break;
                if (fread(&p2, sizeof(uint32_t), 1, f3) != 1) break;
                if (fread(&next, sizeof(uint32_t), 1, f3) != 1) break;
                if (fread(&count, sizeof(uint32_t), 1, f3) != 1) break;
                uint32_t triple[3] = { p1, p2, next };
                addNgramWithCount(model->ngrams, triple, 3, count);
            }
        }
        fclose(f3);
    }
}

// Predict the next token given a context
typedef struct {
    uint32_t token;
    uint32_t count;
} TokenCount;

static int compareTokenCountDesc(const void* a, const void* b) {
    const TokenCount* ta = (const TokenCount*)a;
    const TokenCount* tb = (const TokenCount*)b;
    if (tb->count > ta->count) return 1;
    if (tb->count < ta->count) return -1;
    return 0;
}

void predictNextToken(const LMModel* model, const char* context, 
                     uint32_t* topTokens, float* scores, int k) {
    if (!model || !context || !topTokens || !scores || k <= 0) return;
    
    // Tokenize the context
    Sentence s = initSentence();
    tokenizeLine(context, &s);
    
    if (s.length == 0) return;
    
    // Backward reasoning with multi-order backoff
    // Aggregate candidate scores from longest suffix to shortest, weighting longer fragments higher
    const int maxContext = (s.length < (model->maxN - 1)) ? s.length : (model->maxN - 1);
    typedef struct { uint32_t token; float score; } CandScore;
    CandScore cand[MaxCandidates];
    int candCount = 0;
    // Weights
    const float Decay = 0.85f;       // decay per step farther from last token
    const float BetaUnigram = 0.10f; // prior weight for unigram log-probability
    
    // Temporary buffer for context token IDs
    uint32_t ctxIds[model->maxN];
    
    for (int L = maxContext; L >= 1; L--) {
        // Build suffix of length L
        bool ok = true;
        for (int i = 0; i < L; i++) {
            const char* t = s.sequence[s.length - L + i].text;
            uint32_t id = hashMapGet(model->vocab->tokenToId, t);
            if (id == 0) { ok = false; break; }
            ctxIds[i] = id;
        }
        if (!ok) continue;
        
        // Find prefix node for this suffix
        NgramNode* node = findPrefixNode(model->ngrams, ctxIds, L);
        if (!node || !node->children) continue;
        
        // Denominator: sum of children counts
        uint32_t denom = 0;
        for (NgramNode* ch = node->children; ch; ch = ch->next) denom += ch->count;
        if (denom == 0) continue;
        
        // Weight: prefer longer L and apply decay for more distant fragments
        float w = (float)L * powf(Decay, (float)(maxContext - L));
        
        // Accumulate normalized counts into candidate scores
        for (NgramNode* ch = node->children; ch; ch = ch->next) {
            float contrib = w * ((float)ch->count / (float)denom);
            // Find or insert candidate
            int idx = -1;
            for (int j = 0; j < candCount; j++) { if (cand[j].token == ch->tokenId) { idx = j; break; } }
            if (idx == -1) {
                if (candCount < MaxCandidates) {
                    cand[candCount].token = ch->tokenId;
                    cand[candCount].score = contrib;
                    candCount++;
                }
            } else {
                cand[idx].score += contrib;
            }
        }
    }
    
    // If we have candidates, sort and fill outputs
    int filled = 0;
    if (candCount > 0) {
        // Add unigram prior to each candidate: Beta * log P_unigram
        if (model->totalTokens > 0) {
            for (int i = 0; i < candCount; i++) {
                uint32_t t = cand[i].token;
                uint32_t tarr[1] = { t };
                uint32_t c = getNgramCount(model->ngrams, tarr, 1);
                float p = (c > 0) ? ((float)c / (float)model->totalTokens) : (1.0f / (float)(model->totalTokens + 1));
                cand[i].score += BetaUnigram * logf(fmaxf(p, 1e-9f));
            }
        }
        // Convert to TokenCount-like array for sorting
        TokenCount* arr = (TokenCount*)malloc(sizeof(TokenCount) * candCount);
        if (arr) {
            for (int i = 0; i < candCount; i++) {
                arr[i].token = cand[i].token;
                // Temporarily store score scaled to integer by 1e6 for sorting reuse
                arr[i].count = (uint32_t)(cand[i].score * 1000000.0f);
            }
            if (candCount > 1) qsort(arr, candCount, sizeof(TokenCount), compareTokenCountDesc);
            for (int i = 0; i < k; i++) {
                if (i < candCount) {
                    topTokens[i] = arr[i].token;
                    // Recover approximate score by rescaling; alternatively renormalize below
                    scores[i] = (float)arr[i].count / 1000000.0f;
                    filled++;
                } else { topTokens[i] = 0; scores[i] = 0.0f; }
            }
            free(arr);
            // Optional: renormalize top-k scores to sum to 1
            float sum = 0.0f; for (int i = 0; i < filled; i++) sum += scores[i];
            if (sum > 0.0f) { for (int i = 0; i < filled; i++) scores[i] /= sum; }
        }
    }
    
    if (!filled || filled < k) {
        // Fallback: Unigram ranking
        NgramIterator* it = ngramIterator(model->ngrams);
        if (!it) return;
        size_t uniCount = 0;
        {
            uint32_t token; uint32_t c;
            while (nextNgram(it, &token, 1, &c)) { uniCount++; }
        }
        freeNgramIterator(it);
        TokenCount* arr = NULL;
        if (uniCount > 0) arr = (TokenCount*)malloc(sizeof(TokenCount) * uniCount);
        size_t idx = 0;
        it = ngramIterator(model->ngrams);
        if (arr && it) {
            uint32_t token; uint32_t c;
            while (nextNgram(it, &token, 1, &c)) { arr[idx].token = token; arr[idx].count = c; idx++; }
        }
        if (it) freeNgramIterator(it);
        if (arr && uniCount > 1) qsort(arr, uniCount, sizeof(TokenCount), compareTokenCountDesc);
        // If some slots already filled by bigram, append from unigrams skipping duplicates
        if (arr && model->totalTokens > 0) {
            int outIdx = filled;
            for (size_t j = 0; j < uniCount && outIdx < k; j++) {
                uint32_t cand = arr[j].token;
                int dup = 0;
                for (int t = 0; t < outIdx; t++) {
                    if (topTokens[t] == cand) { dup = 1; break; }
                }
                if (!dup) {
                    topTokens[outIdx] = cand;
                    scores[outIdx] = (float)arr[j].count / (float)model->totalTokens;
                    outIdx++;
                }
            }
            // Zero any remaining
            for (int t = outIdx; t < k; t++) { topTokens[t] = 0; scores[t] = 0.0f; }
        } else {
            for (int i = 0; i < k; i++) { topTokens[i] = 0; scores[i] = 0.0f; }
        }
        if (arr) free(arr);
    }
}
