cd tesseract
./autogen.sh
./configure
make -j$(nproc)
sudo make install
sudo ldconfig
 
sudo mkdir -p /usr/local/share/tessdata/
 
sudo wget -O /usr/local/share/tessdata/eng.traineddata https://github.com/tesseract-ocr/tessdata/raw/main/eng.traineddata
 
sudo wget -O /usr/local/share/tessdata/osd.traineddata https://github.com/tesseract-ocr/tessdata/raw/main/osd.traineddata