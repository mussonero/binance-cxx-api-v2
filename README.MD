## Binance C++ API

This version is derived from https://github.com/dmikushin/binance-cxx-api:

It included updated versions of LibCrul, LibWebSockets, MbedTLS, and JsonCpp:

### Note:
```
LibCrul and LibWebSockets use MbedTLS with builtin CA,cert.
```
### Prerequisites:
```
sudo apt-get install build-essential cmake g++ libssl-dev
```

### Building:

```
git clone --recurse-submodules https://github.com/mussonero/binance-cxx-api-v2.git
cd binance-cxx-api-v2
mkdir build
cd build/
cmake ..
make
./bin/example
```
