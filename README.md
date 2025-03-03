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

### 2) Set up [Lattigo](https://github.com/tuneinsight/lattigo)
Let's generate the FHE keys to store (this will first generate the keys and then run the FHE computation):
```shell
cd ./src/lattigo
go run bgv-main.go
```

### 3) Store and retrieve keys to and from nilDB
```shell
cd ./src/nildb
python main.py --store-keys
```
The returned ID will look something like `6b2da249-e8bf-4f90-a7e6-93cd0cafd0a6`, keep it safe to use it later when retrieving data.

To retrieve:
```shell
python main.py --retrieve-keys 6b2da249-e8bf-4f90-a7e6-93cd0cafd0a6
```
Finally, go back to Lattigo and run it again, it'll find the FHE keys and not generate new ones.
