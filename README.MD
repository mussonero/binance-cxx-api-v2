## Binance C++ API

This version is derived from https://github.com/dmikushin/binance-cxx-api:

It included updated versions of LibCrul, LibWebSockets, MbedTLS, and JsonCpp:

### Note:
```
LibWebSockets use OS prebuilt openSLL.
The reason is lws_tls_client_connect keep crashing when LibWebSockets use MbedTLS.
There is an issue with lws_role_call_alpn_negotiated(wsi, (const char *)prot)
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