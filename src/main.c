#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/common.h"
#include "../include/lmModel.h"
#include "../include/vocab.h"

// Print usage information
void printUsage(const char* programName) {
    printf("Usage: %s <command> [arguments]\n", programName);
    printf("Commands:\n");
    printf("  -h, --help                              Show this help message\n");
    printf("  -v, --version                           Show application version\n");
    printf("  train <corpus.txt> [--model-prefix P]   Train model (default prefix if omitted)\n");
    printf("  run [--model-prefix P] [--top-k N]      Run interactive inference\n");
    printf("  predict <model_prefix> <context> [--top-k N]  Predict next token\n");
    printf("  eval <corpus.txt> [--model-prefix P] [--top-k N]  Evaluate top-k hit rate\n");
    printf("  chat [--model-prefix P] [--temp T] [--max-tokens N]  Chat mode (full responses)\n");
    printf("  generate <model_prefix> <input> [--temp T] [--max-tokens N]  Generate single response\n");
    printf("  interactive                              Alias of 'run' (deprecated)\n");
}

// Evaluate top-k unigram hit rate on a corpus file
static void evaluateCorpus(LMModel* model, const char* filename, int topK) {
    if (!model || !filename) return;
    if (topK <= 0) topK = 5;
    if (topK > 64) topK = 64;

    FILE* f = fopen(filename, "r");
    if (!f) {
        perror("Failed to open corpus for eval");
        return;
    }

    uint64_t total = 0; // number of next-token predictions evaluated
    uint64_t hits = 0;  // number of times gold token is in top-k

    char line[4096];
    while (fgets(line, sizeof(line), f)) {
        Sentence s = initSentence();
        tokenizeLine(line, &s);
        if (s.length <= 1) continue;
        for (int i = 1; i < s.length; i++) {
            // Context: previous token text
            char context[256];
            snprintf(context, sizeof(context), "%s", s.sequence[i - 1].text);

            uint32_t topTokens[64] = {0};
            float scores[64] = {0};
            predictNextToken(model, context, topTokens, scores, topK);

            // Gold token id
            const char* gold = s.sequence[i].text;
            uint32_t goldId = hashMapGet(model->vocab->tokenToId, gold);

            int matched = 0;
            for (int k = 0; k < topK && scores[k] > 0.0f; k++) {
                if (topTokens[k] == goldId) { matched = 1; break; }
            }
            if (matched) hits++;
            total++;
        }
    }
    fclose(f);

    double hitRate = (total > 0) ? (100.0 * (double)hits / (double)total) : 0.0;
    printf("Eval results on %s\n", filename);
    printf("  Pairs evaluated: %llu\n", (unsigned long long)total);
    printf("  Top-%d hits: %llu\n", topK, (unsigned long long)hits);
    printf("  Hit rate: %.2f%%\n", hitRate);
}
// Interactive mode
void interactiveMode(LMModel* model, int topK) {
    if (!model) return;
    
    printf("Interactive mode. Type 'exit' to quit.\n");
    
    char input[1024];
    while (1) {
        printf("\nEnter context: ");
        if (!fgets(input, sizeof(input), stdin)) break;
        
        // Remove newline
        size_t len = strlen(input);
        if (len > 0 && input[len-1] == '\n') {
            input[len-1] = '\0';
        }
        
        // Check for exit command
        if (strcmp(input, "exit") == 0) break;
        
        // Normalize context: tokenize and use the last token (consistent with training)
        char context[256] = {0};
        {
            Sentence s = initSentence();
            tokenizeLine(input, &s);
            if (s.length > 0) {
                snprintf(context, sizeof(context), "%s", s.sequence[s.length - 1].text);
            } else {
                // fallback to raw input if tokenization yields nothing
                snprintf(context, sizeof(context), "%s", input);
            }
        }

        // Predict next tokens
        if (topK <= 0) topK = 5;
        if (topK > 64) topK = 64; // sane cap
        uint32_t topTokens[64] = {0};
        float scores[64] = {0};
        predictNextToken(model, context, topTokens, scores, topK);
        
        // Print predictions
        printf("Predictions:\n");
        for (int i = 0; i < topK && scores[i] > 0.0f; i++) {
            const char* token = getTokenById(model->vocab, topTokens[i]);
            printf("  %s (%.2f%%)\n", token, scores[i] * 100);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    const char* command = argv[1];

    // Help flag
    if (strcmp(command, "-h") == 0 || strcmp(command, "--help") == 0) {
        printUsage(argv[0]);
        return 0;
    }

    // Version flag
    if (strcmp(command, "-v") == 0 || strcmp(command, "--version") == 0) {
        printf("cevia %s\n", AppVersion);
        return 0;
    }
    
    if (strcmp(command, "train") == 0) {
        if (argc < 3) {
            printf("Error: Missing corpus file for train command\n");
            printUsage(argv[0]);
            return 1;
        }
        
        const char* trainingFile = argv[2];
        const char* modelPrefix = DefaultModelPrefix;
        // Optional flags: --model-prefix PREFIX
        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--model-prefix") == 0 && (i + 1) < argc) {
                modelPrefix = argv[i + 1];
                i++;
            }
        }
        
        printf("Training new model from file: %s\n", trainingFile);
        
        // Create and train model
        LMModel* model = createLMModel(4);  // Use 4-grams
        if (!model) {
            printf("Error: Failed to create model\n");
            return 1;
        }
        
        trainFromFile(model, trainingFile);
        saveModel(model, modelPrefix);
        
        printf("Training complete. Model saved with prefix: %s\n", modelPrefix);
        freeLMModel(model);
        
    } else if (strcmp(command, "predict") == 0) {
        if (argc < 4) {
            printf("Error: Missing arguments for predict command\n");
            printUsage(argv[0]);
            return 1;
        }
        
        const char* modelPrefix = argv[2];
        char rawContextBuf[1024] = {0};
        // Concatenate argv[3..] until an option flag (starts with '--')
        {
            int started = 0;
            for (int i = 3; i < argc; i++) {
                if (strncmp(argv[i], "--", 2) == 0) break;
                if (started) strncat(rawContextBuf, " ", sizeof(rawContextBuf) - strlen(rawContextBuf) - 1);
                strncat(rawContextBuf, argv[i], sizeof(rawContextBuf) - strlen(rawContextBuf) - 1);
                started = 1;
            }
            if (!started) { snprintf(rawContextBuf, sizeof(rawContextBuf), "%s", argv[3]); }
        }
        const char* rawContext = rawContextBuf;
        char context[256] = {0};
        int topK = 5;
        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--top-k") == 0 && (i + 1) < argc) {
                topK = atoi(argv[i + 1]);
                i++;
            }
        }
        
        // Load model
        LMModel* model = createLMModel(4);
        if (!model) {
            printf("Error: Failed to create model\n");
            return 1;
        }
        
        loadModel(model, modelPrefix);
        
        // Normalize context similar to training: tokenize and take last token
        {
            Sentence s = initSentence();
            tokenizeLine(rawContext, &s);
            if (s.length > 0) {
                snprintf(context, sizeof(context), "%s", s.sequence[s.length - 1].text);
            } else {
                snprintf(context, sizeof(context), "%s", rawContext);
            }
        }

        // Make prediction
        if (topK <= 0) topK = 5;
        if (topK > 64) topK = 64;
        uint32_t topTokens[64] = {0};
        float scores[64] = {0};
        predictNextToken(model, context, topTokens, scores, topK);
        
        // Print results
        printf("Context: %s\n", context);
        printf("Predictions:\n");
        for (int i = 0; i < topK && scores[i] > 0.0f; i++) {
            const char* token = getTokenById(model->vocab, topTokens[i]);
            printf("  %s (%.2f%%)\n", token, scores[i] * 100);
        }
        
        freeLMModel(model);
        
    } else if (strcmp(command, "run") == 0 || strcmp(command, "interactive") == 0) {
        const char* modelPrefix = DefaultModelPrefix;
        int topK = 5;
        // Optional flags
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--model-prefix") == 0 && (i + 1) < argc) {
                modelPrefix = argv[i + 1];
                i++;
            } else if (strcmp(argv[i], "--top-k") == 0 && (i + 1) < argc) {
                topK = atoi(argv[i + 1]);
                i++;
            }
        }
        
        // Load model
        LMModel* model = createLMModel(4);
        if (!model) {
            printf("Error: Failed to create model\n");
            return 1;
        }
        
        loadModel(model, modelPrefix);
        
        // Start interactive mode
        interactiveMode(model, topK);
        
        freeLMModel(model);
    } else if (strcmp(command, "eval") == 0) {
        if (argc < 2) {
            printUsage(argv[0]);
            return 1;
        }
        const char* modelPrefix = DefaultModelPrefix;
        const char* evalFile = NULL;
        int topK = 5;
        // Parse positional and optional args: eval <corpus.txt> [--model-prefix P] [--top-k N]
        if (argc >= 3) {
            evalFile = argv[2];
        }
        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--model-prefix") == 0 && (i + 1) < argc) {
                modelPrefix = argv[i + 1];
                i++;
            } else if (strcmp(argv[i], "--top-k") == 0 && (i + 1) < argc) {
                topK = atoi(argv[i + 1]);
                i++;
            }
        }
        if (!evalFile) {
            printf("Error: Missing corpus file for eval command\n");
            printUsage(argv[0]);
            return 1;
        }

        // Load model
        LMModel* model = createLMModel(4);
        if (!model) {
            printf("Error: Failed to create model\n");
            return 1;
        }
        loadModel(model, modelPrefix);
        evaluateCorpus(model, evalFile, topK);
        freeLMModel(model);
    } else if (strcmp(command, "chat") == 0) {
        const char* modelPrefix = DefaultModelPrefix;
        float temperature = 0.7f;
        int maxTokens = 20;
        
        // Parse optional flags
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--model-prefix") == 0 && (i + 1) < argc) {
                modelPrefix = argv[i + 1];
                i++;
            } else if (strcmp(argv[i], "--temp") == 0 && (i + 1) < argc) {
                temperature = atof(argv[i + 1]);
                i++;
            } else if (strcmp(argv[i], "--max-tokens") == 0 && (i + 1) < argc) {
                maxTokens = atoi(argv[i + 1]);
                i++;
            }
        }
        
        // Load model
        LMModel* model = createLMModel(4);
        if (!model) {
            printf("Error: Failed to create model\n");
            return 1;
        }
        loadModel(model, modelPrefix);
        
        // Chat mode
        printf("Cevia Chat Mode (type 'exit' to quit)\n");
        printf("Temperature: %.2f, Max tokens: %d\n\n", temperature, maxTokens);
        
        char input[1024];
        while (1) {
            printf("You: ");
            if (!fgets(input, sizeof(input), stdin)) break;
            
            // Remove newline
            size_t len = strlen(input);
            if (len > 0 && input[len-1] == '\n') {
                input[len-1] = '\0';
            }
            
            // Check for exit
            if (strcmp(input, "exit") == 0) break;
            if (strlen(input) == 0) continue;
            
            // Generate response
            char response[2048];
            generateResponse(model, input, response, maxTokens, temperature);
            
            printf("Cevia: %s\n\n", response);
        }
        
        freeLMModel(model);
        
    } else if (strcmp(command, "generate") == 0) {
        if (argc < 4) {
            printf("Error: Missing arguments for generate command\n");
            printUsage(argv[0]);
            return 1;
        }
        
        const char* modelPrefix = argv[2];
        char inputBuf[1024] = {0};
        float temperature = 0.7f;
        int maxTokens = 20;
        
        // Concatenate input until option flag
        {
            int started = 0;
            for (int i = 3; i < argc; i++) {
                if (strncmp(argv[i], "--", 2) == 0) break;
                if (started) strncat(inputBuf, " ", sizeof(inputBuf) - strlen(inputBuf) - 1);
                strncat(inputBuf, argv[i], sizeof(inputBuf) - strlen(inputBuf) - 1);
                started = 1;
            }
        }
        
        // Parse optional flags
        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--temp") == 0 && (i + 1) < argc) {
                temperature = atof(argv[i + 1]);
                i++;
            } else if (strcmp(argv[i], "--max-tokens") == 0 && (i + 1) < argc) {
                maxTokens = atoi(argv[i + 1]);
                i++;
            }
        }
        
        // Load model
        LMModel* model = createLMModel(4);
        if (!model) {
            printf("Error: Failed to create model\n");
            return 1;
        }
        loadModel(model, modelPrefix);
        
        // Generate response
        char response[2048];
        generateResponse(model, inputBuf, response, maxTokens, temperature);
        
        printf("Input: %s\n", inputBuf);
        printf("Response: %s\n", response);
        
        freeLMModel(model);
        
    } else {
        printf("Error: Unknown command '%s'\n", command);
        printUsage(argv[0]);
        return 1;
    }
    
    return 0;
}

