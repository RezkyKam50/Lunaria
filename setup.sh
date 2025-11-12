sudo chmod +x ./tools/*.sh && ./models/*.sh
sudo chmod +x ./install-desktop.sh

echo "Compiling LLM Engine module: 'llama.cpp' from source..."
./tools/buildLLama.sh

echo "Compiling Voice Recognition module: 'whisper.cpp' from source..."
./tools/buildWhisper.sh

echo "Downloading example models..."
./models/installModel.sh

echo "Compiling Lunaria from source..."
./tools/buildQtApp.sh

echo "Making desktop shortcut for Lunaria..."
./install-desktop.sh


