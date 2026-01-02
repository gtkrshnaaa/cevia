#ifndef CEVIA_H
#define CEVIA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle to model (forward declaration)
typedef struct LMModel CeviaModel;

// ============================================================================
// Model Lifecycle
// ============================================================================

/**
 * Create a new Cevia language model
 * @param maxN Maximum n-gram order (typically 3 or 4)
 * @return Pointer to model, or NULL on failure
 */
CeviaModel* cevia_create(int maxN);

/**
 * Free model and all associated resources
 * @param model Model to free
 */
void cevia_free(CeviaModel* model);

// ============================================================================
// Training & Persistence
// ============================================================================

/**
 * Train model from a text corpus file
 * @param model Model to train
 * @param corpus_file Path to corpus file (one sentence per line)
 */
void cevia_train_from_file(CeviaModel* model, const char* corpus_file);

/**
 * Save model to disk (creates .vocab, .uni, .bi, .tri files)
 * @param model Model to save
 * @param prefix Path prefix (e.g., "data/bin/model")
 */
void cevia_save(const CeviaModel* model, const char* prefix);

/**
 * Load model from disk
 * @param model Model to load into
 * @param prefix Path prefix (e.g., "data/bin/model")
 */
void cevia_load(CeviaModel* model, const char* prefix);

// ============================================================================
// Inference
// ============================================================================

/**
 * Predict next tokens given context
 * @param model Trained model
 * @param context Input context string
 * @param top_tokens Output array of top-k token strings (caller allocates)
 * @param scores Output array of scores (caller allocates)
 * @param k Number of top predictions to return
 */
void cevia_predict(const CeviaModel* model,
                   const char* context,
                   char** top_tokens,
                   float* scores,
                   int k);

/**
 * Generate text response using auto-regressive generation
 * @param model Trained model
 * @param input Input prompt
 * @param output Output buffer (caller allocates, min 2048 bytes)
 * @param max_tokens Maximum tokens to generate
 * @param temperature Sampling temperature (0.0 = greedy, 1.0 = random)
 */
void cevia_generate(const CeviaModel* model,
                   const char* input,
                   char* output,
                   int max_tokens,
                   float temperature);

// ============================================================================
// Evaluation
// ============================================================================

/**
 * Evaluate model on a test corpus
 * @param model Trained model
 * @param corpus_file Path to test corpus
 * @param top_k Top-k accuracy to compute
 * @return Hit rate (0.0 to 1.0)
 */
float cevia_evaluate(const CeviaModel* model,
                    const char* corpus_file,
                    int top_k);

// ============================================================================
// Utilities
// ============================================================================

/**
 * Get library version string
 * @return Version string (e.g., "1.0.0")
 */
const char* cevia_version(void);

/**
 * Get vocabulary size
 * @param model Model
 * @return Number of unique tokens in vocabulary
 */
uint32_t cevia_vocab_size(const CeviaModel* model);

/**
 * Get total tokens processed during training
 * @param model Model
 * @return Total token count
 */
uint64_t cevia_total_tokens(const CeviaModel* model);

#ifdef __cplusplus
}
#endif

#endif // CEVIA_H
