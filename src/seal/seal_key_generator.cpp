// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <iostream>
#include <string>
#include <sstream>
#include "seal/seal.h"
#include <fstream>
#include <vector>
#include <filesystem>
#include <cstring>
#include <sstream>
using namespace seal;

// Base64 encoding/decoding functions
static const std::string base64_chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string base64_encode(const std::string& input) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    int in_len = input.length();
    std::string::const_iterator bytes_to_encode = input.begin();

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2);

        for (j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
            ret += '=';
    }

    return ret;
}

std::string base64_decode(const std::string& encoded_string) {
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && encoded_string[in_] != '=' && 
           (isalnum(encoded_string[in_]) || encoded_string[in_] == '+' || encoded_string[in_] == '/')) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; i < 3; i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (j = 0; j < 4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; j < i - 1; j++) ret += char_array_3[j];
    }

    return ret;
}

// Helper function to serialize to string
template<typename T>
std::string serialize_to_string(const T& obj) {
    std::ostringstream oss;
    obj.save(oss);
    return oss.str();
}

// Helper function to deserialize from string
template<typename T>
void deserialize_from_string(const std::string& str, T& obj, const SEALContext& context) {
    std::istringstream iss(str);
    obj.load(context, iss);
}

float test_serialization(seal::PublicKey public_key, seal::SecretKey secret_key, seal::SEALContext context, double scale, seal::RelinKeys relin_keys)
{
    Encryptor encryptor(context, public_key);
    Evaluator evaluator(context);
    Decryptor decryptor(context, secret_key);

    CKKSEncoder encoder(context);

    Plaintext plain1, plain2, plain_result;
    encoder.encode(2.3, scale, plain1);
    encoder.encode(4.5, scale, plain2);

    Ciphertext encrypted1, encrypted2, encrypted_prod, encrypted_result;

    encryptor.encrypt(plain1, encrypted1);
    encryptor.encrypt(plain2, encrypted2);


    // multiply
    evaluator.multiply(encrypted1, encrypted2, encrypted_prod);
    // relinearize
    evaluator.relinearize_inplace(encrypted_prod, relin_keys);
    // rescale
    evaluator.rescale_to_next_inplace(encrypted_prod);
    parms_id_type last_parms_id = encrypted_prod.parms_id();

    evaluator.mod_switch_to_inplace(encrypted2, last_parms_id);
    encrypted2.scale() = scale;
    encrypted_prod.scale() = scale;
    evaluator.add(encrypted_prod, encrypted2, encrypted_result);

    decryptor.decrypt(encrypted_result, plain_result);
    std::vector<double> result;
    encoder.decode(plain_result, result);
    return result[0];

}

void generate_and_save_keys(double scale, const std::string& output_dir, const std::string& pk_path, const std::string& sk_path, const std::string& parms_path){
    // Create output directory if it doesn't exist
    std::filesystem::create_directories(output_dir);

    std::cout << "Example: Serialization" << std::endl;

    EncryptionParameters parms(scheme_type::ckks);
    size_t poly_modulus_degree = 8192;
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, { 60, 40, 40, 60 }));

    SEALContext context(parms);
    KeyGenerator keygen(context);
    auto secret_key = keygen.secret_key();
    PublicKey public_key;
    keygen.create_public_key(public_key);

    // Serialize and encode parameters
    std::string parms_str = serialize_to_string(parms);
    std::string parms_b64 = base64_encode(parms_str);
    std::ofstream parms_stream(parms_path);
    parms_stream << parms_b64;
    parms_stream.close();

    // Serialize and encode public key
    std::string pk_str = serialize_to_string(public_key);
    std::string pk_b64 = base64_encode(pk_str);
    std::ofstream pk_stream(pk_path);
    pk_stream << pk_b64;
    pk_stream.close();

    // Serialize and encode secret key
    std::string sk_str = serialize_to_string(secret_key);
    std::string sk_b64 = base64_encode(sk_str);
    std::ofstream sk_stream(sk_path);
    sk_stream << sk_b64;
    sk_stream.close();

    RelinKeys relin_keys_reloaded;
    keygen.create_relin_keys(relin_keys_reloaded);
    // Test the serialization
    if (test_serialization(public_key, secret_key, context, scale, relin_keys_reloaded) - 14.85 > 0.1) {
        std::cerr << "[ERROR] Input Serialization Check Failed" << std::endl;
        exit(1);
    }
    std::cout << "[OK] Input Serialization Check Passed" << std::endl;
}

void load_keys(double scale, const std::string& pk_path, const std::string& sk_path, const std::string& parms_path){
    // Load and decode parameters
    std::ifstream parms_stream(parms_path);
    std::string parms_b64((std::istreambuf_iterator<char>(parms_stream)), std::istreambuf_iterator<char>());
    parms_stream.close();
    std::string parms_str = base64_decode(parms_b64);
    
    EncryptionParameters parms;
    std::istringstream parms_iss(parms_str);
    parms.load(parms_iss);

    SEALContext context(parms);

    // Load and decode public key
    std::ifstream pk_stream(pk_path);
    std::string pk_b64((std::istreambuf_iterator<char>(pk_stream)), std::istreambuf_iterator<char>());
    pk_stream.close();
    std::string pk_str = base64_decode(pk_b64);
    
    PublicKey pk;
    deserialize_from_string(pk_str, pk, context);

    // Load and decode secret key
    std::ifstream sk_stream(sk_path);
    std::string sk_b64((std::istreambuf_iterator<char>(sk_stream)), std::istreambuf_iterator<char>());
    sk_stream.close();
    std::string sk_str = base64_decode(sk_b64);
    
    SecretKey sk;
    deserialize_from_string(sk_str, sk, context);

    KeyGenerator keygen(context, sk);
    
    RelinKeys rlk;
    keygen.create_relin_keys(rlk);

    // Test the serialization
    if (test_serialization(pk, sk, context, scale, rlk) - 14.85 > 0.1) {
        std::cerr << "[ERROR] Output Serialization Check Failed" << std::endl;
        exit(1);
    }
    std::cout << "[OK] Output Serialization Check Passed" << std::endl;
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]\n"
              << "Options:\n"
              << "  -d, --dir <directory>    Directory to store/load keys (default: ./seal)\n"
              << "  -h, --help              Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::string output_dir = "./keys";
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
        else if (arg == "-d" || arg == "--dir") {
            if (i + 1 < argc) {
                output_dir = argv[++i];
            } else {
                std::cerr << "Error: Directory path not provided\n";
                print_usage(argv[0]);
                return 1;
            }
        }
        else {
            std::cerr << "Unknown option: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    auto scale = pow(2.0, 40);
    
    // Construct file paths using the output directory
    std::string pk_path = output_dir + "/seal-public-key.b64";
    std::string sk_path = output_dir + "/seal-secret-key.b64";
    std::string parms_path = output_dir + "/seal-params.b64";

    if (!std::filesystem::exists(output_dir)) {
        std::cout << "Generating and saving keys to directory: " << output_dir << std::endl;
        generate_and_save_keys(scale, output_dir, pk_path, sk_path, parms_path);
        std::cout << std::endl;
    }
    
    std::cout << "Loading and testing keys from directory: " << output_dir << std::endl;
    load_keys(scale, pk_path, sk_path, parms_path);

    return 0;
}
