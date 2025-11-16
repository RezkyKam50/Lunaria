#include "llamaworker.h"
#include <QString>
#include <vector>
#include <cstring>

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
     
    ctx = llama_init_from_model(model, ctx_params);
    
    if (!ctx) {
        emit errorOccurred("Failed to initialize context");
        return;
    }
      
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
        llama_sampler_chain_add(sampler, llama_sampler_init_greedy());
    }
}

QString LlamaWorker::applyChatTemplate(const std::vector<ChatMessage> &messages, bool add_assistant) {
    if (!model) {
        return QString();
    }
    
    const char *tmpl = llama_model_chat_template(model, nullptr);
    if (!tmpl) {
        return QString();
    }
     
    std::vector<llama_chat_message> llama_messages;
    for (const auto &msg : messages) {
        llama_chat_message llama_msg;
        llama_msg.role = strdup(msg.role.toStdString().c_str());
        llama_msg.content = strdup(msg.content.toStdString().c_str());
        llama_messages.push_back(llama_msg);
    }
     
    int required_size = llama_chat_apply_template(
        tmpl, 
        llama_messages.data(), 
        llama_messages.size(), 
        add_assistant, 
        nullptr, 
        0
    );
    
    if (required_size < 0) {
 
        for (auto &msg : llama_messages) {
            free(const_cast<char*>(msg.role));
            free(const_cast<char*>(msg.content));
        }
        return QString();
    }
     
    std::vector<char> formatted(required_size);
    int actual_size = llama_chat_apply_template(
        tmpl,
        llama_messages.data(),
        llama_messages.size(),
        add_assistant,
        formatted.data(),
        formatted.size()
    );
     
    for (auto &msg : llama_messages) {
        free(const_cast<char*>(msg.role));
        free(const_cast<char*>(msg.content));
    }
    
    if (actual_size < 0) {
        return QString();
    }
    
    return QString::fromStdString(std::string(formatted.begin(), formatted.begin() + actual_size));
}

void LlamaWorker::generateResponseWithMessages(const std::vector<ChatMessage> &messages, const GenerationSettings &settings) {
    if (!ctx || !model) {
        emit errorOccurred("Model not loaded");
        return;
    }
    
    updateSampler(settings);
    
    const llama_vocab *vocab = llama_model_get_vocab(model);
     
    QString formatted_prompt = applyChatTemplate(messages, true);
    
    if (formatted_prompt.isEmpty()) {
        emit errorOccurred("Failed to apply chat template");
        return;
    }
    
    std::string prompt_str = formatted_prompt.toStdString();
     
    bool is_first = llama_memory_seq_pos_max(llama_get_memory(ctx), 0) == -1;
     
    std::vector<llama_token> tokens;
    int n_tokens = -llama_tokenize(vocab, prompt_str.c_str(), prompt_str.size(), nullptr, 0, is_first, true);
    
    if (n_tokens < 0) {
        emit errorOccurred("Failed to tokenize prompt");
        return;
    }
    
    tokens.resize(n_tokens);
    if (llama_tokenize(vocab, prompt_str.c_str(), prompt_str.size(), tokens.data(), tokens.size(), is_first, true) < 0) {
        emit errorOccurred("Failed to tokenize prompt");
        return;
    }
     
    llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());
     
    int n_ctx = llama_n_ctx(ctx);
    int n_ctx_used = llama_memory_seq_pos_max(llama_get_memory(ctx), 0) + 1;
    
    if (n_ctx_used + batch.n_tokens > n_ctx) {
        emit errorOccurred("Context size exceeded");
        return;
    }
    
    if (llama_decode(ctx, batch) != 0) {
        emit errorOccurred("Failed to evaluate prompt");
        return;
    }
    
    QString response;
    int max_tokens = settings.maxTokens;
    
    for (int i = 0; i < max_tokens; i++) {
 
        llama_token new_token = llama_sampler_sample(sampler, ctx, -1);
         
        if (llama_vocab_is_eog(vocab, new_token)) {
            break;
        }
         
        char buf[256];
        int n = llama_token_to_piece(vocab, new_token, buf, sizeof(buf), 0, true);
        
        if (n < 0) {
            break;
        }
        
        QString token_str = QString::fromUtf8(buf, n);
        
        emit partialResponse(token_str);
        response += token_str;
         
        n_ctx_used = llama_memory_seq_pos_max(llama_get_memory(ctx), 0) + 1;
        if (n_ctx_used >= n_ctx - 1) {
            emit errorOccurred("Context limit reached");
            break;
        }
         
        batch = llama_batch_get_one(&new_token, 1);
        
        if (llama_decode(ctx, batch) != 0) {
            emit errorOccurred("Failed to decode token");
            break;
        }
    }
    
    emit responseGenerated(response);
}

void LlamaWorker::generateResponse(const QString &prompt, const GenerationSettings &settings) {
    std::vector<ChatMessage> messages;
    messages.push_back({"user", prompt});
    generateResponseWithMessages(messages, settings);
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