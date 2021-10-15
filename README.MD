## Binance C++ API
API Library for Binance Bitcoin Exchange.

This version included updated versions of LibCurl, LibWebSockets, LibWolfSSL, and JsonCpp:

Thread safety : Lag free : Support multi-threads websockets connections and more..

### Note:
```
LibWebSockets and LibCurl use LibWolfSSL without builtin CA-cert.
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
./bin/httpPerformanceTest | grep ":ms"
```
### output:
```
LINKBTC:getDepth time duration->291:ms
ETHTUSD:getDepth time duration->286:ms
XRPBNB:getDepth time duration->286:ms
LINKETH:getDepth time duration->284:ms
LINKBTC:getDepth time duration->286:ms
BNBUSDT:getDepth time duration->284:ms
Thread 0 finished in->1434:ms
ETHTUSD:getDepth time duration->284:ms
XRPBNB:getDepth time duration->285:ms
LINKETH:getDepth time duration->287:ms
LINKBTC:getDepth time duration->284:ms
BNBUSDT:getDepth time duration->281:ms
Thread 1 finished in->1426:ms
ETHTUSD:getDepth time duration->286:ms
XRPBNB:getDepth time duration->287:ms
LINKETH:getDepth time duration->287:ms
LINKBTC:getDepth time duration->287:ms
BNBUSDT:getDepth time duration->289:ms
Thread 2 finished in->1435:ms
~~~
~~~
```