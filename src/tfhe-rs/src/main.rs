use tfhe::prelude::*;
use tfhe::{generate_keys, set_server_key, ConfigBuilder, FheUint32, FheUint8};
use serde::{Serialize, Deserialize};
use serde_with::{serde_as, base64::Base64};
use std::fs::File;
use std::io::Write;
use base64;

#[serde_as]
#[derive(Serialize, Deserialize)]
struct SerializableClientKey {
    #[serde_as(as = "Base64")]
    key_data: Vec<u8>,
}

#[serde_as]
#[derive(Serialize, Deserialize)]
struct SerializableServerKey {
    #[serde_as(as = "Base64")]
    key_data: Vec<u8>,
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Basic configuration to use homomorphic integers
    let config = ConfigBuilder::default().build();

    let client_key_filename = "keys/tfhe-client-key.b64";
    let server_key_filename = "keys/tfhe-server-key.b64";

    // Declare variables outside the if/else block
    let (client_key, server_keys) = if std::path::Path::new(client_key_filename).exists() &&
                                     std::path::Path::new(server_key_filename).exists() {
        println!("Reading existing keys from files...");
        // Read existing keys from files
        let client_key_b64 = std::fs::read_to_string(client_key_filename)?;
        let server_key_b64 = std::fs::read_to_string(server_key_filename)?;

        // Decode base64
        let client_key_bytes = base64::decode(&client_key_b64)?;
        let server_key_bytes = base64::decode(&server_key_b64)?;

        // Deserialize keys
        let client_key = bincode::deserialize(&client_key_bytes)?;
        let server_keys = bincode::deserialize(&server_key_bytes)?;

        (client_key, server_keys)
    } else {
        println!("Generating new keys...");
        // Key generation
        let (client_key, server_keys) = generate_keys(config);

        // Convert keys to byte vectors
        let client_key_bytes = bincode::serialize(&client_key)?;
        let server_keys_bytes = bincode::serialize(&server_keys)?;

        // Convert to base64
        let client_key_b64 = base64::encode(&client_key_bytes);
        let server_keys_b64 = base64::encode(&server_keys_bytes);

        // Create keys directory if it doesn't exist
        std::fs::create_dir_all("keys")?;

        // Write to files
        let mut client_key_file = File::create(client_key_filename)?;
        client_key_file.write_all(client_key_b64.as_bytes())?;

        let mut server_keys_file = File::create(server_key_filename)?;
        server_keys_file.write_all(server_keys_b64.as_bytes())?;

        (client_key, server_keys)
    };

    let clear_a = 1344u32;
    let clear_b = 5u32;
    let clear_c = 7u8;

    // Encrypting the input data using the (private) client_key
    // FheUint32: Encrypted equivalent to u32
    let mut encrypted_a = FheUint32::try_encrypt(clear_a, &client_key)?;
    let encrypted_b = FheUint32::try_encrypt(clear_b, &client_key)?;

    // FheUint8: Encrypted equivalent to u8
    let encrypted_c = FheUint8::try_encrypt(clear_c, &client_key)?;

    // On the server side:
    set_server_key(server_keys);

    // Clear equivalent computations: 1344 * 5 = 6720
    let encrypted_res_mul = &encrypted_a * &encrypted_b;

    // Clear equivalent computations: 6720 >> 5 = 210
    encrypted_a = &encrypted_res_mul >> &encrypted_b;

    // Clear equivalent computations: let casted_a = a as u8;
    let casted_a: FheUint8 = encrypted_a.cast_into();

    // Clear equivalent computations: min(210, 7) = 7
    let encrypted_res_min = &casted_a.min(&encrypted_c);

    // Operation between clear and encrypted data:
    // Clear equivalent computations: 7 & 1 = 1
    let encrypted_res = encrypted_res_min & 1_u8;

    // Decrypting on the client side:
    let clear_res: u8 = encrypted_res.decrypt(&client_key);
    assert_eq!(clear_res, 1_u8);

    Ok(())
}
