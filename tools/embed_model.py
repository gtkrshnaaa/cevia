#!/usr/bin/env python3
"""
Embed Cevia model data into C source files for single-file distribution.
Usage: python3 tools/embed_model.py data/bin/cevia_id
"""

import sys
import struct
import os

def read_vocab(vocab_file):
    """Read vocabulary file and return list of tokens."""
    tokens = []
    with open(vocab_file, 'rb') as f:
        # Read count (uint32_t)
        count_bytes = f.read(4)
        if len(count_bytes) < 4:
            return tokens
        count = struct.unpack('I', count_bytes)[0]
        
        for i in range(count):
            # Read token length (uint16_t)
            len_bytes = f.read(2)
            if len(len_bytes) < 2:
                break
            length = struct.unpack('H', len_bytes)[0]
            
            # Read token string
            token_bytes = f.read(length)
            if len(token_bytes) < length:
                break
            token = token_bytes.decode('utf-8', errors='replace')
            
            tokens.append((token, i))  # ID is just the index
    
    return tokens

def read_ngrams(ngram_file, n):
    """Read n-gram file and return list of n-grams."""
    ngrams = []
    with open(ngram_file, 'rb') as f:
        while True:
            # Read n token IDs + count
            chunk_size = (n + 1) * 4
            chunk = f.read(chunk_size)
            if len(chunk) < chunk_size:
                break
            
            values = struct.unpack('I' * (n + 1), chunk)
            ngrams.append(values)
    
    return ngrams

def embed_vocab(tokens, output_file):
    """Generate C source file with embedded vocabulary."""
    with open(output_file, 'w') as f:
        f.write('// Auto-generated embedded vocabulary\n')
        f.write('#include <stdint.h>\n\n')
        f.write(f'const uint32_t EMBEDDED_VOCAB_SIZE = {len(tokens)};\n\n')
        
        # Token strings
        f.write('const char* EMBEDDED_VOCAB_TOKENS[] = {\n')
        for token, _ in tokens:
            # Escape special characters
            escaped = token.replace('\\', '\\\\').replace('"', '\\"')
            f.write(f'    "{escaped}",\n')
        f.write('};\n\n')
        
        # Token IDs
        f.write('const uint32_t EMBEDDED_VOCAB_IDS[] = {\n')
        for _, token_id in tokens:
            f.write(f'    {token_id},\n')
        f.write('};\n')
    
    print(f'✓ Embedded {len(tokens)} vocabulary tokens')

def embed_ngrams(ngrams, n, output_file, name):
    """Generate C source file with embedded n-grams."""
    with open(output_file, 'w') as f:
        f.write(f'// Auto-generated embedded {n}-grams\n')
        f.write('#include <stdint.h>\n\n')
        f.write(f'const uint32_t EMBEDDED_{name}_SIZE = {len(ngrams)};\n\n')
        f.write(f'const uint32_t EMBEDDED_{name}[][{n+1}] = {{\n')
        
        for ngram in ngrams:
            values_str = ', '.join(str(v) for v in ngram)
            f.write(f'    {{{values_str}}},\n')
        
        f.write('};\n')
    
    print(f'✓ Embedded {len(ngrams)} {n}-grams')

def main():
    if len(sys.argv) < 2:
        print('Usage: python3 tools/embed_model.py <model_prefix>')
        print('Example: python3 tools/embed_model.py data/bin/cevia_id')
        sys.exit(1)
    
    prefix = sys.argv[1]
    
    # Check if model files exist
    vocab_file = f'{prefix}.vocab'
    uni_file = f'{prefix}.uni'
    bi_file = f'{prefix}.bi'
    tri_file = f'{prefix}.tri'
    
    if not os.path.exists(vocab_file):
        print(f'Error: {vocab_file} not found')
        print('Please train the model first: make train')
        sys.exit(1)
    
    print('Embedding model data...')
    
    # Read and embed vocabulary
    tokens = read_vocab(vocab_file)
    embed_vocab(tokens, 'src/embedded_vocab.c')
    
    # Read and embed unigrams
    if os.path.exists(uni_file):
        unigrams = read_ngrams(uni_file, 1)
        embed_ngrams(unigrams, 1, 'src/embedded_uni.c', 'UNI')
    
    # Read and embed bigrams
    if os.path.exists(bi_file):
        bigrams = read_ngrams(bi_file, 2)
        embed_ngrams(bigrams, 2, 'src/embedded_bi.c', 'BI')
    
    # Read and embed trigrams
    if os.path.exists(tri_file):
        trigrams = read_ngrams(tri_file, 3)
        embed_ngrams(trigrams, 3, 'src/embedded_tri.c', 'TRI')
    
    print('\n✓ Model data embedded successfully!')
    print('  Generated files:')
    print('    - src/embedded_vocab.c')
    print('    - src/embedded_uni.c')
    print('    - src/embedded_bi.c')
    print('    - src/embedded_tri.c')
    print('\nNow run: make MODE=embedded')

if __name__ == '__main__':
    main()
