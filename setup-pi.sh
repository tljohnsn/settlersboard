#!/bin/bash
git clone https://github.com/tljohnsn/settlersboard.git

sudo apt-get -y update
sudo apt-get -y install libgtk2.0-dev emacs23-nox
mkdir ~/.ssh
echo "ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAIEAwk9g2BbEL7k2DwpEKBNScNspOcZnmGxzBvV/nSdtlXeyuAazr2tIOw13TAl574vneVdFqV+eVwpD4dCVvfatWrBq0byiPVD7kDhu2p6HEK0I9HCn0MsQY56c6Riv4cy/cybp9capOHIx58wrQ6A8DsbZ7wZrsVGYgiUgeZD+tsM= tljohnsn@smack.office.useractive.com
ssh-rsa AAAAB3NzaC1yc2EAAAABJQAAAIEAlRchb+Hm1VM7dYqDZ0D6zCtnf/dH5ba4NkGZ9gFGkHdgQLlepCCYuOUnoCr0N40lQWGdOuCVsWVEsClzRNmLTBXNgF4uDVx+S1Zw/uw/zJ89jWT/WdPr6+hT2s7z8OBe1HGI3zyW7ftzBpUWE9W2qiiPXwUXUAOvn+pX15DPrjE= newlaptopkey
" >>~/.ssh/authorized_keys
chmod 600 ~/.ssh/authorized_keys
sudo sed -i -e "s/.*disable_overscan=[01]/disable_overscan=1/" /boot/config.txt

cd ~/settlersboard
gcc -o sbc settlersboard.c `pkg-config --libs --cflags gtk+-2.0` -O3 -ffast-math

echo "[Desktop]
Session=settlersboard

">~/.dmrc

echo "#\!/bin/bash
xset s off
xset +dpms
xset dpms 3600 3610 3620
cd ~/settlersboard
./sbc
" >~/runsettlersboard.sh
chmod 755 ~/runsettlersboard.sh

echo "[Desktop Entry]
Name=SettlersBoard
Exec=/home/pi/runsettlersboard.sh
" | sudo tee /usr/share/xsessions/settlersboard.desktop
