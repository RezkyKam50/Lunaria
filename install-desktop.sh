#!/bin/bash

BINARY_PATH="$PWD/Lunaria"
ICON_PATH="$PWD/media/Lunaria.png"
DESKTOP_FILE="$HOME/.local/share/applications/lunaria.desktop"
 
cat > "$DESKTOP_FILE" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=Lunaria
Comment=Lightweight LLM Chat App
Exec=$BINARY_PATH
Icon=$ICON_PATH
Terminal=false
Categories=Utility;AudioVideo;
Keywords=AI;Chat;Voice;Assistant;
StartupNotify=true
EOF
 
chmod +x "$DESKTOP_FILE"
 
update-desktop-database "$HOME/.local/share/applications/"

echo "Lunaria desktop entry installed!"
echo "You can now find it in your application launcher."