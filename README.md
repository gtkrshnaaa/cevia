# **Cevia: Stateless Backward Reasoning Language Model (C Implementation)**

## **1. Overview**

This documentation describes a **stateless, pattern-based language model implemented in C**. It is designed to process large curated text datasets to generate predictions for the next token in a sequence using **backward reasoning**. The model is:

* **Stateless:** Each inference is independent; no hidden state is stored between calls.
* **Dataset-driven:** All knowledge is built from external text files.
* **Compiler-friendly:** Optimized for readability and high-performance C execution.

The model supports:

* Tokenization from large text files.
* N-gram and pattern-based statistics.
* Candidate scoring using backward reasoning.
* Top-k token predictions.

---

## **2. Installation**

1. Clone the repository:

```bash
git clone https://github.com/gtkrshnaaa/cevia.git
cd cevia
```

2. Compile with GCC (example):

```bash
make
```

This will produce the executable for building the vocabulary, updating n-grams, and performing inference.

3. Prepare dataset: Place `.txt` files with curated text in the `data/` folder.

---

## **3. Usage**

### Build Vocabulary

```bash
./build_vocab data/corpus.txt vocab.bin
```

* Reads all text lines from `corpus.txt`.
* Tokenizes each line.
* Maps tokens to IDs and stores vocabulary in `vocab.bin`.

### Build N-grams and Patterns

```bash
./build_ngrams vocab.bin data/corpus.txt ngram.bin pattern.bin
```

* Uses the vocabulary to convert tokens to IDs.
* Updates n-gram counts (up to MaxN).
* Extracts patterns and stores them in `pattern.bin`.

### Predict Next Token

```bash
./predict vocab.bin ngram.bin pattern.bin "prefix tokens here"
```

* Loads model from disk.
* Tokenizes the input prefix.
* Performs **backward reasoning** to predict the next token.

---

## **4. Architecture**

### Components

1. **Tokenizer & Sentence Module**

   * Converts text lines into arrays of `Token`.
   * Maintains `Sentence` structs with array of tokens.

2. **Vocabulary (Vocab)**

   * Maps strings to token IDs and back.
   * Implemented with a hash map (string → int).

3. **N-gram Index (NgramIndex)**

   * Stores prefix sequences and counts of next tokens.
   * Supports fast lookup using **rolling hash**.

4. **Pattern Index (PatternIndex)**

   * Supports wildcard patterns.
   * Stores frequency counts for next-token predictions.

5. **Language Model (LMModel)**

   * Encapsulates `Vocab`, `NgramIndex`, and `PatternIndex`.
   * Provides functions for inference and scoring.

---

## **5. Algorithm: Backward Reasoning**

### Backward Reasoning Algorithm

* For position `t` in a sequence:

  1. Extract all contiguous suffixes ending at `t`.
  2. Match suffixes against known patterns.
  3. Assign scores to candidate next tokens, favoring longer matches but applying decay for distant tokens.

**How it works:**

The backward reasoning algorithm analyzes a sequence of tokens by considering every possible contiguous fragment that ends at the current token. Longer fragments carry more weight, but a decay function reduces influence of distant words. Candidate token scores aggregate n-gram likelihoods, pattern probabilities, and optional global priors.

---

## **6. API Reference (Core Functions)**

### Vocabulary

```c
Vocab vocabInit(void);
int vocabGetOrAdd(Vocab *v, const char tok[]);
void buildVocabFromFile(Vocab *v, const char filename[]);
```

### Sentence

```c
Sentence initSentence(void);
Sentence addToken(Sentence s, const char word[]);
void tokenizeLine(const char line[], Sentence *s);
```

### N-gram

```c
void updateNgrams(NgramIndex *idx, int *tokens, int tlen);
uint64_t rollingHashSeq(int *arr, int len);
int predictNextToken(NgramIndex *idx, int *prefix, int plen);
```

---

## **7. Naming Conventions**

| Identifier Type                  | Convention  |
| -------------------------------- | ----------- |
| Constants & Macros               | PascalCase  |
| Structs & Typedefs               | PascalCase  |
| Functions, Variables, Parameters | camelCase   |
| Library Functions                | snake\_case |

**Memory Handling:**

* Prefer return-by-value for `struct`s.
* Avoid pointer arithmetic where possible.
* Ensures better compiler optimization and safer memory usage.

---

## **8. Optimizations**

* Use custom hash maps with **open addressing**.
* Quantize counts to `int32` or `int16`.
* Use memory pools for pattern and fragment lists.
* Parallelize scoring with OpenMP/pthreads.
* Bloom filters for fast fragment existence checking.

---

## **9. Evaluation**

* Perplexity on held-out corpus.
* Top-k accuracy for next-token predictions.
* Human evaluation for fluency and semantic coherence.

---

## **10. Roadmap**

1. **Prototype 1:** Tokenizer + unigram + n-gram counts → CLI inference.
2. **Prototype 2:** Pattern mining + scoring + disk format + pruning.
3. **Prototype 3:** Optimized C-level execution, quantization, mmap, multi-threading.
4. **Experiment:** Run on curated datasets to evaluate perplexity and top-k accuracy.

---

## **11. File Structure**

```
/src
    tokenizer.c
    vocab.c
    ngram.c
    pattern.c
    lmModel.c
/include
    tokenizer.h
    vocab.h
    ngram.h
    pattern.h
    lmModel.h
/data
    corpus.txt
/bin
    buildvocab
    buildngrams
    predict
Makefile
README.md
```

---

## **12. Notes**

* Fully dataset-driven: no hardcoded tokens.
* Stateless inference ensures deterministic, reproducible predictions.
* Designed for large text corpora; memory-efficient and compiler-friendly.
* Naming conventions provide clarity between **internal code** and **library functions**.

---
