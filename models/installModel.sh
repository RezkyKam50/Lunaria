#!/bin/bash

download_file() {
    local url="$1"
    local output="$2"

    if command -v aria2c >/dev/null 2>&1; then
        echo "Downloading $output with aria2c..."
        aria2c -s 16 -x 16 -o "$output" "$url"
    else
        echo "aria2c not found, using wget instead..."
        wget -O "$output" "$url"
    fi
}
 
download_file "https://huggingface.co/unsloth/Qwen2.5-VL-7B-Instruct-GGUF/resolve/main/Qwen2.5-VL-7B-Instruct-Q4_K_M.gguf?download=true" "LLM.gguf"
download_file "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base-q5_1.bin?download=true" "TTS.bin"
