# Install 
sudo apt-get install libegl1-mesa-dev
sudo apt-get install libgstreamer1.0-dev

# compile with below command
g++ -Wall -std=c++11  test.cpp -o test $(pkg-config --cflags --libs gstreamer-1.0) -ldl
