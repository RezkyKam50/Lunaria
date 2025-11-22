echo "Running autobuild..."

./tools/LlamaFlags.sh 
./tools/WhisperFlags.sh 
./tools/buildLlama.sh 
./tools/buildWhisper.sh 
./tools/buildQtApp.sh

