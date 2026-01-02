#include "../include/lmModel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

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

// Save model as C source files for embedding
void saveModel(const LMModel* model, const char* basePath) {
    if (!model || !basePath) return;
    
    // Save vocabulary (already generates C file)
    char filename[1024];
    snprintf(filename, sizeof(filename), "%s.vocab", basePath);
    saveVocabulary(model->vocab, filename);
    
    // Generate embedded_ngrams.c with all n-grams
    FILE* f = fopen("src/embedded_ngrams.c", "w");
    if (!f) {
        perror("Failed to create embedded ngrams file");
        return;
    }
    
    fprintf(f, "// Auto-generated embedded n-grams\n");
    fprintf(f, "#include <stdint.h>\n\n");
    
    // Save total tokens
    fprintf(f, "const uint64_t EMBEDDED_TOTAL_TOKENS = %luULL;\n\n", model->totalTokens);
    
    // Count and save unigrams
    uint32_t unigramCount = 0;
    {
        NgramIterator* it = ngramIterator(model->ngrams);
        uint32_t token, count;
        while (nextNgram(it, &token, 1, &count)) {
            unigramCount++;
        }
        freeNgramIterator(it);
    }
    
    fprintf(f, "const uint32_t EMBEDDED_UNI_SIZE = %u;\n", unigramCount);
    fprintf(f, "const uint32_t EMBEDDED_UNI[][2] = {\n");
    {
        NgramIterator* it = ngramIterator(model->ngrams);
        uint32_t token, count;
        while (nextNgram(it, &token, 1, &count)) {
            fprintf(f, "    {%u, %u},\n", token, count);
        }
        freeNgramIterator(it);
    }
    fprintf(f, "};\n\n");
    
    // Count and save bigrams
    uint32_t bigramCount = 0;
    if (model->ngrams && model->ngrams->root) {
        NgramNode* first = model->ngrams->root->children;
        while (first) {
            NgramNode* second = first->children;
            while (second) {
                bigramCount++;
                second = second->next;
            }
            first = first->next;
        }
    }
    
    fprintf(f, "const uint32_t EMBEDDED_BI_SIZE = %u;\n", bigramCount);
    fprintf(f, "const uint32_t EMBEDDED_BI[][3] = {\n");
    if (model->ngrams && model->ngrams->root) {
        NgramNode* first = model->ngrams->root->children;
        while (first) {
            NgramNode* second = first->children;
            while (second) {
                fprintf(f, "    {%u, %u, %u},\n", 
                       first->tokenId, second->tokenId, second->count);
                second = second->next;
            }
            first = first->next;
        }
    }
    fprintf(f, "};\n\n");
    
    // Count and save trigrams
    uint32_t trigramCount = 0;
    if (model->ngrams && model->ngrams->root) {
        NgramNode* first = model->ngrams->root->children;
        while (first) {
            NgramNode* second = first->children;
            while (second) {
                NgramNode* third = second->children;
                while (third) {
                    trigramCount++;
                    third = third->next;
                }
                second = second->next;
            }
            first = first->next;
        }
    }
    
    fprintf(f, "const uint32_t EMBEDDED_TRI_SIZE = %u;\n", trigramCount);
    fprintf(f, "const uint32_t EMBEDDED_TRI[][4] = {\n");
    if (model->ngrams && model->ngrams->root) {
        NgramNode* first = model->ngrams->root->children;
        while (first) {
            NgramNode* second = first->children;
            while (second) {
                NgramNode* third = second->children;
                while (third) {
                    fprintf(f, "    {%u, %u, %u, %u},\n",
                           first->tokenId, second->tokenId, 
                           third->tokenId, third->count);
                    third = third->next;
                }
                second = second->next;
            }
            first = first->next;
        }
    }
    fprintf(f, "};\n");
    
    fclose(f);
    printf("Generated embedded n-grams: src/embedded_ngrams.c\n");
    printf("  Unigrams: %u\n", unigramCount);
    printf("  Bigrams: %u\n", bigramCount);
    printf("  Trigrams: %u\n", trigramCount);
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

// Helper: Sample token from probability distribution with temperature
static uint32_t sampleToken(uint32_t* tokens, float* scores, int k, float temperature) {
    if (k <= 0) return 0;
    
    // Greedy sampling (temperature = 0)
    if (temperature <= 0.01f) {
        return tokens[0];
    }
    
    // Temperature-scaled sampling
    float adjusted[64];  // Max k=64
    if (k > 64) k = 64;
    
    float sum = 0.0f;
    for (int i = 0; i < k; i++) {
        if (scores[i] <= 0.0f) break;
        adjusted[i] = expf(logf(scores[i] + 1e-9f) / temperature);
        sum += adjusted[i];
    }
    
    if (sum <= 0.0f) return tokens[0];
    
    // Normalize
    for (int i = 0; i < k; i++) {
        adjusted[i] /= sum;
    }
    
    // Sample from distribution
    float r = (float)rand() / (float)RAND_MAX;
    float cumulative = 0.0f;
    for (int i = 0; i < k; i++) {
        cumulative += adjusted[i];
        if (r <= cumulative) {
            return tokens[i];
        }
    }
    
    return tokens[0];  // Fallback
}

// Helper: Score response quality
static float scoreResponse(const char* response, int tokenCount) {
    float score = 0.0f;
    
    // Length scoring (prefer 5-15 tokens)
    if (tokenCount < 3) {
        score -= 10.0f;  // Too short
    } else if (tokenCount > 20) {
        score -= (tokenCount - 20) * 0.5f;  // Too long penalty
    } else if (tokenCount >= 5 && tokenCount <= 15) {
        score += 5.0f;  // Ideal length
    }
    
    // Check for repetition
    Sentence s = initSentence();
    tokenizeLine(response, &s);
    for (int i = 0; i < s.length - 1; i++) {
        for (int j = i + 1; j < s.length; j++) {
            if (strcmp(s.sequence[i].text, s.sequence[j].text) == 0) {
                score -= 2.0f;  // Repetition penalty
            }
        }
    }
    
    return score;
}

// Helper: Advanced stopping conditions
static int shouldStop(const char* generated, int tokenCount, const char* lastToken, float lastScore) {
    if (tokenCount <= 0) return 0;
    
    // Check for sentence endings (punctuation)
    if (lastToken) {
        size_t len = strlen(lastToken);
        if (len > 0) {
            char last = lastToken[len - 1];
            if (last == '.' || last == '?' || last == '!') return 1;
        }
    }
    
    // Check for natural ending words (only after minimum length)
    if (tokenCount >= 5) {
        if (lastToken && (
            strcmp(lastToken, "ya") == 0 ||
            strcmp(lastToken, "oke") == 0 ||
            strcmp(lastToken, "siap") == 0 ||
            strcmp(lastToken, "pasti") == 0 ||
            strcmp(lastToken, "deh") == 0 ||
            strcmp(lastToken, "dong") == 0 ||
            strcmp(lastToken, "kok") == 0
        )) {
            return 1;
        }
    }
    
    // Stop if confidence too low
    if (lastScore < 0.03f && tokenCount >= 3) {
        return 1;
    }
    
    // Force stop if too long
    if (tokenCount >= 25) {
        return 1;
    }
    
    return 0;
}

// Helper: Detect repetition (improved)
static int hasRepetition(uint32_t* tokenHistory, int count) {
    if (count < 2) return 0;
    
    // Check if last 3 tokens are the same
    if (count >= 3) {
        if (tokenHistory[count-1] == tokenHistory[count-2] &&
            tokenHistory[count-2] == tokenHistory[count-3]) {
            return 1;
        }
    }
    
    // Check for 2-token loops (e.g., "yuk masuk yuk masuk")
    if (count >= 4) {
        if (tokenHistory[count-1] == tokenHistory[count-3] &&
            tokenHistory[count-2] == tokenHistory[count-4]) {
            return 1;
        }
    }
    
    return 0;
}

// Auto-regressive text generation
void generateResponse(const LMModel* model, const char* input,
                     char* output, int maxTokens, float temperature) {
    if (!model || !input || !output) return;
    
    // Initialize random seed
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }
    
    // Tokenize input to get starting context
    Sentence s = initSentence();
    tokenizeLine(input, &s);
    
    if (s.length == 0) {
        output[0] = '\0';
        return;
    }
    
    // Build initial context from last tokens (up to 7 for balance)
    char context[512] = "";
    int contextStart = (s.length > 7) ? (s.length - 7) : 0;
    for (int i = contextStart; i < s.length; i++) {
        if (i > contextStart) strcat(context, " ");
        strcat(context, s.sequence[i].text);
    }
    
    // Generation buffer
    char generated[2048] = "";
    uint32_t tokenHistory[100];
    int tokenCount = 0;
    
    // Generation loop
    for (int i = 0; i < maxTokens && i < 100; i++) {
        // Predict next token
        uint32_t topTokens[10];
        float scores[10];
        predictNextToken(model, context, topTokens, scores, 10);
        
        // Check if we got valid predictions
        if (scores[0] <= 0.0f) break;
        
        // Sample token based on temperature
        uint32_t nextToken = sampleToken(topTokens, scores, 10, temperature);
        
        // Get token text
        const char* tokenText = getTokenById(model->vocab, nextToken);
        if (!tokenText || strlen(tokenText) == 0) break;
        
        // Append to generated text
        if (strlen(generated) > 0) {
            strcat(generated, " ");
        }
        strcat(generated, tokenText);
        
        // Update context: keep last 7 tokens for balance
        char newContext[512];
        Sentence ctxSent = initSentence();
        tokenizeLine(context, &ctxSent);
        
        // Add new token
        if (ctxSent.length < 7) {
            snprintf(newContext, sizeof(newContext), "%s %s", context, tokenText);
        } else {
            // Keep last 6 tokens + new token
            newContext[0] = '\0';
            for (int j = ctxSent.length - 6; j < ctxSent.length; j++) {
                if (strlen(newContext) > 0) strcat(newContext, " ");
                strcat(newContext, ctxSent.sequence[j].text);
            }
            strcat(newContext, " ");
            strcat(newContext, tokenText);
        }
        snprintf(context, sizeof(context), "%s", newContext);
        
        // Store in history
        tokenHistory[tokenCount] = nextToken;
        tokenCount++;
        
        // Check stopping conditions with score awareness
        if (shouldStop(generated, tokenCount, tokenText, scores[0])) break;
        if (hasRepetition(tokenHistory, tokenCount)) break;
    }
    
    // Copy to output
    snprintf(output, 2048, "%s", generated);
}

