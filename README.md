# nilDB FHE Key Storage

MPC key storage experiments for various FHE cryptosystems using Nillion's nilDB

### 1) Set up nilDB
First, register a new organization to get access to nilDB [here](https://docs.nillion.com/build/secretVault-secretDataAnalytics/access).
Then, copy and edit the `.env` file:
```shell
cp .env.sample .env
```

Next, create a nilDB schema:
```shell
cd ./src/nildb
python main.py --create-schema
```

### 2) Set up the FHE library:

#### 2.1) [Lattigo](https://github.com/tuneinsight/lattigo)
Let's generate the FHE keys to store (this will first generate the keys and then run the FHE computation):
```shell
cd ./src/lattigo
go run bgv-main.go
```

#### 2.2) [TFHE-rs](https://github.com/zama-ai/tfhe-rs)
Let's generate the FHE keys to store (this will first generate the keys and then run the FHE computation):
```shell
cd ./src/tfhe-rs
cargo run -r
```
#### 2.2) [SEAL](https://github.com/microsoft/SEAL#)
Let's generate the FHE keys to store (this will first generate the keys and then run the FHE computation):
```shell
sudo apt-get update -y
sudo apt-get install build-essential cmake
# Clone SEAL
git clone https://github.com/microsoft/SEAL
cd SEAL
# Create build
cmake -S . -B build
# Compile build
cmake --build build
# Install SEAL
sudo cmake --install build

```
Now let's compile the script that enables us to do everything together.
```shell
g++ -I/usr/local/include/SEAL-4.1 seal_key_generator.cpp /usr/local/lib/libseal-4.1.a -o seal_key_generator.out
```

Finally, execute it with:
```shell
./seal_key_generator.out
# Or specify the directory where you want the keys stored as:
./seal_key_generator.out -d my_seal_keys
```

### 3) Store and retrieve keys to and from nilDB

```shell
cd ./src/nildb
# For Lattigo keys
python main.py --store-keys ../lattigo/keys
# For TFHE-rs keys
python main.py --store-keys ../tfhe-rs/keys
# For SEAL keys
python main.py --store-keys ../seal/keys
```
The returned ID will look something like `8100e495-5168-40a5-be0e-91654ef6ee11`, keep it safe to use it later when retrieving data.

To retrieve:
```shell
# For Lattigo keys
python main.py --retrieve-keys ../lattigo/keys --record-ids 8100e495-5168-40a5-be0e-91654ef6ee11
# For TFHE-rs keys
python main.py --retrieve-keys ../tfhe-rs/keys --record-ids 8100e495-5168-40a5-be0e-91654ef6ee11
# For SEAL keys
python main.py --retrieve-keys ../seal/keys --record-ids 8100e495-5168-40a5-be0e-91654ef6ee11
```
Finally, go back to Lattigo / TFHE-RS / SEAL and run it again, it'll find the FHE keys and not generate new ones.
