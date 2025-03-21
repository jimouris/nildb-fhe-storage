"""Schema Create example using the SecretVault wrapper"""
"""Data Create and Read example using the SecretVault wrapper"""

import argparse
import asyncio
import json
import sys
import os
from secretvaults import SecretVaultWrapper, OperationType
from org_config import org_config

# Update schema ID with your own value
SCHEMA_ID = os.getenv("FHE_SCHEMA_ID")
RECORD_ID = os.getenv("RECORD_ID")
LATTIGO_KEYS_DIR = "../lattigo/keys"
TFHERS_KEYS_DIR = "../tfhe-rs/keys"


async def create_schema():
    """
    Main function to initialize the SecretVaultWrapper and create a new schema.
    """
    try:
        # Load the schema from schema_match.json
        with open("schema.json", "r", encoding="utf8") as schema_file:
            schema = json.load(schema_file)


        # Initialize the SecretVaultWrapper instance with the org configuration
        org = SecretVaultWrapper(org_config["nodes"], org_config["org_credentials"])
        await org.init()

        # Create a new schema
        new_schema = await org.create_schema(schema, "Web3 Experience Survey")
        print("📚 New Schema:", new_schema)
        print("Store schema in the .env file as FHE_SCHEMA_ID.")

        # Optional: Delete the schema
        # await org.delete_schema(new_schema)
    except RuntimeError as error:
        print(f"❌ Failed to use SecretVaultWrapper: {str(error)}")
        sys.exit(1)


async def read_write_keys(read=False, secret_key_filename=None, public_key_filename=None, params_filename=None, record_ids=None):
    """
    Main function to demonstrate writing to and reading from nodes using the SecretVaultWrapper.
    """
    try:
        # Initialize the SecretVaultWrapper instance with the org configuration and schema ID
        collection = SecretVaultWrapper(
            org_config["nodes"],
            org_config["org_credentials"],
            SCHEMA_ID,
            operation=OperationType.STORE.value,
        )
        await collection.init()

        if not read:
            max_chunk_length = (1 << 12) // 3 # Each params, secret key, and public key can occupy at most 1/3 of the max record size
            print("Max length:", max_chunk_length)
            data = {}
            with open(secret_key_filename, "r") as f:
                secret_key = f.read()
                # Process this in batches
                data["secret_key"] = [
                    { "%allot": secret_key[i: i + 4096] }
                    for i in range(0, len(secret_key), 4096)
                ]

            with open(public_key_filename, "r") as f:
                public_key = f.read()
                data["public_key"] = [
                    public_key[i: i + 4096]
                    for i in range(0, len(public_key), 4096)
                ]
            print("Loaded public key")
            if params_filename:
                with open(params_filename, "r") as f:
                    parameters = f.read()
                    data["parameters"] = parameters

            else:
                data["parameters"] = ""

            # Split data into n chunks of max_chunk_length
            max_data_len = max([len(v) for v in data.values()])
            records = [{k: v[i: i + max_chunk_length] for k, v in data.items()} for i in range(0, max_data_len, max_chunk_length)]
            print("🔏 Created IDs:")
            for record in records:
                data_written = await collection.write_to_nodes([record])
                

                # Extract unique created IDs from the results
                new_ids = list(
                    {
                        created_id
                        for item in data_written
                        if item.get("result")
                        for created_id in item["result"]["data"]["created"]
                    }
                )

                print("".join(new_ids), end=" ")
            print("")
        else:
            if not isinstance(record_ids, list):
                record_ids = [record_ids]

            data = {}
            for record_id in record_ids:
                filter_by_id = {"_id": record_id}
                print(f"🔍 Reading data for ID: {record_id}")

                # Read data from nodes
                data_read = await collection.read_from_nodes(filter_by_id)
                if len(data_read) == 0:
                    print("❌ No records found")
                    sys.exit(1)

                # Get first record since we filtered by ID
                record = data_read[0]

                for k, v in record.items():
                    if k not in data:
                        data[k] = ""
                    data[k] += "".join(v)

            # Write parameters to file
            if params_filename:
                with open(params_filename, "w") as f:
                    f.write(data["parameters"])

            # Write public key to file
            with open(public_key_filename, "w") as f:
                public_key = "".join(data["public_key"])
                f.write(public_key)

            # Combine and write secret key shares
            with open(secret_key_filename, "w") as f:
                # Combine all secret key chunks into a single string
                secret_key = "".join(data["secret_key"])
                f.write(secret_key)

            print("✅ Successfully stored keys and parameters to files")

    except RuntimeError as error:
        print(f"❌ Failed to use SecretVaultWrapper: {str(error)}")
        sys.exit(1)


# Run the async main function
if __name__ == "__main__":
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='FHE Key Management')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--create-schema', action='store_true', help='Create new schema')
    group.add_argument('--store-keys', nargs='?', type=str, const=LATTIGO_KEYS_DIR, metavar='KEY_DIR', help='Store keys to nilDB. Optionally specify key directory (default: LATTIGO_KEYS_DIR)')
    group.add_argument('--retrieve-keys', nargs='?', type=str, const=LATTIGO_KEYS_DIR, metavar='KEY_DIR', help='Retrieve keys from nilDB. Optionally specify key directory (default: LATTIGO_KEYS_DIR)')
    parser.add_argument('--record-ids', type=str, nargs='+', help='Record IDs to retrieve (required with --retrieve-keys)')

    args = parser.parse_args()

    # Validate arguments
    if args.record_ids and not args.retrieve_keys:
        parser.error("--record-ids can only be used with --retrieve-keys")
    if args.retrieve_keys and not args.record_ids:
        parser.error("--record-ids is required when using --retrieve-keys")

    if args.create_schema:
        asyncio.run(create_schema())
    elif args.store_keys:
        # Determine key filename based on directory
        if "tfhe-rs" in args.store_keys:
            print(f"📤 Storing TFHE-rs keys from directory: {args.store_keys}")
            secret_key_filename = f"{args.store_keys}/tfhe-client-key.b64"
            public_key_filename = f"{args.store_keys}/tfhe-server-key.b64"
            params_filename = None
        elif "seal" in args.store_keys:
            print(f"📤 Storing SEAL keys from directory: {args.store_keys}")
            secret_key_filename = f"{args.store_keys}/seal-secret-key.b64"
            public_key_filename = f"{args.store_keys}/seal-public-key.b64"
            params_filename = f"{args.store_keys}/seal-params.b64"
        else:  # lattigo case
            print(f"📤 Storing Lattigo keys from directory: {args.store_keys}")
            secret_key_filename = f"{args.store_keys}/bgv-secret-key.b64"
            public_key_filename = f"{args.store_keys}/bgv-public-key.b64"
            params_filename = f"{args.store_keys}/bgv-params.b64"

        asyncio.run(read_write_keys(read=False, secret_key_filename=secret_key_filename,
                                  public_key_filename=public_key_filename, params_filename=params_filename))
    elif args.retrieve_keys:
        # Determine key filename based on directory
        if "tfhe-rs" in args.retrieve_keys:
            print(f"📥 Retrieving TFHE-rs keys to directory: {args.retrieve_keys}")
            secret_key_filename = f"{args.retrieve_keys}/tfhe-client-key.b64"
            public_key_filename = f"{args.retrieve_keys}/tfhe-server-key.b64"
            params_filename = None
        elif "seal" in args.retrieve_keys:
            print(f"📥 Retrieving SEAL keys to directory: {args.retrieve_keys}")
            secret_key_filename = f"{args.retrieve_keys}/seal-secret-key.b64"
            public_key_filename = f"{args.retrieve_keys}/seal-public-key.b64"
            params_filename = f"{args.retrieve_keys}/seal-params.b64"
        else:  # lattigo case
            print(f"📥 Retrieving Lattigo keys to directory: {args.retrieve_keys}")
            secret_key_filename = f"{args.retrieve_keys}/bgv-secret-key.b64"
            public_key_filename = f"{args.retrieve_keys}/bgv-public-key.b64"
            params_filename = f"{args.retrieve_keys}/bgv-params.b64"

        asyncio.run(read_write_keys(read=True, secret_key_filename=secret_key_filename,
                                  public_key_filename=public_key_filename, params_filename=params_filename,
                                  record_ids=args.record_ids))
    else:
        parser.print_help()
        sys.exit(1)

    # Exit after running requested operation
    sys.exit(0)
