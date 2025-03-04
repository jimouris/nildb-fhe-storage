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

### 3) Store and retrieve keys to and from nilDB

```shell
cd ./src/nildb
# For Lattigo keys
python main.py --store-keys ../lattigo/keys
# For TFHE-rs keys
python main.py --store-keys ../tfhe-rs/keys
```
The returned ID will look something like `8100e495-5168-40a5-be0e-91654ef6ee11`, keep it safe to use it later when retrieving data.

To retrieve:
```shell
# For Lattigo keys
python main.py --retrieve-keys ../lattigo/keys --record-id 8100e495-5168-40a5-be0e-91654ef6ee11
# For TFHE-rs keys
python main.py --retrieve-keys ../tfhe-rs/keys --record-id 8100e495-5168-40a5-be0e-91654ef6ee11
```
Finally, go back to Lattigo and run it again, it'll find the FHE keys and not generate new ones.
