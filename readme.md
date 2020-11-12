## Prerequisite

For Ubuntu 20.10 (need g++ 10)
```bash
sudo apt-get update
sudo apt-get install -y libgtest-dev libfmt-dev cmake git g++
```

## Run test

```
git clone https://github.com/zerolfx/small-compiler
cd small-compiler
mkdir build && cd build
cmake ..
make -j
cd ../test
../build/test/test
```