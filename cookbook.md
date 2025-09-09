
# **Stateless Language Model (Cevia) Cookbook (C Implementation)**

## **Introduction**

This cookbook describes how to implement a **stateless language model** entirely in C. The model:

1. Mimics human-like reasoning over sequences of tokens.
2. Stores all “knowledge” in compile-time or file-loaded structures (unigrams, n-grams, pattern tables).
3. Is fully dataset-driven; no hardcoded words or sentences.
4. Uses clear naming conventions for readability and compiler optimization.

The model supports **backward reasoning**, analyzing all prior tokens to predict the next token.

---

## **1. Core Concepts**

### Stateless Reasoning

* Each inference is independent; no hidden state is stored between calls.
* Knowledge comes from:

  * Unigram counts
  * N-gram counts
  * Pattern tables (including wildcards)

---

### Backward Reasoning Algorithm

* For position `t` in a sequence:

  1. Extract all contiguous suffixes ending at `t`.
  2. Match suffixes against known patterns.
  3. Assign scores to candidate next tokens, favoring longer matches but applying decay for distant tokens.

**How it works:**

The backward reasoning algorithm analyzes a sequence of tokens by considering every possible contiguous fragment that ends at the current token. This allows the model to leverage context from both immediate and distant preceding tokens. Each fragment is matched against the n-gram or pattern tables, producing evidence for candidate next tokens.

Longer fragments typically provide stronger evidence because they capture more specific contextual information. However, to avoid overfitting to distant words, a **decay function** reduces the influence of fragments originating further back in the sequence.

Candidate token scores are then aggregated from all matched fragments. This aggregation combines n-gram likelihoods, pattern-based probabilities, and optional global priors to produce a final score distribution. The top-scoring tokens can then be selected as predictions or sampled probabilistically.

This approach enables the model to reason in a **human-like fashion**, by “looking backward” across the full input context while maintaining stateless inference at runtime.

---

### Pattern-first Approach

* Beyond n-grams, use **pattern primitives**: frequent sequences with optional wildcard slots.
* Example: `["like", "to", "eat", *]` → candidate words: `"rice"`, `"bread"`, `"noodles"`.

### Scoring Formula

* Combine multiple evidence sources:

```
score[x] = β * log P_unigram(x)
         + Σ_{f ∈ fragments} weight(f) * [ λ1 * log P_ngram(x|f) + λ2 * log P_pattern(x|f) ]
```

* Weighting: longer fragments get higher weight; distant tokens decay.
* Backoff: if fragment has zero count, consider shorter suffix.

---

## **2. Naming Conventions**

| Identifier Type                  | Convention                       |
| -------------------------------- | -------------------------------- |
| Constants & Macros               | PascalCase                       |
| Structs & Typedefs               | PascalCase                       |
| Functions, Variables, Parameters | camelCase                        |
| External Libraries               | snake\_case (e.g., libc, sqlite) |

### Memory and Pointers

* Prefer return-by-value and array notation.
* Minimize raw pointer arithmetic.
* Ensures better compiler optimization (register allocation, vectorization, UB avoidance).

### File/Headers

* Header guards: PascalCase

```c
#ifndef LanguageModelHeader
#define LanguageModelHeader
#endif
```

* Avoid underscores except for library identifiers.

---

## **3. Data Structures**

```c
typedef uint32_t Tid; // Token ID

typedef struct {
    HashMap *tokenToId; // string -> Tid
    char **idToToken;   // array of strings
    size_t size;
} Vocab;

typedef struct {
    HashMap *ngramIndex; // key: uint64 hash(fragment), value: HashMap*(next_tid -> count)
} NgramIndex;

typedef struct {
    HashMap *patternIndex; // key: pattern hash, value: list of (Tid, freq)
} PatternIndex;

typedef struct {
    Vocab vocab;
    NgramIndex ngrams;
    PatternIndex patterns;
    uint64_t totalTokens;
} LMModel;
```

* `Vocab` → maps between token strings and IDs.
* `NgramIndex` → stores prefix sequences and next-token counts.
* `PatternIndex` → supports wildcard patterns and frequency mapping.
* `LMModel` → main language model structure.

---

## **4. Tokenization & Sentence Handling**

```c
#define MaxTokens 128
#define MaxWordLen 32

typedef struct {
    char text[MaxWordLen];
} Token;

typedef struct {
    Token sequence[MaxTokens];
    int length;
} Sentence;

Sentence initSentence(void) {
    Sentence s;
    s.length = 0;
    return s;
}

Sentence addToken(Sentence s, const char word[]) {
    if(s.length < MaxTokens) {
        strncpy(s.sequence[s.length].text, word, MaxWordLen - 1); // snake_case (libc)
        s.sequence[s.length].text[MaxWordLen - 1] = '\0';
        s.length++;
    }
    return s;
}
```

---

### Tokenizer From File

```c
#include <ctype.h>

void tokenizeLine(const char line[], Sentence *s) {
    int i = 0, j = 0;
    char word[MaxWordLen] = {0};
    while(line[i] != '\0') {
        if(isspace(line[i]) || ispunct(line[i])) {
            if(j > 0) {
                word[j] = '\0';
                *s = addToken(*s, word);
                j = 0;
            }
        } else {
            if(j < MaxWordLen - 1) word[j++] = line[i];
        }
        i++;
    }
    if(j > 0) {
        word[j] = '\0';
        *s = addToken(*s, word);
    }
}
```

---

### Build Vocabulary From File

```c
#include <stdio.h>
#include <stdlib.h>
#include "hashmap.h"

Vocab vocabInit(void) {
    Vocab v;
    v.tokenToId = hashmapNew();
    v.idToToken = NULL;
    v.size = 0;
    return v;
}

int vocabGetOrAdd(Vocab *v, const char tok[]) {
    int *pid = hashmapGet(v->tokenToId, tok);
    if(pid) return *pid;

    int id = (int)v->size++;
    char *copy = strdup(tok);
    v->idToToken = realloc(v->idToToken, sizeof(char*) * v->size);
    v->idToToken[id] = copy;
    hashmapPut(v->tokenToId, copy, &id);
    return id;
}

void buildVocabFromFile(Vocab *v, const char filename[]) {
    FILE *fp = fopen(filename, "r");
    if(!fp) { perror("fopen"); exit(1); }
    char line[4096];
    while(fgets(line, sizeof(line), fp)) {
        Sentence s = initSentence();
        tokenizeLine(line, &s);
        for(int i=0;i<s.length;i++) vocabGetOrAdd(v, s.sequence[i].text);
    }
    fclose(fp);
}
```

---

## **5. N-gram Update**

```c
#define MaxN 4

typedef struct {
    HashMap *ngramIndex;
} NgramIndex;

uint64_t rollingHashSeq(int *arr, int len) {
    uint64_t h = 1469598103934665603ULL; // FNV offset
    for(int i=0;i<len;i++){
        h ^= (uint64_t)arr[i];
        h *= 1099511628211ULL;
    }
    return h;
}

void updateNgrams(NgramIndex *idx, int *tokens, int tlen) {
    for(int i=0;i<tlen;i++){
        for(int n=1;n<=MaxN && i+n <= tlen;n++){
            int prefLen = n-1;
            int *pref = &tokens[i];
            int next = tokens[i+n-1];

            uint64_t key = rollingHashSeq(pref, prefLen);
            HashMap *nextMap = hashmapGet(idx->ngramIndex, &key);
            if(!nextMap){
                nextMap = hashmapNew();
                hashmapPut(idx->ngramIndex, &key, nextMap);
            }
            int *cnt = hashmapGet(nextMap, &next);
            if(cnt) (*cnt)++;
            else { int init=1; hashmapPut(nextMap, &next, &init); }
        }
    }
}
```

---

## **6. Stateless Inference**

```c
int predictNextToken(NgramIndex *idx, int *prefix, int plen) {
    for(int i=plen-1;i>=0;i--){
        int *fragment = &prefix[i];
        int fragLen = plen - i;
        uint64_t key = rollingHashSeq(fragment, fragLen);
        HashMap *nextMap = hashmapGet(idx->ngramIndex, &key);
        if(nextMap) {
            int bestTid = -1, bestCount=0;
            HashMapIter it = hashmapIter(nextMap);
            int tid; int cnt;
            while(hashmapNext(&it, &tid, &cnt)){
                if(cnt>bestCount){ bestCount=cnt; bestTid=tid; }
            }
            return bestTid;
        }
    }
    return -1;
}
```

---

## **7. C-Level Optimizations**

* Custom hashmaps with open addressing + rolling hash.
* Quantize counts: `float32`, `int32`, or `int16`.
* Memory pool for fragment lists → cache-friendly layout.
* Parallel candidate scoring → OpenMP / pthreads.
* Bloom filter on load → prune unnecessary lookups.

---

## **8. Evaluation Metrics**

* Perplexity on held-out corpus.
* Top-k accuracy.
* Human evaluation for fluency and coherence.

---

## **9. Roadmap**

1. **Prototype 1**: Tokenizer + unigram/n-gram counts + CLI inference.
2. **Prototype 2**: Pattern mining + scoring + disk format + pruning.
3. **Prototype 3**: Full optimization: quantization, mmap, multithreading.

---

## **10. Summary Principles**

1. Stateless, dataset-driven, inference at compile/run-time.
2. Pattern-based backward reasoning for human-like predictions.
3. Clear naming conventions: PascalCase (struct/constants), camelCase (functions/variables), snake\_case (library).
4. Pointer minimization & ASCII clean → compiler-friendly.
5. Transparent and modular: easy to extend for pattern scoring, top-k sampling, or probabilistic inference.

---

