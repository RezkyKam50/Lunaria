#include "llamaworker.h"
#include <QString>
#include <vector>

LlamaWorker::LlamaWorker() : ctx(nullptr), model(nullptr), sampler(nullptr) {}

LlamaWorker::~LlamaWorker() {
    cleanup();
}

void LlamaWorker::loadModel(const QString &modelPath, const ContextSettings &settings) {
    cleanup();
     
    llama_backend_init();
     
    llama_model_params model_params = llama_model_default_params();
     
    model = llama_model_load_from_file(modelPath.toStdString().c_str(), model_params);
    
    if (!model) {
        emit errorOccurred("Can't load model");
        return;
    }
     
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx        = settings.contextSize;
    ctx_params.n_threads    = settings.threadCount;
    ctx_params.n_batch      = settings.batchSize;
    ctx_params.kv_unified   = true;
     
    ctx = llama_init_from_model(model, ctx_params);
    
    if (!ctx) {
        emit errorOccurred("Failed to initialize context");
        return;
    }
     
    // Initialize with default sampler (will be updated on generation)
    llama_sampler_chain_params sampler_params = llama_sampler_chain_default_params();
    sampler = llama_sampler_chain_init(sampler_params);
    llama_sampler_chain_add(sampler, llama_sampler_init_greedy());
    
    emit modelLoaded();
}

void LlamaWorker::updateSampler(const GenerationSettings &settings) {
    if (sampler) {
        llama_sampler_free(sampler);
    }
    
    llama_sampler_chain_params sampler_params = llama_sampler_chain_default_params();
    sampler = llama_sampler_chain_init(sampler_params);
    
    // Add sampling parameters based on settings
    if (settings.temperature > 0.0) {
        llama_sampler_chain_add(sampler, 
            llama_sampler_init_temp(settings.temperature));
        
        if (settings.topK > 0) {
            llama_sampler_chain_add(sampler, 
                llama_sampler_init_top_k(settings.topK));
        }
        
        if (settings.topP < 1.0) {
            llama_sampler_chain_add(sampler, 
                llama_sampler_init_top_p(settings.topP, 1));
        }
        
        llama_sampler_chain_add(sampler, llama_sampler_init_dist(0));
    } else {
        // Use greedy sampling if temperature is 0
        llama_sampler_chain_add(sampler, llama_sampler_init_greedy());
    }
}

void LlamaWorker::generateResponse(const QString &prompt, const GenerationSettings &settings) {
    if (!ctx || !model) {
        emit errorOccurred("Model not loaded");
        return;
    }
    
    // Update sampler with current settings
    updateSampler(settings);
     
    const llama_vocab *vocab = llama_model_get_vocab(model);
     
    std::vector<llama_token> tokens;
    tokens.resize(prompt.length() + 256);
    
    int n_tokens = llama_tokenize(
                                    vocab,
                                    prompt.toStdString().c_str(),
                                    prompt.length(),
                                    tokens.data(),
                                    tokens.size(),
                                    true,   // add_bos
                                    false   // special
    );
    
    if (n_tokens < 0) {
        tokens.resize(-n_tokens);
        n_tokens = llama_tokenize(
                                    vocab,
                                    prompt.toStdString().c_str(),
                                    prompt.length(),
                                    tokens.data(),
                                    tokens.size(),
                                    true,
                                    false
                                );
    }
    
    tokens.resize(n_tokens);
     
    llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());
     
    if (llama_decode(ctx, batch) != 0) {
        emit errorOccurred("Failed to evaluate prompt");
        return;
    }
     
    QString response;
    
    int max_tokens      = settings.maxTokens;
    int n_cur           = tokens.size();
    int n_ctx           = llama_n_ctx(ctx);
    
    for (int i = 0; i < max_tokens; i++) {
        llama_token new_token = llama_sampler_sample(sampler, ctx, -1);
         
        if (llama_vocab_is_eog(vocab, new_token)) {
            break;
        }
         
        char buf[256];
        int n = llama_token_to_piece(vocab, new_token, buf, sizeof(buf), 0, false);
        
        if (n < 0) {
            break;
        }
        
        QString token_str = QString::fromUtf8(buf, n);
        response += token_str;
         
        emit partialResponse(token_str);
         
        if (n_cur >= n_ctx - 1) {
            emit errorOccurred("Context limit reached");
            break;
        }
         
        llama_sampler_accept(sampler, new_token);
         
        batch = llama_batch_get_one(&new_token, 1);
        
        if (llama_decode(ctx, batch) != 0) {
            emit errorOccurred("Failed to decode token");
            break;
        }
        
        n_cur++;
    }
    
    emit responseGenerated(response);
}

void LlamaWorker::cleanup() {
    if (sampler) {
        llama_sampler_free(sampler);
        sampler = nullptr;
    }
    if (ctx) {
        llama_free(ctx);
        ctx = nullptr;
    }
    if (model) {
        llama_model_free(model);
        model = nullptr;
    }
    llama_backend_free();
}